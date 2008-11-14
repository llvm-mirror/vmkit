//===---------------- JIT.cc - Initialize the JIT -------------------------===//
//
//                     The Micro Virtual Machine
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/CallingConv.h"
#include <llvm/Constants.h>
#include <llvm/DerivedTypes.h>
#include <llvm/Instructions.h>
#include <llvm/Type.h>
#include "llvm/Support/MutexGuard.h"
#include "llvm/Target/TargetOptions.h"

#include <cstdio>

#include "mvm/JIT.h"
#include "mvm/MvmMemoryManager.h"
#include "mvm/Object.h"
#include "mvm/Threads/Thread.h"

using namespace mvm;
using namespace llvm;


extern "C" void printFloat(float f) {
  printf("%f\n", f);
}

extern "C" void printDouble(double d) {
  printf("%f\n", d);
}

extern "C" void printLong(sint64 l) {
  printf("%lld\n", (long long int)l);
}

extern "C" void printInt(sint32 i) {
  printf("%d\n", i);
}

extern "C" void printObject(mvm::Object* obj) {
  printf("%s\n", obj->printString());
}

namespace mvm {
  namespace llvm_runtime {
    #include "LLVMRuntime.inc"
  }
}

void MvmModule::initialise(bool Fast) {
  llvm::NoFramePointerElim = true;
  llvm::ExceptionHandling = true;
  globalModule = new llvm::Module("bootstrap module");
  globalModuleProvider = new ExistingModuleProvider (globalModule);
  memoryManager = new MvmMemoryManager();
  


  executionEngine = ExecutionEngine::createJIT(globalModuleProvider, 0,
                                               memoryManager, Fast);
  
  std::string str = 
    executionEngine->getTargetData()->getStringRepresentation();
  globalModule->setDataLayout(str);

  
  Module module("unused");
  module.setDataLayout(str);
  mvm::llvm_runtime::makeLLVMModuleContents(&module);
  
  llvm_atomic_cmp_swap_i8 = (uint8 (*)(uint8*, uint8, uint8))
    (uintptr_t)executionEngine->getPointerToFunction(
      module.getFunction("runtime.llvm.atomic.cmp.swap.i8"));
  llvm_atomic_cmp_swap_i16 = (uint16 (*)(uint16*, uint16, uint16))
    (uintptr_t)executionEngine->getPointerToFunction(
      module.getFunction("runtime.llvm.atomic.cmp.swap.i16"));
  llvm_atomic_cmp_swap_i32 = (uint32 (*)(uint32*, uint32, uint32))
    (uintptr_t)executionEngine->getPointerToFunction(
      module.getFunction("runtime.llvm.atomic.cmp.swap.i32"));
  llvm_atomic_cmp_swap_i64 = (uint64 (*)(uint64*, uint64, uint64))
    (uintptr_t)executionEngine->getPointerToFunction(
      module.getFunction("runtime.llvm.atomic.cmp.swap.i64"));
  
  // Type declaration
  ptrType = PointerType::getUnqual(Type::Int8Ty);
  ptr32Type = PointerType::getUnqual(Type::Int32Ty);
  ptrPtrType = PointerType::getUnqual(ptrType);
  pointerSizeType = module.getPointerSize() == llvm::Module::Pointer32 ?
    Type::Int32Ty : Type::Int64Ty;
  
  // Constant declaration
  constantLongMinusOne = ConstantInt::get(Type::Int64Ty, (uint64_t)-1);
  constantLongZero = ConstantInt::get(Type::Int64Ty, 0);
  constantLongOne = ConstantInt::get(Type::Int64Ty, 1);
  constantZero = ConstantInt::get(Type::Int32Ty, 0);
  constantInt8Zero = ConstantInt::get(Type::Int8Ty, 0);
  constantOne = ConstantInt::get(Type::Int32Ty, 1);
  constantTwo = ConstantInt::get(Type::Int32Ty, 2);
  constantThree = ConstantInt::get(Type::Int32Ty, 3);
  constantFour = ConstantInt::get(Type::Int32Ty, 4);
  constantFive = ConstantInt::get(Type::Int32Ty, 5);
  constantSix = ConstantInt::get(Type::Int32Ty, 6);
  constantSeven = ConstantInt::get(Type::Int32Ty, 7);
  constantEight = ConstantInt::get(Type::Int32Ty, 8);
  constantMinusOne = ConstantInt::get(Type::Int32Ty, (uint64_t)-1);
  constantMinInt = ConstantInt::get(Type::Int32Ty, MinInt);
  constantMaxInt = ConstantInt::get(Type::Int32Ty, MaxInt);
  constantMinLong = ConstantInt::get(Type::Int64Ty, MinLong);
  constantMaxLong = ConstantInt::get(Type::Int64Ty, MaxLong);
  constantFloatZero = ConstantFP::get(Type::FloatTy, 0.0f);
  constantFloatOne = ConstantFP::get(Type::FloatTy, 1.0f);
  constantFloatTwo = ConstantFP::get(Type::FloatTy, 2.0f);
  constantDoubleZero = ConstantFP::get(Type::DoubleTy, 0.0);
  constantDoubleOne = ConstantFP::get(Type::DoubleTy, 1.0);
  constantMaxIntFloat = ConstantFP::get(Type::FloatTy, MaxIntFloat);
  constantMinIntFloat = ConstantFP::get(Type::FloatTy, MinIntFloat);
  constantMinLongFloat = ConstantFP::get(Type::FloatTy, MinLongFloat);
  constantMinLongDouble = ConstantFP::get(Type::DoubleTy, MinLongDouble);
  constantMaxLongFloat = ConstantFP::get(Type::FloatTy, MaxLongFloat);
  constantMaxIntDouble = ConstantFP::get(Type::DoubleTy, MaxIntDouble);
  constantMinIntDouble = ConstantFP::get(Type::DoubleTy, MinIntDouble);
  constantMaxLongDouble = ConstantFP::get(Type::DoubleTy, MaxLongDouble);
  constantMaxLongDouble = ConstantFP::get(Type::DoubleTy, MaxLongDouble);
  constantFloatInfinity = ConstantFP::get(Type::FloatTy, MaxFloat);
  constantFloatMinusInfinity = ConstantFP::get(Type::FloatTy, MinFloat);
  constantDoubleInfinity = ConstantFP::get(Type::DoubleTy, MaxDouble);
  constantDoubleMinusInfinity = ConstantFP::get(Type::DoubleTy, MinDouble);
  constantDoubleMinusZero = ConstantFP::get(Type::DoubleTy, -0.0);
  constantFloatMinusZero = ConstantFP::get(Type::FloatTy, -0.0f);
  constantThreadIDMask = ConstantInt::get(pointerSizeType, mvm::Thread::IDMask);
  constantLockedMask = ConstantInt::get(pointerSizeType, 0x7FFFFF00);
  constantThreadFreeMask = ConstantInt::get(pointerSizeType, 0x7FFFFFFF);
  constantPtrOne = ConstantInt::get(pointerSizeType, 1);

  constantPtrNull = Constant::getNullValue(ptrType); 
  constantPtrSize = ConstantInt::get(Type::Int32Ty, sizeof(void*));
  arrayPtrType = PointerType::getUnqual(ArrayType::get(Type::Int8Ty, 0));
}


