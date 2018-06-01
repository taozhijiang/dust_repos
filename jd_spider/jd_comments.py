#!/usr/bin/python 
#-*- coding: utf-8 -*- 
#encoding=utf-8

import jd_config
import jd_utils

import urllib.request
import urllib.error
import urllib
import codecs
import zlib
import re
import os
import sys
import threading
import time
from bs4 import BeautifulSoup
import random

import errno

import http.client

import socket

import paramiko 
from scp import SCPClient

exitFlag = 0

jd_item_url = "http://item.jd.com/%d.html"
jd_consult_url = "http://club.jd.com/allconsultations/%d-%d-1.html"
jd_comment_url = "http://club.jd.com/review/%d-%d-%d-0.html"
jd_headers = {"Referer":"http://www.jd.com/",
              "Host": "club.jd.com",
              "Content-type": "application/x-www-form-urlencoded; charset=UTF-8",
              "Accept": "*/*",
              "Connection": "keep-alive",
              "Accept-Encoding":"gzip, deflate",
               "User-Agent":"Mozilla/5.0 (Windows NT 6.1; WOW64; rv:37.0) Gecko/20100101 Firefox/37.0",
               "Cookie":"user-key=23013c2b-329d-422c-9aa1-d0b6600c187a; cn=0; ipLocation=%u5317%u4EAC; areaId=1; ipLoc-djd=1-72-2799-0; __jda=122270672.763825332.1431154352.1432700135.1432782483.14; __jdb=122270672.3.763825332|14.1432782483; __jdc=122270672; __jdv=122270672|jd.com|-|-|not set; __jdu=763825332"
               }

def random_jd_header(refer = None):
    global jd_headers
    jd_headers['User-Agent'] = jd_config.random_agent()
    if refer:
        jd_headers['Referer'] = refer
    return jd_headers


result_path = ""


class CommentThread(threading.Thread):
    def __init__(self, SERV_HOST, SERV_PORT, threadID):
        threading.Thread.__init__(self)
        self.host = SERV_HOST
        self.port = SERV_PORT
        self.CID  = 1233
        self.sk = None
        self.threadID = threadID
        self.up_path = ""
        
    def run(self):
        print ("启动商品评论线程 %d ...\n" % self.threadID)
        jda = JdAnysisComment(self.threadID)
        proc_result = None
        ssh = paramiko.SSHClient()
        ssh.load_system_host_keys()
        ssh.set_missing_host_key_policy(paramiko.MissingHostKeyPolicy())

        while True:
            self.sk = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
            self.sk.connect((self.host, self.port))
            
            #if proc_result:
            #    # 反馈上轮处理的结果
            #    jproc_result = repr(proc_result) + ','
            #    self.sk.sendall(jproc_result.encode())
            
            #两者之间会有竞争条件，原始链接的套接字可能被服务器关闭，
            #而如果重新开个套接字连接又比较浪费，所以当前暂时一起发送    
            req_url = {'CLIENT':self.CID,'TYPE':'REQ_URL','DATA':'1'}
            if proc_result:
                # 反馈上轮处理的结果
                jproc_result = repr(proc_result) + ','
                jreq_url = repr(req_url) + ','
                self.sk.sendall((jproc_result + jreq_url).encode())
            else:
                jreq_url = repr(req_url) + ','
                self.sk.sendall((jreq_url).encode())
                
            proc_result = None
            # 请求待处理的URL
            
            # 等待服务器返回
            try:
                rep_url = self.sk.recv(1024).decode()
            except Exception as e:
                print("jd_comments Socket Error:", str(e))
                continue
            finally:
                # 关闭，节约资源
                self.sk.close()
            
            if not rep_url:
                print("服务器响应错误！")
                continue
            
            
            jrep_url = eval(rep_url)[0]
            if jrep_url and jrep_url['CLIENT'] == self.CID and jrep_url['TYPE'] == 'REP_URL':
                full_url = jrep_url['DATA']['PURL']
                # 用户服务器告诉客户端SCP上传文件的目录
                if self.up_path == "":
                    self.up_path = jrep_url['DATA']['PATH']
            else:
                full_url = ""

            if not full_url:
                print("服务器返回的URL为空！")
                continue
              
            # 处理URL对应的产品评论 
            print("客户端线程[%d]:%s" %(self.threadID, full_url))
            ret = jda.get_product_comments(full_url)
            
            # 反馈服务器处理结果
            # PID-COUNT-LUCK
            if ret:
                try:
                    proc_result = {'CLIENT':self.CID,'TYPE':'FINISH','DATA':{'PURL':ret[0],'CNT':ret[1],'LUCK':ret[2],'PATH':ret[3], 'PID':ret[4]}}
                    ssh.connect(jd_config.SERVER_ADDR, port=int(22), username=jd_config.SERVER_USER, password=jd_config.SERVER_PASS)        
                    with SCPClient(ssh.get_transport()) as scp:
                        product_id = ret[4]
                        result_file = "%s/%d_comm.txt"%(jd_config.JDSPR_RESULT_LOCAL, product_id)
                        result_f = "%s/%d_comm.txt"%(self.up_path, product_id)
                        print('传送结果:%s-->%s' %(result_file, result_f))
                        scp.put(result_file, result_f)            
                        try:
                            os.remove(result_file)
                        except:
                            pass                    
                    ssh.close()
                except Exception as e:
                    print("客户线程异常："+str(e))
                    continue 
            else:
                proc_result = {'CLIENT':self.CID,'TYPE':'FINISH','DATA':{'PURL':'','CNT':0,'LUCK':0,'PATH':'', 'PID':0}}
            
	    
        print ("退出商品评论线程 %d ..." % self.threadID)        
        
        
