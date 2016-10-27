#/usr/bin/python3

import urllib.request
import urllib.error
import urllib

from bs4 import BeautifulSoup
import urllib3

boundary = "----WebKitFormBoundarykZ1A526DaehtrHcA"

def create_header():
	head = urllib3.util.make_headers(keep_alive=True, accept_encoding="gzip, deflate", user_agent='Mozilla/5.0 (Linux; Android 6.0; Nexus 5 Build/MRA58N) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/46.0.2490.76 Mobile Safari/537.36', basic_auth=None)
	head['Host'] = 'app.bjsf.gov.cn'
	head['Origin'] = 'http://app.bjsf.gov.cn'
	head['Cache-Control'] = 'max-age=0'
	head['Content-Type'] = 'multipart/form-data; boundary=----WebKitFormBoundarykZ1A526DaehtrHcA'
	head['Accept'] = 'text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8'
	head['Referer'] = 'http://app.bjsf.gov.cn/tabid/219/Default.aspx'
	
	return head

def create_body(state_str):
	data = []
	data.append("--%s" % boundary)
	data.append('Content-Disposition: form-data; name="__EVENTTARGET"\r\n')
	data.append('ess$ctr740$LawOfficeSearchList$lbtnNextPage')

	data.append("--%s" % boundary)
	data.append('Content-Disposition: form-data; name="__EVENTARGUMENT"\r\n')
	data.append('')

	if state_str:
		data.append("--%s" % boundary)
		data.append('Content-Disposition: form-data; name="__VIEWSTATE"\r\n')
		data.append(state_str)
	
	data.append("--%s" % boundary)
	data.append('Content-Disposition: form-data; name="__VIEWSTATEENCRYPTED"\r\n')
	data.append('')
	
	data.append("--%s" % boundary)
	data.append('Content-Disposition: form-data; name="ScrollTop"\r\n')
	data.append('')

	data.append("--%s" % boundary)
	data.append('Content-Disposition: form-data; name="__essVariable"\r\n')
	data.append('{"__scdoff":"1"}')

	data.append("--%s" % boundary)
	data.append('Content-Disposition: form-data; name="ess$ctr740$LawOfficeSearchList$txtName"\r\n')
	data.append('')

	data.append("--%s" % boundary)
	data.append('Content-Disposition: form-data; name="ess$ctr740$LawOfficeSearchList$txtCodeNum"\r\n')
	data.append('')
	
	data.append("--%s" % boundary)
	data.append('Content-Disposition: form-data; name="ess$ctr740$LawOfficeSearchList$txtReponseName"\r\n')
	data.append('')
	
	data.append("--%s" % boundary)
	data.append('Content-Disposition: form-data; name="ess$ctr740$LawOfficeSearchList$ddlType"\r\n')
	data.append('1')
		
	data.append("--%s" % boundary)
	data.append('Content-Disposition: form-data; name="ess$ctr740$LawOfficeSearchList$ddlCountry"\r\n')
	data.append('-1')
	
	data.append("--%s" % boundary)
	data.append('Content-Disposition: form-data; name="ess$ctr740$LawOfficeSearchList$txtPageNum"\r\n')
	data.append('')
	
	data.append("--%s" % boundary)
	
	return '\r\n'.join(data)

def track_info(tbody, fp):
	peoples = tbody.findAll('tr')
	for item in peoples:
		tds = item.findAll('td')
		if tds[0].string == '名称(中文)' and tds[1].string == '统一社会信用代码':
			continue
		#print("%s %s %s %s %s" %(tds[0].a.string, tds[1].string, tds[2].string, tds[3].span.string, tds[0].a.get('href')))
		fp.write("%s %s %s %s %s\n" %(tds[0].a.string, tds[1].string, tds[2].span.string, tds[3].span.string, tds[0].a.get('href')))
	return	
	
	
if __name__ == "__main__":
	
	url = 'http://app.bjsf.gov.cn/tabid/219/Default.aspx'	
	result_file = 'beijing_firm.txt'	
	
	fp = open(result_file, 'w')	
	# 第一次请求，STAT字符串是空的
	stat_str = ""
	header = create_header()
	
	req = urllib.request.Request(url, headers=header)
	response = urllib.request.urlopen(req)
	soup = BeautifulSoup(response.read().decode('utf-8'), 'lxml')
	this_stat = soup.find('input', attrs={"id":"__VIEWSTATE"})['value']
	info_soup = soup.find('table', attrs={"class":"datagrid-main"})
	if info_soup:
		track_info(info_soup, fp)
	
	page_id = 2
	while True:
		print("Current page: %d" % page_id)
		page_id += 1
		body = create_body(this_stat)
		req = urllib.request.Request(url, headers=header, data=body.encode('ascii'))
		response = urllib.request.urlopen(req)
		soup = BeautifulSoup(response.read().decode('utf-8'), 'lxml')
		this_stat = soup.find('input', attrs={"id":"__VIEWSTATE"})['value']
		info_soup = soup.find('table', attrs={"class":"datagrid-main"})
		if info_soup:
			track_info(info_soup, fp)
		if soup.find('a', attrs={'id':"ess_ctr740_LawOfficeSearchList_lbtnNextPage", 'disabled':'disabled'}) :
			break
	
	fp.close()
	print("Done!")
