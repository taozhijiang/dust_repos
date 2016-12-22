#!/usr/bin/python3

import zlib
from bs4 import BeautifulSoup
from seleniumrequests import Firefox 

#import torndb
import time

import urllib.request
import urllib.error
import urllib
import urllib3

import re
import os
import sys

import socket

from tornado.options import define, options
define("dbhost", default="192.168.1.6", help="database host name/ip")
define("dbname", default="v5_law", help="database name")
define("dbuser", default="root", help="database username")
define("dbpass", default=None, help="database passwd")
define("dbtimezone", default="+8:00")

#db_conn = torndb.Connection("192.168.1.6", "v5_law", "root");

item_prefix = 'http://www.hshfy.sh.cn/shfy/gweb/flws_view.jsp?pa='
def track_info(item):
	ud = item.get('onclick')
	ud = ud[len('showone(\''):-2]
	url = item_prefix + ud
	req = urllib.request.Request(url)
	bodys = None
	while True:
		try:
			response = urllib.request.urlopen(req, timeout=10)
		except socket.timeout:
			print("Request time out: " + url )
			continue
			
		if response.info().get('Content-Encoding') == 'gzip':
			r_read = zlib.decompress(response.read(), 16+zlib.MAX_WBITS)
		else:    
			r_read = response.read()
			
		soup = BeautifulSoup(r_read.decode('gbk'))
		bodys = soup.find('div', attrs={"class":"wsTable"})
		if not bodys:
			print("Error1:" + url)
			sys.exit()
		
		break
	
	print(bodys)
	tds = item.findAll('td')
	ah = tds[0].text
	bt = tds[1].text
	lb = tds[2].text
	ay = tds[3].text
	cb = tds[4].text
	jb = tds[5].text
	ri = tds[6].text
	print(ah)
	print(bt)
	print(lb)
	print(ay)
	print(cb)
	print(jb)
	print(ri)

	pass

if __name__ == "__main__":
	url_prefix = "http://www.hshfy.sh.cn/shfy/gweb/flws_list_content.jsp"
	page_id = 1
	
	while True:
		print("Current page: %d" % page_id)
		url = url_prefix 
		
		webdriver = Firefox()
		response = webdriver.request('POST', url_prefix, data={'fydm':'200', 'ajlb':'%E6%B0%91%E4%BA%8B', 'pagesnum':'2'})
		r_read = response.text

		soup = BeautifulSoup(r_read)
		info_soup = soup.findAll('tr', attrs={"style":"cursor:hand"})
		if info_soup:
			print("LEN: %d" %(len(info_soup)))
			for item in info_soup:
				track_info(item)
		
		webdriver.close()
		page_id += 1
		if page_id > 234:
			break
	
	db_conn.close()
	print("Done!")
