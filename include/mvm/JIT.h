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
#include "llvm/Target/TargetData.h"

#include "types.h"

#include "mvm/Allocator.h"
#include "mvm/Method.h"
#include "mvm/MvmMemoryManager.h"
#include "mvm/Threads/Locks.h"

namespace mvm {

class Thread;

/// JITInfo - This class can be derived from to hold private JIT-specific
/// information. Objects of type are accessed/created with
/// <Class>::getInfo and destroyed when the <Class> object is destroyed.
struct JITInfo : public mvm::PermanentObject {
  virtual ~JITInfo() {}
};

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

class MvmModule : public llvm::Module {
public:
  
  MvmModule(const std::string& ModuleID);
  
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

  static llvm::ExecutionEngine* executionEngine;
  static mvm::LockNormal protectEngine;

  static uint64 getTypeSize(const llvm::Type* type);
  static void AddStandardCompilePasses(llvm::FunctionPassManager*);
  static void runPasses(llvm::Function* func, llvm::FunctionPassManager*);
  static void initialise(bool Fast = false);

  static llvm::ConstantInt* constantInt8Zero;
  static llvm::ConstantInt* constantZero;
  static llvm::ConstantInt* constantOne;
  static llvm::ConstantInt* constantTwo;
  static llvm::ConstantInt* constantThree;
  static llvm::ConstantInt* constantFour;
  static llvm::ConstantInt* constantFive;
  static llvm::ConstantInt* constantSix;
  static llvm::ConstantInt* constantSeven;
  static llvm::ConstantInt* constantEight;
  static llvm::ConstantInt* constantMinusOne;
  static llvm::ConstantInt* constantLongMinusOne;
  static llvm::ConstantInt* constantLongZero;
  static llvm::ConstantInt* constantLongOne;
  static llvm::ConstantInt* constantMinInt;
  static llvm::ConstantInt* constantMaxInt;
  static llvm::ConstantInt* constantMinLong;
  static llvm::ConstantInt* constantMaxLong;
  static llvm::ConstantFP*  constantFloatZero;
  static llvm::ConstantFP*  constantFloatOne;
  static llvm::ConstantFP*  constantFloatTwo;
  static llvm::ConstantFP*  constantDoubleZero;
  static llvm::ConstantFP*  constantDoubleOne;
  static llvm::ConstantFP*  constantMaxIntFloat;
  static llvm::ConstantFP*  constantMinIntFloat;
  static llvm::ConstantFP*  constantMinLongFloat;
  static llvm::ConstantFP*  constantMinLongDouble;
  static llvm::ConstantFP*  constantMaxLongFloat;
  static llvm::ConstantFP*  constantMaxIntDouble;
  static llvm::ConstantFP*  constantMinIntDouble;
  static llvm::ConstantFP*  constantMaxLongDouble;
  static llvm::ConstantFP*  constantDoubleInfinity;
  static llvm::ConstantFP*  constantDoubleMinusInfinity;
  static llvm::ConstantFP*  constantFloatInfinity;
  static llvm::ConstantFP*  constantFloatMinusInfinity;
  static llvm::ConstantFP*  constantFloatMinusZero;
  static llvm::ConstantFP*  constantDoubleMinusZero;
  static llvm::Constant*    constantPtrNull;
  static llvm::ConstantInt* constantPtrSize;
  static llvm::ConstantInt* constantThreadIDMask;
  static llvm::ConstantInt* constantLockedMask;
  static llvm::ConstantInt* constantThreadFreeMask;
  static const llvm::PointerType* ptrType;
  static const llvm::PointerType* ptr32Type;
  static const llvm::PointerType* ptrPtrType;
  static const llvm::Type* arrayPtrType;
  static const llvm::Type* pointerSizeType;


  static llvm::Module *globalModule;
  static llvm::ExistingModuleProvider *globalModuleProvider;
  static mvm::MvmMemoryManager *memoryManager;

  static int disassemble(unsigned int* addr);

  static int getBacktrace(void** stack, int size);
  static Code* getCodeFromPointer(void* addr);
  static void addMethodInfo(void* end, Code* c);

  static uint8  (*llvm_atomic_cmp_swap_i8)  (uint8* ptr,  uint8 cmp,
                                             uint8 val);
  static uint16 (*llvm_atomic_cmp_swap_i16) (uint16* ptr, uint16 cmp,
                                             uint16 val);
  static uint32 (*llvm_atomic_cmp_swap_i32) (uint32* ptr, uint32 cmp,
                                             uint32 val);
  static uint64 (*llvm_atomic_cmp_swap_i64) (uint64* ptr, uint64 cmp,
                                             uint64 val);

};

// TODO: find what macro for gcc < 4.2
#if (__WORDSIZE == 64)

#define __sync_bool_compare_and_swap(ptr, cmp, val) \
  (mvm::MvmModule::llvm_atomic_cmp_swap_i64((uint64*)(ptr), (uint64)(cmp), \
                                            (uint64)(val)) == (uint64)(cmp))

#define __sync_val_compare_and_swap(ptr, cmp,val) \
  mvm::MvmModule::llvm_atomic_cmp_swap_i64((uint64*)(ptr), (uint64)(cmp), \
                                           (uint64)(val))


#else



#define __sync_bool_compare_and_swap(ptr, cmp, val) \
  (mvm::MvmModule::llvm_atomic_cmp_swap_i32((uint32*)(ptr), (uint32)(cmp), \
                                            (uint32)(val)) == (uint32)(cmp))

#define __sync_val_compare_and_swap(ptr, cmp,val) \
  mvm::MvmModule::llvm_atomic_cmp_swap_i32((uint32*)(ptr), (uint32)(cmp), \
                                           (uint32)(val))
#endif

} // end namespace mvm

#endif // MVM_JIT_H
