# Project directories
VCPKG_INC_DIR := /usr/local/var/vcpkg/installed/x64-osx/include
LLVM_INC_DIR := /usr/local/opt/llvm/include
VCPKG_LIB_DIR := /usr/local/var/vcpkg/installed/x64-osx/lib
LLVM_LIB_DIR := /usr/local/opt/llvm/lib

# Compiler settings
CC := /usr/local/opt/llvm/bin/clang++
STD_FLAGS := -std=c++20
WAR_FLAGS := -Wall -g
OPT_FLAGS := -O2
INC_DIR_FLAGS := -I${VCPKG_INC_DIR} -I${LLVM_INC_DIR}
LIB_DIR_FLAGS := -L${VCPKG_LIB_DIR} -L${LLVM_LIB_DIR}
LIB_FLAGS := -lm -pthread -lboost_program_options -lboost_filesystem -lomp -ltiff -ltiffxx -ljpeg -llzma -lz
MISC_FLAGS := -fopenmp

override CPPFLAGS += ${OPT_FLAGS} ${WAR_FLAGS} ${STD_FLAGS} ${INC_DIR_FLAGS} ${LIB_DIR_FLAGS} ${LIB_FLAGS} ${MISC_FLAGS}

# Implicit rules
% : %.cpp
	$(CC) $(CPPFLAGS) $< -o $@

# Default rule
.PHONY: all
all : deskew