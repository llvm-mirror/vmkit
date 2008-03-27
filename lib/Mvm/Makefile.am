SUBDIRS = CommonThread Allocator GCMmap2
DIST_SUBDIRS = CommonThread Allocator GCMmap2

bin_PROGRAMS = main

THREADDIR=CommonThread
ALLOCDIR=Allocator
GCDIR=GCMmap2
INCLUDEDIRS=-I@srcdir@/$(THREADDIR) -I@srcdir@/$(ALLOCDIR) -I@srcdir@/$(GCDIR)

LIBTHREAD=$(THREADDIR)/libuvm_@vvmthreadtype@_thread.a
LIBALLOC=$(ALLOCDIR)/libuvm_alloc.a
LIBGC=$(GCDIR)/libuvm_gc_@vvmgctype@.a @GC_LIBS@
LIBSUVM=$(LIBGC) $(LIBALLOC) $(LIBTHREAD)
PREFIX=@prefix@

main_SOURCES = Main.cc Object.cc Sigsegv.cc JIT.cc CommandLine.cpp CommandLine.h MvmMemoryManager.h MvmMemoryManager.cpp EscapeAnalysis.cpp Disassembler.cc
main_CXXFLAGS = $(INCLUDEDIRS) -DPREFIX=\"$(PREFIX)\" -W -Wall -ansi -Wno-unused-parameter -pedantic -Wno-long-long -fno-omit-frame-pointer -O2 -g
main_LDFLAGS=@rdynamic@
main_LDADD = $(LIBSUVM) @LLVMDYLIB@
