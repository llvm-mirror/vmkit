SUBDIRS = VMCore
bin_PROGRAMS = main

THREADDIR=../Mvm/CommonThread
ALLOCDIR=../Mvm/Allocator
GCDIR=../Mvm/GCMmap2
INCLUDEDIRS=-I@srcdir@/$(THREADDIR) -I@srcdir@/$(ALLOCDIR) -I@srcdir@/$(GCDIR)

LIBTHREAD=$(THREADDIR)/libuvm_@vvmthreadtype@_thread.a
LIBALLOC=$(ALLOCDIR)/libuvm_alloc.a
LIBGC=$(GCDIR)/libuvm_gc_@vvmgctype@.a @GC_LIBS@
PREFIX=@prefix@
LIBSUVM=$(LIBGC) $(LIBALLOC) $(LIBTHREAD)

main_SOURCES = ../Mvm/Object.cpp ../Mvm/Sigsegv.cpp Main.cpp ../Mvm/MvmMemoryManager.cpp ../Mvm/JIT.cpp ../Mvm/EscapeAnalysis.cpp

main_CXXFLAGS = $(INCLUDEDIRS) -DPREFIX=\"$(PREFIX)\" -W -Wall -ansi -Wno-unused-parameter -Wno-long-long -Wno-unused-function -fno-omit-frame-pointer -g -Werror -O2 -DSINGLE_VM
main_LDADD = VMCore/.libs/libN3.a $(LIBSUVM) @LLVMDYLIB@
main_LDFLAGS = @rdynamic@
