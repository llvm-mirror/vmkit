##===- lib/JnJVM/Classpath/Makefile ------------------------*- Makefile -*-===##
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
  MODULE_NAME = Classpath
else
  LIBRARYNAME = Classpath
endif

include $(LEVEL)/Makefile.common

CXX.Flags += -I$(PROJ_SRC_DIR)/../VMCore $(CLASSPATH_FLAGS)

ifeq ($(ISOLATE_BUILD), 1)
  CXX.Flags += -I$(PROJ_SRC_DIR)/../Isolate
endif
