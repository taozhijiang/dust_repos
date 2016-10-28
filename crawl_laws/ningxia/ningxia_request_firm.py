#/usr/bin/python3

import zlib

import urllib.request
import urllib.error
import urllib

from bs4 import BeautifulSoup
import urllib3

def create_header():
	head = urllib3.util.make_headers(keep_alive=True, accept_encoding="gzip, deflate", user_agent='Mozilla/5.0 (Linux; Android 6.0; Nexus 5 Build/MRA58N) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/46.0.2490.76 Mobile Safari/537.36', basic_auth=None)
	head['Host'] = 'www.nxlawyers.org'
	head['Cache-Control'] = 'max-age=0'
	head['Accept'] = 'text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8'
	head['Accept-Encoding'] = 'gzip, deflate'
	head['Accept-Language'] = 'zh-CN,zh;q=0.8'
	return head

def create_body(page_id):
	data = 'currentPage=' + repr(page_id)
	return data

def track_info(k, tbody, fp):
	info1 = tbody.find("table", attrs={"width":"659"})
	tds = info1.findAll('td')
	info = tds[1].span.string + "， "
	info += tds[2].text + tds[3].text + "， "
	info += tds[5].text + tds[6].text + "， "
	info += tds[7].text + tds[8].text + "， "
	info += tds[9].text + tds[10].text + "， "
	info += tds[11].text + tds[12].text + "， "
	info += tds[13].text + tds[14].text + "， "
	info += tds[15].text + tds[16].text + "， "
	info += tds[17].text + tds[18].text + "， "
	info += tds[19].text + tds[20].text.strip() + "， "
	info += tds[21].text + tds[22].text + "， "
	
	info += "律所律师："
	ais = tbody.find('div', attrs={"class":"cengdt"}).findAll('a')
	for ai in ais:
		info += ai.string + "、"
	fp.write(info+"\n")
	return	
	
	
if __name__ == "__main__":
	
	result_file = 'ningxia_firm.txt'	
	detail_file = 'ningxia_detail_firm.txt'
	
	fp = open(result_file, 'w')
	header = create_header()
	
	# Stage-I，找出所有的律师事务所
	req = urllib.request.Request(url='http://www.nxlawyers.org/lssws.asp', headers=header)
	response = urllib.request.urlopen(req)
	if response.info().get('Content-Encoding') == 'gzip':
		r_read = zlib.decompress(response.read(), 16+zlib.MAX_WBITS)
	else:    
		r_read = response.read()
	soup = BeautifulSoup(r_read.decode('gbk'), 'lxml')
	info_soup = soup.find('div', attrs={"class":"cengdt"}).findAll('td', attrs={"height":"30"})
	lsss = {}
	for item in info_soup:
		lsss[item.a.string] = "http://www.nxlawyers.org/"+item.a.get("href")
		fp.write("%s http://www.nxlawyers.org/%s \n" %(item.a.string, item.a.get("href")))
	fp.close()
	
	print("Total: %d" %(len(lsss)))
	cnt = 0
	fp = open(detail_file, 'w')
	
	for (k, v) in lsss.items():
		print("current: %d" %(cnt)); cnt += 1
		req = urllib.request.Request(v, headers=header)
		response = urllib.request.urlopen(req)
		if response.info().get('Content-Encoding') == 'gzip':
			r_read = zlib.decompress(response.read(), 16+zlib.MAX_WBITS)
		else:    
			r_read = response.read()
		soup = BeautifulSoup(r_read.decode('gbk'), 'lxml')
		info_soup = soup.find('table', attrs={"width":"950"})
		if info_soup:
			track_info(k, info_soup, fp)
	
	fp.close()
	print("Done!")
