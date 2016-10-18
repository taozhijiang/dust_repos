#/usr/bin/python3

import zlib

import urllib.request
import urllib.error
import urllib

from bs4 import BeautifulSoup
import urllib3


def create_header():
	head = urllib3.util.make_headers(keep_alive=True, accept_encoding="gzip, deflate", user_agent='Mozilla/5.0 (Linux; Android 6.0; Nexus 5 Build/MRA58N) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/46.0.2490.76 Mobile Safari/537.36', basic_auth=None)
	head['Host'] = 'www.gxlawyer.org.cn'
	head['Cache-Control'] = 'max-age=0'
	head['Accept'] = 'text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8'
	head['Referer'] = 'http://www.gxlawyer.org.cn'
	head['Accept-Encoding'] = 'gzip, deflate'
	head['Accept-Language'] = 'zh-CN,zh;q=0.8'

	return head


def track_info(tbody, fp):
	info = {}
	tds = tbody.findAll('td')
	info[tds[0].div.string] = tds[1].string
	info[tds[2].div.string] = tds[3].a.string
	info[tds[4].div.string] = tds[5].string
	info[tds[6].div.string] = tds[7].string
	info[tds[8].div.string] = tds[9].string
	info[tds[10].div.string] = tds[11].string
	info[tds[12].div.string] = tds[13].string
	info[tds[14].div.string] = tds[15].string
	info[tds[16].div.string] = tds[17].string
	info[tds[18].div.string] = tds[19].string
	info[tds[20].div.string] = tds[21].string
	info[tds[22].div.string] = tds[23].string
	info[tds[24].div.string] = tds[25].string
	info[tds[26].div.string] = tds[27].string
	fp.write(repr(info)+'\n')
	return
	
if __name__ == "__main__":
	
	result_file = 'guangxi_law.txt'	
	detail_file = 'guangxi_detail.txt'
	
	fp = open(result_file, 'r')	
	fw = open(detail_file, 'w')
	header = create_header()
	
	cnt = 0
	for line in fp:
		print("Current cnt: %d" % cnt)
		cnt += 1
		url = line[line.index("http://www.gxlawyer.org.cn/"):]
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
