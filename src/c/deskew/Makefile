# Project directories
VCPKG_INC_DIR := /usr/local/var/vcpkg/installed/x64-osx/include
X11_INC_DIR := /usr/X11R6/include
VCPKG_LIB_DIR := /usr/local/var/vcpkg/installed/x64-osx/lib
X11_LIB_DIR := /usr/X11R6/lib

# Compiler settings
CC := g++
STD_FLAGS := -std=c++11
WAR_FLAGS := -Wall -g
OPT_FLAGS := -O2
INC_DIR_FLAGS := -I${VCPKG_INC_DIR} -I${X11_INC_DIR}
LIB_DIR_FLAGS := -L${VCPKG_LIB_DIR} -L${X11_LIB_DIR}
LIB_FLAGS := -lm -pthread -lX11 -lboost_program_options -lboost_filesystem

override CPPFLAGS += ${OPT_FLAGS} ${WAR_FLAGS} ${STD_FLAGS} ${INC_DIR_FLAGS} ${LIB_DIR_FLAGS} ${LIB_FLAGS} 

# Implicit rules
% : %.cpp
	$(CC) $(CPPFLAGS) $< -o $@

# Default rule
.PHONY: all
all : deskew