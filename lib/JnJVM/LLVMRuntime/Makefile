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

VMKIT_RUNTIME = $(PROJ_SRC_DIR)/runtime-default.ll

ifeq ($(ISOLATE_BUILD), 1)
VMKIT_RUNTIME += $(PROJ_SRC_DIR)/runtime-isolate.ll
endif

ifeq ($(SERVICE_BUILD), 1)
VMKIT_RUNTIME += $(PROJ_SRC_DIR)/runtime-service.ll $(PROJ_SRC_DIR)/runtime-isolate.ll $(PROJ_SRC_DIR)/runtime-multi-mmap.ll
else
ifeq ($(GC_MULTI_MMAP), 1)
VMKIT_RUNTIME += $(PROJ_SRC_DIR)/runtime-multi-mmap.ll
else
ifeq ($(GC_SINGLE_MMAP), 1)
VMKIT_RUNTIME += $(PROJ_SRC_DIR)/runtime-single-mmap.ll
else
ifeq ($(GC_BOEHM), 1)
VMKIT_RUNTIME += $(PROJ_SRC_DIR)/runtime-boehm.ll
endif
endif
endif
endif

BUILT_SOURCES = LLVMRuntime.inc

include $(LEVEL)/Makefile.common
