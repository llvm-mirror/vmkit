##===- tools/precompiler/trainer/Makefile ------------------*- Makefile -*-===##
# 
#                     The VMKit project
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
# 
##===----------------------------------------------------------------------===##
LEVEL = ../..

EXTRA_DIST = HelloWorld.java

include $(LEVEL)/Makefile.config

all:: Precompiled.bc BootstrapClasses.bc

include $(LEVEL)/Makefile.common

PRECOMPILER := $(BIN_DIR)/precompiler$(EXEEXT)

ifndef VERBOSE
  J3.Flags := > /dev/null
endif

HelloWorld.class: HelloWorld.java
	$(Echo) "Compiling trainer"
	$(Verb) javac -source $(JAVAC_TARGET) -target $(JAVAC_TARGET) $< -d .

generated.bc: $(PRECOMPILER) HelloWorld.class
	$(Echo) "Pre-compiling bootstrap code"
	$(Verb) $(PRECOMPILER) -cp $$PWD HelloWorld $(J3.Flags)

Precompiled.bc: HelloWorld.class $(PREPARE_CODE) generated.bc
	$(Echo) "Building precompiled bootstrap code"
	$(Verb) $(MKDIR) $(BUILD_DIR)
	$(Verb) $(LLC) -disable-branch-fold -disable-cfi -disable-debug-info-print -disable-fp-elim $(PRECOMPILER_FLAGS) -load=$(PREPARE_CODE) generated.bc -o $(BUILD_DIR)/Precompiled.s
	$(Verb) $(CC) -c $(BUILD_DIR)/Precompiled.s -o $(BUILD_DIR)/Precompiled.o
	$(Verb) $(Archive) $(LIB_DIR)/libPrecompiled.a $(BUILD_DIR)/Precompiled.o
	$(Verb) $(Ranlib) $(LIB_DIR)/libPrecompiled.a
	$(Verb) $(CP) generated.bc Precompiled.bc

classes.bc: $(PRECOMPILER)
	$(Echo) "Pre-compiling classes"
	$(Verb) $(PRECOMPILER) -emit-class-bytes $(J3.Flags)

BootstrapClasses.bc: classes.bc
	$(Echo) "Building precompiled classes"
	$(Verb) $(MKDIR) $(BUILD_DIR)
	$(Verb) $(LLC) classes.bc -o $(BUILD_DIR)/BootstrapClasses.s
	$(Verb) $(CC) -c $(BUILD_DIR)/BootstrapClasses.s -o $(BUILD_DIR)/BootstrapClasses.o
	$(Verb) $(Archive) $(LIB_DIR)/libBootstrapClasses.a $(BUILD_DIR)/BootstrapClasses.o
	$(Verb) $(Ranlib) $(LIB_DIR)/libBootstrapClasses.a
	$(Verb) $(CP) classes.bc BootstrapClasses.bc

clean-local::
	$(Verb) $(RM) -f HelloWorld.class generated.bc classes.bc Precompiled.bc BootstrapClasses.bc
