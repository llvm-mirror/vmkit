##===- tools/n3/Makefile -----------------------------------*- Makefile -*-===##
# 
#                     The vmkit project
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
# 
##===----------------------------------------------------------------------===##
LEVEL = ../..

include $(LEVEL)/Makefile.config

TOOLNAME = n3-pnetlib
LINK_COMPONENTS = jit nativecodegen scalaropts instrumentation ipa ipo bitreader asmparser linker

ifeq ($(WITH_LLVM_GCC), 1)
  MODULESNAME = vmkit
  USEDMODULES = N3.bc PNetLib.bc Allocator.bc CommonThread.bc Mvm.bc \
		MvmCompiler.bc $(GCLIB).bc
  BUILT_SOURCES = vmkit.s
  SOURCES = vmkit.s $(notdir $(wildcard $(PROJ_SRC_DIR)/*.cpp))
else
  USEDLIBS = N3.a PNetLib.a Allocator.a Mvm.a MvmCompiler.a \
		     $(GCLIB).a CommonThread.a
endif

include $(LEVEL)/Makefile.common


LIBS += $(PNETLIB)/engine/libILEngine.a $(PNETLIB)/image/libILImage.a $(PNETLIB)/support/libILSupport.a \
	$(PNETLIB)/libffi/.libs/libffi.a $(PNETLIB)//dumpasm/libILDumpAsm.a

