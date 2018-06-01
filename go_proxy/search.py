#!/usr/bin/python3
#-*- coding: utf-8 -*-
#encoding=utf-8

import time
import sys
import os
import re
from bs4 import BeautifulSoup
import urllib.request
import urllib.error
import urllib.parse
import urllib

import zlib 

from db import SearchDB

import cgi

print('Content-type:text/html; charset=utf-8')
print()

GOOGLE_URL = "https://www.google.com/search" 
#GOOGLE_URL = "https://www.google.com/search?q=%s" 
GOOGLE_HEAD = {"Host":"www.google.com",
              "Referer":"https://www.google.com/",
              "Connection":"keep-alive",
              "Accept-Encoding":"gzip, deflate",
              "User-Agent":"Mozilla/5.0 (Windows NT 6.1; WOW64; rv:37.0) Gecko/20100101 Firefox/37.0",
              }

    
htmls_head = """
<!DOCTYPE html>
<html xmlns="http://www.w3.org/1999/xhtml" lang="zh-CN" xml:lang="zh-CN">
<head>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
<title>FreeSign, Search What You Want!</title>
<link rel="icon" href="https://freesign.net/favicon.ico" type="image/x-icon" />
<link rel="shortcut icon" href="https://freesign.net/favicon.ico" type="image/x-icon" />
</head>
<body style="background-color:#EEEEEE ">
"""

htmls_style = """
<style type="text/css"> 

.g_content_items {   
    font-family: tahoma, 'microsoft yahei', 微软雅黑;
}

.lsbb {
  background-image: -moz-linear-gradient(top,#4d90fe,#4787ed);
  background-image: -ms-linear-gradient(top,#4d90fe,#4787ed);
  background-image: -o-linear-gradient(top,#4d90fe,#4787ed);
  background-image: -webkit-gradient(linear,left top,left bottom,from(#4d90fe),to(#4787ed));
  background-image: -webkit-linear-gradient(top,#4d90fe,#4787ed);
  background-image: linear-gradient(top,#4d90fe,#4787ed);
  border: 1px solid #3079ed;
  border-radius: 2px;
  background-color: #4d90fe;
  height: 30px;
  width: 68px;
}

em {
    color: red
}

f1 {
    font-family: tahoma, 'microsoft yahei', 微软雅黑;
    font-size:25px;
}

div#container{	width:900px;   
                font-family: tahoma, 'microsoft yahei', 微软雅黑;
				position:absolute;
				left:60px;}
div#header {    background-color:#99bbbb;}
div#content {   background-color:#C7EDCC; 
                float:left;}
div#footer {    background-color:#99bbbb; 
                clear:both;
                font-family: sansserif;
                text-align:center;
                height:40px;
                }

</style> 
"""

htmls_search_e = """
<form action="/search.py" method="GET" align=left>
    <span style="font-size:17px"><a href="./index.htm">回到首页</a></span>
    <input type='text' name='q' value='%s' style="background-color:#EEEEEE;height:20px;width:300px;font-size:15px"/>
    <input class='lsbb' type='submit' name='Search' value='搜一下' >
</form>
"""

htmls_body2 = """
    <span>搜索字段为空，请不要骗我...</span>
"""


htmls_end = """    
</body>
</html>
"""


search_params = cgi.FieldStorage()
search_keywords = search_params.getvalue("q")
if search_keywords:
    search_keywords = search_keywords.strip()
    
htmls_search = htmls_search_e % " "
#如果搜索字段为空
if not search_keywords:
    print(htmls_head)
    print(htmls_style)
    print('<div id="container">')
    print('<a href="./index.htm"> <img hidefocus="true" src="./freesign.jpg" height="24" /> </a>')
    print('<a href="http://blog.freesign.net"> 我的博客</a>  <br/>')
    print('<div id="header">')    
    print(htmls_search)
    print('</div>')
    print('<div id="content">')
    print(htmls_body2)
    print('</div>') 
    print('</div>') 
    print(htmls_end)
    os.exit(0)

    
search_start = search_params.getvalue("start", default="0")
search_q = urllib.parse.urlencode({'q':search_keywords, 'start': search_start})  

google_q = GOOGLE_URL + '?' + search_q


htmls_search = htmls_search_e % search_keywords

try:
    request = urllib.request.Request(google_q, headers = GOOGLE_HEAD)
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
    
soup = BeautifulSoup(g_read)
stats = soup.find('div', attrs={"id":"resultStats"})
#g_group = soup.find('div', attrs={"class":"srg"})
g_group = soup.find('div', attrs={"id":"ires"})
g_items = g_group.findAll('div', attrs={"class":"g"})
navs = soup.find('table', attrs={"id":"nav"})

db = SearchDB()

if ip:
    htmls_body1 = """
	<span>欢迎使用[%s](第%d次使用者)，搜索关键词：%s，搜索状态：%s</span>
    """ % (ip, db.db_statistics()/2, search_keywords, stats.text)
else:
    htmls_body1 = """
	<span>欢迎使用，搜索关键词：%s，搜索状态：%s</span>
    """ % (search_keywords, stats.text)

db.db_insert_search(ip, search_keywords)
#db.db_dump()
#db.db_statistics()


print(htmls_head)
print(htmls_style)
print('<div id="container">')
print('<a href="./index.htm"> <img hidefocus="true" src="./freesign.jpg" height="24" /> </a>')
print('<a href="http://blog.freesign.net"> 我的博客</a>  <br/>')
print('<div id="header">')
print(htmls_search)
print('</div>')
print('<div id="content">')
print(htmls_body1)
print ('<h4>搜索结果:</h4>')


#for item, item_desc in zip(items, items_desc):
for g_item in g_items:
    if g_item:
        item = g_item.find('h3', attrs={"class":"r"})
        item_desc = g_item.find('span', attrs={"class":"st"})
        item_caches = g_item.findAll('li', attrs={"class":"action-menu-item ab_dropdownitem"})
        print ( '<div class="g_content_items">' )
        print ( '<span style="font-size:16px"><a href=' + item.a["href"] + '>' + item.a.text + '</a></span>')
        print ( '<br />' )
        print ( '<span style="word-break:break-all;font-family:Arial;color:#093">' + urllib.parse.unquote(item.a["href"]) + '</span>')
        print ( '<br />' )
        print ( '<span> <a style="font-size:16px" target="_blank" href="./cache.py?' + urllib.parse.urlencode({'url': item.a["href"]}) + '"> 页面快照&nbsp;</a></span>')
        if item_caches:
            for i_cache in item_caches:
                # Here, because we can not access http://webcache.googleusercontent.com/ until
                # we crack there anti-robot ...
                if i_cache:
                    if not re.match(r'^(http|https)://',i_cache.a["href"]) and re.match(r'^/search?',i_cache.a["href"]):
                        print ('<span><a style="font-size:16px" href=' + repr(i_cache.a["href"])+'>' + '&nbsp;类似结果&nbsp;' + '</a></span>')
        print ( '<br />' )
        print ( '<span style="font-size:14px;line-height:18px;font-family:Tahoma">')
        print ( item_desc )
        print ( '</span>' )
        print ( '<br />' )
        print ( '<br />' )
        print ( '</div>' )
    
if navs:
    print('<div id="footer">')
    tabs = navs.findAll('td')
    if tabs:
        print('<table align="center" ><tr style="text-align:center; font-size:18px">')
        for tab in tabs:
            if tab:
                print(tab)
        print('</table></tr>')
    print('</div>') 

print('</div>') 
print('</div>') 
print(htmls_end)

