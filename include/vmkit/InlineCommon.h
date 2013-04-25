#ifndef _INLINE_COMMON_H_
#define _INLINE_COMMON_H_

#include <llvm/CallingConv.h>
#include <llvm/Constants.h>
#include <llvm/DerivedTypes.h>
#include <llvm/Instructions.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Type.h>

namespace vmkit {
	extern "C" void makeLLVMFunctions_FinalMMTk(llvm::Module*);
}

#endif
