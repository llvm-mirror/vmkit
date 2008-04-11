noinst_LIBRARIES = libuvm_alloc.a
noinst_PROGRAMS = mainuvm_alloc

libuvm_alloc_a_SOURCES = gcalloc.cpp gcalloc.h gcchunk.cpp gcchunk.h gcerror.cpp gcerror.h gcmapper.cpp gcmapper.h osdep.h
libuvm_alloc_a_CXXFLAGS = -O2 -W -Wall -Werror -ansi -pedantic -Wno-variadic-macros -Wno-unused-parameter

mainuvm_alloc_SOURCES = main.cpp
mainuvm_alloc_CXXFLAGS =
mainuvm_alloc_LDADD = libuvm_alloc.a

CLEANFILES = *~ *.bak .*.sw?
