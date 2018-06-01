# forkp: a high-availability process manage framework      

[![Language](https://img.shields.io/badge/Language-GCC-green.svg)](https://gcc.gnu.org/) 
[![Author](https://img.shields.io/badge/author-taozj-blue.svg)](https://taozj.org/) 
[![License](https://img.shields.io/badge/license-BSD-red.svg)](http://yanyiwu.mit-license.org)
  
## Purpose
　　This is the server-side process manage framework, which is highly inspired by Nginx master-worker process manage methods. I have described the main idea at [this article](https://taozj.org/2016/11/%E6%B5%85%E8%B0%88%E5%A4%9A%E8%BF%9B%E7%A8%8B%E7%A8%8B%E5%BA%8F%E7%9A%84%E5%BC%80%E5%8F%91%E5%8F%8A%E8%BF%9B%E7%A8%8B%E7%9A%84%E6%8E%A7%E5%88%B6%E5%92%8C%E7%AE%A1%E7%90%86/). The master process will detect, monitor and control all its child process, which will guarantee the high-availability project deployment.    
　　c++11 standard is natived supported, but if you are required on some legacy environment such as RHEL/CentOS 6.x, then official g++ 4.4 branch only support c++0x standard, in this case you may install boost libraries for boost::function, boost::shared_ptr and so on. Pay attention to the g++ arguments in Makefile.   
　　There are two way to use this forkp:   
　　(1). Treat it as general multi-process program library, so you can develop Nginx-like master & worker multi-process project base on this work without start from zero, child process will be monitored and auto restarted when needed. In addition, it is some what different from Nginx, you can start different processes at different time, forkp will guarantee exactly as your call of spawnWorkers;   
　　(2). Treat it as general program or service monitor, you can start different already binary program, and forkp will wait for SIGCHLD and kill-0 method to detect whether target program still alive. Flexible add and remove program will be added in near futuer.   
　　Child process stdout and stderr will be atomaticly redirected and recorded at log directory, KILL the fork master process with SIGNALS specified at general.hpp will give you some basic controls.   
　　This project is being tested in production envrionment, and some other features will come soon, hope you enjoy it!   

