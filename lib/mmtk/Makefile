#===- ./Makefile -------------------------------------------*- Makefile -*--===#
# 
#                     The vmkit project
#===------------------------------------------------------------------------===#

LEVEL := ../..

BASE_OBJ_FILES=mmtk-llvm
MODULE=mmtk

include $(LEVEL)/Makefile.rules

J3=$(BIN_DIR)/j3

$(BUILD_DIR)/mmtk-llvm.bc: $(BUILD_DIR)/.dir $(J3) $(SELF)
	$(Echo) "Generating the llvm bytecode of mmtk"
	$(Verb) $(J3) -Xaot -cp $(MMTK_PATH)/vmmagic-stub/classes/:$(MMTK_PATH)/options/classes/ -jar $(MMTK_PATH)/mmtk/mmtk.jar 