#!/usr/bin/python
#-*- coding: utf-8 -*-
#encoding=utf-8

import jd_db
import jd_config
import jd_spider

import socket

import queue

import os
import time
import threading

from jd_spider import UrlExtendThread
from jd_spider import ConosultThread

# Server Side Configuration
HOST = jd_config.SERVER_ADDR
PORT = jd_config.SERVER_PORT

#用于保存展开页面的线程
threads_extend = []
#用于保存抓取商品资讯的线程
threads_conosult = []
#保存处理抓取商品评论的客户线程
threads_process = []

# Max buffer size
global_q = queue.Queue(maxsize=10)

# SQLITE数据库没有保护结构，用于保护数据库的互斥访问
gdb_lock = threading.RLock()

# 由DistributeThread分发，用于处理客户端请求
class ProcessPoolThread(threading.Thread):
    def __init__(self, tid):
        self.cur_thread = threading.Thread.__init__(self)
        self.jdb = jd_db.Jd_Db(jd_config.SQLITE_DB)
        self.threadID = tid
    
    def run(self):
        while True:
            # The Queue get是阻塞的
            conn, addr = global_q.get()
            self.handle_process(conn, addr)

    def handle_process(self, conn, addr):
        # 多个返回可能会被合并到一个数据包中
        print("PROCESS T[%d]正在处理%s" %(self.threadID, repr(addr)))
        req_datas = conn.recv(2048).decode()
        jreq_datas = eval(req_datas)
        for req_item in jreq_datas:
            if req_item['CLIENT'] != 0 and req_item['TYPE'] == 'REQ_URL':
                with jd_spider.gdb_lock:
                    full_url = self.jdb.db_query_comment()
                print("{%d}分配[%d]-%s" % (self.threadID, req_item['CLIENT'], full_url))
                rep_url = {'CLIENT':req_item['CLIENT'],'TYPE':'REP_URL','DATA':{'PURL':full_url, 'PATH':jd_config.JDSPR_RESULT_SERVER}}
                jrep_url = repr(rep_url) + ','
                conn.sendall(jrep_url.encode())        
            elif req_item['CLIENT'] != 0 and req_item['TYPE'] == 'FINISH':
                # 客户处理完成的线程，主要处理客户上载的文件，更新数据库记录
                print('{%s}处理结果:产品[%s]，数量[%d]，LUCK[%d]，PATH[%s]'% (self.cur_thread, req_item['DATA']['PURL'], \
                                    req_item['DATA']['CNT'], req_item['DATA']['LUCK'], req_item['DATA']['PATH']))
                if req_item['DATA']['PURL'] and req_item['DATA']['CNT']:
                    with jd_spider.gdb_lock:
                        self.jdb.db_update_comment(req_item['DATA']['PURL'], comment = 2)
                    local_f = "%s/%d_comm.txt" %(jd_config.JDSPR_RESULT_SERVER, req_item['DATA']['PID'])
                    local_path = jd_config.JDSPR_RESULT + req_item['DATA']['PATH']
                    if os.path.exists(local_f):
                        if not os.path.exists(local_path):
                            os.makedirs(local_path)
                        cmd = 'mv "%s" "%s" ' %(local_f, local_path)
                        os.system(cmd)
            else:
                print("UKNOWN CLIENT REQUEST!")
                
        return


class DistributeThread(threading.Thread):
    def __init__(self, host, port, tid):
        threading.Thread.__init__(self)   
        self.host = host
        self.port = port
        self.threadID = tid
        self.sk = socket.socket(socket.AF_INET,socket.SOCK_STREAM)         

    def run(self):
        print ("启动商品评论服务器分发线程 %d ...\n" % self.threadID)
        self.sk.bind((self.host,self.port))
        self.sk.listen(1)
    
        while True:
            conn, addr = self.sk.accept()
            print("SERVER[%d], Recv request from %s" % (self.threadID, addr))
            global_q.put((conn, addr))
            
        print ("我怎么可能退出。。。。")
        return


if __name__ == '__main__':
    
    if not os.path.exists(jd_config.PRJ_PATH):
        os.makedirs(jd_config.PRJ_PATH)
    
    if not os.path.exists(jd_config.JDSPR_RESULT):
        os.makedirs(jd_config.JDSPR_RESULT)
        
    if not os.path.exists(jd_config.JDSPR_RESULT_SERVER):
        os.makedirs(jd_config.JDSPR_RESULT_SERVER)
	
    jdb = jd_db.Jd_Db(jd_config.SQLITE_DB)

    jd_spider.get_product_ids(jd_config.JDSPR_START_URL, jdb, 0)

    print("初始化 -- 开启URL抓取线程")
    for i in range(2):
        t = UrlExtendThread(i)
        t.start()
        time.sleep(2)
        threads_extend.append(t)

    print("初始化 -- 开启商品咨询抓取线程")
    for i in range(10, 15):
        t = ConosultThread(i)
        t.start()
        time.sleep(2)
        threads_conosult.append(t)
        
    print("初始化 -- 开启商品评论分发和处理线程")
    thread_distr = DistributeThread(HOST, PORT, 99)
    thread_distr.start()    
    for i in range(20, 25):
        t = ProcessPoolThread(i)
        t.start()
        time.sleep(2)
        threads_process.append(t)
    
    print("===========================")
    print("启动完成 -- 开启商品评论分发线程")
    
    
    while True:
        time.sleep(25)
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
        if thread_distr.isAlive():
            print ('A ', end = '')
        else:
            print ('D ', end = '')
        print (' | ', end = '')
        for item in threads_process:
            if item.isAlive():
                print ('A ', end = '')
            else:
                print ('D ', end = '')
        print (' | ', end = '')	
        print("")        
        
    print ("服务器主程序结束...")
    
