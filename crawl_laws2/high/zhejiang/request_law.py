#/usr/bin/python3

import zlib

import urllib.request
import urllib.error
import urllib.parse
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
import time

from tornado.options import define, options
define("dbhost", default="192.168.1.6", help="database host name/ip")
define("dbname", default="v5_law", help="database name")
define("dbuser", default="root", help="database username")
define("dbpass", default=None, help="database passwd")
define("dbtimezone", default="+8:00")

db_conn = torndb.Connection("192.168.1.6", "v5_law", "root");

def create_header():
	head = urllib3.util.make_headers(keep_alive=True, accept_encoding="gzip, deflate", user_agent='Mozilla/5.0 (Linux; Android 6.0; Nexus 5 Build/MRA58N) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/46.0.2490.76 Mobile Safari/537.36', basic_auth=None)
	head['Host'] = 'www.zjsfgkw.cn'
	head['Cache-Control'] = 'max-age=0'
	head['DNT'] = '1'
	head['Accept'] = 'text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8'
	head['Accept-Encoding'] = 'gzip, deflate, sdch'
	head['Accept-Language'] = 'zh-CN,zh;q=0.8,en;q=0.6,ja;q=0.4'
	return head

header = create_header()
	
def request_body(url):
	global header
	
	url = 'http://www.zjsfgkw.cn'+urllib.parse.quote(url)
	cnt = 0
	while True:
	
		if cnt >= 10:
			print("Give up ...")
			return '【内容为空】'
	
		ret = ""	
		req = urllib.request.Request(url, headers=header)
		try:
			response = urllib.request.urlopen(req, timeout=10)
			if response.info().get('Content-Encoding') == 'gzip':
				r_read = zlib.decompress(response.read(), 16+zlib.MAX_WBITS)
			else:    
				r_read = response.read()
		except Exception as e:
			print(str(e))
			print("Request error: " + url )
			cnt += 1
			time.sleep(2)
			continue
			
		soup = BeautifulSoup(r_read.decode('utf-8'), 'lxml')
		bodys = soup.find('body')
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
			
		cnt += 1
		print("Retry: " + url)
		
	return ret

def track_info(info_soup):
	global header
	dat = json.loads(info_soup)
	for item in dat['list']:
		# 这里的link需要额外请求一次才能得到真正文书内容的url
		link = 'http://www.zjsfgkw.cn/document/JudgmentDetail/'+repr(item['DocumentId'])
		
		url = ""
		neirong = ""
		cnt = 0
		while True:
			req = urllib.request.Request(link, headers=header)
			
			if cnt >= 10:
				print("Give up ...")
				neirong = '【内容为空】'
				break
			
			try:
				response = urllib.request.urlopen(req, timeout=10)
				if response.info().get('Content-Encoding') == 'gzip':
					r_read = zlib.decompress(response.read(), 16+zlib.MAX_WBITS)
				else:    
					r_read = response.read()
			except:
				print("Request time out: " + link )
				time.sleep(2)
				cnt += 1
				continue
		
			soup = BeautifulSoup(r_read.decode('utf-8'), 'lxml')
			detail_content = soup.find('div', attrs={"class":"books_detail_content"})
			url = detail_content.iframe.get('src')
			if not url:
				cnt += 1
				continue
			
			neirong = request_body(url)
			neirong = db_conn.escape_string(neirong)
			break
			
		print(item['AH'])
		try:
			sql = ''' INSERT INTO v5_high_court(`案件名`, `法院名称`,`地区`,`发布时间`,`案号`,`url`,`文书内容`) VALUES('【内容为空】', '{0}', '{1}', '{2}', '{3}', '{4}', '{5}'); '''
			sql = sql.format(item['CourtName'], '浙江', item['JARQ'], item['AH'], 'http://www.zjsfgkw.cn'+url, neirong)
			db_conn.execute(sql)
		except:
			print("SQL Error:" + sql)
	return	
	

if __name__ == "__main__":
	
	header = create_header()
	url_prefix = "http://www.zjsfgkw.cn/document/JudgmentSearch"
	page_id = 1
	
	while True:
		print("Current page: %d" % page_id)
		req = urllib.request.Request(url_prefix, headers=header, data=('pagesize=10&ajlb=%E6%B0%91%E4%BA%8B&cbfy=1300&ah=&jarq1=20060101&jarq2=20161222&key=&pageno='+repr(page_id)).encode('ascii'))
		
		try:
			response = urllib.request.urlopen(req)
			if response.info().get('Content-Encoding') == 'gzip':
				r_read = zlib.decompress(response.read(), 16+zlib.MAX_WBITS)
			else:    
				r_read = response.read()
		except:
			print("Request url failed for page_id:%d " % page_id)
			print("Retry: " + url_prefix)
			time.sleep(2)
			continue
		
		info_soup = r_read.decode('utf-8')
		if info_soup:
			track_info(info_soup)
		
		page_id += 1
		if page_id > 1803:
			break
	
	db_conn.close()
	print("Done!")
