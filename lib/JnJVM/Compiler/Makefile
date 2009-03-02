##===- lib/JnJVM/Compiler/Makefile -------------------------*- Makefile -*-===##
# 
#                     The VMKit project
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
# 
##===----------------------------------------------------------------------===##
LEVEL = ../../..

LIBRARYNAME = JnjvmCompiler

EXTRA_DIST = ExceptionsCheck.inc ExceptionsDwarf.inc

include $(LEVEL)/Makefile.config

include $(LEVEL)/Makefile.common

CXX.Flags += -I$(PROJ_OBJ_DIR)/../LLVMRuntime -I$(PROJ_SRC_DIR)/../Classpath $(CLASSPATH_FLAGS) -I$(PROJ_SRC_DIR)/../VMCore

ifeq ($(ISOLATE_BUILD), 1)
  CXX.Flags += -I$(PROJ_SRC_DIR)/../Isolate
endif
