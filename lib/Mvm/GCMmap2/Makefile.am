noinst_LIBRARIES = libuvm_gc_mmap2.a
noinst_PROGRAMS = mainuvm_gc_mmap2

THREADDIR=../CommonThread
ALLOCDIR=../Allocator
INCLUDEDIRS=-I@srcdir@/$(THREADDIR) -I@srcdir@/$(ALLOCDIR)

LIBTHREAD=$(THREADDIR)/libuvm_common_thread.a
LIBALLOC=$(ALLOCDIR)/libuvm_alloc.a

libuvm_gc_mmap2_a_SOURCES = gc.cpp gccollector.cpp gcinit.cpp gccollector.h
if HAVE_PTHREAD
libuvm_gc_mmap2_a_SOURCES += gcthread.cpp
endif
libuvm_gc_mmap2_a_CXXFLAGS =$(INCLUDEDIRS) -O2 -W -Wall -ansi -pedantic -Wno-unused-parameter -Wno-variadic-macros

mainuvm_gc_mmap2_SOURCES = main.cpp
mainuvm_gc_mmap2_CXXFLAGS = -O2
mainuvm_gc_mmap2_LDADD = libuvm_gc_mmap2.a $(LIBTHREAD) $(LIBALLOC)
mainuvm_gc_mmap2_LDFLAGS =

CLEANFILES = *~ *.bak .*.sw?
