##===- lib/j3/ClassLib/OpenJDK/Makefile --------------------*- Makefile -*-===##
# 
#                     The vmkit project
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
# 
##===----------------------------------------------------------------------===##
$(error must adapt the open jdk building process)

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

###############################################################################
# COMPILE_OPENJDK_ZIP: Build the specified java sources and zip them up
###############################################################################
ifeq ($(COMPILE_OPENJDK_ZIP), 1)


# Some variables
VMKIT_OPENJDK_ZIP=$(PROJ_OBJ_DIR)/vmkit-openjdk.zip
CLASSES_DIR=$(PROJ_OBJ_DIR)/classes

all-local:: $(VMKIT_OPENJDK_ZIP)
clean-local::
	-$(Verb) $(RM) -f $(CLASSES_DIR) $(VMKIT_OPENJDK_ZIP)

$(VMKIT_OPENJDK_ZIP): $(addprefix $(PROJ_SRC_DIR)/,$(OPENJDK_JAVA_SOURCES))
	$(Echo) "**[13] Building OpenJDK java sources into zip"
	-$(Verb) $(RM) -rf $@ $(CLASSES_DIR)
	$(Verb) $(MKDIR) $(CLASSES_DIR)
	$(Verb) $(JAVAC) $^ -d $(CLASSES_DIR)
	cd $(CLASSES_DIR) && $(ZIP) -qr $(VMKIT_OPENJDK_ZIP) .

endif

