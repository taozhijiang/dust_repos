#!/usr/bin/python
#-*- coding: utf-8 -*-
#encoding=utf-8


import urllib2
import urllib
import codecs
import re
import os
from BeautifulSoup import BeautifulSoup

from jd_utils import log_info, log_warn

import jd_utils
import wb_cfg


tt_main_url = "http://toutiao.io/"
tt_headers = { "Host":"toutiao.io",
               "User-Agent":"Mozilla/5.0 (Windows NT 6.3; WOW64; rv:37.0) Gecko/20100101 Firefox/37.0",
               "Connection":"keep-alive"
               }


class Toutiao:
    def __init__(self, logger = None):
	if os.path.exists(wb_cfg.TTFILE):
	    os.remove(wb_cfg.TTFILE)
        self.log = logger
    
    def get_toutiao(self):
	fetch_result = []
	try:
	    request = urllib2.Request(tt_main_url, headers = tt_headers)
	    html = urllib2.urlopen(request).read()
	except UnicodeDecodeError:
	    log_info(u"开发者头条模块", u"GBK/Unicode Code Decode Error!", self.log)
	    return
	except Exception:
	    log_warn(u"开发者头条模块", u"Unkown Error!", self.log)
	    return	
	soup = BeautifulSoup(html)
	contents = soup.findAll('div', attrs = {"class":"content"} )
	for item in contents:
	    title = item.find('h3', attrs = {"class":"title"})
	    summary = item.find('p', attrs = {"class":"summary"})
	    if title and summary:
		if title.a.string and title.a['href'] and summary.a.string:
		    #str_msg = u"{'title':'%s', 'summary':'%s', 'link':'%s'}" % (title.a.string, summary.a.string, title.a['href'])
		    fetch_item = dict(TITLE=title.a.string.encode('utf-8'), SUMMARY=summary.a.string.encode('utf-8'), LINK=title.a['href'].encode('utf-8'))
		    fetch_result.append(fetch_item)
		    
	log_info(u"开发者头条模块", u"Todady's Toutiao Fetched Finished!", self.log)
	log_info(u"开发者头条模块", u"We've Fetched %s items!" % len(fetch_result), self.log)
	
	try:
	    fp = codecs.open(wb_cfg.TTFILE, 'ab',encoding = 'utf-8')
	    msg = repr(fetch_result)
	    fp.write(msg)
	except Exception:
	    log_info(u"开发者头条模块",u"Read Write File Error", self.log)
	finally:
	    fp.close()
	
    def encasp_toutiao(self, tt_item):
	web_msg = u"【开发者头条】" + u"标题：" + tt_item['TITLE'].decode('utf-8') + \
	    u"\n主题：" + tt_item['SUMMARY'].decode('utf-8') + \
	    u"\n链接：" + tt_item['LINK'].decode('utf-8') + \
	    u"\n[Nicol's Robot %s]" % (jd_utils.current_time())
	return web_msg	
	
		
if __name__ == "__main__":
    tt = Toutiao()
    tt.get_toutiao()
	
	
        
