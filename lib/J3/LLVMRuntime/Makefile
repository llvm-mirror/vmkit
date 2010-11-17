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

VMKIT_RUNTIME = $(PROJ_SRC_DIR)/../../Mvm/Compiler/LLVMRuntime.gen.ll \
								$(PROJ_SRC_DIR)/runtime-default.ll

ifeq ($(ISOLATE_BUILD), 1)
VMKIT_RUNTIME += $(PROJ_SRC_DIR)/runtime-isolate.ll
endif

ifeq ($(SERVICE_BUILD), 1)
VMKIT_RUNTIME += $(PROJ_SRC_DIR)/runtime-service.ll $(PROJ_SRC_DIR)/runtime-isolate.ll
endif

ifeq ($(SINGLE_BUILD), 1)
VMKIT_RUNTIME += $(PROJ_SRC_DIR)/runtime-single.ll
endif

BUILT_SOURCES = LLVMRuntime.inc

include $(LEVEL)/Makefile.common
