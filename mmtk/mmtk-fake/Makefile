##===- mmtk/mmtk-j3/Makefile -------------------------------*- Makefile -*-===##
#
#                     The vmkit project
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
##===----------------------------------------------------------------------===##

LEVEL = ../..

include $(LEVEL)/Makefile.config

ifeq ($(WITH_LLVM_GCC), 1)
  MODULE_NAME = MMTKFake
else
  LIBRARYNAME = MMTKFake
endif

LOADABLE_MODULE = 1
USEDLIBS =

CXX.Flags += -I$(PROJ_OBJ_DIR)/../../lib/JnJVM/VMCore

include $(LEVEL)/Makefile.common

