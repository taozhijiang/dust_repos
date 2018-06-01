#!/usr/bin/python
#-*- coding: utf-8 -*-
#encoding=utf-8

import logging
import os
import jd_utils

from jd_config import LOG_FILE_NAME, LOG_FORMAT, LOG_PATH, DEBUG_SW

class Jd_Logger(object):
    """
    Global logging method. Saved to path as defined in setting.LOG_PATH and
    settings.LOG_FILE_NAME.
    """
    def __init__(self, cls_name="Default", *args, **kwargs):
        self.path = kwargs.get('path', LOG_PATH)
        self.log_format = kwargs.get('log_format', LOG_FORMAT)
        self.log_file_name = kwargs.get('log_file_name', LOG_FILE_NAME)
        self.log_file = os.path.join(self.path, self.log_file_name)
        self.debug = kwargs.get('debug', DEBUG_SW)
        self.cls_name = cls_name
        self.verbose = kwargs.get('verbose', False)
        
        # setup the logger object
        logging.basicConfig(filename=self.log_file, format=self.log_format)
    
    def log(self, cls_name, method_name, message, type):
        """
        Instantiate the logger obj and log to file depending on the msg type.
        """
        logger = logging.getLogger("%s - %s:" % (cls_name, method_name))
        
        # print the message out if the logger is set to be verbose
        if self.verbose:
            print ("%s: [%s - %s()] %s:" % (type.upper(), cls_name, method_name, message))
        
        # add extra params
        extras = {
            'class': cls_name,
            'method': method_name,
        }
        
        # determine the logging type and set as required - options are info, warning, error
        if type == 'info':
            if self.debug:
                logger.setLevel(logging.INFO)
                logger.info(jd_utils.encoding(message), extra=extras)
                
        elif type == 'warning':
            logger.setLevel(logging.WARNING)
            logger.warning(message, extra=extras)
            
        elif type == 'error':
            logger.setLevel(logging.ERROR)
            logger.error(message, extra=extras)
            raise Exception("%s: %s" % (type.upper(), message))
            
        elif type == 'critical':
            logger.setLevel(logging.CRITICAL)
            logger.critical(message, extra=extras)
            raise Exception("%s: %s" % (type.upper(), message))

    # USED externals
    def info(self, method_name, message):
        self.log(self.cls_name, method_name, message, type='info')
    
    def warning(self, method_name, message):
        self.log(self.cls_name, method_name, message, type='warning')
    
    def error(self, method_name, message):
        self.log(self.cls_name, method_name, message, type='error')
    
    def critical(self, method_name, message):
        self.log(self.cls_name, method_name, message, type='critical')

if __name__ == "__main__":
    log_me = MyEng_Logger("TESTING MODULE")
    log_me.info( "Method1", "Haha Message")
    log_me.warning( "Method2", "Haha Message2")
    log_me.error( "Method3", "Haha Message3")
    log_me.critical( "Method4", "Haha Message4")