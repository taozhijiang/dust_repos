#!/usr/bin/python3
#-*- coding: utf-8 -*-
#encoding=utf-8

import os
import time
import sqlite3
import hashlib
import re

db_path = '/srv/www/database/'
db_file = db_path + 'search.db'

def current_time():
    return time.strftime('%Y-%m-%d %H:%M:%S',time.localtime(time.time()))

class SearchDB:
    def __init__(self):
        global db_path
        global db_file
        self.db_file = db_file
        if not os.path.exists(self.db_file):
            if not os.path.exists(db_path):
                os.makedirs(db_path)
            self.db_init()
        else:
            pass
    
    def db_init(self):
        try:
            conn = sqlite3.connect(self.db_file, timeout = 300)
            sql_creat_table = '''
                        create table if not exists search_info(
                        id integer primary key autoincrement, 
                        search_keys varchar(256) DEFAULT NULL,
                        cache_urls  varchar(256) DEFAULT NULL,
                        host_ipaddr varchar(20)  DEFAULT NULL,
                        search_time varchar(50)  DEFAULT NULL
                        );'''
            conn.execute(sql_creat_table)
            conn.commit()
        except Exception as e:
            print ('-1--'+str(e)+'--')
        finally:
            conn.close()
    
    def do_db_insert(self, host_ipaddr, search_keys, cache_url):
        sql_insert_data = '''insert into search_info(search_keys, cache_urls, host_ipaddr, search_time) values ('%s', '%s', '%s', '%s')'''%(search_keys, cache_url, host_ipaddr, current_time())
        try:
            conn = sqlite3.connect(self.db_file,  timeout = 2000,  check_same_thread = False)
            total = conn.execute(sql_insert_data)
            conn.execute(sql_insert_data)
            conn.commit()
        except Exception as e:
            print ('-2--'+str(e)+'--')
        finally:
            conn.close()
            
    def db_insert_search(self, host_ipaddr, search_keys):
        self.do_db_insert(host_ipaddr, search_keys, "")
        
    def db_insert_cache(self, host_ipaddr, cache_url):
        self.do_db_insert(host_ipaddr, "", cache_url)
            
    def db_dump(self):
        sql_dump_data = '''select * from search_info'''
        try:
            conn = sqlite3.connect(self.db_file, timeout = 2000,  check_same_thread = False)
            cursor = conn.execute(sql_dump_data)
            for row in cursor:
                print (row[0],row[1],row[2],row[3],row[4])
        except Exception as e:
            print ('-3--'+str(e)+'--')
        finally:
            conn.close()
        
    def db_statistics(self):
        total = 0
        sql_query_total = ''' select count(*) from search_info ''' 
        try:
            conn = sqlite3.connect(self.db_file, timeout = 2000,  check_same_thread = False)
            total = conn.execute(sql_query_total).fetchone()[0]
        except Exception as e:
            print ('-4--'+str(e)+'--')
        finally:
            conn.close()
        #print("总访问记录总数：%d" % total)
        return total
