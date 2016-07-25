#!/usr/bin/python3

#######################
#
# 身份证自动读数系统
#
#######################

import numpy as np
import imutils
import os

import pytesseract
from PIL import Image

import cv2

from skimage.morphology import disk
from skimage.filters import rank

show_img = True

def img_show_hook(title, img):
    global show_img
    if show_img:
        cv2.imshow(title, img)
        cv2.waitKey(0)    
    return


def img_sobel_binary(im, blur_sz):
    
    # 高斯模糊，滤除多余的直角干扰
    img_blur = cv2.GaussianBlur(im,blur_sz,0)
    if len(img_blur.shape) == 3:
        blur_gray = cv2.cvtColor(img_blur,cv2.COLOR_BGR2GRAY) 
    else:
        blur_gray = img_blur
    
    # 提取Sobel直角特征
    sobelx = cv2.Sobel(blur_gray,cv2.CV_16S,1,0,ksize=3)
    abs_sobelx = np.absolute(sobelx)
    sobel_8u = np.uint8(abs_sobelx)
    img_show_hook("Sobel特征", sobel_8u)
    
    # OTSU提取二值图像    
    ret, thd = cv2.threshold(sobel_8u, 0, 255, cv2.THRESH_BINARY+cv2.THRESH_OTSU)    
    thd_abs = cv2.convertScaleAbs(thd)
    bgimg = cv2.addWeighted(thd_abs, 1, 0, 0, 0)
    
    img_show_hook("OTSU二值图像", bgimg)
    
    return bgimg


def img_contour_extra(im):
    # 腐蚀和膨胀
    kernel = cv2.getStructuringElement(cv2.MORPH_RECT,(13,7))
    bgmask = cv2.morphologyEx(im, cv2.MORPH_CLOSE, kernel)
    
    img_show_hook("膨胀腐蚀结果", bgmask)
    
    # 获得连通区域
    # 该函数会破坏原始参数
    im2, contours, hierarchy = cv2.findContours(bgmask.copy(), cv2.RETR_EXTERNAL, #外部轮廓
                                cv2.CHAIN_APPROX_SIMPLE)
    return contours


def img_contour_select(ctrs, im):
    # 剔除明显不满足条件的区域
    cand_rect = []
    for item in ctrs:
        epsilon = 0.02*cv2.arcLength(item, True)
        approx = cv2.approxPolyDP(item, epsilon, True)  
        if len(approx) <= 8:
            rect = cv2.minAreaRect(item)
            #只考虑水平的情况
            if rect[2] < -10 and rect[2] > -80:
                continue
            if rect[1][0] < 10 or rect[1][1] < 10:
                continue
            #ratio = (rect[1][1]+0.00001) / rect[1][0]
            #if ratio > 1 or ratio < 0.9:
            #    continue
            box = cv2.boxPoints(rect)
            box_d = np.int0(box)
            cv2.drawContours(im, [box_d], 0, (0,255,0), 3)
            cand_rect.append(box)
    img_show_hook("候选区域", im)   
    return cand_rect

def img_tesseract_detect(c_rect, im):
    # 由于使用minAreaRect获得的图像有-90~0的角度，所以给出的坐标顺序也不一定是
    # 转换时候给的，这里需要判断出图像的左上、左下、右上、右下的坐标，便于后面的变换
    pts = c_rect.reshape(4, 2)
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

    width = rect[3][0] - rect[0][0]
    height = rect[3][1] - rect[0][1]
    
    width = (int)((50.0 / height) * width)
    height = 50
    
    dst = np.float32([[0,0],[0,height],[width,0],[width,height]])
    
    M = cv2.getPerspectiveTransform(rect, dst)
    warp = cv2.warpPerspective(im, M, (width, height))
    
    img_show_hook("剪裁识别图像", warp) 
    
    warp = np.array(warp, dtype=np.uint8)
    radius = 13
    selem = disk(radius)
    
    #　使用局部自适应OTSU阈值处理
    local_otsu = rank.otsu(warp, selem)
    l_otsu = np.uint8(warp >= local_otsu)
    l_otsu *= 255
    
    kernel = cv2.getStructuringElement(cv2.MORPH_RECT,(2,2))
    l_otsu = cv2.morphologyEx(l_otsu, cv2.MORPH_CLOSE, kernel)    
    
    img_show_hook("局部自适应OTSU图像", l_otsu) 
    
    print("识别结果：")
    print(pytesseract.image_to_string(Image.fromarray(l_otsu), lang="chi-sim"))
    
    cv2.waitKey(0)
    return 
    
    
if __name__ == "__main__":
    
    print("...图片文字识别系统...")
    
    F1 = "IMG_2082_Z.JPG"
    F2 = "IMG_4438.JPG"
    
    img = cv2.imread(F1)
    img = imutils.resize(img, width = 600)
    img_gray = cv2.cvtColor(img,cv2.COLOR_BGR2GRAY)
    
    sb_img = img_sobel_binary(img, (5,5))
    contours = img_contour_extra(sb_img)
    cand_rect = img_contour_select(contours, img)
    for item in cand_rect:
        img_tesseract_detect(item, img_gray)
    

# http://www.pyimagesearch.com/2014/03/10/building-pokedex-python-getting-started-step-1-6/
