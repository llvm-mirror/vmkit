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
LINK_COMPONENTS = jit nativecodegen scalaropts instrumentation ipa ipo
USEDLIBS = N3.a Mono.a Allocator.a CommonThread.a Mvm.a MvmCompiler.a \
	   $(GCLIB).a

include $(LEVEL)/Makefile.common

CXX.Flags += `pkg-config --cflags glib-2.0 gthread-2.0`
LIBS += `pkg-config --libs glib-2.0 gthread-2.0`
CXX.Flags += `pkg-config --cflags gmodule-2.0`
LIBS += `pkg-config --libs gmodule-2.0`

