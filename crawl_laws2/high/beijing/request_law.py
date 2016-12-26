#/usr/bin/python3

import zlib

import urllib.request
import urllib.error
import urllib

from bs4 import BeautifulSoup
import urllib3

import torndb
import time

import re
import os
import sys

import socket
from selenium import webdriver


from tornado.options import define, options
define("dbhost", default="192.168.1.6", help="database host name/ip")
define("dbname", default="v5_law", help="database name")
define("dbuser", default="root", help="database username")
define("dbpass", default=None, help="database passwd")
define("dbtimezone", default="+8:00")

db_conn = torndb.Connection("192.168.1.6", "v5_law", "root");

def create_header():
	head = urllib3.util.make_headers(keep_alive=True, accept_encoding="gzip, deflate", user_agent='Mozilla/5.0 (Linux; Android 6.0; Nexus 5 Build/MRA58N) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/46.0.2490.76 Mobile Safari/537.36', basic_auth=None)
	head['Host'] = 'bjgy.chinacourt.org'
	head['Cache-Control'] = 'max-age=0'
	head['DNT'] = '1'
	head['Accept'] = 'text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8'
	head['Accept-Encoding'] = 'gzip, deflate, sdch'
	head['Accept-Language'] = 'zh-CN,zh;q=0.8,en;q=0.6,ja;q=0.4'
	return head

def request_body(url):
	ret = ""
	
	browser = webdriver.PhantomJS()
	response = browser.get(url)
	content = browser.page_source
	soup = BeautifulSoup(content, 'lxml')
	bodys = soup.find('div', attrs={"class":"paper_content"})
	if not bodys:
		print("Error1:" + url)
		sys.exit()
	body = bodys.findAll('div')
	if not body:
		body = bodys.findAll('p')
		if not body:
			print("Error2:" + url)
			sys.exit()
			
	for item in body:
		if item and item.text:
			ret += item.text.strip() + "\n"
		
	return ret	
	
def track_info(info_soup):
	items = info_soup.findAll('li')
	for item in items:
		a = item.find('a')
		title = a.text
		link = 'http://bjgy.chinacourt.org/'+a.get('href')
		
		neirong = request_body(link)
		print(neirong)
		neirong = db_conn.escape_string(neirong)
		sql = ''' INSERT INTO v5_high_court(`案件名`,`地区`,`url`,`文书内容`) VALUES( '{0}', '{1}', '{2}', '{3}' ); '''
		sql = sql.format(title, '北京', link, neirong)
		db_conn.execute(sql)
		
	return	
	
	
if __name__ == "__main__":
	
	header = create_header()
	url_prefix = "http://bjgy.chinacourt.org/paper/more/paper_mid/MzA0gAMA/page/%d.shtml"
	page_id = 1
	
	while True:
		print("Current page: %d" % page_id)
		url = url_prefix % page_id
		
		req = urllib.request.Request(url, headers=header)
		try:
			response = urllib.request.urlopen(req, timeout=10)
		except socket.timeout:
			print("Request time out: " + url )
			continue
			
		if response.info().get('Content-Encoding') == 'gzip':
			r_read = zlib.decompress(response.read(), 16+zlib.MAX_WBITS)
		else:    
			r_read = response.read()
			
		soup = BeautifulSoup(r_read.decode('utf-8'), "lxml")
		info_soup = soup.find('div', attrs={"id":"list", "class":"font14"})
		if info_soup:
			track_info(info_soup)
		
		page_id += 1
		if page_id > 250:
			break
	
	db_conn.close()
	print("Done!")
