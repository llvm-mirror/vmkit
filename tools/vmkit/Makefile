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

TOOLNAME = vmkit


ifeq ($(WITH_LLVM_GCC), 1)

  MODULESNAME = vmkit

  ifeq ($(WITH_JNJVM), 1)
    USEDMODULES += JnJVM.bc Classpath.bc JnjvmCompiler.bc
  endif

  ifeq ($(ISOLATE_SHARING_BUILD), 1) 
    USEDMODULES += Isolate.bc
  endif

  ifeq ($(WITH_N3_PNETLIB), 1)
    USEDMODULES += N3.bc PNetLib.bc
  endif

  USEDMODULES += Allocator.bc CommonThread.bc Mvm.bc MvmCompiler.bc $(GCLIB).bc

  BUILT_SOURCES = vmkit.s
  SOURCES = vmkit.s $(notdir $(wildcard $(PROJ_SRC_DIR)/*.cpp))


else

  ifeq ($(WITH_JNJVM), 1)
    USEDLIBS += JnJVM.a Classpath.a JnjvmCompiler.a
  endif

  ifeq ($(ISOLATE_SHARING_BUILD), 1) 
    USEDLIBS += Isolate.a
  endif

  ifeq ($(WITH_N3_PNETLIB), 1)
    USEDLIBS += N3.a PNetLib.a
  endif

  USEDLIBS += Allocator.a CommonThread.a Mvm.a MvmCompiler.a $(GCLIB).a

endif

LINK_COMPONENTS = jit nativecodegen scalaropts instrumentation ipa ipo bitreader asmparser linker

include $(LEVEL)/Makefile.common

ifeq ($(WITH_N3_PNETLIB), 1)
LIBS += $(PNETLIB)/engine/libILEngine.a $(PNETLIB)/image/libILImage.a $(PNETLIB)/support/libILSupport.a \
        $(PNETLIB)/libffi/.libs/libffi.a $(PNETLIB)//dumpasm/libILDumpAsm.a
endif


