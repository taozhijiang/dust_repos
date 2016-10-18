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
	head['Referer'] = 'http://app.bjsf.gov.cn/tabid/220/Default.aspx'
	head['Accept-Encoding'] = 'gzip, deflate'
	head['Accept-Language'] = 'zh-CN,zh;q=0.8'

	return head


def track_info(tbody, fp):
	info = {}
	tds = tbody.findAll('td')
	
	info['姓名'] = tds[2].span.string
	info['职称'] = tds[5].span.string
	info['所在律所'] = tds[7].a.span.string
	info['主管机关'] = tds[9].span.string
	info['毕业院校'] = tds[11].span.string
	info['民族'] = tds[13].span.string
	info['最高学历'] = tds[15].span.string
	info['党派'] = tds[17].span.string
	info['资格证号'] = tds[19].span.string
	info['资格证取得时间'] = tds[21].span.string
	info['执业证号'] = tds[23].span.string
	info['执业证取得时间'] = tds[25].span.string
	info['联系电话'] = tds[27].span.string
	info['电子邮件'] = tds[29].span.string
		
	fp.write(repr(info)+'\n')
	return
	
if __name__ == "__main__":
	
	result_file = 'chongqing_law.txt'	
	detail_file = 'chongqing_detail.txt'
	
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
		info_soup = soup.find('div', attrs={"class":"lsctkk"})
		if info_soup:
			track_info(info_soup, fw)
	fp.close()
	fw.close()

	print("Done!")
