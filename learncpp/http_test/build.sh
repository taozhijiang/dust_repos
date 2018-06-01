#!/bin/bash

g++ -std=c++11 http_client_test.cpp -o http_client_test -pthread 
chmod u+x http_client_test