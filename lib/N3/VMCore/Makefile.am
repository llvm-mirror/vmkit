EXTRA_DIST = SignatureNames.def OpcodeNames.def
lib_LTLIBRARIES = libN3.la

PREFIX=@prefix@

libN3_la_SOURCES = \
	Assembly.cpp Assembly.h CLIAccess.h CLISignature.cpp LockedMap.cpp LockedMap.h N3.cpp N3.h \
	Reader.cpp Reader.h VMArray.cpp VMArray.h VMCache.cpp VMCache.h VMClass.cpp VMClass.h \
	VMObject.cpp VMObject.h VMThread.cpp VMThread.h VirtualMachine.cpp VirtualMachine.h \
	VirtualTables.cpp CLIString.cpp CLIString.h N3Initialise.cpp CLIJitMeta.cpp CLIJit.h CLIJit.cpp \
	Opcodes.cpp NativeUtil.h NativeUtil.cpp PNetLib.cpp N3ModuleProvider.cpp N3ModuleProvider.h \
	PNetLib.h BackTrace.cpp LowerArrayLength.cpp

libN3_la_CXXFLAGS =-DPREFIX=\"$(PREFIX)\" -W -Wall -ansi -Wno-unused-parameter -Wno-long-long -Wno-unused-function -fno-omit-frame-pointer -g -O2
