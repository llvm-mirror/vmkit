##===- tools/vmjc/Makefile --------------------------------*- Makefile -*-===##
# 
#                     The VMKit project
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
# 
##===----------------------------------------------------------------------===##
LEVEL = ../..

include $(LEVEL)/Makefile.config

EXTRA_DIST = libvmjc

TOOLNAME = vmjc


ifeq ($(WITH_LLVM_GCC), 1)
  MODULESNAME = vmkit
  USEDMODULES = JnJVM.bc Classpath.bc JnjvmCompiler.bc Allocator.bc \
		CommonThread.bc Mvm.bc MvmCompiler.bc $(GCLIB).bc

  BUILT_SOURCES = vmkit.s
  SOURCES = vmkit.s $(notdir $(wildcard $(PROJ_SRC_DIR)/*.cpp))

else

  USEDLIBS = JnJVM.a Classpath.a JnJVM.a JnjvmCompiler.a Allocator.a \
	     Mvm.a MvmCompiler.a $(GCLIB).a CommonThread.a
endif

LINK_COMPONENTS = jit nativecodegen scalaropts instrumentation ipa ipo bitwriter bitreader asmparser linker

include $(LEVEL)/Makefile.common
