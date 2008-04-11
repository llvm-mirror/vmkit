SUBDIRS = VMCore
bin_PROGRAMS = main

PREFIX=@prefix@

main_SOURCES = ../Mvm/Object.cpp ../Mvm/Sigsegv.cpp Main.cpp ../Mvm/MvmMemoryManager.cpp ../Mvm/JIT.cpp ../Mvm/EscapeAnalysis.cpp

main_CXXFLAGS = -DPREFIX=\"$(PREFIX)\" -W -Wall -ansi -Wno-unused-parameter -Wno-long-long -Wno-unused-function -fno-omit-frame-pointer -g -Werror -O2
main_LDADD = VMCore/.libs/libN3.a @GC_LIBS@ @GCTHREAD_LIBS@ @LLVMDYLIB@
main_LDFLAGS = @rdynamic@
