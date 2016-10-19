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


def track_info(tbody, fp):
	info = {}
	tds = tbody.findAll('td')
	info['姓名'] = tds[1].span.string
	info['性别'] = tds[3].string
	info['民族'] = tds[6].string
	info['政治面貌'] = tds[8].string
	info['学历'] = tds[10].string
	info['执业范围'] = tds[12].string
	info['执业类别'] = tds[14].string
	info['职称'] = tds[16].string
	info['联系电话'] = tds[18].string
	info['首次执业时间'] = tds[20].string
	info['执业证号'] = tds[22].string
	info['个人简介'] = tds[24].string
	info['个人主页'] = tds[26].string
	info['邮箱'] = tds[28].string
	info['服务律所'] = tds[30].string
	fp.write(repr(info)+'\n')
	return
	
if __name__ == "__main__":
	
	result_file = 'ningxia_law.txt'	
	detail_file = 'ningxia_detail.txt'
	
	fp = open(result_file, 'r')	
	fw = open(detail_file, 'w')
	header = create_header()
	
	cnt = 0
	for line in fp:
		print("Current cnt: %d" % cnt)
		cnt += 1
		url = line[line.index("http://www.nxlawyers.org/showls.asp"):line.index('&swsmc')]
			
		while True:
			try:
				req = urllib.request.Request(url, headers=header)
				response = urllib.request.urlopen(req)
				if response.info().get('Content-Encoding') == 'gzip':
					r_read = zlib.decompress(response.read(), 16+zlib.MAX_WBITS)
				else:    
					r_read = response.read()
				soup = BeautifulSoup(r_read.decode('gbk'), 'lxml')
				info_soup = soup.find('table', attrs={"width":"659"})
				if info_soup:
					track_info(info_soup, fw)
			except Exception as err:  
				print(err)  
				continue
				
			break;
			
	fp.close()
	fw.close()

	print("Done!")
