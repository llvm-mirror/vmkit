##===- tools/llcj/libjnjvm/Makefile-------------------------*- Makefile -*-===##
# 
#                     The vmkit project
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
# 
##===----------------------------------------------------------------------===##
LEVEL = ../../..

include $(LEVEL)/Makefile.config

VMKIT_LIBRARYNAME = jnjvmjit
VMKIT_BUILD_ARCHIVE = 1
USEDLIBS = Allocator CommonThread Mvm JnJVM Classpath $(GCLIB) MvmCompiler \
	   JnjvmCompiler

include $(LEVEL)/Makefile.common
