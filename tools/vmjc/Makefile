##===- tools/vmjc/Makefile ---------------------------------*- Makefile -*-===##
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

TOOL=vmjc
USE=J3 Classpath J3Compiler Vmkit VmkitCompiler MMTk CommonThread
LINK_COMPONENTS = jit nativecodegen scalaropts instrumentation ipa ipo bitwriter

include $(LEVEL)/Makefile.common
