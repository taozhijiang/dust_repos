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
	head = urllib3.util.make_headers(keep_alive=True, accept_encoding="gzip, deflate", user_agent='Mozilla/5.0 (Linux; Android 6.0; Nexus 5 Build/MRA58N) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/46.0.2490.76 Mobile Safari/537.36', basic_auth=None)
	head['Host'] = 'www.gdcourts.gov.cn'
	head['Origin'] = 'http://www.gdcourts.gov.cn'
	head['Cache-Control'] = 'max-age=0'
	head['DNT'] = '1'
	head['Accept'] = 'text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8'
	head['Accept-Encoding'] = 'gzip, deflate'
	head['Accept-Language'] = 'zh-CN,zh;q=0.8,en;q=0.6,ja;q=0.4'
	head['Content-Type'] = 'application/x-www-form-urlencoded'
	head['Referer'] = 'http://www.gdcourts.gov.cn/gdgy/s/cpwsgk/msaj1'
	head['Cookie'] = 'JSESSIONID=7EAA8E95D7CF80CD162133E6D9CF2E05; JSESSIONID=61A107CC4D2427EC8B40C4D08D321CAF; pageNo=5; goPageNo=4; pageSize=7; _gscu_757982982=822041668f1nsc24; _gscs_757982982=t82477728ncdd1416|pv:2; _gscbrs_757982982=1'
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
			print("Request error: " + url )
			continue
			
		soup = BeautifulSoup(r_read.decode('utf-8'), 'lxml')
		bodys = soup.find('body')
		if not bodys:
			print("Error1:" + url)
			#sys.exit()
			return '【内容为空】'
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
	trs = info_soup.findAll('tr')
	for item in trs:
		tds = item.findAll('td')
		fy = tds[0].text.strip()
		title = tds[1].text.strip()
		a = tds[1].a.get('onclick')
		link = "http://www.gdcourts.gov.cn/gdgy/s/cpwsgk/findWsnrByid?wsid="+a[len('ckwsnr(\''):a.index('\', \'')]
		anhao = tds[2].text.strip()
		neirong = request_body(link)
		neirong = db_conn.escape_string(neirong)
		print(title)
		sql = ''' INSERT INTO v5_high_court(`案件名`,`地区`,`法院名称`,`url`,`文书内容`, `案号`) VALUES( '{0}', '{1}', '{2}', '{3}', '{4}', '{5}'); '''
		sql = sql.format(title, '广东', fy, link, neirong, anhao)
		db_conn.execute(sql)
	return	
	

if __name__ == "__main__":
	
	header = create_header()
	url_prefix = "http://www.gdcourts.gov.cn/gdgy/s/cpwsgk/msaj1"
	page_id = 1	
	
	while True:
		print("Current page: %d" % page_id)
		body = { "flag":"1", "pageNo":repr(page_id), "pageSize":"20", "fymc2":"广东省高院", "fjm2":"J00", "spcx2":"二审"}
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
		info_soup = soup.find('table', attrs={"cellspacing":"10"})
		if info_soup:
			track_info(info_soup)
			
		page_id += 1
		# 一审 20*3
		# 二审 20*50 最多一千条
		if page_id > 50:
			break
	
	#db_conn.close()
	print("Done!")
