##===- lib/JnJVM/Classpath/Makefile ------------------------*- Makefile -*-===##
# 
#                     The vmkit project
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
# 
##===----------------------------------------------------------------------===##
LEVEL = ../../..

LIBRARYNAME = Classpath

include $(LEVEL)/Makefile.config
include $(LEVEL)/Makefile.common

CXX.Flags += -I$(PROJ_SRC_DIR)/../VMCore $(CLASSPATH_FLAGS)

ifeq ($(ISOLATE_BUILD), 1)
  CXX.Flags += -I$(PROJ_SRC_DIR)/../Isolate
endif
