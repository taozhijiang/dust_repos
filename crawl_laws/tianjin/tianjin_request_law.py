#/usr/bin/python3

import urllib.request
import urllib.error
import urllib

from bs4 import BeautifulSoup
import urllib3

def create_header():
	head = urllib3.util.make_headers(keep_alive=True, accept_encoding="gzip, deflate", user_agent='Mozilla/5.0 (Linux; Android 6.0; Nexus 5 Build/MRA58N) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/46.0.2490.76 Mobile Safari/537.36', basic_auth=None)
	head['Host'] = 'lg.tjsf.gov.cn'
	head['Cache-Control'] = 'max-age=0'
	head['Origin'] = 'http://lg.tjsf.gov.cn'
	head['Content-Type'] = 'application/x-www-form-urlencoded'
	head['Accept'] = 'text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8'
	head['Referer'] = 'http://lg.tjsf.gov.cn/tianjinlawyermanager/justice/search/resultperson.jsp'
	head['Accept-Encoding'] = 'gzip, deflate'
	head['Accept-Language'] = 'en-US,en;q=0.8'

	return head

def create_body(page_id):
	data = 'currentPage=' + repr(page_id)
	return data

def track_info(tbody, fp):
	trs = tbody.findAll('tr')
	for item in trs[1:]:
		tds = item.findAll('td')
		if len(tds) != 5: 
			continue
		fp.write("%s %s %s %s %s %s\n" %(tds[0].a.string, tds[1].string, tds[2].string, tds[3].string, tds[4].string, tds[0].a.get('href')))
	return	
	
	
if __name__ == "__main__":
	
	url = 'http://lg.tjsf.gov.cn/tianjinlawyermanager/justice/search/resultperson.jsp'	
	result_file = 'tianjin_law.txt'	
	
	fp = open(result_file, 'w')
	header = create_header()
	
	page_id = 1
	while True:
		print("Current page: %d" % page_id)
		body = create_body(page_id)
		req = urllib.request.Request(url, headers=header, data=body.encode('ascii'))
		response = urllib.request.urlopen(req)
		soup = BeautifulSoup(response.read().decode('gbk'), 'lxml')
		info_soup = soup.find('table', attrs={"cellpadding":"2"})
		if info_soup:
			track_info(info_soup, fp)
		
		page_id += 1
		if page_id > 291:
			break
	
	fp.close()
	print("Done!")
