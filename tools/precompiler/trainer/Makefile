##===- tools/precompiler/trainer/Makefile ------------------*- Makefile -*-===##
# 
#                     The VMKit project
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
# 
##===----------------------------------------------------------------------===##
LEVEL = ../../..

include $(LEVEL)/Makefile.config

MODULE_WITH_GC = Precompiled

BUILT_SOURCES = generated.bc

include $(LEVEL)/Makefile.common

PRECOMPILER := $(ToolDir)/precompiler$(EXEEXT)

generated.bc: $(PRECOMPILER) HelloWorld.java
	$(Echo) "Building precompiled bootstrap code"
	$(Verb) javac HelloWorld.java
	$(Verb) $(PRECOMPILER) -cp $$PWD HelloWorld
