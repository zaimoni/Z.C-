# GNU compatibility core
srcdir = /CPP_App/Z.C++
prefix = /CPP_App/Z.C++
exec_prefix = $(prefix)
bindir = $(exec_prefix)/bin
includedir = $(exec_prefix)/include
libdir = $(exec_prefix)/lib

AR = ar
CC = gcc -std=gnu11
CXX = g++ -std=gnu++11

EXEEXT = exe
OBJEXT = o

CFLAGS = -fargument-alias -finline -Os -Wall --pedantic
CXXFLAGS =
ARCH_FLAGS = 
LINK_FLAGS = -Llib/host.zcc

OTHER_INCLUDEDIR =
C_MACROS = 
CXX_MACROS = -D__STDC_LIMIT_MACROS -DZAIMONI_HAVE_ACCURATE_MSIZE
