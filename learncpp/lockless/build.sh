#!/bin/bash

g++ -std=c++11 main.cpp -o lockless
chmod u+x lockless

g++ -std=c++11 -c test_lock.cpp
g++ -std=c++11 test_lock.o -lboost_system -lboost_thread -lboost_date_time  -o test_lock
chmod u+x test_lock

g++ -std=c++11 -c test_freelock.cpp
g++ -std=c++11 test_freelock.o -lboost_system -lboost_thread -lboost_date_time  -o test_freelock
chmod u+x test_freelock
