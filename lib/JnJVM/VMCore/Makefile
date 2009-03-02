##===- lib/JnJVM/VMCore/Makefile ---------------------------*- Makefile -*-===##
# 
#                     The vmkit project
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
# 
##===----------------------------------------------------------------------===##
LEVEL = ../../..

LIBRARYNAME = JnJVM

include $(LEVEL)/Makefile.config

include $(LEVEL)/Makefile.common

CXX.Flags += -I$(PROJ_OBJ_DIR)/../Classpath -I$(PROJ_OBJ_DIR)/../LLVMRuntime -I$(PROJ_SRC_DIR)/../Classpath $(CLASSPATH_FLAGS) -I$(PROJ_SRC_DIR)/../../../include/jnjvm

ifeq ($(ISOLATE_BUILD), 1)
  CXX.Flags += -I$(PROJ_SRC_DIR)/../Isolate
endif
