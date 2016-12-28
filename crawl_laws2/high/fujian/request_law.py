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
import json

from tornado.options import define, options
define("dbhost", default="192.168.1.6", help="database host name/ip")
define("dbname", default="v5_law", help="database name")
define("dbuser", default="root", help="database username")
define("dbpass", default=None, help="database passwd")
define("dbtimezone", default="+8:00")

db_conn = torndb.Connection("192.168.1.6", "v5_law", "root");

def create_header():
	head = urllib3.util.make_headers(keep_alive=True, accept_encoding="gzip, deflate", user_agent='Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/55.0.2883.87 Safari/537.36', basic_auth=None)
	head['Host'] = 'www.fjcourt.gov.cn'
	head['Origin'] = 'http://www.fjcourt.gov.cn'
	head['Cache-Control'] = 'max-age=0'
	head['DNT'] = '1'
	head['Accept'] = 'text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8'
	head['Accept-Encoding'] = 'gzip, deflate'
	head['Accept-Language'] = 'zh-CN,zh;q=0.8,en;q=0.6,ja;q=0.4'
	head['Content-Type'] = 'application/x-www-form-urlencoded'
	head['Referer'] = 'http://www.fjcourt.gov.cn/page/public/RefereeclericalMore.aspx?cate=0902&yikikata=70665499-665889ff0101e871960a7e2ee788efc3'
	head['Cookie'] = 'yikikata=716a1be1-c4cca662bce22065006a7a3d59983f3a; gsScrollPos=; brmidyrvj=both; ASP.NET_SessionId=fchvv0qvpgtt30ru33ywlmau; yikikata=716a0e76-1b7a43c210955456d4593913e45fe97f; _gscu_1086777894=82203576fiuw3312; _gscs_1086777894=t82911655lrlc2h18|pv:3; _gscbrs_1086777894=1'
	return head
	
def request_body(url):
	while True:
		ret = ""
		r_read = ""
		req = urllib.request.Request(url, headers=create_header())
		try:
			response = urllib.request.urlopen(req, timeout=10)
			if response.info().get('Content-Encoding') == 'gzip':
				r_read = zlib.decompress(response.read(), 16+zlib.MAX_WBITS)
			else:    
				r_read = response.read()
		except:
			print("Request time out: " + url )
			continue
			
		soup = BeautifulSoup(r_read.decode('utf-8'), 'lxml')
		bodys = soup.find('div', attrs={"class":"bd-article"})
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
	items = info_soup.findAll('li')
	for item in items:
		link = 'http://www.fjcourt.gov.cn'+item.a.get('href')
		a = item.a.text.strip()
		fy = a[a.index('[')+1:a.index(']')].strip()
		anj = a[len(fy)+3:]
		title = anj[:anj.index('[')].strip()
		time = item.span.text[1:-2].strip()
		neirong = request_body(link)
		neirong = db_conn.escape_string(neirong)
		print(title)
		if time[-2:] == '-0':
			time = time[:-1] + '1'
		try:
			sql = ''' INSERT INTO v5_high_court(`案件名`,`地区`,`法院名称`,`url`,`文书内容`, `发布时间`) VALUES( '{0}', '{1}', '{2}', '{3}', '{4}', '{5}'); '''
			sql = sql.format(title, '福建', fy, link, neirong, time)
			db_conn.execute(sql)
		except:
			print("SQL Error:" + sql)
	return	
	

if __name__ == "__main__":

	url_prefix = "http://www.fjcourt.gov.cn/page/public/RefereeclericalMore.aspx?cate=0902&yikikata=70665499-665889ff0101e871960a7e2ee788efc3"
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
	#if info_soup:
	#	track_info(info_soup)
	
	page_id = 1
	while True:
		print("Current page: %d" % page_id)
		#print(this_stat)
		body = {'__VIEWSTATE':this_stat, '__EVENTARGUMENT':repr(page_id), '__EVENTTARGET':'ctl00$cplContent$AspNetPager1', 'ctl00$cplContent$AspNetPager1_input':repr(page_id-1)}
		dat = urllib.parse.urlencode(body).encode('ascii')
		req = urllib.request.Request(url_prefix, headers=header, data=dat )
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
		this_stat = soup.find('input', attrs={"id":"__VIEWSTATE"})['value']
		info_soup = soup.find('ul', attrs={"class":"module-case-items"})
		if info_soup:
			track_info(info_soup)
		
		page_id += 1
		if page_id > 42931:
			break
	
	#db_conn.close()
	print("Done!")
