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

TOOLNAME = jnjvm
USEDLIBS = Allocator CommonThread Mvm JnJVM Classpath $(GCLIB)
LINK_COMPONENTS = jit nativecodegen scalaropts instrumentation ipa ipo

include $(LEVEL)/Makefile.common
