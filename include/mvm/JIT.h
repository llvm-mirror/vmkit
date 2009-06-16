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

#include <cfloat>
#include <cmath>
#include <string>

#include "types.h"

#include "llvm/Target/TargetMachine.h"

namespace llvm {
  class Constant;
  class ConstantFP;
  class ConstantInt;
  class ExecutionEngine;
  class ExistingModuleProvider;
  class Function;
  class FunctionPassManager;
  class Module;
  class ModuleProvider;
  class PointerType;
  class TargetData;
  class TargetMachine;
  class Type;
}

namespace mvm {

class LockNormal;
class LockRecursive;

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

class MvmModule {

public:
  
  explicit MvmModule(llvm::Module*);
 
  llvm::Function* exceptionEndCatch;
  llvm::Function* exceptionBeginCatch;
  llvm::Function* unwindResume;
  llvm::Function* exceptionSelector;
  llvm::Function* personality;
  llvm::Function* llvmGetException;

  llvm::Function* printFloatLLVM;
  llvm::Function* printDoubleLLVM;
  llvm::Function* printLongLLVM;
  llvm::Function* printIntLLVM;
  llvm::Function* printObjectLLVM;

  llvm::Function* setjmpLLVM;

  llvm::Function* func_llvm_fabs_f32;
  llvm::Function* func_llvm_fabs_f64;
  llvm::Function* func_llvm_sqrt_f64;
  llvm::Function* func_llvm_sin_f64;
  llvm::Function* func_llvm_cos_f64;
  llvm::Function* func_llvm_tan_f64;
  llvm::Function* func_llvm_asin_f64;
  llvm::Function* func_llvm_acos_f64;
  llvm::Function* func_llvm_atan_f64;
  llvm::Function* func_llvm_atan2_f64;
  llvm::Function* func_llvm_exp_f64;
  llvm::Function* func_llvm_log_f64;
  llvm::Function* func_llvm_pow_f64;
  llvm::Function* func_llvm_ceil_f64;
  llvm::Function* func_llvm_floor_f64;
  llvm::Function* func_llvm_rint_f64;
  llvm::Function* func_llvm_cbrt_f64;
  llvm::Function* func_llvm_cosh_f64;
  llvm::Function* func_llvm_expm1_f64;
  llvm::Function* func_llvm_hypot_f64;
  llvm::Function* func_llvm_log10_f64;
  llvm::Function* func_llvm_log1p_f64;
  llvm::Function* func_llvm_sinh_f64;
  llvm::Function* func_llvm_tanh_f64;

  llvm::Function* llvm_memcpy_i32;
  llvm::Function* llvm_memset_i32;
  llvm::Function* llvm_frameaddress;
  llvm::Function* llvm_atomic_lcs_i8;
  llvm::Function* llvm_atomic_lcs_i16;
  llvm::Function* llvm_atomic_lcs_i32;
  llvm::Function* llvm_atomic_lcs_i64;
  llvm::Function* llvm_atomic_lcs_ptr;


  static llvm::Constant* constantInt8Zero;
  static llvm::Constant* constantZero;
  static llvm::Constant* constantOne;
  static llvm::Constant* constantTwo;
  static llvm::Constant* constantThree;
  static llvm::Constant* constantFour;
  static llvm::Constant* constantFive;
  static llvm::Constant* constantSix;
  static llvm::Constant* constantSeven;
  static llvm::Constant* constantEight;
  static llvm::Constant* constantMinusOne;
  static llvm::Constant* constantLongMinusOne;
  static llvm::Constant* constantLongZero;
  static llvm::Constant* constantLongOne;
  static llvm::Constant* constantMinInt;
  static llvm::Constant* constantMaxInt;
  static llvm::Constant* constantMinLong;
  static llvm::Constant* constantMaxLong;
  static llvm::Constant*  constantFloatZero;
  static llvm::Constant*  constantFloatOne;
  static llvm::Constant*  constantFloatTwo;
  static llvm::Constant*  constantDoubleZero;
  static llvm::Constant*  constantDoubleOne;
  static llvm::Constant*  constantMaxIntFloat;
  static llvm::Constant*  constantMinIntFloat;
  static llvm::Constant*  constantMinLongFloat;
  static llvm::Constant*  constantMinLongDouble;
  static llvm::Constant*  constantMaxLongFloat;
  static llvm::Constant*  constantMaxIntDouble;
  static llvm::Constant*  constantMinIntDouble;
  static llvm::Constant*  constantMaxLongDouble;
  static llvm::Constant*  constantDoubleInfinity;
  static llvm::Constant*  constantDoubleMinusInfinity;
  static llvm::Constant*  constantFloatInfinity;
  static llvm::Constant*  constantFloatMinusInfinity;
  static llvm::Constant*  constantFloatMinusZero;
  static llvm::Constant*  constantDoubleMinusZero;
  static llvm::Constant*    constantPtrNull;
  static llvm::Constant* constantPtrLogSize;
  static llvm::Constant* constantThreadIDMask;
  static llvm::Constant* constantStackOverflowMask;
  static llvm::Constant* constantFatMask;
  static llvm::Constant* constantPtrOne;
  static llvm::Constant* constantPtrZero;
  static const llvm::PointerType* ptrType;
  static const llvm::PointerType* ptr32Type;
  static const llvm::PointerType* ptrPtrType;
  static const llvm::Type* arrayPtrType;
  static const llvm::Type* pointerSizeType;

  static llvm::ExecutionEngine* executionEngine;
  static mvm::LockNormal protectEngine;
  static llvm::Module *globalModule;
  static llvm::ExistingModuleProvider *globalModuleProvider;
  static llvm::FunctionPassManager* globalFunctionPasses;
  static const llvm::TargetData* TheTargetData;
  
  static uint64 getTypeSize(const llvm::Type* type);
  static void runPasses(llvm::Function* func, llvm::FunctionPassManager*);
  static void initialise(llvm::CodeGenOpt::Level = llvm::CodeGenOpt::Default,
                         llvm::Module* TheModule = 0,
                         llvm::TargetMachine* TheTarget = 0);

  static int disassemble(unsigned int* addr);
  
  static void protectIR();
  static void unprotectIR();

  static void copyDefinitions(llvm::Module* Dst, llvm::Module* Src);

  static void AddStandardCompilePasses();

  static const char* getHostTriple();
};

} // end namespace mvm

#endif // MVM_JIT_H
