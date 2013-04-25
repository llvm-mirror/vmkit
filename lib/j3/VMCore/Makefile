##===- lib/j3/VMCore/Makefile ------------------------------*- Makefile -*-===##
# 
#                     The vmkit project
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
# 
##===----------------------------------------------------------------------===##
LEVEL = ../../..

EXTRA_DIST = JniClasspath.inc JniOpenJDK.inc

include $(LEVEL)/Makefile.config

MODULE_WITH_GC = J3
INCLUDES+=-I$(PROJ_SRC_ROOT)/lib/j3/ClassLib/$(CLASSPATH_DIR) -I$(PROJ_SRC_ROOT)/lib/j3/ClassLib

include $(LEVEL)/Makefile.common

