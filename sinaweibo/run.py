#!/usr/bin/env python
# coding:utf-8
# manning  2015-5-12
import time
import os
import sys
import codecs
import random

from jd_utils import log_info, log_warn
from sinaweibo import SinaWeibo
from twitter_me import Twitter_Me
import jd_utils
import jd_logger
import wb_cfg
from toutiao_spider import Toutiao

if __name__ == "__main__":

    if not os.path.exists(wb_cfg.LOG_PATH):
	os.makedirs(wb_cfg.LOG_PATH)
	
    main_log = jd_logger.Jd_Logger(u"微博机器人", verbose = False)
    log_info(u"MAIN",u"Application Starting Up!", main_log)	

    if len(os.sys.argv) < 2:
	log_warn(u"MAIN",u"No arguments specified, give up!", main_log)
	sys.exit(-1)
	
    app_t = os.sys.argv[1]
    
    if app_t == "twitter":
	sw = SinaWeibo()
	t = Twitter_Me(sw)
	t.repost_to_sina()	    
    elif app_t == "toutiao":
	sw = SinaWeibo(logger = main_log)    

	#好友圈转发
	log_info(u"MAIN",u"Enter Repost Mode！", main_log) 
	sw.repost_friend(12000)
	log_info(u"MAIN",u"Task Finieshed！", main_log)   
    
    elif app_t == "repost":
	sw = SinaWeibo(logger = main_log)
	
	#抓取开发者头条转发
	tt = Toutiao(logger = main_log)
	tt.get_toutiao()
	if os.path.exists(wb_cfg.TTFILE):
	    try:
		fp = codecs.open(wb_cfg.TTFILE, 'rb',encoding = 'utf-8')
		content = eval(fp.read())
	    except Exception:
		log_warn(u"MAIN",u"Read Write File Error", main_log)
		sys.exit(-1)
	    finally:
		fp.close()
	    
	    for item in content:
		web_msg = tt.encasp_toutiao(item)
		try:
		    ret = sw.post_statuses(web_msg)	
		except Exception, e:
		    log_warn(u"MAIN",u"Runtime Error:%s" % e, main_log)
		    continue	    
		
		log_info(u"MAIN",u"Already Send a Status Message！", main_log)
		time.sleep(random.randint(300,1000))  #自动发送一条          
	    
	    log_info(u"MAIN",u"Today's Toutiao Retwitter Finished！", main_log) 	
	
    else:
	log_warn(u"MAIN",u"Unknow arguments specified, give up!", main_log)
    
    
    sys.exit(0)