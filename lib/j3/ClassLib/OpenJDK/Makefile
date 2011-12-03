##===- lib/j3/ClassLib/OpenJDK/Makefile --------------------*- Makefile -*-===##
# 
#                     The vmkit project
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
# 
##===----------------------------------------------------------------------===##
LEVEL = ../../../..

EXTRA_DIST = OpenJDK.inc

include $(LEVEL)/Makefile.config

MODULE_WITH_GC = Classpath

COMPILE_OPENJDK_ZIP=1
OPENJDK_JAVA_SOURCES=java/lang/VMString.java

include $(LEVEL)/Makefile.common

CXX.Flags += -I$(PROJ_SRC_DIR)/../../VMCore
CXX.Flags += -I$(PROJ_SRC_ROOT)/include/j3
CXX.Flags += -I$(PROJ_SRC_DIR)/.. -I$(PROJ_OBJ_DIR)/..
CXX.Flags += -I$(PROJ_SRC_ROOT)/include/j3/openjdk
CXX.Flags += -DVMKitOpenJDKZip=\"$(VMKIT_OPENJDK_ZIP)\"
