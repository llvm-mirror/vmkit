##===- lib/N3/VMCore/Makefile ------------------------------*- Makefile -*-===##
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
  MODULE_NAME = N3
else
  LIBRARYNAME = N3
endif

include $(LEVEL)/Makefile.common

CXX.Flags += -I$(PROJ_OBJ_DIR)/../LLVMRuntime
