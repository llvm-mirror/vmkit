//===------------------ JIT.h - JIT facilities ----------------------------===//
//
//                     The Micro Virtual Machine
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef MVM_JIT_H
#define MVM_JIT_H

#include <float.h>

#include "llvm/Constants.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Function.h"
#include "llvm/Module.h"
#include "llvm/ModuleProvider.h"
#include "llvm/PassManager.h"
#include "llvm/Type.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/Target/TargetData.h"

#include "types.h"

#include "mvm/Threads/Locks.h"
#include "mvm/MvmMemoryManager.h"

namespace mvm {

namespace jit {

const double MaxDouble = +INFINITY; //1.0 / 0.0;
const double MinDouble = -INFINITY;//-1.0 / 0.0;
const double MaxLongDouble =  9223372036854775807.0;
const double MinLongDouble = -9223372036854775808.0;
const double MaxIntDouble = 2147483647.0;
const double MinIntDouble = -2147483648.0;
const uint64 MaxLong = 9223372036854775807LL;
const uint64 MinLong = -9223372036854775808ULL;
const uint32 MaxInt =  2147483647;
const uint32 MinInt =  -2147483648U;
const float MaxFloat = +INFINITY; //(float)(((float)1.0) / (float)0.0);
const float MinFloat = -INFINITY; //(float)(((float)-1.0) / (float)0.0);
const float MaxLongFloat = (float)9223372036854775807.0;
const float MinLongFloat = (float)-9223372036854775808.0;
const float MaxIntFloat = (float)2147483647.0;
const float MinIntFloat = (float)-2147483648.0;
const float NaNFloat = NAN; //(float)(((float)0.0) / (float)0.0);
const double NaNDouble = NAN; //0.0 / 0.0;

extern llvm::Function* exceptionEndCatch;
extern llvm::Function* exceptionBeginCatch;
extern llvm::Function* unwindResume;
extern llvm::Function* exceptionSelector;
extern llvm::Function* personality;
extern llvm::Function* llvmGetException;

extern llvm::Function* printFloatLLVM;
extern llvm::Function* printDoubleLLVM;
extern llvm::Function* printLongLLVM;
extern llvm::Function* printIntLLVM;
extern llvm::Function* printObjectLLVM;

extern llvm::Function* setjmpLLVM;

extern llvm::Function* func_llvm_fabs_f32;
extern llvm::Function* func_llvm_fabs_f64;
extern llvm::Function* func_llvm_sqrt_f64;
extern llvm::Function* func_llvm_sin_f64;
extern llvm::Function* func_llvm_cos_f64;
extern llvm::Function* func_llvm_tan_f64;
extern llvm::Function* func_llvm_asin_f64;
extern llvm::Function* func_llvm_acos_f64;
extern llvm::Function* func_llvm_atan_f64;
extern llvm::Function* func_llvm_atan2_f64;
extern llvm::Function* func_llvm_exp_f64;
extern llvm::Function* func_llvm_log_f64;
extern llvm::Function* func_llvm_pow_f64;
extern llvm::Function* func_llvm_ceil_f64;
extern llvm::Function* func_llvm_floor_f64;
extern llvm::Function* func_llvm_rint_f64;
extern llvm::Function* func_llvm_cbrt_f64;
extern llvm::Function* func_llvm_cosh_f64;
extern llvm::Function* func_llvm_expm1_f64;
extern llvm::Function* func_llvm_hypot_f64;
extern llvm::Function* func_llvm_log10_f64;
extern llvm::Function* func_llvm_log1p_f64;
extern llvm::Function* func_llvm_sinh_f64;
extern llvm::Function* func_llvm_tanh_f64;

extern llvm::Function* llvm_memcpy_i32;
extern llvm::Function* llvm_memset_i32;

extern llvm::ExecutionEngine* executionEngine;

extern uint64 getTypeSize(const llvm::Type* type);
extern void AddStandardCompilePasses(llvm::FunctionPassManager*);
extern void runPasses(llvm::Function* func, llvm::FunctionPassManager*);
extern void initialise();


//extern mvm::Lock* protectTypes;
//extern mvm::Lock* protectConstants;
extern mvm::Lock* protectEngine;
extern llvm::ConstantInt* constantInt8Zero;
extern llvm::ConstantInt* constantZero;
extern llvm::ConstantInt* constantOne;
extern llvm::ConstantInt* constantTwo;
extern llvm::ConstantInt* constantThree;
extern llvm::ConstantInt* constantFour;
extern llvm::ConstantInt* constantFive;
extern llvm::ConstantInt* constantSix;
extern llvm::ConstantInt* constantSeven;
extern llvm::ConstantInt* constantEight;
extern llvm::ConstantInt* constantMinusOne;
extern llvm::ConstantInt* constantLongMinusOne;
extern llvm::ConstantInt* constantLongZero;
extern llvm::ConstantInt* constantLongOne;
extern llvm::ConstantInt* constantMinInt;
extern llvm::ConstantInt* constantMaxInt;
extern llvm::ConstantInt* constantMinLong;
extern llvm::ConstantInt* constantMaxLong;
extern llvm::ConstantFP*  constantFloatZero;
extern llvm::ConstantFP*  constantFloatOne;
extern llvm::ConstantFP*  constantFloatTwo;
extern llvm::ConstantFP*  constantDoubleZero;
extern llvm::ConstantFP*  constantDoubleOne;
extern llvm::ConstantFP*  constantMaxIntFloat;
extern llvm::ConstantFP*  constantMinIntFloat;
extern llvm::ConstantFP*  constantMinLongFloat;
extern llvm::ConstantFP*  constantMinLongDouble;
extern llvm::ConstantFP*  constantMaxLongFloat;
extern llvm::ConstantFP*  constantMaxIntDouble;
extern llvm::ConstantFP*  constantMinIntDouble;
extern llvm::ConstantFP*  constantMaxLongDouble;
extern llvm::ConstantFP*  constantDoubleInfinity;
extern llvm::ConstantFP*  constantDoubleMinusInfinity;
extern llvm::ConstantFP*  constantFloatInfinity;
extern llvm::ConstantFP*  constantFloatMinusInfinity;
extern llvm::ConstantFP*  constantFloatMinusZero;
extern llvm::ConstantFP*  constantDoubleMinusZero;
extern llvm::Constant*    constantPtrNull;
extern llvm::ConstantInt* constantPtrSize;
extern const llvm::PointerType* ptrType;
extern const llvm::Type* arrayPtrType;


extern llvm::Module *globalModule;
extern llvm::ExistingModuleProvider *globalModuleProvider;
extern mvm::MvmMemoryManager *memoryManager;

extern void protectTypes();
extern void unprotectTypes();

extern void protectConstants();
extern void unprotectConstants();
extern int disassemble(unsigned int* addr);


} // end namespace jit

} // end namespace mvm

#endif // MVM_JIT_H