MvmModule::MvmModule(const std::string& ModuleID) : llvm::Module(ModuleID) {
  Module* module = this;
  std::string str = executionEngine->getTargetData()->getStringRepresentation();
  module->setDataLayout(str);
  
  mvm::llvm_runtime::makeLLVMModuleContents(module);
  
  printFloatLLVM = module->getFunction("printFloat");
  printDoubleLLVM = module->getFunction("printDouble");
  printLongLLVM = module->getFunction("printLong");
  printIntLLVM = module->getFunction("printInt");
  printObjectLLVM = module->getFunction("printObject");

  unwindResume = module->getFunction("_Unwind_Resume_or_Rethrow");
  
  llvmGetException = module->getFunction("llvm.eh.exception");
  exceptionSelector = (getPointerSize() == llvm::Module::Pointer32 ?
                module->getFunction("llvm.eh.selector.i32") : 
                module->getFunction("llvm.eh.selector.i64"));
  
  personality = module->getFunction("__gxx_personality_v0");
  exceptionEndCatch = module->getFunction("__cxa_end_catch");
  exceptionBeginCatch = module->getFunction("__cxa_begin_catch");

  func_llvm_sqrt_f64 = module->getFunction("llvm.sqrt.f64");
  func_llvm_sin_f64 = module->getFunction("llvm.sin.f64");
  func_llvm_cos_f64 = module->getFunction("llvm.cos.f64");
  
  func_llvm_tan_f64 = module->getFunction("tan");
  func_llvm_asin_f64 = module->getFunction("asin");
  func_llvm_acos_f64 = module->getFunction("acos");
  func_llvm_atan_f64 = module->getFunction("atan");
  func_llvm_exp_f64 = module->getFunction("exp");
  func_llvm_log_f64 = module->getFunction("log");
  func_llvm_ceil_f64 = module->getFunction("ceil");
  func_llvm_floor_f64 = module->getFunction("floor");
  func_llvm_cbrt_f64 = module->getFunction("cbrt");
  func_llvm_cosh_f64 = module->getFunction("cosh");
  func_llvm_expm1_f64 = module->getFunction("expm1");
  func_llvm_log10_f64 = module->getFunction("log10");
  func_llvm_log1p_f64 = module->getFunction("log1p");
  func_llvm_sinh_f64 = module->getFunction("sinh");
  func_llvm_tanh_f64 = module->getFunction("tanh");
  func_llvm_fabs_f64 = module->getFunction("fabs");
  func_llvm_rint_f64 = module->getFunction("rint");
    
  func_llvm_hypot_f64 = module->getFunction("hypot");
  func_llvm_pow_f64 = module->getFunction("pow");
  func_llvm_atan2_f64 = module->getFunction("atan2");
    
  func_llvm_fabs_f32 = module->getFunction("fabsf");

  setjmpLLVM = module->getFunction("setjmp");
  
  llvm_memcpy_i32 = module->getFunction("llvm.memcpy.i32");
  llvm_memset_i32 = module->getFunction("llvm.memset.i32");
  llvm_frameaddress = module->getFunction("llvm.frameaddress");

  llvm_atomic_lcs_i8 = module->getFunction("llvm.atomic.cmp.swap.i8.p0i8");
  llvm_atomic_lcs_i16 = module->getFunction("llvm.atomic.cmp.swap.i16.p0i16");
  llvm_atomic_lcs_i32 = module->getFunction("llvm.atomic.cmp.swap.i32.p0i32");
  llvm_atomic_lcs_i64 = module->getFunction("llvm.atomic.cmp.swap.i64.p0i64");
}

