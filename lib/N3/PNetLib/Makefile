##===- lib/N3/PNetLib/Makefile -----------------------------*- Makefile -*-===##
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
  MODULE_NAME = PNetLib
else
  LIBRARYNAME = PNetLib
endif

include $(LEVEL)/Makefile.common
CXX.Flags += -I$(PROJ_SRC_DIR)/../VMCore
