# $Id: Makefile.am,v 1.6 2008/03/21 11:12:38 varth Exp $
SUBDIRS = VMCore Classpath
EXTRA_DIST = OpcodeNames.def
bin_PROGRAMS = main
#lib_LTLIBRARIES = libjnjvm.la

main_SOURCES = ../Mvm/Object.cpp ../Mvm/Sigsegv.cpp Main.cpp ../Mvm/MvmMemoryManager.cpp ../Mvm/JIT.cpp ../Mvm/EscapeAnalysis.cpp
main_CXXFLAGS = -DPREFIX=\"$(PREFIX)\" -W -Wall -ansi -Wno-unused-parameter -Wno-long-long -Wno-unused-function -fno-omit-frame-pointer -g -Werror -O2
main_LDADD = VMCore/.libs/libJnJVM.a Classpath/.libs/libClasspath.a @GC_LIBS@ @GCTHREAD_LIBS@ @LLVMDYLIB@
main_LDFLAGS = @rdynamic@

#libjnjvm.la:
#	gcc -shared VMCore/.libs/libJnJVM.$(DYLIB_EXTENSION) Classpath/.libs/libClasspath.$(DYLIB_EXTENSION) -o libjnjvm.$(DYLIB_EXTENSION)