llvm::ExecutionEngine* MvmModule::executionEngine;
mvm::LockNormal MvmModule::protectEngine;

llvm::ConstantInt* MvmModule::constantInt8Zero;
llvm::ConstantInt* MvmModule::constantZero;
llvm::ConstantInt* MvmModule::constantOne;
llvm::ConstantInt* MvmModule::constantTwo;
llvm::ConstantInt* MvmModule::constantThree;
llvm::ConstantInt* MvmModule::constantFour;
llvm::ConstantInt* MvmModule::constantFive;
llvm::ConstantInt* MvmModule::constantSix;
llvm::ConstantInt* MvmModule::constantSeven;
llvm::ConstantInt* MvmModule::constantEight;
llvm::ConstantInt* MvmModule::constantMinusOne;
llvm::ConstantInt* MvmModule::constantLongMinusOne;
llvm::ConstantInt* MvmModule::constantLongZero;
llvm::ConstantInt* MvmModule::constantLongOne;
llvm::ConstantInt* MvmModule::constantMinInt;
llvm::ConstantInt* MvmModule::constantMaxInt;
llvm::ConstantInt* MvmModule::constantMinLong;
llvm::ConstantInt* MvmModule::constantMaxLong;
llvm::ConstantFP*  MvmModule::constantFloatZero;
llvm::ConstantFP*  MvmModule::constantFloatOne;
llvm::ConstantFP*  MvmModule::constantFloatTwo;
llvm::ConstantFP*  MvmModule::constantDoubleZero;
llvm::ConstantFP*  MvmModule::constantDoubleOne;
llvm::ConstantFP*  MvmModule::constantMaxIntFloat;
llvm::ConstantFP*  MvmModule::constantMinIntFloat;
llvm::ConstantFP*  MvmModule::constantMinLongFloat;
llvm::ConstantFP*  MvmModule::constantMinLongDouble;
llvm::ConstantFP*  MvmModule::constantMaxLongFloat;
llvm::ConstantFP*  MvmModule::constantMaxIntDouble;
llvm::ConstantFP*  MvmModule::constantMinIntDouble;
llvm::ConstantFP*  MvmModule::constantMaxLongDouble;
llvm::ConstantFP*  MvmModule::constantDoubleInfinity;
llvm::ConstantFP*  MvmModule::constantDoubleMinusInfinity;
llvm::ConstantFP*  MvmModule::constantFloatInfinity;
llvm::ConstantFP*  MvmModule::constantFloatMinusInfinity;
llvm::ConstantFP*  MvmModule::constantFloatMinusZero;
llvm::ConstantFP*  MvmModule::constantDoubleMinusZero;
llvm::Constant*    MvmModule::constantPtrNull;
llvm::ConstantInt* MvmModule::constantPtrSize;
llvm::ConstantInt* MvmModule::constantThreadIDMask;
llvm::ConstantInt* MvmModule::constantLockedMask;
llvm::ConstantInt* MvmModule::constantThreadFreeMask;
llvm::ConstantInt* MvmModule::constantPtrOne;
const llvm::PointerType* MvmModule::ptrType;
const llvm::PointerType* MvmModule::ptr32Type;
const llvm::PointerType* MvmModule::ptrPtrType;
const llvm::Type* MvmModule::pointerSizeType;
const llvm::Type* MvmModule::arrayPtrType;

