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
LIBSUVM=$(LIBLLVM) $(LIBGC) $(LIBALLOC) $(LIBTHREAD)

main_SOURCES = ../Mvm/Object.cc ../Mvm/Sigsegv.cc Main.cpp ../Mvm/MvmMemoryManager.cpp ../Mvm/JIT.cc ../Mvm/EscapeAnalysis.cpp


main_CXXFLAGS = $(libJnJVM_la_CXXFLAGS)
main_LDADD = VMCore/.libs/libN3.a $(LIBSUVM) @LLVMDYLIB@ -lopcodes -lbfd -liberty
main_LDFLAGS = @rdynamic@
