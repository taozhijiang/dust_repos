#/usr/bin/python3

import urllib.request
import urllib.error
import urllib

from bs4 import BeautifulSoup
import urllib3


def create_header():
	head = urllib3.util.make_headers(keep_alive=True, accept_encoding="gzip, deflate", user_agent='Mozilla/5.0 (Linux; Android 6.0; Nexus 5 Build/MRA58N) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/46.0.2490.76 Mobile Safari/537.36', basic_auth=None)
	head['Host'] = 'app.bjsf.gov.cn'
	head['Cache-Control'] = 'max-age=0'
	head['Content-Type'] = 'multipart/form-data; boundary=----WebKitFormBoundary5McuiGEqTeOmjPcv'
	head['Accept'] = 'text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8'
	head['Referer'] = 'http://app.bjsf.gov.cn/tabid/220/Default.aspx'
	head['Accept-Encoding'] = 'gzip, deflate, sdch'
	head['Accept-Language'] = 'zh-CN,zh;q=0.8'

	return head


def track_info(tbody, fp):
	info = {}
	tds = tbody.findAll('td')
	info['lblName'] = tds[1].font.string.strip('\xa0')
	info['lblSex'] = tds[3].font.string.strip('\xa0')
	info['lblFolk'] = tds[5].font.string.strip('\xa0')
	info['lblEdu'] = tds[7].font.string.strip('\xa0')
	info['lbl执业证号'] = tds[9].font.string.strip('\xa0')
	info['lbl资格证号'] = tds[11].font.string.strip('\xa0')
	info['lbl发证时间'] = tds[13].font.string.strip('\xa0')
	info['lbl办公地址'] = tds[15].font.string.strip('\xa0')
	info['lbl所在事务所'] = tds[17].a.font.string.strip('\xa0')
	info['lbl邮编'] = tds[19].font.string.strip('\xa0')
	info['lbl电话'] = tds[21].font.string.strip('\xa0')
	info['lbl传真'] = tds[23].font.string.strip('\xa0')
	fp.write(repr(info)+'\n')
	return
	
if __name__ == "__main__":
	
	url_prefix = 'http://lg.tjsf.gov.cn/tianjinlawyermanager/justice/search/showperson.jsp?' #personcode=1299000000015
	result_file = 'tianjin_law.txt'	
	detail_file = 'tianjin_detail.txt'
	
	fp = open(result_file, 'r')	
	fw = open(detail_file, 'w')
	header = create_header()
	
	cnt = 0
	for line in fp:
		print("Current cnt: %d" % cnt)
		cnt += 1
		url_item = line[line.index("personcode="):]
		url = url_prefix+url_item
		req = urllib.request.Request(url, headers=header)
		response = urllib.request.urlopen(req)
		soup = BeautifulSoup(response.read().decode('gbk'), 'lxml')
		info_soup = soup.find('table', attrs={"bordercolor":"#FFFFFF"})
		if info_soup:
			track_info(info_soup, fw)
	fp.close()
	fw.close()

	print("Done!")
