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
	head['Host'] = 'www.zjsfgkw.cn'
	head['Cache-Control'] = 'max-age=0'
	head['DNT'] = '1'
	head['Accept'] = 'text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8'
	head['Accept-Encoding'] = 'gzip, deflate, sdch'
	head['Accept-Language'] = 'zh-CN,zh;q=0.8,en;q=0.6,ja;q=0.4'
	return head

def request_body(url):
	if url in skip_list:
		return '【内容为空】';
		
	while True:
		ret = ""	
		req = urllib.request.Request(url, headers=create_header())
		try:
			response = urllib.request.urlopen(req, timeout=10)
		except socket.timeout:
			print("Request time out: " + url )
			continue
			
		if response.info().get('Content-Encoding') == 'gzip':
			r_read = zlib.decompress(response.read(), 16+zlib.MAX_WBITS)
		else:    
			r_read = response.read()
			
		soup = BeautifulSoup(r_read.decode('utf-8'), 'lxml')
		bodys = soup.find('div', attrs={"class":"txt_txt"})
		if not bodys:
			print("Error1:" + url)
			sys.exit()
		body = bodys.findAll('div')
		if body:
			for item in body:
				if item and item.string:
					ret += item.string.strip() + "\n"
			if len(ret) > 100:
				break
				
		body = bodys.findAll('p')
		if not body:
			print("Error2:" + url)
			sys.exit()
		for item in body:
			if item and item.text:
				ret += item.text.strip() + "\n"
		if len(ret) > 100:
			break
		
		print("Retry: " + url)
		
	return ret

def track_info(info_soup):
	dat = json.loads(info_soup)
	for item in dat['list']:
		#print(item)
		link = 'http://www.zjsfgkw.cn/document/JudgmentDetail/'+repr(item['DocumentId'])
		print(link)
	return	
	

if __name__ == "__main__":
	
	header = create_header()
	url_prefix = "http://www.zjsfgkw.cn/document/JudgmentSearch"
	page_id = 1
	
	while True:
		print("Current page: %d" % page_id)
		req = urllib.request.Request(url_prefix, headers=header, data=('pagesize=10&ajlb=%E6%B0%91%E4%BA%8B&cbfy=1300&ah=&jarq1=20060101&jarq2=20161222&key=&pageno='+repr(page_id)).encode('ascii'))
		try:
			response = urllib.request.urlopen(req, timeout=10)
		except socket.timeout:
			print("Request time out: " + url )
			continue
			
		if response.info().get('Content-Encoding') == 'gzip':
			r_read = zlib.decompress(response.read(), 16+zlib.MAX_WBITS)
		else:    
			r_read = response.read()
		
		info_soup = r_read.decode('utf-8')
		if info_soup:
			track_info(info_soup)
		
		page_id += 1
		if page_id > 596:
			break
	
	#db_conn.close()
	print("Done!")
