#!/usr/bin/python3.4
from jd_db import Jd_Db
import jd_config

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
import json

import errno

import http.cookiejar


exitFlag = 0
gdb_lock = threading.RLock()

jd_item_url = "http://product.suning.com/%d.html"
jd_consult_url = "http://zone.suning.com/review/ajax/wcs_consulation/000000000%d-0070096878-false-%d-5-5-1-getConsultationItem.html"
jd_comment_url = "http://review.suning.com/ajax/review_lists/style-000000000%d--total-%d-default-20-----reviewList.htm"
jd_headers = {"Referer":"http://product.suning.com/",
              "Content-type": "application/x-www-form-urlencoded; charset=UTF-8",
              "Accept": "*/*",
              "Connection": "keep-alive",
              "Accept-Encoding":"gzip, deflate",
               "User-Agent":"Mozilla/5.0 (Windows NT 6.1; WOW64; rv:37.0) Gecko/20100101 Firefox/37.0",
#               "Cookie":"user-key=23013c2b-329d-422c-9aa1-d0b6600c187a; cn=0; ipLocation=%u5317%u4EAC; areaId=1; ipLoc-djd=1-72-2799-0; __jda=122270672.763825332.1431154352.1432700135.1432782483.14; __jdb=122270672.3.763825332|14.1432782483; __jdc=122270672; __jdv=122270672|jd.com|-|-|not set; __jdu=763825332"
               }

# 获取一个保存cookie的对象
cj = http.cookiejar.CookieJar()
# 将一个保存cookie对象，和一个HTTP的cookie的处理器绑定
cookie_support = urllib.request.HTTPCookieProcessor(cj)
# 创建一个opener，将保存了cookie的http处理器，还有设置一个handler用于处理http的URL的打开
opener = urllib.request.build_opener(cookie_support, urllib.request.HTTPHandler)
# 将包含了cookie、http处理器、http的handler的资源和urllib2对象板顶在一起
urllib.request.install_opener(opener)

def random_jd_header(refer = None):
    global jd_headers
    jd_headers['User-Agent'] = jd_config.random_agent()
    if refer:
        jd_headers['Referer'] = refer
    return jd_headers


result_path = jd_config.JDSPR_RESULT

__metaclass__ = type


class UrlExtendThread(threading.Thread):
    def __init__(self, threadID):
        threading.Thread.__init__(self)
        self.threadID = threadID

    def run(self):
        print ("启动线程 %d ...\n" % self.threadID)
        jdb = Jd_Db()
        while True:
            #if jdb.db_unprocess_count() > 200000:
            #    #print ("系统负载重，暂停展开网页...\n")
            #    time.sleep(60)
            #    continue

            with gdb_lock:
                while True:
                    full_url = jdb.db_query_extend()
                    if full_url:
                        break
                    else:
                        time.sleep(20)

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
        jdb = Jd_Db()
        while True:
            with gdb_lock:
                full_url = jdb.db_query_process()

            if full_url:
                jda.get_product_consults(full_url)
            else:
                print("咨询线程[%d]提取产品为空，等待..." % self.threadID)
                time.sleep(20)

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

                product_html = g_read #jd_utils.encoding(g_read)
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

        product_ts = None
        product_soup = BeautifulSoup(product_html.decode('utf-8', 'ignore'),"lxml")
        #product_name = product_soup.find('h1')

        #产品类别
        product_type = product_soup.find('div', attrs={"class":"breadcrumb"})
        if product_type:
            product_ts = product_type.findAll('a', attrs={"id":"category1"})

        #if not product_name or not product_ts:
        #    print("1.产品名称和类别提取错误，返回！Check[%s]" % product_url)
        #    print(self.agent['User-Agent'])
        #    return

        result_file = None
        try:
            i = 0
            for pt_item in product_ts:
                if pt_item:
                    pt_item = ''.join(pt_item.string.split('/'))
                    result_path = result_path + "/" + pt_item + "/"
                    i = i + 1
                    #目录类别的深度
                    if i > 3:
                        break
                else:
                    print("2.提取产品名称和目录错误！:%s, Check[%s]" % ( str(e), product_url) )
                    return

            if not os.path.exists(result_path):
                os.makedirs(result_path)
            result_file = "%s/%d.txt"%(result_path, product_id)
            result_short = "%s/%d.txt"%(jd_config.JDSPR_RESULT, product_id)
            if os.path.exists(result_file):
                return

            print ("咨询线程[%d]正在处理商品 %d" % ( self.tid, product_id ))
            f = codecs.open(result_file, 'wb',encoding = 'utf-8')
            #f.write("产品名称：" + product_name.text + "\n")
        except Exception as e:
            print("准备结果保存文件错误:%s, Check[%s]" % ( str(e), product_url) )
            if result_file and os.path.exists(result_file):
                try:
                    os.remove(result_file)
                except:
                    pass
            return

        count = 0
        retries = 0
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

                    consult_html = g_read 

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
                        time.sleep(random.randint(1,5))
                        #print ("咨询线程[%d]重试中...[%d]" % ( self.tid, flag) )
                        continue
                    print ("2.其它异常:"+str(e))
                    f.close()
                    return

            consult_soup = BeautifulSoup(consult_html.decode('utf-8', 'ignore'),"lxml")
            (page_all, count_t) = self.get_page_consult(consult_soup, f)
            
            if page_all == 0:
                if result_file and os.path.exists(result_file):
                    try:
                        os.remove(result_file)
                    except:
                        pass
                    return
            
            count = count + count_t
            if page_id >= page_all:
                break
            else:
                page_id += 1
            
        print ("咨询线程[%d]处理完毕，咨询[%d]，%d" % ( self.tid, count, product_id ))
        f.close()

    def get_page_consult(self, page_soup, store = 0):
        count = 0
        result_str = ""
        page_item = page_soup.find('p').string
        page_item = page_item[len('getConsultationItem(') : -1]
        DATA = eval(page_item)
        if DATA['totalCount'] == 0:
            return (0, 0)
        for item in DATA['consulationList']:
            tmp_str = 'Q:'+ item['content'] + '\n' + 'A:' + item['answer'] + '\n'
            #print(tmp_str)
            result_str += tmp_str
            count += 1
        if(store):
            store.write(result_str)
        else:
            print (result_str)
        return (DATA['totalCount'],count)


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

            url_html = g_read

            url_soup = BeautifulSoup(url_html.decode('utf-8', 'ignore'),"lxml")
            url_extend = url_soup.findAll('a', attrs = {"href": re.compile(r"^http://\w+.suning.com/.+\.(htm|html)$")})
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
        m = re.match(r'^http://product.suning.com/\d+.html$', url_str)
        if m:
            #jdb.db_insert_product(m.string)
            prds.append( m.string )
        else:
            #("http://red.jd.com/", "http://tuan.jd.com/", "http://auction.jd.com/", "http://jr.jd.com/", "http://smart.jd.com/")
            if not re.match(r'^http://(help|red|tuan|auction|jr|smart|gongyi|app|en|media|m|myjd|chat|read|chongzhi|z|giftcard|fw|you|mobile|wiki|me).suning.com', url_str) and not re.match(r'^http://www.suning.com/compare/', url_str):
                no_prds.append( url_str )
                #with gdb_lock:
                #    jdb.db_insert_no_product(url_str)

    # Really need to do with database
    if prds or no_prds :
        print ('线程[%d] 插入数据库...' % tid)
        for item in prds:
            jdb.db_insert_product(item)
        for item in no_prds:
            jdb.db_insert_no_product(item)

