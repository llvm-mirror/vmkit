##===- lib/JnJVM/LLVMRuntime/Makefile ----------------------*- Makefile -*-===##
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

LL_FILES = runtime-default.ll

ifeq ($(ISOLATE_BUILD), 1)
LL_FILES += runtime-isolate.ll
endif

ifeq ($(SERVICE_BUILD), 1)
LL_FILES += runtime-service.ll runtime-isolate.ll runtime-multi-mmap.ll
else
ifeq ($(GC_MULTI_MMAP), 1)
LL_FILES += runtime-multi-mmap.ll
else
ifeq ($(GC_SINGLE_MMAP), 1)
LL_FILES += runtime-single-mmap.ll
else
ifeq ($(GC_BOEHM), 1)
LL_FILES += runtime-boehm.ll
endif
endif
endif
endif

llvmruntimell: $(LL_FILES)
	$(Echo) "Generating LLVM runtime"
	$(Verb) cat $(LL_FILES) > LLVMRuntime.ll

clean::
	rm -f LLVMRuntime.ll LLVMRuntime.bc LLVMRuntime.cpp
