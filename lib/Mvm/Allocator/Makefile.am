# $Id: Makefile.am,v 1.2 2008/03/21 11:12:38 varth Exp $
noinst_LIBRARIES = libuvm_alloc.a
noinst_PROGRAMS = mainuvm_alloc

libuvm_alloc_a_SOURCES = gcalloc.cc gcalloc.h gcchunk.cc gcchunk.h gcerror.cc gcerror.h gcmapper.cc gcmapper.h osdep.h
libuvm_alloc_a_CXXFLAGS = -O2

mainuvm_alloc_SOURCES = main.cc
mainuvm_alloc_CXXFLAGS =
mainuvm_alloc_LDADD = libuvm_alloc.a

CLEANFILES = *~ *.bak .*.sw?
