#!/usr/bin/python
#-*- coding: utf-8 -*-
#encoding=utf-8

import jd_db
import jd_config
import jd_spider
import jd_logger


import os
import time
import threading

from jd_spider import UrlExtendThread
from jd_spider import ConosultThread

threads_extend = []
threads_conosult = []
threads_comment = []

gdb_lock = threading.RLock()


if __name__ == '__main__':
    
    if not os.path.exists(jd_config.PRJ_PATH):
        os.makedirs(jd_config.PRJ_PATH)
        
    main_log = jd_logger.Jd_Logger("MAIN")
    
    if not os.path.exists(jd_config.JDSPR_RESULT):
        os.makedirs(jd_config.JDSPR_RESULT)
	
    jdb = jd_db.Jd_Db(jd_config.SQLITE_DB)

    jd_spider.get_product_ids(jd_config.JDSPR_START_URL, jdb, 0)

    main_log.info("初始化","开启URL抓取线程")
    for i in range(2):
        t = UrlExtendThread(i)
        t.start()
        time.sleep(2)
        threads_extend.append(t)


    main_log.info("初始化","开启商品咨询抓取线程")
    for i in range(10, 12):
        t = ConosultThread(i)
        t.start()
        time.sleep(7)
        threads_conosult.append(t)
 
		
    while True:
        time.sleep(30)
        with jd_spider.gdb_lock:
            jdb.db_statistics()
        print ("线程状态：", end = '')
        for item in threads_extend:
            if item.isAlive():
                print ('A ', end = '')
            else:
                print ('D ', end = '')
        print (' | ', end = '')
        for item in threads_conosult:
            if item.isAlive():
                print ('A ', end = '')
            else:
                print ('D ', end = '')
        print (' | ', end = '')	
        for item in threads_comment:
            if item.isAlive():
                print ('A ', end = '')
            else:
                print ('D ', end = '')
        print("")        
        
    print ("程序结束...")
    
