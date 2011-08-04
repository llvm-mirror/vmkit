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

BUILT_SOURCES = Precompiled.bc

include $(LEVEL)/Makefile.common

PRECOMPILER := $(ToolDir)/precompiler$(EXEEXT)

ifndef VERBOSE
  J3.Flags := > /dev/null
endif

Precompiled.bc: $(PRECOMPILER) HelloWorld.java $(LibDir)/StaticGCPass$(SHLIBEXT) $(LibDir)/StaticGCPrinter$(SHLIBEXT)
	$(Echo) "Building precompiled bootstrap code"
	$(Verb) javac HelloWorld.java
	$(Verb) $(PRECOMPILER) -cp $$PWD HelloWorld $(J3.Flags)
	$(Verb) $(MKDIR) $(ObjDir)
	$(Verb) $(LOPT) generated.bc -load=$(LibDir)/StaticGCPass$(SHLIBEXT) -StaticGCPass -o $(LibDir)/Precompiled.bc
	$(Verb) $(LLC) -O0 -fast-isel=false -load=$(LibDir)/StaticGCPrinter$(SHLIBEXT) -disable-fp-elim $(LibDir)/Precompiled.bc -o $(ObjDir)/Precompiled.s
	$(Verb) $(LLVMCC) -c $(ObjDir)/Precompiled.s -o $(ObjDir)/Precompiled.o
	$(Verb) $(Archive) $(LibDir)/libPrecompiled.a $(ObjDir)/Precompiled.o
	$(Verb) $(Ranlib) $(LibDir)/libPrecompiled.a
	$(Verb) $(MV) generated.bc Precompiled.bc

clean-local::
	$(Verb) $(RM) -f HelloWorld.class Precompiled.bc
