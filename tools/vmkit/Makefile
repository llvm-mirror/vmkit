##===- tools/vmkit/Makefile --------------------------------*- Makefile -*-===##
# 
#                     The VMKit project
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
# 
##===----------------------------------------------------------------------===##
LEVEL = ../..

include $(LEVEL)/Makefile.config

TOOLNAME = vmkit


ifeq ($(WITH_LLVM_GCC), 1)

  MODULESNAME = vmkit

  ifeq ($(WITH_J3), 1)
    USEDMODULES += J3.bc Classpath.bc J3Compiler.bc
  endif

  ifeq ($(WITH_N3_PNETLIB), 1)
    USEDMODULES += N3.bc PNetLib.bc
  endif

  USEDMODULES += Allocator.bc CommonThread.bc Mvm.bc MvmCompiler.bc

  ifeq ($(GC_MMTK), 1)
    USEDMODULES += FinalMMTk.bc InlineMMTk.bc
  else
    USEDMODULES += $(GCLIB).bc
  endif

  BUILT_SOURCES = vmkit.s
  SOURCES = vmkit.s $(notdir $(wildcard $(PROJ_SRC_DIR)/*.cpp))


else

  USEDLIBS += Allocator.a Mvm.a MvmCompiler.a $(GCLIB).a CommonThread.a

  ifeq ($(WITH_J3), 1)
    USEDLIBS += J3.a Classpath.a J3.a J3Compiler.a
  endif

  ifeq ($(WITH_N3_PNETLIB), 1)
    USEDLIBS += N3.a PNetLib.a
  endif

  USEDLIBS += Allocator.a Mvm.a MvmCompiler.a $(GCLIB).a CommonThread.a

endif

LINK_COMPONENTS = jit nativecodegen scalaropts instrumentation ipa ipo bitreader asmparser linker

include $(LEVEL)/Makefile.common

ifeq ($(WITH_N3_PNETLIB), 1)
LIBS += $(PNETLIB)/engine/libILEngine.a $(PNETLIB)/image/libILImage.a $(PNETLIB)/support/libILSupport.a \
        $(PNETLIB)/libffi/.libs/libffi.a $(PNETLIB)//dumpasm/libILDumpAsm.a
endif


