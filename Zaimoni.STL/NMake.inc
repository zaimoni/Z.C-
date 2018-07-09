# GNU compatibility core
# prefix = ...
# exec_prefix = $(prefix)
# bindir = $(exec_prefix)/bin
# includedir = $(exec_prefix)/include
# libdir = $(exec_prefix)/lib

# install host-built libraries here for Z.C++ here
# (stage 1 of three-stage bootstrap)
host_libdir = lib/host.zcc

AR = lib
# CC, CXX handled by NMake defaults

EXEEXT = exe
OBJEXT = obj
