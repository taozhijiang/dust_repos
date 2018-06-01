#!/usr/bin/python
#-*- coding: utf-8 -*-
#encoding=utf-8

import time

def encoding(data):
    types = ['utf-8', 'gb18030',  'gb2312','gbk','iso-8859-1', 'big5', 'ascii', 'utf-16', 'hz', 'iso2022_jp_2', 'big5hkscs', 'cp950', 'str']   #可以添加其他字符编码
    for typ in types:
        try:
            return data.decode(typ).encode("utf-8")
        except:
            pass
    return None
    
    
def current_time():
    return time.strftime('%Y-%m-%d-%H-%M-%S',time.localtime(time.time()))