llvm::Module *MvmModule::globalModule;
llvm::ExistingModuleProvider *MvmModule::globalModuleProvider;
mvm::MvmMemoryManager *MvmModule::memoryManager;


uint8  (*MvmModule::llvm_atomic_cmp_swap_i8)  (uint8* ptr, uint8 cmp,
                                              uint8 val);
uint16 (*MvmModule::llvm_atomic_cmp_swap_i16) (uint16* ptr, uint16 cmp,
                                              uint16 val);
uint32 (*MvmModule::llvm_atomic_cmp_swap_i32) (uint32* ptr, uint32 cmp,
                                              uint32 val);
uint64 (*MvmModule::llvm_atomic_cmp_swap_i64) (uint64* ptr, uint64 cmp,
                                              uint64 val);


uint64 MvmModule::getTypeSize(const llvm::Type* type) {
  return executionEngine->getTargetData()->getABITypeSize(type);
}

void MvmModule::runPasses(llvm::Function* func,  
                          llvm::FunctionPassManager* pm) {
  pm->run(*func);
}

#if defined(__MACH__) && !defined(__i386__)
#define FRAME_IP(fp) (fp[2])
#else
#define FRAME_IP(fp) (fp[1])
#endif

int MvmModule::getBacktrace(void** stack, int size) {
  void** addr = (void**)__builtin_frame_address(0);
  int cpt = 0;
  void* baseSP = mvm::Thread::get()->baseSP;
  while (addr && cpt < size && addr < baseSP && addr < addr[0]) {
    addr = (void**)addr[0];
    stack[cpt++] = (void**)FRAME_IP(addr);
  }
  return cpt;
}

static LockNormal lock;
static std::map<void*, const llvm::Function*> pointerMap;

const llvm::Function* MvmModule::getCodeFromPointer(void* Addr) {
  lock.lock();
  std::map<void*, const llvm::Function*>::iterator I =
    pointerMap.lower_bound(Addr);
  
  lock.unlock();
  if (I != pointerMap.end()) {
    const llvm::Function* F = I->second;
    if (Addr >= executionEngine->getPointerToGlobal(F)) return F;
  }

  return 0;
}

void MvmModule::addMethodInfo(void* Addr, const llvm::Function* F) {
  lock.lock();
  pointerMap.insert(std::make_pair(Addr, F));
  lock.unlock();
}
