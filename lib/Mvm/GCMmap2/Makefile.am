# $Id: Makefile.am,v 1.3 2008/03/21 16:25:31 varth Exp $
noinst_LIBRARIES = libuvm_gc_mmap2.a
noinst_PROGRAMS = mainuvm_gc_mmap2

THREADDIR=../CommonThread
ALLOCDIR=../Allocator
INCLUDEDIRS=-I@srcdir@/$(THREADDIR) -I@srcdir@/$(ALLOCDIR)

LIBTHREAD=$(THREADDIR)/libuvm_@vvmthreadtype@_thread.a
LIBALLOC=$(ALLOCDIR)/libuvm_alloc.a

libuvm_gc_mmap2_a_SOURCES = gc.cc gccollector.cc gcinit.cc gccollector.h
if HAVE_PTHREAD
libuvm_gc_mmap2_a_SOURCES += gcthread.cc
endif
libuvm_gc_mmap2_a_CXXFLAGS =$(INCLUDEDIRS) -O2

mainuvm_gc_mmap2_SOURCES = main.cc
mainuvm_gc_mmap2_CXXFLAGS = -O2
mainuvm_gc_mmap2_LDADD = libuvm_gc_mmap2.a $(LIBTHREAD) $(LIBALLOC)
mainuvm_gc_mmap2_LDFLAGS =

CLEANFILES = *~ *.bak .*.sw?
