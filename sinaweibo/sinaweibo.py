#!/usr/bin/env python
# coding:utf-8
# manning  2015-5-12

import os
import urllib
import urllib2
import requests

import time
import random

import jd_logger
import jd_utils
import wb_cfg
from weibo import Client

from jd_utils import log_info, log_warn

class SinaWeibo:
    def __init__ (self, username = wb_cfg.USER, passwd = wb_cfg.PASSWD, logger = None):
        self.APP_KEY = wb_cfg.APP_KEY
        self.APP_SECRET = wb_cfg.APP_SECRET
        self.CALLBACK = wb_cfg.CALLBACK
        self.USER = username
        self.PASSWD = passwd
        self.UID = wb_cfg.UID
        self.log = logger
        try:
            self.client = Client(self.APP_KEY, self.APP_SECRET, self.CALLBACK, 
                                token=None, 
                                username=self.USER, 
                                password=self.PASSWD)
            log_info(u"微博模块", u"User %s Login Successful!" % self.USER, self.log)
        except Exception:
            log_warn(u"微博模块", u"User %s Login Failed!" % self.USER, self.log)

        
     
    def get_timeline(self):
        if self.client:
            return self.client.get('users/show', uid = self.UID)
        
    def get_friend_status(self, cnt = 20):
        if self.client:
            return self.client.get('statuses/friends_timeline', uid = self.UID, count = cnt)
    
    def post_statuses(self, str_msg):
        if self.client:
            self.client.post('statuses/update', status = str_msg, uid = self.UID)
    
    def repost_friend(self, iterval):
        # 每相隔多久时间，转发一次朋友圈的微博
        # 朋友圈
        statuses = (self.get_friend_status( cnt = 100 ))["statuses"]
        if statuses:
            for item in statuses:
                #if item['text'].find('nicol:') != -1:
                #    log_info(u"微博模块",u"Alreadyed include me, skip it[%d]!\n" % item['id'], self.log)
                #    continue;
                #if item['id'] == wb_cfg.UID :
                #    log_info(u"微博模块",u"My Own's weibo, skip it[%d]!\n" % item['id'], self.log)
                #    print item['text']
                #    continue;
                if not REPOST_WHITE.has_key(item['user']['id']):
                    log_info(u"微博模块",u"Not in the repost_white list, skip [%d]!\n" % item['id'], self.log)
                    continue                  
                else:
                    try:
                        ret = self.client.post('statuses/repost', id = item['id'], status = u"[Nicol]强势转发微博", is_comment = 1)
                    except Exception, e:
                        log_warn(u"微博模块",u"Runtime Error:%s" % e, self.log)
                        continue
                    
                    log_info(u"微博模块",u"Repost item for [%d]!\n" % item['id'],self.log)
                
                #等待一会儿
                time.sleep(random.randint(300,1000))
        
        
        
if __name__ == "__main__":
    sw = SinaWeibo()
    sw.repost_friend(200)