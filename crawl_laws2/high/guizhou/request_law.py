#/usr/bin/python3

import zlib

import urllib.request
import urllib.error
import urllib

from bs4 import BeautifulSoup
import urllib3

#import torndb
import time

import re
import os
import sys

import socket
import json

from tornado.options import define, options
define("dbhost", default="192.168.1.6", help="database host name/ip")
define("dbname", default="v5_law", help="database name")
define("dbuser", default="root", help="database username")
define("dbpass", default=None, help="database passwd")
define("dbtimezone", default="+8:00")

#db_conn = torndb.Connection("192.168.1.6", "v5_law", "root");

def create_header():
	head = urllib3.util.make_headers(keep_alive=True, accept_encoding="gzip, deflate", user_agent='Mozilla/5.0 (Linux; Android 6.0; Nexus 5 Build/MRA58N) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/46.0.2490.76 Mobile Safari/537.36', basic_auth=None)
	head['Host'] = '58.16.66.85:81'
	head['Cache-Control'] = 'max-age=0'
	head['DNT'] = '1'
	head['Accept'] = 'text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8'
	head['Accept-Encoding'] = 'gzip, deflate'
	head['Accept-Language'] = 'zh-CN,zh;q=0.8,en;q=0.6,ja;q=0.4'
	head['Content-Type'] = 'application/x-www-form-urlencoded'
	head['Referer'] = 'http://58.16.66.85:81/susong51/fymh/3250/cpws.htm?st=1&q=&ajlb=1&wszl=&jbfy=3250&ay=&ah=&startCprq=&endCprq=&page=2'
	head['Cookie'] = 'JSESSIONID=3D480DCDBAD94BA36A606F46C335A129.lb1sswy3'
	return head
	
def request_body(url):
	return

def track_info(info_soup):
	trs = info_soup.findAll('tr', attrs={"class":"tr_stripe"})
	for item in trs:
		tds = item.findAll('td')
		print(tds[1].text.strip())
		print(tds[2].text.strip())
		print(tds[3].text.strip())
		
		a = tds[1].get('onclick')
		link = "http://58.16.66.85:81/susong51/cpws/paperView.htm?id="+a[len('javascript:cpwsDetail(\''):a.index('\');')]
		print(link)
		
	return	
	

if __name__ == "__main__":
	
	header = create_header()
	url_prefix = "http://58.16.66.85:81/susong51/fymh/3250/cpws.htm?st=1&q=&ajlb=1&wszl=&jbfy=3250&ay=&ah=&startCprq=&endCprq=&page="
	page_id = 1
	
	while True:
		print("Current page: %d" % page_id)
		req = urllib.request.Request(url_prefix+repr(page_id), headers=header)
		try:
			response = urllib.request.urlopen(req)
			if response.info().get('Content-Encoding') == 'gzip':
				r_read = zlib.decompress(response.read(), 16+zlib.MAX_WBITS)
			else:    
				r_read = response.read()
		except urllib.error.HTTPError as e:
			print(e.code)
			print(e.read())
		soup = BeautifulSoup(r_read.decode('utf-8'), 'lxml')
		info_soup = soup.find('table', attrs={"class":"fd_table_03 "})
		if info_soup:
			track_info(info_soup)
			
		page_id += 1
		# 一审 20*3
		# 二审 20*50 最多一千条
		if page_id > 3:
			break
	
	#db_conn.close()
	print("Done!")
