#/usr/bin/python3

import json
import time
import urllib.request
import urllib.error
import urllib

from bs4 import BeautifulSoup
import urllib3

def create_header():
	head = urllib3.util.make_headers(keep_alive=True, accept_encoding="gzip, deflate", user_agent='Mozilla/5.0 (Linux; Android 6.0; Nexus 5 Build/MRA58N) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/46.0.2490.76 Mobile Safari/537.36', basic_auth=None)
	head['Host'] = '111.160.0.142:8091'
	head['Cache-Control'] = 'max-age=0'
	head['Accept'] = 'text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8'
	head['Accept-Encoding'] = 'gzip, deflate'
	head['Accept-Language'] = 'en-US,en;q=0.8'

	return head

def track_info(tbody, fp):
	trs = tbody.findAll('tr')
	for item in trs[1:]:
		tds = item.findAll('td')
		if len(tds) != 5: 
			continue
		fp.write("%s %s %s %s %s %s\n" %(tds[0].a.string, tds[1].string, tds[2].string, tds[3].string, tds[4].string, tds[0].a.get('href')))
	return	
	
	
if __name__ == "__main__":
	
	url_prefix = 'http://111.160.0.142:8091/lawyer/files/getAllLawoffice?pagesize=20&page='	
	result_file = 'tianjin_firm.txt'	
	
	fp = open(result_file, 'w')
	header = create_header()
	
	page_id = 1
	while True:
		print("Current page: %d" % page_id)
		url = url_prefix + repr(page_id)
		req = urllib.request.Request(url, headers=header)
		response = urllib.request.urlopen(req)
		if response.info().get('Content-Encoding') == 'gzip':
			r_read = zlib.decompress(response.read(), 16+zlib.MAX_WBITS)
		else:    
			r_read = response.read().decode('utf-8')
		
		data = json.loads(r_read)['data']['items']
		for item in data:
			info = {}
			info['事务所名称'] = item['lawofficename']
			info['组织形式'] = item['organizational']
			info['批准日期'] = item['approve_date']	
			info['执业许可证号'] = item['permit']
			info['主管机关'] = item['regulation']	
			info['设立资产'] = item['asset']	
			info['地址'] = item['lawfirmaddress']	
			info['电话'] = item['tel']	
			info['传真'] = item['fax']	
			info['邮编'] = item['postcode']	
			info['执业状态'] = item['is_valid']	
			info['考核年度'] = item['jckhyear']	
			info['执业证有效期至'] = item['zyxkenddate']	
			info['合伙人名单'] = item['lawofficeusername']	
			fp.write(repr(info)+"\n")
		
		#time.sleep(2)
		
		page_id += 1
		if page_id > 34:
			break
	
	fp.close()
	print("Done!")
