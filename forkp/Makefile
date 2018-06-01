DEBUG ?= 1
CXX = g++
CXXFLAGS = -g -O0 -std=c++11
PACKAGE = forkp
PACKAGE_NAME = $(PACKAGE)
PACKAGE_STRING = $(PACKAGE_NAME)1.0
PACKAGE_VERSION = 1.0
SHELL = /bin/bash
VERSION = 1.0
SRC_DIRS = ./source
EXTRAFLAGS =  -I/home/user/MacOS/ReadTheCode/boost/installdir/include -I./include/ -L/home/user/MacOS/ReadTheCode/boost/installdir/lib
#EXTRAFLAGS =  -I/home/taozj/root/include -I./include/ -L/home/taozj/root/lib
EXTRAFLAGS += -DBOOST_LOG_DYN_LINK 
EXTRAFLAGS += -lboost_system -lboost_date_time -lboost_log -lboost_log_setup -lboost_thread
EXTRAFLAGS += -lpthread
EXTRAFLAGS += -Wall -Wextra -march=native

OBJDIR = ./obj

vpath %.cpp $(SRC_DIRS)

srcs = $(filter-out main.cpp, $(notdir $(wildcard $(SRC_DIRS)/*.cpp)))
objs = $(srcs:%.cpp=$(OBJDIR)/%.o)

all : $(PACKAGE)
.PHONY : all

ifeq ($(DEBUG),1)
TARGET_DIR=Debug
else
TARGET_DIR=Release
endif

$(PACKAGE) : $(objs) 
	@test -d $(OBJDIR) || mkdir $(OBJDIR)
	@test -d $(TARGET_DIR) || mkdir $(TARGET_DIR)
	$(CXX) -c $(CXXFLAGS) $(SRC_DIRS)/main.cpp -o $(OBJDIR)/main.o  $(EXTRAFLAGS)
	$(CXX) $(OBJDIR)/main.o $^ $(CXXFLAGS) -o $(TARGET_DIR)/$(PACKAGE)  $(EXTRAFLAGS)

$(objs) : $(OBJDIR)/%.o: %.cpp
	@test -d $(OBJDIR) || mkdir $(OBJDIR)
	$(CXX) -MMD -c $(CXXFLAGS) $< -o $@  $(EXTRAFLAGS) 

#check header for obj reconstruction
-include $(OBJDIR)/*.d

.PHONY : clean 
clean :	
	-rm -fr $(OBJDIR)
	-rm -fr $(TARGET_DIR)/$(PACKAGE)
