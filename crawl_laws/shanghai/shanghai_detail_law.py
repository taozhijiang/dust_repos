#/usr/bin/python3

import urllib.request
import urllib.error
import urllib

import zlib

from bs4 import BeautifulSoup
import urllib3


def create_header():
	head = urllib3.util.make_headers(keep_alive=True, accept_encoding="gzip, deflate", user_agent='Mozilla/5.0 (Linux; Android 6.0; Nexus 5 Build/MRA58N) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/46.0.2490.76 Mobile Safari/537.36', basic_auth=None)
	head['Host'] = 'www.lawyers.org.cn'
	head['Accept'] = 'text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8'
	head['Accept-Encoding'] = 'gzip, deflate, sdch'
	head['Accept-Language'] = 'zh-CN,zh;q=0.8'

	return head


def track_info(tbody, fp):
	info = {}
	tds = tbody.findAll('td')
	if len(tds) != 26:
		return
	for i in range(13):
		info[tds[i*2].div.string] = tds[i*2+1].string	
	fp.write(repr(info)+'\n')
	return
	
if __name__ == "__main__":
	
	result_file = 'shanghai_law.txt'	
	detail_file = 'shanghai_detail.txt'
	
	fp = open(result_file, 'r')	
	fw = open(detail_file, 'w')
	header = create_header()
	
	cnt = 0
	for line in fp:
		print("Current cnt: %d" % cnt)
		cnt += 1
		url = line[line.index("http://www.lawyers.org.cn/"):].strip()
		req = urllib.request.Request(url, headers=header)
		response = urllib.request.urlopen(req)
		if response.info().get('Content-Encoding') == 'gzip':
			r_read = zlib.decompress(response.read(), 16+zlib.MAX_WBITS)
		else:    
			r_read = response.read()
		soup = BeautifulSoup(r_read.decode('utf-8'), 'lxml')
		info_soup = soup.find('div', attrs={"class":"lawyer_detail"})
		if info_soup:
			track_info(info_soup, fw)
	fp.close()
	fw.close()

	print("Done!")
