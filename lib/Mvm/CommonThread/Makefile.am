noinst_LIBRARIES = libuvm_common_thread.a

GCDIR = ../GCMmap2
GCLIBS = $(GCDIR)/libuvm_gc_mmap2.a
ALLOCDIR = ../Allocator
ALLOCLIBS = $(ALLOCDIR)/libuvm_alloc.a

libuvm_common_thread_a_SOURCES = cterror.h ctthread.cc cterror.cc ctlock.cc
libuvm_common_thread_a_CXXFLAGS = -frepo -O2

CLEANFILES = *~ *.bak .*.sw?
