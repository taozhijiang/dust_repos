#!/usr/bin/python
#-*- coding: utf-8 -*-
#encoding=utf-8

import time
import jd_logger

def encoding(data):
    types = ['utf-8', 'ascii', 'utf-16', 'gb2312','gbk','iso-8859-1']   #可以添加其他字符编码
    for typ in types:
	try:
	    return data.decode(typ).encode("utf-8")
	except:
	    pass
    return None
    
    
def current_time():
    return time.strftime('%m-%d %H:%M',time.localtime(time.time()))

def log_info(method_name, message, logger = None):
    if logger:
	logger.info(method_name, message)
    else:
	print method_name + u" : " + message

def log_warn(method_name, message, logger = None):
    if logger:
	logger.warning(method_name, message)
    else:
	print method_name + u" : " + message
	