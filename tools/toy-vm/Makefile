##===- tools/toy-vm/Makefile --------------------------------*- Makefile -*-===##
# 
#                     The vmkit project
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
# 
##===----------------------------------------------------------------------===##
LEVEL = ../..

include $(LEVEL)/Makefile.config

TOOLNAME = toy-vm

ifeq ($(WITH_LLVM_GCC), 1)

  MODULESNAME = toy-vm
  USEDMODULES = J3.bc Classpath.bc J3Compiler.bc Allocator.bc CommonThread.bc Mvm.bc MvmCompiler.bc

  ifeq ($(GC_MMTK), 1)
    USEDMODULES += FinalMMTk.bc InlineMMTk.bc
  else
    USEDMODULES += $(GCLIB).bc
  endif

  BUILT_SOURCES = toy-vm.s
  SOURCES = toy-vm.s $(notdir $(wildcard $(PROJ_SRC_DIR)/*.cpp))

else

USEDLIBS = J3.a Classpath.a J3.a J3Compiler.a Allocator.a \
	   Mvm.a MvmCompiler.a $(GCLIB).a Allocator.a CommonThread.a

endif

LINK_COMPONENTS = jit nativecodegen scalaropts instrumentation ipa ipo bitreader asmparser linker

include $(LEVEL)/Makefile.common

