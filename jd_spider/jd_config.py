#!/usr/bin/env python
# coding:utf-8
# manning  2015-6-11
import os
import random

PWD = os.getcwd()
PRJ_PATH = PWD+"/jd_spider_data/"
SQLITE_PATH = PRJ_PATH + "/database/"
SQLITE_DB = "jd_db.db"

# New distribute configure
# For node use to upload results
SERVER_ADDR = "192.3.90.124"
# SERVER_ADDR = "192.168.1.163"
# SERVER_ADDR = "127.0.0.1"
SERVER_PORT = 12399
SERVER_USER = "fakeuser"
SERVER_PASS = "fakeuser"

# 服务器最终汇总评论结果的路径
JDSPR_RESULT = PRJ_PATH+"/www.jd.com"
# 客户端用来临时存储抓取结果的路径
JDSPR_RESULT_LOCAL = PRJ_PATH+"/jd_local"
# 服务器分发给客户端，用户客户端SCP上传结果的路径
JDSPR_RESULT_SERVER = PRJ_PATH+"/jd_server"

JDSPR_START_URL = "http://channel.jd.com/electronic.html"

# debug mode setting - logs info msgs
DEBUG_SW = True

# logging settings
LOG_PATH = PRJ_PATH
LOG_FORMAT = "%(levelname)s [%(class)s.%(method)s]: %(message)s (%(asctime)-15s)"
LOG_FILE_NAME = "jd_spider.log"




IGNORE_EXT = ('js','css','png','jpg','gif','bmp','svg','exif',\
            'jpeg','exe','doc','docx','ppt','pptx','pdf','ico',\
            'wmv','avi','swf','apk','xml','xls','thmx')


USER_AGENTS = [
    "Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1; AcooBrowser; .NET CLR 1.1.4322; .NET CLR 2.0.50727)",
    "Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 6.0; Acoo Browser; SLCC1; .NET CLR 2.0.50727; Media Center PC 5.0; .NET CLR 3.0.04506)",
    "Mozilla/4.0 (compatible; MSIE 7.0; AOL 9.5; AOLBuild 4337.35; Windows NT 5.1; .NET CLR 1.1.4322; .NET CLR 2.0.50727)",
    "Mozilla/5.0 (Windows; U; MSIE 9.0; Windows NT 9.0; en-US)",
    "Mozilla/5.0 (compatible; MSIE 9.0; Windows NT 6.1; Win64; x64; Trident/5.0; .NET CLR 3.5.30729; .NET CLR 3.0.30729; .NET CLR 2.0.50727; Media Center PC 6.0)",
    "Mozilla/5.0 (compatible; MSIE 8.0; Windows NT 6.0; Trident/4.0; WOW64; Trident/4.0; SLCC2; .NET CLR 2.0.50727; .NET CLR 3.5.30729; .NET CLR 3.0.30729; .NET CLR 1.0.3705; .NET CLR 1.1.4322)",
    "Mozilla/4.0 (compatible; MSIE 7.0b; Windows NT 5.2; .NET CLR 1.1.4322; .NET CLR 2.0.50727; InfoPath.2; .NET CLR 3.0.04506.30)",
    "Mozilla/5.0 (Windows; U; Windows NT 5.1; zh-CN) AppleWebKit/523.15 (KHTML, like Gecko, Safari/419.3) Arora/0.3 (Change: 287 c9dfb30)",
    "Mozilla/5.0 (X11; U; Linux; en-US) AppleWebKit/527+ (KHTML, like Gecko, Safari/419.3) Arora/0.6",
    "Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US; rv:1.8.1.2pre) Gecko/20070215 K-Ninja/2.1.1",
    "Mozilla/5.0 (Windows; U; Windows NT 5.1; zh-CN; rv:1.9) Gecko/20080705 Firefox/3.0 Kapiko/3.0",
    "Mozilla/5.0 (X11; Linux i686; U;) Gecko/20070322 Kazehakase/0.4.5",
    "Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.9.0.8) Gecko Fedora/1.9.0.8-1.fc10 Kazehakase/0.5.6",
    "Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/535.11 (KHTML, like Gecko) Chrome/17.0.963.56 Safari/535.11",
    "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_7_3) AppleWebKit/535.20 (KHTML, like Gecko) Chrome/19.0.1036.7 Safari/535.20",
    "Opera/9.80 (Macintosh; Intel Mac OS X 10.6.8; U; fr) Presto/2.9.168 Version/11.52",
    "Mozilla/5.0 (Windows NT 6.1; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/42.0.2311.152 Safari/537.36"
    "Mozilla/5.0 (Windows NT 6.1; WOW64; rv:37.0) Gecko/20100101 Firefox/37.0",
]

def random_agent():
    return random.choice(USER_AGENTS)
