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
LINK_COMPONENTS = jit nativecodegen scalaropts instrumentation ipa ipo
USEDLIBS = N3.a PNetLib.a Allocator.a CommonThread.a Mvm.a MvmCompiler.a \
	   $(GCLIB).a

include $(LEVEL)/Makefile.common


LIBS += $(PNETLIB)/engine/libILEngine.a $(PNETLIB)/image/libILImage.a $(PNETLIB)/support/libILSupport.a \
	$(PNETLIB)/libffi/.libs/libffi.a $(PNETLIB)//dumpasm/libILDumpAsm.a

