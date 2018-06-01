#!/usr/bin/python3
#-*- coding: utf-8 -*-
#encoding=utf-8

import time
import sys
import os
import re
import urllib.request
import urllib.error
import urllib.parse
import urllib

import zlib

import codecs

from db import SearchDB

import cgi

SELF_HEAD = {"Host":"www.google.com",
               "Referer":"https://www.google.com/",
               "Connection":"keep-alive",
               "Accept-Encoding":"gzip, deflate",
               "User-Agent":"Mozilla/5.0 (Windows NT 6.1; WOW64; rv:37.0) Gecko/20100101 Firefox/37.0",
               }

def encoding(data):
    types = ['utf-8', 'gb18030',  'gb2312','gbk','iso-8859-1', 'big5', 'ascii', 'utf-16', 'hz', 'iso2022_jp_2', 'big5hkscs', 'cp950', 'str']   #可以添加其他字符编码
    for typ in types:
        try:
            return data.decode(typ).encode("utf-8")
        except:
            pass
    return None

print('Content-type:text/html; charset=utf-8')
print()


cache_params = cgi.FieldStorage()
cache_url = cache_params.getvalue("url")
cache_url = cache_url.strip()
if cache_url[0] == "'":
    cache_url = cache_url[1:]
if cache_url[-1] == "'":
    cache_url = cache_url[0:cache_url.length-1]
    
prot_name, rest = urllib.parse.splittype(cache_url)  
host_name, rest = urllib.parse.splithost(rest)  
SELF_HEAD['Host'] = host_name
SELF_HEAD['Referer'] = prot_name+"://"+host_name+"/"
    
try:
    request = urllib.request.Request(cache_url, headers = SELF_HEAD)
    g_response = urllib.request.urlopen(request)

    if g_response.info().get('Content-Encoding') == 'gzip':
        g_read = zlib.decompress(g_response.read(), 16+zlib.MAX_WBITS)
    else:    
        g_read = g_response.read()
except Exception as e:
    print ("异常:"+str(e))
    
    
if 'HTTP_X_FORWARDED_FOR' in os.environ.keys():
    ip = os.environ['HTTP_X_FORWARDED_FOR']
elif 'REMOTE_ADDR' in os.environ.keys():
    ip = os.environ['REMOTE_ADDR']
else:  
    ip = ""
    
db = SearchDB()
#db.db_insert_cache(ip, cache_url)
    
match = re.search(r"""(?<![-\w])                  #1
                      (?:(?:en)?coding|charset)   #2
                      (?:=(["'])?([-\w]+)(?(1)\1) #3
                      |:\s*([-\w]+))""".encode("utf8"),
                                       g_read, re.IGNORECASE|re.VERBOSE)
encoding = match.group(match.lastindex) if match else b"utf8"
    
print (g_read.decode(encoding.decode("UTF-8"),'ignore'))

#db = SearchDB()
#db.db_insert(search_keywords, ip)
#db.db_dump()
#db.db_statistics()
