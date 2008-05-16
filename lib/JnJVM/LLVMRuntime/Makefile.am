all: llvmruntimecpp

LLVMAS = @llvmprefix@/Release/bin/llvm-as
LLC = @llvmprefix@/Release/bin/llc

llvmruntimebc : llvmruntimell
	$(LLVMAS) -f LLVMRuntime.ll

llvmruntimecpp : llvmruntimebc
	$(LLC) -march=cpp -cppgen=contents -f LLVMRuntime.bc

LL_FILES = runtime-default.ll

if ISOLATE_BUILD
LL_FILES += runtime-isolate.ll
endif

if SERVICE_BUILD
LL_FILES += runtime-service.ll runtime-isolate.ll runtime-multi-mmap.ll
else
if GC_MULTI_MMAP
LL_FILES += runtime-multi-mmap.ll
else
if GC_SINGLE_MMAP
LL_FILES += runtime-single-mmap.ll
else
if GC_BOEHM
LL_FILES += runtime-boehm.ll
endif
endif
endif
endif

llvmruntimell: $(LL_FILES)
	cat $(LL_FILES) > LLVMRuntime.ll

clean:
	rm -f LLVMRuntime.ll LLVMRuntime.bc LLVMRuntime.cpp
