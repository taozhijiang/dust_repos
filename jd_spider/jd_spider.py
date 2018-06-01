#!/usr/bin/python -*- coding: utf-8 -*- encoding=utf-8
from jd_db import Jd_Db
import jd_config
import jd_logger

import urllib.request
import urllib.error
import urllib
import codecs
import zlib
import re
import os
import sys
import jd_utils
import threading
import time
from bs4 import BeautifulSoup
import random

import errno

import http.client



exitFlag = 0
gdb_lock = threading.RLock()

jd_item_url = "http://item.jd.com/%d.html"
jd_consult_url = "http://club.jd.com/allconsultations/%d-%d-1.html"
jd_comment_url = "http://club.jd.com/review/%d-3-%d-0.html"
jd_headers = {"Referer":"http://www.jd.com/",
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


result_path = jd_config.JDSPR_RESULT

__metaclass__ = type

jdspr_log = jd_logger.Jd_Logger("JD_SPR")

class UrlExtendThread(threading.Thread):
    def __init__(self, threadID):
        threading.Thread.__init__(self)
        self.threadID = threadID
        
    def run(self):
        print ("启动线程 %d ...\n" % self.threadID)
        jdb = Jd_Db(jd_config.SQLITE_DB)
        while True:
            if jdb.db_unprocess_count() > 200000:                
                #print ("系统负载重，暂停展开网页...\n")
                time.sleep(60)
                continue 
                
            with gdb_lock:
                while True:
                    full_url = jdb.db_query_extend()
                    if full_url:
                        #if re.match(r'^http://(help|red|tuan|auction|jr|smart|gongyi|app|en|media|m|myjd|chat|read|chongzhi|z|giftcard|fw|you|mobile).jd.com', full_url) or re.match(r'^http://www.jd.com/compare/', full_url) or re.match(r'^http://club.jd.com/consultation/', full_url) :
                        #    print("线程[%d]正在处理：%s [删除]" % (self.threadID, full_url) )
                        #    jdb.db_drop_rubbish(full_url)
                        #else:
                        #    break
                        break
                    
            print("线程[%d]正在处理：%s" % (self.threadID, full_url) )
            get_product_ids(full_url, jdb, self.threadID)
                    
        print ("退出线程 %d ..." % self.threadID)


class ConosultThread(threading.Thread):
    def __init__(self, threadID):
        threading.Thread.__init__(self)
        self.threadID = threadID
        
    def run(self):
        print ("启动商品咨询线程 %d ...\n" % self.threadID)
        jda = JdAnysisConsult(self.threadID)
        jdb = Jd_Db(jd_config.SQLITE_DB)
        while True:
            with gdb_lock:
                full_url = jdb.db_query_process()
                
            if full_url:
                jda.get_product_consults(full_url)
            else:
                print("咨询线程[%d]提取产品为空，等待..." % self.threadID)
                time.sleep(10)
	    
        print ("退出商品咨询线程 %d ..." % self.threadID)

        
class JdAnysisConsult:
    def __init__(self, tid = 0):
        self.tid = tid
        self.agent = None
        pass
    
    def get_product_consults(self, product_url):
        result_path = jd_config.JDSPR_RESULT
        self.agent = None
        page_id = 1    
        product_id = int(product_url.split('.')[2].split('/')[1])
        product_url = jd_item_url % product_id
        
        flag = 0
        while True:
            try:
                self.agent = random_jd_header(product_url)
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
                    print ("咨询线程[%d]网络异常，放弃该产品" % self.tid) 
                    return
                if e.errno == errno.ECONNRESET:
                    flag = flag + 1
                    time.sleep(10)
                    print ("咨询线程[%d]重试中...[%d]"%( self.tid, flag) )
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
            
            if not os.path.exists(result_path):
                os.makedirs(result_path) 
            result_file = "%s/%d.txt"%(result_path,product_id)
            if os.path.exists(result_file):
                return
            
            #print ("产品保存地址：%s",result_path)    
            print ("咨询线程[%d]正在处理商品 %d" % ( self.tid, product_id ))
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
        while  True:
            product_consult_url = jd_consult_url % ( product_id, page_id )
            flag = 0
            progress = "."
            while True:
                progress = progress + "."
                try:
                    self.agent = random_jd_header(product_url)
                    request = urllib.request.Request(product_consult_url, headers = self.agent)
                    g_response = urllib.request.urlopen(request)
                    if g_response.info().get('Content-Encoding') == 'gzip':
                        g_read = zlib.decompress(g_response.read(), 16+zlib.MAX_WBITS)
                    else:    
                        g_read = g_response.read()
                
                    consult_html = jd_utils.encoding(g_read)
                
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
                        print ("咨询线程[%d]网络异常，放弃该产品" % self.tid) 
                        f.close()
                        return
                    if e.errno == errno.ECONNRESET:
                        flag = flag + 1
                        time.sleep(10)
                        print ("咨询线程[%d]重试中...[%d]" % ( self.tid, flag) )
                        continue
                    print ("2.其它异常:"+str(e))
                    f.close()
                    return
            
            consult_soup = BeautifulSoup(consult_html)
            count = count + self.get_page_consult(consult_soup, f)
            if count == 0 and progress == "..":
                print("咨询线程[%d] - 商品咨询为空，删除商品文件:%s" %( self.tid , result_file))
                if result_file and os.path.exists(result_file):
                    try:
                        os.remove(result_file)
                    except:
                        pass
                return
                
            pagination = consult_soup.find('div', attrs = {"class":"Pagination"})
            if not pagination:
                break;
            if not pagination.findAll('a',attrs = {"class":"next"}) :
                break;
            else:
                page_id = page_id + 1;
                f.flush()
            
        print ("咨询线程[%d]处理完毕，咨询[%d] %d" % ( self.tid, count, product_id ))
        f.close()
        
    def get_page_consult(self, page_soup, store = 0):
        count = 0
        result_str = ""
        liResult = page_soup.findAll('div', attrs = {"class":"Refer_List" })
        for consult in liResult:
            consult_EntryArray = consult.findAll('div', attrs = {"class": ["refer refer_bg", "refer"]})
            for consult_item in consult_EntryArray:
                ask_anwser = consult_item.findAll('dl', attrs = {"class": ["ask", "answer"]})
                if ask_anwser and ask_anwser[0].dd.a.string and ask_anwser[1].dd.string :
                    ask = ask_anwser[0].dd.a.string.strip()
                    anw = ask_anwser[1].dd.string.strip()   
                    tail_t = anw.find("感谢您对京东的支持！祝您购物愉快！")
                    if tail_t > 0:
                        anw = anw[:tail_t]
                        result_str = result_str + "?>" + ask + "\n=>" + anw + "\n"               
                        count = count + 1
        if(store):
            store.write(result_str)
        else:
            print (result_str)
        return count

        
def get_product_ids(url, jdb, tid):
    flag = 0
    while True:
        try:
            request = urllib.request.Request(url, headers = jd_headers)
            g_response = urllib.request.urlopen(request)
            if g_response.info().get('Content-Encoding') == 'gzip':
                g_read = zlib.decompress(g_response.read(), 16+zlib.MAX_WBITS)
            else:    
                g_read = g_response.read()
                
            url_html = jd_utils.encoding(g_read)
            
            url_soup = BeautifulSoup(url_html)
            url_extend = url_soup.findAll('a', attrs = {"href": re.compile(r"^http://\w+.jd.com/.+\.(htm|html)$")})
            break
        except http.client.IncompleteRead:
            continue
        except Exception as e:
            if flag > 3 :
                print ("网络异常，放弃展开URL") 
                return
            if e.errno == errno.ECONNRESET:
                flag = flag + 1
                time.sleep(20)
                print ("重试中...[%d]" % flag)
                continue
            print ("展开产品链接异常:" + str(e) )
            return
    
    prds = []
    no_prds = []
    for url_item in url_extend:
        url_str = url_item.get("href")
        m = re.match(r'^http://item.jd.com/\d+.html$', url_str)
        if m:
            #jdb.db_insert_product(m.string)
            prds.append( m.string )           
        else:
            #("http://red.jd.com/", "http://tuan.jd.com/", "http://auction.jd.com/", "http://jr.jd.com/", "http://smart.jd.com/")
            if not re.match(r'^http://(help|red|tuan|auction|jr|smart|gongyi|app|en|media|m|myjd|chat|read|chongzhi|z|giftcard|fw|you|mobile|wiki|me).jd.com', url_str) and not re.match(r'^http://www.jd.com/compare/', url_str) and not re.match(r'^http://club.jd.com/consultation/', url_str) :
                no_prds.append( url_str )              
                #with gdb_lock:
                #    jdb.db_insert_no_product(url_str)
        
    # Really need to do with database
    if prds or no_prds :
        with gdb_lock:
            print ('线程[%d] 插入数据库...' % tid)
            for item in prds:
                jdb.db_insert_product(item)
            for item in no_prds:
                jdb.db_insert_no_product(item)

