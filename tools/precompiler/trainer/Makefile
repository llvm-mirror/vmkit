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

generated.bc: $(PRECOMPILER) HelloWorld.java $(LibDir)/StaticJ3GC$(SHLIBEXT) $(LibDir)/StaticGCPass$(SHLIBEXT) $(LibDir)/StaticGCPrinter$(SHLIBEXT)
	$(Echo) "Building precompiled bootstrap code"
	$(Verb) javac HelloWorld.java
	$(Verb) $(PRECOMPILER) -cp $$PWD HelloWorld
	$(Verb) $(MKDIR) $(ObjDir)
	$(Verb) $(LOPT) generated.bc -load=$(LibDir)/StaticGCPass$(SHLIBEXT) -StaticGCPass -o $(LibDir)/Precompiled.bc
	$(Verb) $(LLC) -O0 -fast-isel=false -load=$(LibDir)/StaticJ3GC$(SHLIBEXT) -load=$(LibDir)/StaticGCPrinter$(SHLIBEXT) -disable-fp-elim $(LibDir)/Precompiled.bc -o $(ObjDir)/Precompiled.s
	$(Verb) $(Compile.Wrapper) $(LLVMCC) $(CPP.Flags) $(C.Flags) $(CFLAGS) $(CPPFLAGS) $(TargetCommonOpts) $(CompileCommonOpts) -c $(ObjDir)/Precompiled.s -o $(ObjDir)/Precompiled.o

	$(Verb) $(Archive) $(LibDir)/libPrecompiled.a $(ObjDir)/Precompiled.o
	$(Verb) $(Ranlib) $(LibDir)/libPrecompiled.a

clean-local::
	$(Verb) $(RM) -f HelloWorld.class generated.bc
