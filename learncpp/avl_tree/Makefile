DEBUG ?= 1
CXX = g++
CXXFLAGS = -g -O0 -std=c++11
PACKAGE = avltree
PACKAGE_NAME = $(PACKAGE)
PACKAGE_STRING = $(PACKAGE_NAME)1.0
PACKAGE_VERSION = 1.0
SHELL = /bin/bash
VERSION = 1.0

all : $(PACKAGE)
.PHONY : all

ifeq ($(DEBUG),1)
TARGET_DIR=Debug
else
TARGET_DIR=Release
endif

$(PACKAGE) : main.cpp avl_tree.hpp
	@test -d $(TARGET_DIR) || mkdir $(TARGET_DIR)
	$(CXX) $(CXXFLAGS) $(EXTRAFLAGS) main.cpp -o $(TARGET_DIR)/$(PACKAGE)

.PHONY : clean 
clean :	
	-rm -fr $(TARGET_DIR)/$(PACKAGE)
