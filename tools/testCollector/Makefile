##===- tools/testCollector/Makefile ------------------------*- Makefile -*-===##
# 
#                     The vmkit project
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
# 
##===----------------------------------------------------------------------===##
LEVEL = ../..

TOOLNAME = testCollector
USEDLIBS=GCMmap2.a CommonThread.a Allocator.a GCMmap2.a Mvm.a GCMmap2.a

LINK_COMPONENTS = support

include $(LEVEL)/Makefile.common

CXX.Flags += -I$(LEVEL)/lib/Mvm/Allocator -I$(LEVEL)/lib/Mvm/GCMmap2
