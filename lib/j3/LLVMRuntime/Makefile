##===- lib/JnJVM/LLVMRuntime/Makefile ----------------------*- Makefile -*-===##
# 
#                     The vmkit project
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
# 
##===----------------------------------------------------------------------===##
LEVEL = ../../..

include $(LEVEL)/Makefile.config

VMKIT_RUNTIME_IN=$(PROJ_SRC_DIR)/runtime-default.ll
VMKIT_RUNTIME_IN+=$(PROJ_SRC_DIR)/runtime-mmtk-thread.ll
VMKIT_RUNTIME_IN+=$(PROJ_SRC_DIR)/runtime-single.ll

VMKIT_RUNTIME_OUT=LLVMRuntime.inc

include $(LEVEL)/Makefile.common