class JdAnysisComment:
    def __init__(self, tid = 0):
        self.tid = tid
        self.agent = None     
        pass
    
    def get_product_comments(self, product_url):
        result_path = ""
        self.agent = random_jd_header()
        page_id = 1    
        product_id = int(product_url.split('.')[2].split('/')[1])
        product_url = jd_item_url % product_id
        
        flag = 0
        while True:
            try:
                request = urllib.request.Request(product_url, headers = self.agent)
                g_response = urllib.request.urlopen(request)
                if g_response.info().get('Content-Encoding') == 'gzip':
                    g_read = zlib.decompress(g_response.read(), 16+zlib.MAX_WBITS)
                else:    
                    g_read = g_response.read()
                
                product_html = jd_utils.encoding(g_read)
                #操作正常
                break
            except UnicodeDecodeError:
                print ("GBK/Unicode编解码错误!")
                return
            except http.client.IncompleteRead:
                continue
            except Exception as e:
                if flag > 3 :
                    print ("评论线程[%d]网络异常，放弃该产品" % self.tid) 
                    return
                if e.errno == errno.ECONNRESET:
                    flag = flag + 1
                    time.sleep(10)
                    print ("评论线程[%d]重试中...[%d]"%( self.tid, flag) )
                    continue
                print ("1.其它异常:"+str(e))
                return
	
        product_name = None
        product_ts = None
        product_soup = BeautifulSoup(product_html)
        product_name = product_soup.find('h1')
        
        #产品类别
        product_type = product_soup.find('div', attrs={"class":"breadcrumb"})
        if product_type:
            product_ts = product_type.findAll('a')
        
        if not product_name or not product_ts:
            print("1.产品名称和类别提取错误，返回！Check[%s]" % product_url)
            print(self.agent['User-Agent'])
            return
        
        result_file = None
        try:
            i = 0    
            for pt_item in product_ts:
                if pt_item:
                    result_path = result_path + "/" + pt_item.string + "/"
                    i = i + 1
                    #目录类别的深度
                    if i > 3:
                        break
                else:
                    print("2.提取产品名称和目录错误！:%s, Check[%s]" % ( str(e), product_url) )
                    return
            
            #if not os.path.exists(result_path):
            #    os.makedirs(result_path) 
            result_file = "%s/%d_comm.txt"%(jd_config.JDSPR_RESULT_LOCAL, product_id)
            if os.path.exists(result_file):
                return
            
            #print ("产品保存地址：%s",result_path)    
            print ("评论线程[%d]正在处理商品 %d" % ( self.tid, product_id ))
            f = codecs.open(result_file, 'wb',encoding = 'utf-8')   
            f.write("产品名称：" + product_name.text + "\n")
        except Exception as e:
            print("3.提取产品名称和目录错误！:%s, Check[%s]" % ( str(e), product_url) )
            if result_file and os.path.exists(result_file):
                try:
                    os.remove(result_file)
                except:
                    pass
            return
            
        count = 0 
        retries = 0
        while  True:
            # random page url to avoid block
            product_comment_url = jd_comment_url % ( product_id, random.randint(3,10), page_id )
            #print ("=============> DOING... " + product_comment_url)
            flag = 0
            progress = "."
            while True:
                progress = progress + "."
                try:
                    self.agent = random_jd_header(product_comment_url)
                    request = urllib.request.Request(product_comment_url, headers = self.agent)
                    g_response = urllib.request.urlopen(request)
                    
                    if g_response.info().get('Content-Encoding') == 'gzip':
                        g_read = zlib.decompress(g_response.read(), 16+zlib.MAX_WBITS)
                    else:    
                        g_read = g_response.read()
                    
                    comment_html = jd_utils.encoding(g_read)
                
                    #操作正常
                    break
                except UnicodeDecodeError:
                    print ("GBK/Unicode编解码错误!")
                    f.close()
                    return
                except http.client.IncompleteRead:
                    continue
                except Exception as e:
                    if flag > 3 :
                        print ("评论线程[%d]网络异常，放弃该产品" % self.tid) 
                        f.close()
                        return
                    if e.errno == errno.ECONNRESET:
                        flag = flag + 1
                        time.sleep(2)
                        print ("评论线程[%d]重试中...[%d]" % ( self.tid, flag) )
                        continue
                    print ("2.其它异常:"+str(e))
                    f.close()
                    return
            
            comment_soup = BeautifulSoup(comment_html)
            count_t = self.get_page_comment(comment_soup, product_comment_url , f)
            
            # Retry about max 10 times here:
            # I hate JD
            lucky_flag = 1
            if count_t == 0:
                if retries < 10:
                    retries = retries + 1
                    if count != 0:
                        # Refresh user agent
                        self.agent = random_jd_header(product_comment_url)
                        time.sleep( random.randint(3, 9))
                    print("评论线程[%d] R[%d] %s" %( self.tid, retries, product_comment_url))               
                    continue
                else:
                    lucky_flag = 0
            
            retries = 0
            count = count + count_t
            
            if count == 0 and progress == "..":
                print("评论线程[%d] - 商品咨询为空，删除商品文件:%s" %( self.tid , result_file))
                if result_file and os.path.exists(result_file):
                    try:
                        os.remove(result_file)
                    except:
                        pass
                return                  
            
                           
            pagination = comment_soup.find('div', attrs = {"class":"pagin fr"})
            if not pagination:
                break
            if not pagination.findAll('a',attrs = {"class":"next"}) :
                break
            else:
                page_id = page_id + 1;
                f.flush()
            
        print ("评论线程[%d]处理完毕，产品[%d]，评论[%d]，LUCK[%s]，PATH[%s]" % ( self.tid, product_id, count, lucky_flag, result_path ))
        f.close()
        return (product_url, count, lucky_flag, result_path, product_id)
        
    def get_page_comment(self, page_soup, debug_url, store = 0):
        count = 0
        result_str = ""
        comm_part = page_soup.find('div', attrs = {"class":"m", "id":"comments-list" })
        if not comm_part:
            print("评论线程[%d]无法找到评论部分..." % ( self.tid ))
            return 0
        liResult = comm_part.findAll('div', attrs = {"class":"mc", "id":re.compile(r"comment-.*") })
        if not liResult:
            return 0
        for comm in liResult:
            comm_content = comm.find('div', attrs = {"class": "comment-content"})
            if comm_content:
                dl_content = comm_content.findAll('dl')
                for dl_item in dl_content:
                    if dl_item.dt and dl_item.dt.string and dl_item.dt.string == '心　　得：':
                        if dl_item.dd.string:
                            strs = dl_item.dd.string.strip()
                            if strs:
                                result_str = result_str + strs + "\n"
                                count = count + 1
        if(store):
            store.write(result_str)
        else:
            print (result_str)
        return count

