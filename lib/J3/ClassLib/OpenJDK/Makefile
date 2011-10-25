##===- lib/J3/ClassLib/OpenJDK/Makefile --------------------*- Makefile -*-===##
# 
#                     The vmkit project
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
# 
##===----------------------------------------------------------------------===##
LEVEL = ../../../..

EXTRA_DIST =

include $(LEVEL)/Makefile.config

MODULE_WITH_GC = Classpath

include $(LEVEL)/Makefile.common

CXX.Flags += -I$(PROJ_SRC_DIR)/../../VMCore -I$(PROJ_SRC_ROOT)/include/j3 -I..
