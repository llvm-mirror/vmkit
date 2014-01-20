#===- ./Makefile -------------------------------------------*- Makefile -*--===#
# 
#                     The vmkit project
#===------------------------------------------------------------------------===#

LEVEL := ../..

PARALLEL_DIRS:=magic

BASE_OBJ_FILES=mmtk-lowered
MODULE=mmtk

include $(LEVEL)/Makefile.rules

MMTK_JAR=$(MMTK_PATH)/mmtk/mmtk.jar 
LOWER=$(LIB_DIR)/mmtk-pass$(SHLIBEXT)
J3=$(BIN_DIR)/j3

$(BUILD_DIR)/mmtk-lowered.bc: $(BUILD_DIR)/mmtk-bc.bc $(LOWER)
	$(Echo) "Lowering '$(notdir $<)'"
	$(Verb) cp $< $@

$(BUILD_DIR)/mmtk-bc.bc: $(MMTK_JAR) $(BUILD_DIR)/.dir $(J3) $(SELF)
	$(Echo) "Compiling '$(notdir $<)'"
	$(Verb) $(J3) -Xaot $@ -cp $(MMTK_PATH)/vmmagic-stub/classes/:$(MMTK_PATH)/options/classes/ -jar $(MMTK_JAR)