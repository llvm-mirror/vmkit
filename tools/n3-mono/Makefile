##===- tools/n3/Mono/Makefile ------------------------------*- Makefile -*-===##
# 
#                     The vmkit project
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
# 
##===----------------------------------------------------------------------===##
LEVEL = ../..

include $(LEVEL)/Makefile.config

TOOLNAME = n3-mono
LINK_COMPONENTS = jit nativecodegen scalaropts instrumentation ipa ipo


ifeq ($(WITH_LLVM_GCC), 1)
  MODULESNAME = vmkit
  USEDMODULES = N3.bc Mono.bc Allocator.bc CommonThread.bc Mvm.bc \
		MvmCompiler.bc $(GCLIB).bc
  BUILT_SOURCES = vmkit.s
  SOURCES = vmkit.s $(notdir $(wildcard $(PROJ_SRC_DIR)/*.cpp))
else

  USEDLIBS = N3.a Mono.a Allocator.a CommonThread.a Mvm.a MvmCompiler.a \
   	     $(GCLIB).a
endif

include $(LEVEL)/Makefile.common

CXX.Flags += `pkg-config --cflags glib-2.0 gthread-2.0`
LIBS += `pkg-config --libs glib-2.0 gthread-2.0`
CXX.Flags += `pkg-config --cflags gmodule-2.0`
LIBS += `pkg-config --libs gmodule-2.0`

