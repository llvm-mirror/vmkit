##===- tools/precompiler/trainer/Makefile ------------------*- Makefile -*-===##
# 
#                     The VMKit project
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
# 
##===----------------------------------------------------------------------===##
LEVEL = ../..

MODULE=Precompiled
GEN=Precompiled.bc BootstrapClasses.bc
NEED_GC=1

LLC_OPT+= -disable-branch-fold 
#-disable-debug-info-print

include $(LEVEL)/Makefile.common

ifndef VERBOSE
  PRECOMPILER_OPT := > /dev/null
endif

$(BUILD_DIR)/Precompiled.bc: $(BUILD_DIR)/HelloWorld.class $(LIB_DIR)/prepare-code$(SHLIBEXT) $(PRECOMPILER)
	$(Echo) "Pre-compiling bootstrap code"
	$(Verb) $(PRECOMPILER) -cp $(dir $<) $(basename $(notdir $<)) $(PRECOMPILER_OPT) && mv generated.bc $@

$(BUILD_DIR)/BootstrapClasses.bc: $(BUILD_DIR)/.dir
	$(Echo) "Building precompiled classes"
	$(Verb) $(PRECOMPILER) -emit-class-bytes $(PRECOMPILER_OPT) && mv classes.bc $@

$(BUILD_DIR)/HelloWorld.class: HelloWorld.java $(SELF) $(BUILD_DIR)/.dir
	$(Echo) "Compiling trainer '$<'"
	$(Verb) javac -source $(JAVAC_TARGET) -target $(JAVAC_TARGET) $< -d $(dir $@)
