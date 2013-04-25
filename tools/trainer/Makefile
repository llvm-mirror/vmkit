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

include $(LEVEL)/Makefile.common

ifndef VERBOSE
  J3.Flags := > /dev/null
endif

$(BUILD_DIR)/Precompiled.bc: $(BUILD_DIR)/HelloWorld.class $(LIB_DIR)/prepare-code$(SHLIBEXT) $(PRECOMPILER)
	$(Echo) "Pre-compiling bootstrap code"
	$(Verb) $(PRECOMPILER) -cp $(dir $<) $(basename $(notdir $<)) $(J3.Flags) && mv generated.bc $@

$(BUILD_DIR)/BootstrapClasses.bc: $(BUILD_DIR)/.dir
	$(Echo) "Building precompiled classes"
	$(Verb) $(PRECOMPILER) -emit-class-bytes $(J3.Flags) && mv classes.bc $@

$(BUILD_DIR)/HelloWorld.class: HelloWorld.java $(SELF) $(BUILD_DIR)/.dir
	$(Echo) "Compiling trainer '$<'"
	$(Verb) javac -source $(JAVAC_TARGET) -target $(JAVAC_TARGET) $< -d $(dir $@)

LLC_OPT+=-disable-branch-fold -disable-debug-info-print

ifdef ZERO
HelloWorld.class: HelloWorld.java
	$(Echo) "Compiling trainer"
	$(Verb) javac -source $(JAVAC_TARGET) -target $(JAVAC_TARGET) $< -d .

generated.bc: $(PRECOMPILER) HelloWorld.class
	$(Echo) "Pre-compiling bootstrap code"
	$(Verb) $(PRECOMPILER) -cp $$PWD HelloWorld $(J3.Flags)

Precompiled.bc: HelloWorld.class $(LibDir)/StaticGCPass$(SHLIBEXT) $(LibDir)/StaticGCPrinter$(SHLIBEXT) generated.bc
	$(Echo) "Building precompiled bootstrap code"
	$(Verb) $(MKDIR) $(ObjDir)
	$(Verb) $(LLC) -disable-branch-fold -disable-cfi -disable-debug-info-print -disable-fp-elim $(PRECOMPILER_FLAGS) -load=$(LibDir)/StaticGCPrinter$(SHLIBEXT) generated.bc -o $(ObjDir)/Precompiled.s
	$(Verb) $(CC) -c $(ObjDir)/Precompiled.s -o $(ObjDir)/Precompiled.o
	$(Verb) $(Archive) $(LibDir)/libPrecompiled.a $(ObjDir)/Precompiled.o
	$(Verb) $(Ranlib) $(LibDir)/libPrecompiled.a
	$(Verb) $(CP) generated.bc Precompiled.bc

classes.bc: $(PRECOMPILER)
	$(Echo) "Pre-compiling classes"
	$(Verb) $(PRECOMPILER) -emit-class-bytes $(J3.Flags)

BootstrapClasses.bc: classes.bc
	$(Echo) "Building precompiled classes"
	$(Verb) $(MKDIR) $(ObjDir)
	$(Verb) $(LLC) classes.bc -o $(ObjDir)/BootstrapClasses.s
	$(Verb) $(CC) -c $(ObjDir)/BootstrapClasses.s -o $(ObjDir)/BootstrapClasses.o
	$(Verb) $(Archive) $(LibDir)/libBootstrapClasses.a $(ObjDir)/BootstrapClasses.o
	$(Verb) $(Ranlib) $(LibDir)/libBootstrapClasses.a
	$(Verb) $(CP) classes.bc BootstrapClasses.bc

clean-local::
	$(Verb) $(RM) -f HelloWorld.class generated.bc classes.bc Precompiled.bc BootstrapClasses.bc
endif