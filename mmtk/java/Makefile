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
NEED_BC=1
NEED_GC=1

include $(LEVEL)/Makefile.common

$(BUILD_DIR)/%.bc: $(BUILD_DIR)/%-lower.bc $(LIB_DIR)/MMTKMagic$(SHLIBEXT)
	$(Echo) "Lowering '$(notdir $@)'"
	$(Verb) $(LOPT) -load=$(LIB_DIR)/MMTKMagic$(SHLIBEXT) $(OPT_MMTK_FLAGS) -LowerJavaRT -f $< -o $@

$(BUILD_DIR)/%-lower.bc: $(BUILD_DIR)/%.jar $(VMJC) $(LIB_DIR)/MMTKRuntime$(SHLIBEXT) $(LIB_DIR)/MMTKMagic$(SHLIBEXT) 
	$(Echo) "Compiling '$(notdir $<)'"
	$(Verb) $(VMJC) $(VMJCFLAGS) -load=$(LIB_DIR)/MMTKRuntime$(SHLIBEXT) -load=$(LIB_DIR)/MMTKMagic$(SHLIBEXT) \
			-LowerMagic $< -disable-exceptions -disable-cooperativegc \
			-with-clinit=org/mmtk/vm/VM,org/mmtk/utility/*,org/mmtk/policy/*,org/j3/config/* -Dmmtk.hostjvm=org.j3.mmtk.Factory \
			-o $@ -Dmmtk.properties=$(PROJ_SRC_ROOT)/mmtk/java/vmkit.properties -disable-stubs -assume-compiled

$(BUILD_DIR)/%.jar: $(PROJ_OBJ_ROOT)/mmtk/java/build.xml $(BUILD_DIR)/.dir #$(SELF)
	$(Verb) $(ANT) -buildfile $(PROJ_OBJ_ROOT)/mmtk/java/build.xml && mv $(notdir $@) $@


ifdef ZERO
#
#
# OLD BUILD SYSTEM
#
#

# Set the LLVM build file location 
LLVMBuildTool   := $(LLVM_SRC_ROOT)/utils/llvm-build/llvm-build

MMTkLib = $(LIB_DIR)/libFinalMMTk.a

ifeq ($(shell [ ! -e $(MMTkLib) ] && echo "1"), 1)
  COMPILE_MMTK = 1
endif

ifeq ($(COMPILE_MMTK), 1)
all::
	$(Verb) $(ANT) -buildfile $(PROJ_OBJ_ROOT)/mmtk/java/build.xml
	$(Echo) "**[12] Building $(BuildMode) $(JARNAME).jar $(notdir $@) $(OPT_MMTK_FLAGS)"
	$(Verb) $(LOPT) -load=$(LIB_DIR)/JITGCPass$(SHLIBEXT) $(OPT_MMTK_FLAGS) -JITGCPass -f $(LIB_DIR)/MMTKAlloc.bc -o $(LIB_DIR)/MMTKAlloc.bc
	$(Verb) $(VMJC) $(VMJC_MMTK_FLAGS) -load=$(LIB_DIR)/MMTKRuntime$(SHLIBEXT) -load=$(LIB_DIR)/MMTKMagic$(SHLIBEXT) -LowerMagic $(PROJ_OBJ_ROOT)/mmtk/java/$(JARNAME).jar -disable-exceptions -disable-cooperativegc -with-clinit=org/mmtk/vm/VM,org/mmtk/utility/*,org/mmtk/policy/*,org/j3/config/* -Dmmtk.hostjvm=org.j3.mmtk.Factory -o $(JARNAME).bc -Dmmtk.properties=$(PROJ_SRC_ROOT)/mmtk/java/vmkit.properties -disable-stubs -assume-compiled
	$(Verb) $(LOPT) -load=$(LIB_DIR)/MMTKMagic$(SHLIBEXT) $(OPT_MMTK_FLAGS) -LowerJavaRT -f $(JARNAME).bc -o $(JARNAME)-optimized.bc
	$(Verb) $(LLVMLINK) -o $(LIB_DIR)/FinalMMTk.bc $(LIB_DIR)/MMTKAlloc.bc $(JARNAME)-optimized.bc $(LIB_DIR)/MMTKRuntime.bc
	$(Verb) $(LOPT) $(LIB_DIR)/FinalMMTk.bc -load=$(PREPARE_CODE) $(OPT_MMTK_FLAGS) -StaticGCPass -o $(LIB_DIR)/FinalMMTk.bc
	$(Verb) $(MKDIR) $(BUILD_DIR)
	$(Verb) $(LLC) $(LLC_ASM_FLAGS) -disable-fp-elim -disable-cfi -load=$(PREPARE_CODE) $(LIB_DIR)/FinalMMTk.bc -o $(BUILD_DIR)/FinalMMTk.s
	$(Verb) $(GREP) -v '\.loc[^0-9]\+[0-9]\+ 0' $(BUILD_DIR)/FinalMMTk.s > $(BUILD_DIR)/FinalMMTk.s.remove_invalid_dot_loc
	$(Verb) $(MV) -f $(BUILD_DIR)/FinalMMTk.s.remove_invalid_dot_loc $(BUILD_DIR)/FinalMMTk.s
	$(Verb) $(CLANG) -c $(BUILD_DIR)/FinalMMTk.s -o $(BUILD_DIR)/FinalMMTk.o
	$(Verb) $(Archive) $(LIB_DIR)/libFinalMMTk.a $(BUILD_DIR)/FinalMMTk.o
	$(Verb) $(Ranlib) $(LIB_DIR)/libFinalMMTk.a
	$(Verb) $(LLC) $(LLC_CPP_FLAGS) -march=cpp -cppgen=function -cppfor=VTgcmalloc $(LIB_DIR)/FinalMMTk.bc -o $(PROJ_SRC_ROOT)/mmtk/inline/MMTkMallocInline.inc
	$(Verb) $(LLC) $(LLC_CPP_FLAGS) -march=cpp -cppgen=function -cppfor=fieldWriteBarrier $(LIB_DIR)/FinalMMTk.bc -o $(PROJ_SRC_ROOT)/mmtk/inline/MMTkFieldWriteInline.inc
	$(Verb) $(LLC) $(LLC_CPP_FLAGS) -march=cpp -cppgen=function -cppfor=arrayWriteBarrier $(LIB_DIR)/FinalMMTk.bc -o $(PROJ_SRC_ROOT)/mmtk/inline/MMTkArrayWriteInline.inc
	$(Verb) $(LLC) $(LLC_CPP_FLAGS) -march=cpp -cppgen=function -cppfor=nonHeapWriteBarrier $(LIB_DIR)/FinalMMTk.bc -o $(PROJ_SRC_ROOT)/mmtk/inline/MMTkNonHeapWriteInline.inc
endif

clean-local::
	$(Verb) $(RM) -rf classes $(JARNAME).jar $(JARNAME).bc $(JARNAME)-optimized.bc $(BUILD_DIR)
endif