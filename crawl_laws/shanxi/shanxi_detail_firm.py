#/usr/bin/python3

import urllib.request
import urllib.error
import urllib

import zlib

from bs4 import BeautifulSoup
import urllib3


def create_header():
	head = urllib3.util.make_headers(keep_alive=True, accept_encoding="gzip, deflate", user_agent='Mozilla/5.0 (Linux; Android 6.0; Nexus 5 Build/MRA58N) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/46.0.2490.76 Mobile Safari/537.36', basic_auth=None)
	head['Host'] = 'sx.sxlawyer.cn'
	head['Cache-Control'] = 'max-age=0'
	head['Accept'] = 'text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8'
	head['Referer'] = 'http://sx.sxlawyer.cn/lvshiS.aspx'
	head['Accept-Encoding'] = 'gzip, deflate, sdch'
	head['Accept-Language'] = 'zh-CN,zh;q=0.8'

	return head


def track_info(tbody, fp):
	info = {}
	tds = tbody.findAll('td')
	if len(tds) < 30:
		return
	info[tds[0].string] = tds[1].string
	for i in range(19):
		info[tds[i*2+5].string] = tds[i*2+6].text.strip()	
	fp.write(repr(info)+'\n')
	return
	
if __name__ == "__main__":
	
	result_file = 'shanxi_firm.txt'	
	detail_file = 'shanxi_detail_detail.txt'
	url_prefix = 'http://sx.sxlawyer.cn'
	
	fp = open(result_file, 'r')	
	fw = open(detail_file, 'w')
	header = create_header()
	
	cnt = 0
	for line in fp:
		print("Current cnt: %d" % cnt)
		cnt += 1
		url = line[line.index("http://sx.sxlawyer.cn/"):].strip()
		req = urllib.request.Request(url, headers=header)
		response = urllib.request.urlopen(req)
		if response.info().get('Content-Encoding') == 'gzip':
			r_read = zlib.decompress(response.read(), 16+zlib.MAX_WBITS)
		else:    
			r_read = response.read()
		soup = BeautifulSoup(r_read.decode('utf-8'), 'lxml')
		info_soup = soup.find('table', attrs={"class":"daxq danga"})
		if info_soup:
			track_info(info_soup, fw)
	fp.close()
	fw.close()

	print("Done!")
