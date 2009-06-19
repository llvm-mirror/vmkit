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

EXTRA_DIST = libvmjc

TOOLNAME = vmjc
USEDLIBS = JnJVM.a Classpath.a JnjvmCompiler.a Allocator.a CommonThread.a \
	   Mvm.a MvmCompiler.a $(GCLIB).a
LINK_COMPONENTS = jit nativecodegen scalaropts instrumentation ipa ipo bitwriter

include $(LEVEL)/Makefile.common
