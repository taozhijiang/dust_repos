#/usr/bin/python3

import urllib.request
import urllib.error
import urllib

from bs4 import BeautifulSoup
import urllib3

boundary = "----WebKitFormBoundary5McuiGEqTeOmjPcv"

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
	info['事务所中文全称：'] = tbody.find('span', attrs={"id":"ess_ctr741_LawOfficeView_lblName"}).string
	info['英文名称 ：'] = tbody.find('span', attrs={"id":"ess_ctr741_LawOfficeView_lblEName"}).string
	info['事务所地址 ：'] = tbody.find('span', attrs={"id":"ess_ctr741_LawOfficeView_lblADDRESS"}).string
	info['邮编 ：'] = tbody.find('span', attrs={"id":"ess_ctr741_LawOfficeView_lblMAILCODE"}).string
	info['所在区县 ：'] = tbody.find('span', attrs={"id":"ess_ctr741_LawOfficeView_lblCITY"}).string
	info['主管司法局 ：'] = tbody.find('span', attrs={"id":"ess_ctr741_LawOfficeView_lblSUPERINTEND_OFFICE"}).string
	info['统一社会信用代码 ：'] = tbody.find('span', attrs={"id":"ess_ctr741_LawOfficeView_lblBusiness_Code"}).string
	info['发证日期 ：'] = tbody.find('span', attrs={"id":"ess_ctr741_LawOfficeView_lblCERTIFICATE_DATE"}).string
	info['律师事务所主任 ：'] = tbody.find('span', attrs={"id":"ess_ctr741_LawOfficeView_lblDIRECTOR"}).string
	info['总所/分所 ：'] = tbody.find('span', attrs={"id":"ess_ctr741_LawOfficeView_lblIF_HQ"}).string
	info['执业状态 ：'] = tbody.find('span', attrs={"id":"ess_ctr741_LawOfficeView_lblStatus"}).string
	info['状态说明 ：'] = tbody.find('span', attrs={"id":"ess_ctr741_LawOfficeView_lblstate_explain"}).string
	info['组织形式 ：'] = tbody.find('span', attrs={"id":"ess_ctr741_LawOfficeView_lblorganize_format"}).string
	info['注册资金（万元） ：'] = tbody.find('span', attrs={"id":"ess_ctr741_LawOfficeView_lblcapital"}).string
	info['办公电话 ：'] = tbody.find('span', attrs={"id":"ess_ctr741_LawOfficeView_lbltel"}).string
	info['传真 ：'] = tbody.find('span', attrs={"id":"ess_ctr741_LawOfficeView_lblfax"}).string
	info['E-mail ：'] = tbody.find('span', attrs={"id":"ess_ctr741_LawOfficeView_lblemail"}).string
	info['事务所主页 ：'] = tbody.find('span', attrs={"id":"ess_ctr741_LawOfficeView_lblhomepage"}).string
	info['党支部形式'] = tbody.find('span', attrs={"id":"ess_ctr741_LawOfficeView_lblpartybranch"}).string
	info['党支部负责人'] = tbody.find('span', attrs={"id":"ess_ctr741_LawOfficeView_lblparty_director"}).string
	info['场所面积（平米） ：'] = tbody.find('span', attrs={"id":"ess_ctr741_LawOfficeView_lbllocation_area"}).string
	info['场所性质 ：'] = tbody.find('span', attrs={"id":"ess_ctr741_LawOfficeView_lbllocation_kind"}).string
	info['律所简介：'] = tbody.find('span', attrs={"id":"ess_ctr741_LawOfficeView_lblbrief_introduction"}).string
	
	fp.write(repr(info)+'\n')
	return
	
if __name__ == "__main__":
	
	url_prefix = 'http://app.bjsf.gov.cn/tabid/238/Default.aspx?' #itemid=1422612
	result_file = 'beijing_firm.txt'	
	detail_file = 'beijing_firm_detail.txt'
	
	fp = open(result_file, 'r')	
	fw = open(detail_file, 'w')
	header = create_header()
	
	cnt = 0
	for line in fp:
		print("Current cnt: %d" % cnt)
		cnt += 1
		url_item = line[line.index("itemid="):]
		url = url_prefix+url_item
		req = urllib.request.Request(url, headers=header)
		response = urllib.request.urlopen(req)
		soup = BeautifulSoup(response.read().decode('utf-8'), 'lxml')
		info_soup = soup.find('div', attrs={"id":"ess_ctr741_LawOfficeView_InfoDiv"})
		if info_soup:
			track_info(info_soup, fw)
	fp.close()
	fw.close()

	print("Done!")
