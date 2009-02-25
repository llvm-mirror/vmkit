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
USEDLIBS = Allocator CommonThread Mvm JnJVM Classpath JnjvmCompiler $(GCLIB)
LINK_COMPONENTS = jit nativecodegen scalaropts instrumentation ipa ipo bitwriter

include $(LEVEL)/Makefile.common
