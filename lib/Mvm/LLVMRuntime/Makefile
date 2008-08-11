##===- lib/Mvm/LLVMRuntime/Makefile ------------------------*- Makefile -*-===##
# 
#                     The vmkit project
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
# 
##===----------------------------------------------------------------------===##
LEVEL = ../../..

include $(LEVEL)/Makefile.common

all = llvmruntime.cpp

llvmruntimebc	: llvmruntimell
	$(Echo) "Building LLVM runtime with llvm-as"
	$(Verb) $(LLVMAS) -f LLVMRuntime.ll

all :: llvmruntimebc
	$(Echo) "Building LLVM runtime with llc"
	$(Verb) $(LLC) -march=cpp -cppgen=contents -f LLVMRuntime.bc

llvmruntimell: LLVMRuntime.ll

clean::
	rm -f LLVMRuntime.bc LLVMRuntime.cpp
