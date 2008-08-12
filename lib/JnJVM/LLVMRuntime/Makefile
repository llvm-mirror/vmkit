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

VMKIT_RUNTIME = runtime-default.ll

ifeq ($(ISOLATE_BUILD), 1)
VMKIT_RUNTIME += runtime-isolate.ll
endif

ifeq ($(SERVICE_BUILD), 1)
VMKIT_RUNTIME += runtime-service.ll runtime-isolate.ll runtime-multi-mmap.ll
else
ifeq ($(GC_MULTI_MMAP), 1)
VMKIT_RUNTIME += runtime-multi-mmap.ll
else
ifeq ($(GC_SINGLE_MMAP), 1)
VMKIT_RUNTIME += runtime-single-mmap.ll
else
ifeq ($(GC_BOEHM), 1)
VMKIT_RUNTIME += runtime-boehm.ll
endif
endif
endif
endif

BUILT_SOURCES = LLVMRuntime.inc

include $(LEVEL)/Makefile.common
