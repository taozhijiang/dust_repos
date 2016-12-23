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
	head['Host'] = 'www.fjcourt.gov.cn'
	head['Origin'] = 'http://www.fjcourt.gov.cn'
	head['Cache-Control'] = 'max-age=0'
	head['DNT'] = '1'
	head['Accept'] = 'text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8'
	head['Accept-Encoding'] = 'gzip, deflate'
	head['Accept-Language'] = 'zh-CN,zh;q=0.8,en;q=0.6,ja;q=0.4'
	head['Content-Type'] = 'application/x-www-form-urlencoded'
	head['Referer'] = 'http://www.fjcourt.gov.cn/page/public/RefereeclericalMore.aspx?cate=0902'
	head['Cookie'] = 'yikikata=6e842253-10bd35ede5d87d5e4007b5e96c3e57e7; gsScrollPos=; brmidyrvj=both; ASP.NET_SessionId=fchvv0qvpgtt30ru33ywlmau; yikikata=6e851983-63d254046d07240f9cc590d02fdceb09; _gscu_1086777894=82203576fiuw3312; _gscs_1086777894=t82463382309mfm13|pv:19; _gscbrs_1086777894=1'
	return head
	
def request_body(url):
	return

def track_info(info_soup):
	print(info_soup)
	return	
	

if __name__ == "__main__":
	
	header = create_header()
	url_prefix = "http://www.fjcourt.gov.cn/page/public/RefereeclericalMore.aspx?cate=0902"
	page_id = 1
	
	header = create_header()
	req = urllib.request.Request(url_prefix, headers=header)
	response = urllib.request.urlopen(req)
	if response.info().get('Content-Encoding') == 'gzip':
		r_read = zlib.decompress(response.read(), 16+zlib.MAX_WBITS)
	else:    
		r_read = response.read()
	soup = BeautifulSoup(r_read.decode('utf-8'), 'lxml')
	this_stat = soup.find('input', attrs={"id":"__VIEWSTATE"})['value']
	info_soup = soup.find('ul', attrs={"class":"module-case-items"})
	if info_soup:
		track_info(info_soup)
	
	page_id = 2
	while True:
		print("Current page: %d" % page_id)
		body = {'__VIEWSTATE':this_stat, '__EVENTTARGET':'ctl00$cplContent$AspNetPager1', '__EVENTARGUMENT':repr(page_id), 'ctl00$cplContent$AspNetPager1_input':repr(page_id-1)}
		dat = urllib.parse.urlencode(body).encode('ascii')
		req = urllib.request.Request(url_prefix, headers=header, data=dat )
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
		this_stat = soup.find('input', attrs={"id":"__VIEWSTATE"})['value']
		info_soup = soup.find('ul', attrs={"class":"module-case-items"})
		if info_soup:
			track_info(info_soup)
			
		if soup.find('a', attrs={'id':"ess_ctr740_LawOfficeSearchList_lbtnNextPage", 'disabled':'disabled'}) :
			break
		
		page_id += 1
		if page_id > 42918:
			break
	
	#db_conn.close()
	print("Done!")
