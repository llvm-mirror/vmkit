#ifndef _INLINE_COMMON_H_
#define _INLINE_COMMON_H_

#include <llvm/IR/CallingConv.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>

namespace vmkit {
	extern "C" void makeLLVMFunctions_FinalMMTk(llvm::Module*);
}

#endif
