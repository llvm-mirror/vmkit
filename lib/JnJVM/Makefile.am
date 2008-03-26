# $Id: Makefile.am,v 1.6 2008/03/21 11:12:38 varth Exp $
SUBDIRS = VMCore Classpath
EXTRA_DIST = OpcodeNames.def
bin_PROGRAMS = main
lib_LTLIBRARIES = libjnjvm.la

THREADDIR=../Mvm/CommonThread
ALLOCDIR=../Mvm/Allocator
GCDIR=../Mvm/GCMmap2
INCLUDEDIRS=-I@srcdir@/$(THREADDIR) -I@srcdir@/$(ALLOCDIR) -I@srcdir@/$(GCDIR)

LIBTHREAD=$(THREADDIR)/libuvm_@vvmthreadtype@_thread.a
LIBALLOC=$(ALLOCDIR)/libuvm_alloc.a
LIBGC=$(GCDIR)/libuvm_gc_@vvmgctype@.a @GC_LIBS@
LIBSUVM=$(LIBGC) $(LIBALLOC) $(LIBTHREAD)

main_SOURCES = ../Mvm/Object.cc ../Mvm/Sigsegv.cc Main.cpp ../Mvm/MvmMemoryManager.cpp ../Mvm/JIT.cc ../Mvm/EscapeAnalysis.cpp


main_CXXFLAGS = $(libJnJVM_la_CXXFLAGS)
main_LDADD = VMCore/.libs/libJnJVM.a Classpath/.libs/libClasspath.a $(LIBSUVM) @LLVMDYLIB@
main_LDFLAGS = @rdynamic@

libjnjvm.la:
	gcc -shared VMCore/.libs/libJnJVM.so Classpath/.libs/libClasspath.so -o libjnjvm.so
