##===- tools/vtoffset/Makefile -------)---------------------*- Makefile -*-===##
# 
#                     The vmkit project
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
# 
##===----------------------------------------------------------------------===##
LEVEL = ../..

include $(LEVEL)/Makefile.config

TOOLNAME = vtoffset
USEDLIBS = Mvm CommonThread Allocator $(GCLIB)
LINK_COMPONENTS = jit nativecodegen scalaropts instrumentation ipa ipo bitwriter

include $(LEVEL)/Makefile.common
