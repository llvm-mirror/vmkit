##===- tools/n3/Mono/Makefile ------------------------------*- Makefile -*-===##
# 
#                     The vmkit project
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
# 
##===----------------------------------------------------------------------===##
LEVEL = ../..

include $(LEVEL)/Makefile.config

TOOLNAME = n3-mono
LINK_COMPONENTS = engine scalaropts instrumentation ipa ipo
USEDLIBS = Allocator CommonThread Mvm N3 Main $(GCLIB) Mono

include $(LEVEL)/Makefile.common
