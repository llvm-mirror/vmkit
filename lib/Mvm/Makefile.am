SUBDIRS = CommonThread Allocator GCMmap2
DIST_SUBDIRS = CommonThread Allocator GCMmap2

bin_PROGRAMS = main

PREFIX=@prefix@

main_SOURCES = Main.cpp Object.cpp Sigsegv.cpp JIT.cpp CommandLine.cpp CommandLine.h MvmMemoryManager.h MvmMemoryManager.cpp EscapeAnalysis.cpp Disassembler.cpp
main_CXXFLAGS = -DPREFIX=\"$(PREFIX)\" -W -Wall -ansi -Wno-unused-parameter -pedantic -Wno-long-long -fno-omit-frame-pointer -O2 -g
main_LDFLAGS=@rdynamic@
main_LDADD = @LLVMDYLIB@ @GC_LIBS@ @GCTHREAD_LIBS@
