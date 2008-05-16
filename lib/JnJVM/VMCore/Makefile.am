EXTRA_DIST = OpcodeNames.def
lib_LTLIBRARIES = libJnJVM.la

PREFIX=@prefix@

libJnJVM_la_SOURCES = \
	JavaAccess.h JavaArray.cpp JavaArray.h JavaClass.cpp JavaClass.h \
	JavaConstantPool.cpp JavaConstantPool.h JavaJIT.cpp JavaJIT.h JavaObject.cpp \
	JavaObject.h JavaThread.cpp JavaThread.h \
	JavaTypes.cpp JavaTypes.h Jnjvm.cpp Jnjvm.h Reader.cpp Reader.h \
	Zip.h Zip.cpp types.h debug.h VirtualTables.cpp LockedMap.h LockedMap.cpp \
	JavaIsolate.h JavaIsolate.cpp JavaString.h JavaString.cpp JavaInitialise.cpp JavaJITOpcodes.cpp \
	JavaBacktrace.cpp NativeUtil.h NativeUtil.cpp Jni.cpp \
	JavaCache.h JavaCache.cpp JavaUpcalls.h JavaUpcalls.cpp \
	JnjvmModuleProvider.h JnjvmModuleProvider.cpp JavaRuntimeJIT.cpp JavaMetaJIT.cpp JavaJITInitialise.cpp \
	LowerConstantCalls.cpp JnjvmModule.h JnjvmModule.cpp

if SERVICE_BUILD
libJnJVM_la_SOURCES += ServiceDomain.cpp
endif

libJnJVM_la_CXXFLAGS =-DPREFIX=\"$(PREFIX)\" -W -Wall -ansi -Wno-unused-parameter -Wno-long-long -Wno-unused-function -fno-omit-frame-pointer -g -Werror -O2 -I../LLVMRuntime
