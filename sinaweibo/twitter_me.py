#!/usr/bin/env python
# coding:utf-8
# manning  2015-5-12
import os
import urllib
import urllib2

import time
import random

from sinaweibo import SinaWeibo

import jd_logger
import jd_utils
import wb_cfg
from jd_utils import log_info, log_warn

from twython import Twython

from wb_cfg import TW_WHITE_LIST


class Twitter_Me:
    def __init__ (self, Sinaweibo, logger = None):
        self.log = logger
        self.sw = Sinaweibo
        try:
            self.client = Twython(wb_cfg.TW_APP_KEY, wb_cfg.TW_APP_SECRET, \
                                  wb_cfg.TW_AC_TOKEN, wb_cfg.TW_AC_TOKEN_SECRET)
            log_info(u"推特模块", u"User %s Login Successful!" % wb_cfg.TW_OWN_ID, self.log)
        except Exception:
            log_warn(u"推特模块", u"User %s Login Failed!" % self.TW_OWN_ID, self.log)
    
    def get_timeline(self):
        if self.client:
            return self.client.get('users/show', uid = wb_cfg.TW_OWN_ID)

    def get_home_timeline(self, cnt = 200):
        if self.client:
            return self.client.get('statuses/home_timeline', params = {'count':200} )

    def post_statuses(self, str_msg):
        if self.client:
            self.client.post('statuses/update', status = str_msg, uid = self.UID)
            
    def repost_to_sina(self, Sinaweibo = None):
        if not self.sw:
            log_info(u"推特模块",u"Sinaweibo object is null, just return!")
            return
        statuses = self.get_home_timeline()
        if statuses:
            for item in statuses:
                if not item['user']['screen_name'] in TW_WHITE_LIST :
                    log_info(u"推特模块",u"Not in the twitter repost_white list, skip [%d]!\n" % item['id'], self.log)
                    continue                  
                elif item['user']['screen_name'] == "taozhijiang":
                    log_info(u"推特模块",u"Recycle, Give up!\n")
                    break
                else:
                    try:
                        ret = self.sw.post_statuses( str_msg = item['text'])
                    except Exception, e:
                        log_warn(u"推特模块",u"Runtime Error:%s" % e, self.log)
                        continue

                    log_info(u"推特模块",u"Repost item for [%d]!\n" % item['id'], self.log)

                #等待一会儿
                time.sleep(random.randint(200,400))
        
        
        
if __name__ == "__main__":
    sw = SinaWeibo()
    t = Twitter_Me(sw)
    t.get_home_timeline()
    t.repost_to_sina()
    #t.client.update_status(status="Hello from Python! :D")
    #print (t.get_home_timeline())[0]['text']
    #print (t.get_home_timeline())[0]['user']['name']
    #print (t.get_home_timeline())[0]['user']['screen_name']