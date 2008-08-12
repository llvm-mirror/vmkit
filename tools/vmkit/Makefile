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
USEDLIBS = Allocator CommonThread Mvm $(GCLIB) JnJVM Classpath PNetLib N3
LINK_COMPONENTS = engine scalaropts instrumentation ipa ipo

include $(LEVEL)/Makefile.common

LIBS += $(PNETLIB)/engine/libILEngine.a $(PNETLIB)/image/libILImage.a $(PNETLIB)/support/libILSupport.a \
        $(PNETLIB)/libffi/.libs/libffi.a $(PNETLIB)//dumpasm/libILDumpAsm.a
