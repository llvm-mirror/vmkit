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

TOOLNAME = vmjc
USEDLIBS = Allocator CommonThread Mvm JnJVM Classpath $(GCLIB)
LINK_COMPONENTS = jit nativecodegen scalaropts instrumentation ipa ipo bitwriter

ifeq ($(ISOLATE_BUILD), 1) 
    USEDLIBS += Isolate
endif

include $(LEVEL)/Makefile.common
