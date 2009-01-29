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

LIBRARYNAME = Mvm
VMKIT_RUNTIME = $(PROJ_SRC_DIR)/LLVMRuntime.ll
VMKIT_ASSEMBLY = $(PROJ_SRC_DIR)/LLVMAssembly.ll
BUILT_SOURCES = LLVMRuntime.inc LLVMAssembly.o

SOURCES = LLVMAssembly.s $(notdir $(wildcard $(PROJ_SRC_DIR)/*.cpp))

include $(LEVEL)/Makefile.common
