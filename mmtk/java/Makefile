##===- mmtk/java/Makefile ----------------------------------*- Makefile -*-===##
# 
#                     The vmkit project
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
# 
##===----------------------------------------------------------------------===##
LEVEL = ../..

GEN=mmtk-vmkit.bc
MODULE=FinalMMTk
MODULE_USE=MMTKAlloc MMTKRuntime
NEED_GC=1
EXTRACT_FUNCTIONS=VTgcmalloc fieldWriteBarrier arrayWriteBarrier nonHeapWriteBarrier

include $(LEVEL)/Makefile.common

$(BUILD_DIR)/%.bc: $(BUILD_DIR)/%-lower.bc $(LIB_DIR)/MMTKMagic$(SHLIBEXT)
	$(Echo) "Lowering magic '$(notdir $@)'"
	$(Verb) $(LOPT) -load=$(LIB_DIR)/MMTKMagic$(SHLIBEXT) $(OPT_MMTK_FLAGS) -LowerJavaRT -f $< -o $@

$(BUILD_DIR)/%-lower.bc: $(BUILD_DIR)/%.jar $(VMJC) $(LIB_DIR)/MMTKRuntime$(SHLIBEXT) $(LIB_DIR)/MMTKMagic$(SHLIBEXT) 
	$(Echo) "Compiling '$(notdir $<)'"
	$(Verb) $(VMJC) $(VMJCFLAGS) -load=$(LIB_DIR)/MMTKRuntime$(SHLIBEXT) -load=$(LIB_DIR)/MMTKMagic$(SHLIBEXT) \
			-LowerMagic $< -disable-exceptions -disable-cooperativegc \
			-with-clinit=org/mmtk/vm/VM,org/mmtk/utility/*,org/mmtk/policy/*,org/j3/config/* -Dmmtk.hostjvm=org.j3.mmtk.Factory \
			-o $@ -Dmmtk.properties=$(PROJ_SRC_ROOT)/mmtk/java/vmkit.properties -disable-stubs -assume-compiled

$(BUILD_DIR)/%.jar: $(PROJ_OBJ_ROOT)/mmtk/java/build.xml $(BUILD_DIR)/.dir #$(SELF)
	$(Verb) $(ANT) -buildfile $(PROJ_OBJ_ROOT)/mmtk/java/build.xml && mv $(notdir $@) $@

