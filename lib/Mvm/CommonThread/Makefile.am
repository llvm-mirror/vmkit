noinst_LIBRARIES = libuvm_common_thread.a

GCDIR = ../GCMmap2
GCLIBS = $(GCDIR)/libuvm_gc_mmap2.a
ALLOCDIR = ../Allocator
ALLOCLIBS = $(ALLOCDIR)/libuvm_alloc.a

libuvm_common_thread_a_SOURCES = cterror.h ctthread.cpp cterror.cpp ctlock.cpp
libuvm_common_thread_a_CXXFLAGS = -frepo -O2 -W -Wall -Werror -ansi -pedantic -Wno-unused-parameter -DSERVICE_VM

CLEANFILES = *~ *.bak .*.sw?
