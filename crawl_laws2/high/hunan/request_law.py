#/usr/bin/python3

import zlib
import time

import urllib.request
import urllib.error
import urllib

from bs4 import BeautifulSoup
import urllib3

from selenium import webdriver

import torndb
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

db_conn = torndb.Connection("192.168.1.6", "v5_law", "root");

def create_header():
	head = urllib3.util.make_headers(keep_alive=True, accept_encoding="gzip, deflate", user_agent='Mozilla/5.0 (Linux; Android 6.0; Nexus 5 Build/MRA58N) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/46.0.2490.76 Mobile Safari/537.36', basic_auth=None)
	head['Host'] = 'hunanfy.chinacourt.org'
	head['Cache-Control'] = 'max-age=0'
	head['DNT'] = '1'
	head['Accept'] = 'text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8'
	head['Accept-Encoding'] = 'gzip, deflate'
	head['Accept-Language'] = 'zh-CN,zh;q=0.8,en;q=0.6,ja;q=0.4'
	return head

def request_body(url):
	ret = ""
	cnt = 0
	while True:
		browser = webdriver.PhantomJS()
		response = browser.get(url)
		content = browser.page_source
		soup = BeautifulSoup(content, 'lxml')
		bodys = soup.find('div', attrs={"class":"border_in"})
		if not bodys:
			print("Error1:" + url)
			return "【内容为空】"
			
		body = bodys.findAll('div')
		if not body:
			body = bodys.findAll('p')
			if not body:
				print("Error2:" + url)
				cnt += 1
				if cnt >= 10:
					return "【内容错误】"
				time.sleep(2)
				continue
		break
				
	for item in body:
		if item and item.text:
			ret += item.text.strip() + "\n"
		
	return ret	

def track_info(info_soup):
	items = info_soup.findAll('span', attrs={"class":None})
	for item in items:
		title = item.a.text
		link = "http://hunanfy.chinacourt.org"+item.a.get('href')
		
		neirong = request_body(link)
		neirong = db_conn.escape_string(neirong)
		
		print(title)
		try:
			sql = ''' INSERT INTO v5_high_court(`案件名`,`地区`,`url`,`文书内容`) VALUES( '{0}', '{1}', '{2}', '{3}'); '''
			sql = sql.format(title, '湖南', link, neirong)
			db_conn.execute(sql)
		except:
			print("SQL Error:" + sql)
			
	return	
	

if __name__ == "__main__":
	
	header = create_header()
	url_prefix = "http://hunanfy.chinacourt.org/paper/more/paper_mid/MzA0gAMA/page/%d.shtml"
	page_id = 1
	
	while True:
		print("Current page: %d" % page_id)
		url = url_prefix % page_id
		req = urllib.request.Request(url, headers=header)
		
		try:
			response = urllib.request.urlopen(req)
			if response.info().get('Content-Encoding') == 'gzip':
				r_read = zlib.decompress(response.read(), 16+zlib.MAX_WBITS)
			else:    
				r_read = response.read()
		except:
			print("Request url failed for page_id:%d " % page_id)
			print("Retry: " + url_prefix)
			continue
		
		soup = BeautifulSoup(r_read.decode('utf-8'), 'lxml')
		info_soup = soup.find('div', attrs={"id":"main"})
		if info_soup:
			track_info(info_soup)
		
		page_id += 1
		if page_id > 250:
			break
	
	#db_conn.close()
	print("Done!")
