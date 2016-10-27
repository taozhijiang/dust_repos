#/usr/bin/python3

import urllib.request
import urllib.error
import urllib

from bs4 import BeautifulSoup
import urllib3

boundary = "----WebKitFormBoundary5McuiGEqTeOmjPcv"

def create_header():
	head = urllib3.util.make_headers(keep_alive=True, accept_encoding="gzip, deflate", user_agent='Mozilla/5.0 (Linux; Android 6.0; Nexus 5 Build/MRA58N) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/46.0.2490.76 Mobile Safari/537.36', basic_auth=None)
	head['Host'] = '118.125.243.115'
	head['Origin'] = 'http://118.125.243.115'
	head['Referer'] = 'http://118.125.243.115/Ntalker/lawyers.aspx'
	head['Cache-Control'] = 'max-age=0'
	head['Content-Type'] = 'application/x-www-form-urlencoded'
	head['Accept'] = 'text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8'
	head['Accept-Encoding'] = 'gzip, deflate'
	head['Accept-Language'] = 'zh-CN,zh;q=0.8'

	return head


def track_info(tbody, fp):
	info = {}
	tds = tbody.findAll('td')
	
	info['名称'] = tds[2].span.string
	info['执业许可证'] = tds[5].span.string
	info['主管机关'] = tds[7].span.string
	info['组织形式'] = tds[9].span.string
	info['电话'] = tds[11].span.string
	info['网址'] = tds[13].span.string
	info['传真'] = tds[15].span.string
	info['成立时间'] = tds[17].span.string
	info['邮编'] = tds[19].span.string
	info['联系地址<'] = tds[21].span.string
		
	fp.write(repr(info)+'\n')
	return
	
if __name__ == "__main__":
	
	result_file = 'chongqing_firm.txt'	
	detail_file = 'chongqing_detail_firm.txt'
	
	fp = open(result_file, 'r')	
	fw = open(detail_file, 'w')
	header = create_header()
	
	cnt = 0
	for line in fp:
		print("Current cnt: %d" % cnt)
		cnt += 1
		url = line[line.index("http://118.125.243.115/"):]
		req = urllib.request.Request(url, headers=header)
		response = urllib.request.urlopen(req)
		if response.info().get('Content-Encoding') == 'gzip':
			r_read = zlib.decompress(response.read(), 16+zlib.MAX_WBITS)
		else:    
			r_read = response.read()
		soup = BeautifulSoup(r_read.decode('utf-8'), 'lxml')
		info_soup = soup.findAll('div', attrs={"class":"lsctkk"})
		if info_soup:
			track_info(info_soup[0], fw)
	fp.close()
	fw.close()

	print("Done!")
