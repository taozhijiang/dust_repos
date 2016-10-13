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
	info['lblName'] = tbody.find('span', attrs={"id":"ess_ctr742_LawyerView_lblName"}).string
	info['lblSex'] = tbody.find('span', attrs={"id":"ess_ctr742_LawyerView_lblSex"}).string
	# skip LawyerView_Image1
	info['lblBirthday'] = tbody.find('span', attrs={"id":"ess_ctr742_LawyerView_lblBirthday"}).string
	info['lblFolk'] = tbody.find('span', attrs={"id":"ess_ctr742_LawyerView_lblFolk"}).string
	info['lblEdu'] = tbody.find('span', attrs={"id":"ess_ctr742_LawyerView_lblEdu"}).string
	info['lblSpeciality'] = tbody.find('span', attrs={"id":"ess_ctr742_LawyerView_lblSpeciality"}).string
	info['lblParty'] = tbody.find('span', attrs={"id":"ess_ctr742_LawyerView_lblParty"}).string
	info['lblReligion'] = tbody.find('span', attrs={"id":"ess_ctr742_LawyerView_lblReligion"}).string
	info['lblIsCopartner'] = tbody.find('span', attrs={"id":"ess_ctr742_LawyerView_lblIsCopartner"}).string
	info['lblFrist_Partncy_Type'] = tbody.find('span', attrs={"id":"ess_ctr742_LawyerView_lblFrist_Partncy_Type"}).string
	info['lblCompetency_Type'] = tbody.find('span', attrs={"id":"ess_ctr742_LawyerView_lblCompetency_Type"}).string
	info['lblCompetency_Date'] = tbody.find('span', attrs={"id":"ess_ctr742_LawyerView_lblCompetency_Date"}).string
	info['lblFirst_City'] = tbody.find('span', attrs={"id":"ess_ctr742_LawyerView_lblFirst_City"}).string
	info['lblFirst_Date'] = tbody.find('span', attrs={"id":"ess_ctr742_LawyerView_lblFirst_Date"}).string
	info['lblHomeTel'] = tbody.find('span', attrs={"id":"ess_ctr742_LawyerView_lblHomeTel"}).string
	info['lblZIP'] = tbody.find('span', attrs={"id":"ess_ctr742_LawyerView_lblZIP"}).string
	info['lblGreenCard'] = tbody.find('span', attrs={"id":"ess_ctr742_LawyerView_lblGreenCard"}).string
	info['lblPerson_Type'] = tbody.find('span', attrs={"id":"ess_ctr742_LawyerView_lblPerson_Type"}).string
	info['lblStatus'] = tbody.find('span', attrs={"id":"ess_ctr742_LawyerView_lblStatus"}).string
	info['lblLo_Name'] = tbody.find('span', attrs={"id":"ess_ctr742_LawyerView_lblLo_Name"}).string
	info['lblDepartment'] = tbody.find('span', attrs={"id":"ess_ctr742_LawyerView_lblDepartment"}).string
	info['lblPost'] = tbody.find('span', attrs={"id":"ess_ctr742_LawyerView_lblPost"}).string
	info['lblCertificate_Code'] = tbody.find('span', attrs={"id":"ess_ctr742_LawyerView_lblCertificate_Code"}).string
	
	fp.write(repr(info)+'\n')
	return
	
if __name__ == "__main__":
	
	url_prefix = 'http://app.bjsf.gov.cn/tabid/239/Default.aspx?' #itemid=1422612
	url = 'http://www.bjsf.gov.cn/publish/portal0/tab196/?itemid=1422612'
	result_file = 'beijing_law.txt'	
	detail_file = 'beijing_detail.txt'
	
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
		info_soup = soup.find('div', attrs={"id":"ess_ctr742_LawyerView_InfoDiv"})
		if info_soup:
			track_info(info_soup, fw)
	fp.close()
	fw.close()

	print("Done!")
