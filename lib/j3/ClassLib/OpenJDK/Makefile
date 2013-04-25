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

MODULE_WITH_GC = Classpath

INCLUDES+=-I$(PROJ_SRC_ROOT)/lib/j3/VMCore -I$(PROJ_SRC_ROOT)/include/j3 -I$(PROJ_SRC_ROOT)/lib/j3/ClassLib -I$(PROJ_SRC_ROOT)/include/j3/openjdk
COMMON_CFLAGS+= -DVMKitOpenJDKZip=\"$(VMKIT_OPENJDK_ZIP)\"

include $(LEVEL)/Makefile.common

INCLUDES+=-I$(PROJ_SRC_ROOT)/lib/j3/VMCore -I$(PROJ_SRC_ROOT)/include/j3 -I$(PROJ_SRC_ROOT)/lib/j3/ClassLib -I$(PROJ_SRC_ROOT)/include/j3/openjdk
COMMON_CFLAGS+= -DVMKitOpenJDKZip=\"$(VMKIT_OPENJDK_ZIP)\"

$(VMKIT_OPENJDK_ZIP): $(OPENJDK_JAVA_SOURCES)
	$(Echo) "**[13] Building OpenJDK java sources into zip"
	-$(Verb) $(RM) -rf $@ $(CLASSES_DIR)
	$(Verb) $(MKDIR) $(CLASSES_DIR)
	$(Verb) $(JAVAC) $^ -d $(CLASSES_DIR)
	cd $(CLASSES_DIR) && $(ZIP) -qr $(VMKIT_OPENJDK_ZIP) .

