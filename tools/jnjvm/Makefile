##===- tools/jnjvm/Makefile --------------------------------*- Makefile -*-===##
# 
#                     The vmkit project
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
# 
##===----------------------------------------------------------------------===##
LEVEL = ../..

include $(LEVEL)/Makefile.config

TOOLNAME = jnjvm

ifeq ($(WITH_LLVM_GCC), 1)

  MODULESNAME = jnjvm
  USEDMODULES = JnJVM.bc Classpath.bc JnjvmCompiler.bc Allocator.bc CommonThread.bc \
		Mvm.bc MvmCompiler.bc $(GCLIB).bc

  ifeq ($(ISOLATE_SHARING_BUILD), 1) 
    USEDMODULES += Isolate.bc
  endif

  BUILT_SOURCES = jnjvm.s
  SOURCES = jnjvm.s $(notdir $(wildcard $(PROJ_SRC_DIR)/*.cpp))

else

USEDLIBS = JnJVM.a Classpath.a JnjvmCompiler.a Allocator.a CommonThread.a \
	   Mvm.a MvmCompiler.a $(GCLIB).a

  ifeq ($(ISOLATE_SHARING_BUILD), 1) 
    USEDLIBS += Isolate
  endif

endif


LINK_COMPONENTS = jit nativecodegen scalaropts instrumentation ipa ipo bitreader asmparser linker


include $(LEVEL)/Makefile.common
