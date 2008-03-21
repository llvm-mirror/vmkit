# $Id: Makefile.am,v 1.6 2008/03/21 11:12:38 varth Exp $
lib_LTLIBRARIES = libClasspath.la

THREADDIR=../../Mvm/CommonThread
ALLOCDIR=../../Mvm/Allocator
GCDIR=../../Mvm/GCMmap2
INCLUDEDIRS=-I@srcdir@/$(THREADDIR) -I@srcdir@/$(ALLOCDIR) -I@srcdir@/$(GCDIR)\
	    -I../VMCore

PREFIX=@prefix@
LIBSUVM=$(LIBLLVM) $(LIBGC) $(LIBALLOC) $(LIBTHREAD)

libClasspath_la_SOURCES = \
	ClasspathVMSystem.cpp ClasspathVMSystem.h Classpath.cpp \
	ClasspathVMThread.cpp ClasspathVMThread.h ClasspathVMClassLoader.h \
	ClasspathVMClassLoader.cpp ClasspathVMClass.cpp ClasspathVMClass.h ClasspathVMSystemProperties.h \
	ClasspathVMSystemProperties.cpp ClasspathVMObject.h ClasspathVMObject.cpp \
	ClasspathVMRuntime.h ClasspathVMRuntime.cpp ClasspathConstructor.cpp ClasspathConstructor.h \
	ClasspathVMThrowable.h ClasspathVMThrowable.cpp ClasspathMethod.h ClasspathMethod.cpp \
	ClasspathVMField.h ClasspathVMField.cpp ClasspathVMStackWalker.cpp ClasspathVMStackWalker.h

libClasspath_la_CXXFLAGS =$(INCLUDEDIRS) -DPREFIX=\"$(PREFIX)\" -W -Wall -ansi -Wno-unused-parameter -Wno-long-long -Wno-unused-function -fno-omit-frame-pointer -DSINGLE_VM -O2 -g -Werror
