##===- lib/Mvm/Runtime/Makefile ----------------------------*- Makefile -*-===##
# 
#                     The vmkit project
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
# 
##===----------------------------------------------------------------------===##
LEVEL = ../../..

include $(LEVEL)/Makefile.config

ifeq ($(WITH_LLVM_GCC), 1)
  MODULE_NAME = MvmCompiler
else
  LIBRARYNAME = MvmCompiler
endif

VMKIT_RUNTIME = $(PROJ_SRC_DIR)/LLVMRuntime.ll
BUILT_SOURCES = LLVMRuntime.inc

include $(LEVEL)/Makefile.common
