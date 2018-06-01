#!/usr/bin/python3

import numpy as np
import imutils
import os

import pytesseract
from PIL import Image

import cv2

from skimage.morphology import disk
from skimage.filters import rank

F1 = "172_79.jpg"
F2 = "633_88.jpg"

img = cv2.imread(F2)
img = imutils.resize(img, width = 480)
img_org = cv2.cvtColor(img,cv2.COLOR_BGR2GRAY)

# 生活模式下的操作


# 1.　高斯模糊，滤除多余的直角干扰
img_blur = cv2.GaussianBlur(img,(5,5),0)
blur_gray= cv2.cvtColor(img_blur,cv2.COLOR_BGR2GRAY) 

# 2. 提取Sobel直角特征
sobelx = cv2.Sobel(blur_gray,cv2.CV_16S,1,0,ksize=3)
abs_sobelx = np.absolute(sobelx)
sobel_8u = np.uint8(abs_sobelx)
cv2.imshow("Sobel特征", sobel_8u)
cv2.waitKey(0)

# 3. OTSU提取二值图像    
ret, thd = cv2.threshold(sobel_8u, 0, 255, cv2.THRESH_BINARY+cv2.THRESH_OTSU)    
thd_abs = cv2.convertScaleAbs(thd)
bgimg = cv2.addWeighted(thd_abs, 1, 0, 0, 0)

# 4. 腐蚀和膨胀，获得连通区域
kernel = cv2.getStructuringElement(cv2.MORPH_RECT,(13,7))
bgmask = cv2.morphologyEx(bgimg, cv2.MORPH_CLOSE, kernel)

im2, contours, hierarchy = cv2.findContours(bgmask.copy(), cv2.RETR_EXTERNAL, #外部轮廓
                            cv2.CHAIN_APPROX_SIMPLE)

# 5. 剔除明显不满足条件的区域
cand_rect = []
for item in contours:
    epsilon = 0.02*cv2.arcLength(item, True)
    approx = cv2.approxPolyDP(item, epsilon, True)  
    if len(approx) <= 8:
        rect = cv2.minAreaRect(item)
        if rect[1][0] < 20 or rect[1][1] < 20:
            continue
        if rect[1][0] > 150 or rect[1][1] > 150:
            continue        
        ratio = (rect[1][1]+0.00001) / rect[1][0]
        #if ratio > 1 or ratio < 0.9:
        #    continue
        box = cv2.boxPoints(rect)
        box_d = np.int0(box)
        cv2.drawContours(img, [box_d], 0, (0,255,0), 3)
        cand_rect.append(box)

if not cand_rect:
    print("Not detected!");
    os._exit(-1)

cv2.imshow("抓取的图片", img)
cv2.waitKey(0)

# 6. 进行坐标变换
for item in cand_rect:
    
    # 由于使用minAreaRect获得的图像有-90~0的角度，所以给出的坐标顺序也不一定是
    # 转换时候给的，这里需要判断出图像的左上、左下、右上、右下的坐标，便于后面的变换
    pts = item.reshape(4, 2)
    rect = np.zeros((4, 2), dtype = "float32")
    
    # the top-left point has the smallest sum whereas the
    # bottom-right has the largest sum
    s = pts.sum(axis = 1)
    rect[0] = pts[np.argmin(s)]
    rect[3] = pts[np.argmax(s)]
    
    # compute the difference between the points -- the top-right
    # will have the minumum difference and the bottom-left will
    # have the maximum difference
    diff = np.diff(pts, axis = 1)
    rect[2] = pts[np.argmin(diff)]
    rect[1] = pts[np.argmax(diff)]    

    dst = np.float32([[0,0],[0,100],[200,0],[200,100]])
    
    M = cv2.getPerspectiveTransform(rect, dst)
    warp = cv2.warpPerspective(img_org, M, (200, 100))
    
    cv2.imshow("cropped image", warp)
    cv2.waitKey(0)
    
    warp = np.array(warp, dtype=np.uint8)
    radius = 10
    selem = disk(radius)
    
    local_otsu = rank.otsu(warp, selem)
    l_otsu = np.uint8(warp >= local_otsu)
    l_otsu *= 255
    
    kernel = cv2.getStructuringElement(cv2.MORPH_RECT,(6,6))
    l_otsu = cv2.morphologyEx(l_otsu, cv2.MORPH_CLOSE, kernel)    
    cv2.imshow("binary image to be detected", l_otsu)
    cv2.waitKey(0)
    
    print("识别结果：")
    print(pytesseract.image_to_string(Image.fromarray(l_otsu)))
    
    cv2.waitKey(0)

# http://www.pyimagesearch.com/2014/03/10/building-pokedex-python-getting-started-step-1-6/
