#/usr/bin/python3

import urllib.request
import urllib.error
import urllib

import zlib
import urllib.parse

from bs4 import BeautifulSoup
import urllib3

boundary = "----WebKitFormBoundary5McuiGEqTeOmjPcv"

def create_header():
	head = urllib3.util.make_headers(keep_alive=True, accept_encoding="gzip, deflate", user_agent='Mozilla/5.0 (Linux; Android 6.0; Nexus 5 Build/MRA58N) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/46.0.2490.76 Mobile Safari/537.36', basic_auth=None)
	head['Host'] = '118.125.243.115'
	head['Origin'] = 'http://118.125.243.115'
	head['Referer'] = 'http://118.125.243.115/Ntalker/lawfirms.aspx'
	head['Cache-Control'] = 'max-age=0'
	head['Content-Type'] = 'application/x-www-form-urlencoded'
	head['Accept'] = 'text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8'
	head['Accept-Encoding'] = 'gzip, deflate'
	head['Accept-Language'] = 'zh-CN,zh;q=0.8'
	
	return head

def create_body(state_str, page_id):
	data = "__VIEWSTATE="+urllib.parse.quote_plus(state_str)
	data += "&__VIEWSTATEGENERATOR=6DD78E36&key=&areas=&organizational=&Pages=%d&E=&txtx=0"%(page_id)	
	return data

def track_info(tbody, fp):
	data = {}
	laws = tbody.findAll('li')
	for law in laws:
		divs = law.findAll('div')
		if divs:
			fp.write("律师事务所名称:%s 负责人:%s 联系电话:%s 地 址:%s http://118.125.243.115/Ntalker/%s\n" %(divs[0].a.string, divs[1].string, divs[2].string, divs[3].string, divs[0].a.get('href')))
	return	
	
	
if __name__ == "__main__":
	
	url = 'http://118.125.243.115/Ntalker/lawfirms.aspx'	
	result_file = 'chongqing_firm.txt'	
	
	fp = open(result_file, 'w')	
	# 第一次请求，STAT字符串是空的
	stat_str = ""
	header = create_header()
	
	req = urllib.request.Request(url, headers=header)
	response = urllib.request.urlopen(req)
	if response.info().get('Content-Encoding') == 'gzip':
		r_read = zlib.decompress(response.read(), 16+zlib.MAX_WBITS)
	else:    
		r_read = response.read()
	soup = BeautifulSoup(r_read.decode('utf-8'), 'lxml')
	this_stat = soup.find('input', attrs={"id":"__VIEWSTATE"})['value']
	info_soup = soup.find('div', attrs={"class":"lawlsit"})
	if info_soup:
		track_info(info_soup, fp)
	
	page_id = 1
	while True:
		print("Current page: %d" % page_id)
		page_id += 1
		if page_id > 40:
			break
		body = create_body(this_stat, page_id)
		req = urllib.request.Request(url, headers=header, data=body.encode('ascii'))
		response = urllib.request.urlopen(req)
		if response.info().get('Content-Encoding') == 'gzip':
			r_read = zlib.decompress(response.read(), 16+zlib.MAX_WBITS)
		else:    
			r_read = response.read()
		soup = BeautifulSoup(r_read.decode('utf-8'), 'lxml')
		this_stat = soup.find('input', attrs={"id":"__VIEWSTATE"})['value']
		info_soup = soup.find('div', attrs={"class":"lawlsit"})
		if info_soup:
			track_info(info_soup, fp)
	
	fp.close()
	print("Done!")