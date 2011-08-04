##===- lib/JnJVM/Compiler/Makefile -------------------------*- Makefile -*-===##
# 
#                     The VMKit project
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
# 
##===----------------------------------------------------------------------===##
LEVEL = ../../..

include $(LEVEL)/Makefile.config

MODULE_WITH_GC = J3Compiler

include $(LEVEL)/Makefile.common

CXX.Flags += -I$(PROJ_OBJ_DIR)/../LLVMRuntime -I$(PROJ_SRC_DIR)/../Classpath -I$(PROJ_SRC_DIR)/../VMCore
