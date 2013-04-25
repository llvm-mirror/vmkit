##===- tools/j3/Makefile -----------------------------------*- Makefile -*-===##
# 
#                     The VMKit project
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
# 
##===----------------------------------------------------------------------===##
LEVEL = ../..

include $(LEVEL)/Makefile.config

TOOL=j3
USE=Classpath J3 J3Compiler Vmkit VmkitCompiler CommonThread FinalMMTk InlineMMTk Precompiled BootstrapClasses
BUILD_FRAMETABLE = 1
LINK_COMPONENTS = jit nativecodegen scalaropts instrumentation ipa ipo

include $(LEVEL)/Makefile.common
