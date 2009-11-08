//===---------------- JIT.cc - Initialize the JIT -------------------------===//
//
//                     The Micro Virtual Machine
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <llvm/CallingConv.h>
#include <llvm/Constants.h>
#include <llvm/DerivedTypes.h>
#include <llvm/Instructions.h>
#include <llvm/LinkAllPasses.h>
#include <llvm/Linker.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/ModuleProvider.h>
#include <llvm/PassManager.h>
#include <llvm/Type.h>
#include <llvm/Analysis/LoopPass.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Assembly/Parser.h>
#include <llvm/CodeGen/GCStrategy.h>
#include <llvm/Config/config.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include "llvm/ExecutionEngine/JITEventListener.h"
#include "llvm/Support/CommandLine.h"
#include <llvm/Support/Debug.h>
#include <llvm/Support/IRReader.h>
#include <llvm/Support/MutexGuard.h>
#include "llvm/Support/SourceMgr.h"
#include <llvm/Target/TargetData.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Target/TargetSelect.h>

#include "mvm/JIT.h"
#include "mvm/Threads/Locks.h"
#include "mvm/Threads/Thread.h"
#include "mvm/VirtualMachine.h"
#include "mvm/GC/GC.h"
#include "MutatorThread.h"
#include "MvmGC.h"

using namespace mvm;
using namespace llvm;


static cl::list<std::string> 
LoadBytecodeFiles("load-bc", cl::desc("Load bytecode file"), cl::ZeroOrMore,
                  cl::CommaSeparated);

namespace mvm {
  namespace llvm_runtime {
    #include "LLVMRuntime.inc"
  }
  void linkVmkitGC();
}

const char* MvmModule::getHostTriple() {
  return LLVM_HOSTTRIPLE;
}

class MvmJITListener : public llvm::JITEventListener {
public:
  virtual void NotifyFunctionEmitted(const Function &F,
                                     void *Code, size_t Size,
                                     const EmittedFunctionDetails &Details) {
    
    if (F.getParent() == MvmModule::globalModule) {
      llvm::GCFunctionInfo* GFI = 0;
      // We know the last GC info is for this method.
      if (F.hasGC()) {
        GCStrategy::iterator I = mvm::MvmModule::GC->end();
        I--;
        DEBUG(errs() << (*I)->getFunction().getName() << '\n');
        DEBUG(errs() << F.getName() << '\n');
        assert(&(*I)->getFunction() == &F &&
           "GC Info and method do not correspond");
        GFI = *I;
      }
      MethodInfo* MI =
        new(MvmModule::Allocator, "MvmJITMethodInfo") MvmJITMethodInfo(GFI, &F);
      VirtualMachine::SharedRuntimeFunctions.addMethodInfo(MI, Code,
                                              (void*)((uintptr_t)Code + Size));
    }
  }
};

static MvmJITListener JITListener;

void MvmModule::loadBytecodeFile(const std::string& str) {
  SMDiagnostic Err;
  Module* M = ParseIRFile(str, Err, getGlobalContext());
  if (M) {
    M->setTargetTriple(getHostTriple());
    Linker::LinkModules(globalModule, M, 0);
    delete M;
  } else {
    Err.Print("load bytecode", errs());
  }
}

typedef void (*BootType)(uintptr_t Plan);
typedef void (*BootHeapType)(intptr_t initial, intptr_t max);

void MvmModule::initialise(CodeGenOpt::Level level, Module* M,
                           TargetMachine* T) {
  mvm::linkVmkitGC();
  
  llvm_start_multithreaded();
  
  llvm::NoFramePointerElim = true;
  llvm::DisablePrettyStackTrace = true;
#if DWARF_EXCEPTIONS
  llvm::DwarfExceptionHandling = true;
#else
  llvm::DwarfExceptionHandling = false;
#endif
  if (!M) {
    globalModule = new Module("bootstrap module", getGlobalContext());
    globalModuleProvider = new ExistingModuleProvider (globalModule);

    InitializeNativeTarget();

    executionEngine = ExecutionEngine::createJIT(globalModuleProvider, 0,
                                                 0, level, false);

    executionEngine->RegisterJITEventListener(&JITListener);    
    executionEngine->DisableLazyCompilation(false); 
    std::string str = 
      executionEngine->getTargetData()->getStringRepresentation();
    globalModule->setDataLayout(str);
    globalModule->setTargetTriple(getHostTriple());
  
    TheTargetData = executionEngine->getTargetData();
  } else {
    globalModule = M;
    globalModuleProvider = new ExistingModuleProvider (globalModule);
    TheTargetData = T->getTargetData();
  }

  globalFunctionPasses = new FunctionPassManager(globalModuleProvider);

  mvm::llvm_runtime::makeLLVMModuleContents(globalModule);
  
  LLVMContext& Context = globalModule->getContext();
  // Type declaration
  ptrType = PointerType::getUnqual(Type::getInt8Ty(Context));
  ptr32Type = PointerType::getUnqual(Type::getInt32Ty(Context));
  ptrPtrType = PointerType::getUnqual(ptrType);
  pointerSizeType = globalModule->getPointerSize() == Module::Pointer32 ?
    Type::getInt32Ty(Context) : Type::getInt64Ty(Context);
 
  for (std::vector<std::string>::iterator i = LoadBytecodeFiles.begin(),
       e = LoadBytecodeFiles.end(); i != e; ++i) {
    loadBytecodeFile(*i); 
  }

#ifdef WITH_MMTK
  llvm::GlobalVariable* GV = globalModule->getGlobalVariable("MMTkCollectorSize", false);
  if (GV && executionEngine) {
    ConstantInt* C = dyn_cast<ConstantInt>(GV->getInitializer());
    uint64_t val = C->getZExtValue();
    MutatorThread::MMTkCollectorSize = val;
  
    GV = globalModule->getGlobalVariable("MMTkMutatorSize", false);
    assert(GV && "Could not find MMTkMutatorSize");
    C = dyn_cast<ConstantInt>(GV->getInitializer());
    val = C->getZExtValue();
    MutatorThread::MMTkMutatorSize = val;

    Function* F = globalModule->getFunction("JnJVM_org_j3_config_Selected_00024Mutator__0003Cinit_0003E__");
    assert(F && "Could not find <init> from Mutator");
    MutatorThread::MutatorInit = (MutatorThread::MMTkInitType)
      (uintptr_t)executionEngine->getPointerToFunction(F);
    
    F = globalModule->getFunction("JnJVM_org_mmtk_plan_MutatorContext_initMutator__I");
    assert(F && "Could not find init from Mutator");
    MutatorThread::MutatorCallInit = (MutatorThread::MMTkInitIntType)
      (uintptr_t)executionEngine->getPointerToFunction(F);
    
    F = globalModule->getFunction("JnJVM_org_mmtk_plan_MutatorContext_deinitMutator__");
    assert(F && "Could not find deinit from Mutator");
    MutatorThread::MutatorCallDeinit = (MutatorThread::MMTkInitType)
      (uintptr_t)executionEngine->getPointerToFunction(F);
    
    
    GV = globalModule->getGlobalVariable("org_j3_config_Selected_4Mutator_VT", false);
    assert(GV && "Could not find VT from Mutator");
    MutatorThread::MutatorVT = (VirtualTable*)executionEngine->getPointerToGlobal(GV);
  
    F = globalModule->getFunction("JnJVM_org_j3_config_Selected_00024Collector__0003Cinit_0003E__");
    assert(F && "Could not find <init> from Collector");
    MutatorThread::CollectorInit = (MutatorThread::MMTkInitType)
      (uintptr_t)executionEngine->getPointerToFunction(F);
    
    GV = globalModule->getGlobalVariable("org_j3_config_Selected_4Collector_VT", false);
    assert(GV && "Could not find VT from Collector");
    MutatorThread::CollectorVT = (VirtualTable*)executionEngine->getPointerToGlobal(GV);

    GlobalAlias* GA = dyn_cast<GlobalAlias>(globalModule->getNamedValue("MMTkAlloc"));
    assert(GA && "Could not find MMTkAlloc alias");
    F = dyn_cast<Function>(GA->getAliasee());
    gc::MMTkGCAllocator = (gc::MMTkAllocType)
      (uintptr_t)executionEngine->getPointerToFunction(F);
  
    GA = dyn_cast<GlobalAlias>(globalModule->getNamedValue("MMTkPostAlloc"));
    assert(GA && "Could not find MMTkPostAlloc alias");
    F = dyn_cast<Function>(GA->getAliasee());
    gc::MMTkGCPostAllocator = (gc::MMTkPostAllocType)
      (uintptr_t)executionEngine->getPointerToFunction(F);
    
    GA = dyn_cast<GlobalAlias>(globalModule->getNamedValue("MMTkCheckAllocator"));
    assert(GA && "Could not find MMTkCheckAllocator alias");
    F = dyn_cast<Function>(GA->getAliasee());
    gc::MMTkCheckAllocator = (gc::MMTkCheckAllocatorType)
      (uintptr_t)executionEngine->getPointerToFunction(F);
     
    F = globalModule->getFunction("JnJVM_org_mmtk_utility_heap_HeapGrowthManager_boot__Lorg_vmmagic_unboxed_Extent_2Lorg_vmmagic_unboxed_Extent_2");
    assert(F && "Could not find boot from HeapGrowthManager");
    BootHeapType BootHeap = (BootHeapType)
      (uintptr_t)executionEngine->getPointerToFunction(F);
    BootHeap(50 * 1024 * 1024, 100 * 1024 * 1024);
    
    GV = globalModule->getGlobalVariable("org_j3_config_Selected_4Plan_static", false);
    assert(GV && "No global plan.");
    uintptr_t Plan = *((uintptr_t*)executionEngine->getPointerToGlobal(GV));
    
    GA = dyn_cast<GlobalAlias>(globalModule->getNamedValue("MMTkPlanBoot"));
    assert(GA && "Could not find MMTkPlanBoot alias");
    F = dyn_cast<Function>(GA->getAliasee());
    BootType Boot = (BootType)
      (uintptr_t)executionEngine->getPointerToFunction(F);
    Boot(Plan);
    
    GA = dyn_cast<GlobalAlias>(globalModule->getNamedValue("MMTkPlanPostBoot"));
    assert(GA && "Could not find MMTkPlanPostBoot alias");
    F = dyn_cast<Function>(GA->getAliasee());
    Boot = (BootType)(uintptr_t)executionEngine->getPointerToFunction(F);
    Boot(Plan);
    
    GA = dyn_cast<GlobalAlias>(globalModule->getNamedValue("MMTkPlanFullBoot"));
    assert(GA && "Could not find MMTkPlanFullBoot alias");
    F = dyn_cast<Function>(GA->getAliasee());
    Boot = (BootType)(uintptr_t)executionEngine->getPointerToFunction(F);
    Boot(Plan);
    
    
    //===-------------------- TODO: make those virtual. -------------------===//
    F = globalModule->getFunction("JnJVM_org_mmtk_plan_TraceLocal_reportDelayedRootEdge__Lorg_vmmagic_unboxed_Address_2");
    assert(F && "Could not find reportDelayedRootEdge from TraceLocal");
    gc::MMTkDelayedRoot = (gc::MMTkDelayedRootType)
      (uintptr_t)executionEngine->getPointerToFunction(F);
    
    F = globalModule->getFunction("JnJVM_org_mmtk_plan_TraceLocal_processEdge__Lorg_vmmagic_unboxed_ObjectReference_2Lorg_vmmagic_unboxed_Address_2");
    assert(F && "Could not find processEdge from TraceLocal");
    gc::MMTkProcessEdge = (gc::MMTkProcessEdgeType)
      (uintptr_t)executionEngine->getPointerToFunction(F);
    
    F = globalModule->getFunction("JnJVM_org_mmtk_plan_TraceLocal_processRootEdge__Lorg_vmmagic_unboxed_Address_2Z");
    assert(F && "Could not find processEdge from TraceLocal");
    gc::MMTkProcessRootEdge = (gc::MMTkProcessRootEdgeType)
      (uintptr_t)executionEngine->getPointerToFunction(F);
    
    F = globalModule->getFunction("JnJVM_org_mmtk_plan_marksweep_MSTraceLocal_isLive__Lorg_vmmagic_unboxed_ObjectReference_2");
    assert(F && "Could not find isLive from TraceLocal");
    gc::MMTkIsLive = (gc::MMTkIsLiveType)
      (uintptr_t)executionEngine->getPointerToFunction(F);
   
    F = globalModule->getFunction("JnJVM_org_mmtk_plan_TraceLocal_retainForFinalize__Lorg_vmmagic_unboxed_ObjectReference_2");
    assert(F && "Could not find isLive from TraceLocal");
    gc::MMTkRetainForFinalize = (gc::MMTkRetainForFinalizeType)
      (uintptr_t)executionEngine->getPointerToFunction(F);
    
    F = globalModule->getFunction("JnJVM_org_mmtk_plan_TraceLocal_retainReferent__Lorg_vmmagic_unboxed_ObjectReference_2");
    assert(F && "Could not find isLive from TraceLocal");
    gc::MMTkRetainReferent = (gc::MMTkRetainReferentType)
      (uintptr_t)executionEngine->getPointerToFunction(F);
    
    F = globalModule->getFunction("JnJVM_org_mmtk_plan_TraceLocal_getForwardedReference__Lorg_vmmagic_unboxed_ObjectReference_2");
    assert(F && "Could not find isLive from TraceLocal");
    gc::MMTkGetForwardedReference = (gc::MMTkGetForwardedReferenceType)
      (uintptr_t)executionEngine->getPointerToFunction(F);
    
    F = globalModule->getFunction("JnJVM_org_mmtk_plan_TraceLocal_getForwardedReferent__Lorg_vmmagic_unboxed_ObjectReference_2");
    assert(F && "Could not find isLive from TraceLocal");
    gc::MMTkGetForwardedReferent = (gc::MMTkGetForwardedReferentType)
      (uintptr_t)executionEngine->getPointerToFunction(F);
    
    F = globalModule->getFunction("JnJVM_org_mmtk_plan_TraceLocal_getForwardedFinalizable__Lorg_vmmagic_unboxed_ObjectReference_2");
    assert(F && "Could not find isLive from TraceLocal");
    gc::MMTkGetForwardedFinalizable = (gc::MMTkGetForwardedFinalizableType)
      (uintptr_t)executionEngine->getPointerToFunction(F);
    
    F = globalModule->getFunction("Java_org_j3_mmtk_Collection_triggerCollection__I");
    assert(F && "Could not find external collect");
    gc::MMTkTriggerCollection = (gc::MMTkCollectType)
      (uintptr_t)executionEngine->getPointerToFunction(F);

  }
#endif
}


MvmModule::MvmModule(llvm::Module* module) {

  module->setDataLayout(globalModule->getDataLayout());
  module->setTargetTriple(globalModule->getTargetTriple());
  LLVMContext& Context = module->getContext(); 
  
  // Constant declaration
  constantLongMinusOne = ConstantInt::get(Type::getInt64Ty(Context), (uint64_t)-1);
  constantLongZero = ConstantInt::get(Type::getInt64Ty(Context), 0);
  constantLongOne = ConstantInt::get(Type::getInt64Ty(Context), 1);
  constantZero = ConstantInt::get(Type::getInt32Ty(Context), 0);
  constantInt8Zero = ConstantInt::get(Type::getInt8Ty(Context), 0);
  constantOne = ConstantInt::get(Type::getInt32Ty(Context), 1);
  constantTwo = ConstantInt::get(Type::getInt32Ty(Context), 2);
  constantThree = ConstantInt::get(Type::getInt32Ty(Context), 3);
  constantFour = ConstantInt::get(Type::getInt32Ty(Context), 4);
  constantFive = ConstantInt::get(Type::getInt32Ty(Context), 5);
  constantSix = ConstantInt::get(Type::getInt32Ty(Context), 6);
  constantSeven = ConstantInt::get(Type::getInt32Ty(Context), 7);
  constantEight = ConstantInt::get(Type::getInt32Ty(Context), 8);
  constantMinusOne = ConstantInt::get(Type::getInt32Ty(Context), (uint64_t)-1);
  constantMinInt = ConstantInt::get(Type::getInt32Ty(Context), MinInt);
  constantMaxInt = ConstantInt::get(Type::getInt32Ty(Context), MaxInt);
  constantMinLong = ConstantInt::get(Type::getInt64Ty(Context), MinLong);
  constantMaxLong = ConstantInt::get(Type::getInt64Ty(Context), MaxLong);
  constantFloatZero = ConstantFP::get(Type::getFloatTy(Context), 0.0f);
  constantFloatOne = ConstantFP::get(Type::getFloatTy(Context), 1.0f);
  constantFloatTwo = ConstantFP::get(Type::getFloatTy(Context), 2.0f);
  constantDoubleZero = ConstantFP::get(Type::getDoubleTy(Context), 0.0);
  constantDoubleOne = ConstantFP::get(Type::getDoubleTy(Context), 1.0);
  constantMaxIntFloat = ConstantFP::get(Type::getFloatTy(Context), MaxIntFloat);
  constantMinIntFloat = ConstantFP::get(Type::getFloatTy(Context), MinIntFloat);
  constantMinLongFloat = ConstantFP::get(Type::getFloatTy(Context), MinLongFloat);
  constantMinLongDouble = ConstantFP::get(Type::getDoubleTy(Context), MinLongDouble);
  constantMaxLongFloat = ConstantFP::get(Type::getFloatTy(Context), MaxLongFloat);
  constantMaxIntDouble = ConstantFP::get(Type::getDoubleTy(Context), MaxIntDouble);
  constantMinIntDouble = ConstantFP::get(Type::getDoubleTy(Context), MinIntDouble);
  constantMaxLongDouble = ConstantFP::get(Type::getDoubleTy(Context), MaxLongDouble);
  constantMaxLongDouble = ConstantFP::get(Type::getDoubleTy(Context), MaxLongDouble);
  constantFloatInfinity = ConstantFP::get(Type::getFloatTy(Context), MaxFloat);
  constantFloatMinusInfinity = ConstantFP::get(Type::getFloatTy(Context), MinFloat);
  constantDoubleInfinity = ConstantFP::get(Type::getDoubleTy(Context), MaxDouble);
  constantDoubleMinusInfinity = ConstantFP::get(Type::getDoubleTy(Context), MinDouble);
  constantDoubleMinusZero = ConstantFP::get(Type::getDoubleTy(Context), -0.0);
  constantFloatMinusZero = ConstantFP::get(Type::getFloatTy(Context), -0.0f);
  constantThreadIDMask = ConstantInt::get(pointerSizeType, mvm::Thread::IDMask);
  constantStackOverflowMask = 
    ConstantInt::get(pointerSizeType, mvm::Thread::StackOverflowMask);
  constantFatMask = ConstantInt::get(pointerSizeType, 
      pointerSizeType == Type::getInt32Ty(Context) ? 0x80000000 : 0x8000000000000000LL);
  constantPtrOne = ConstantInt::get(pointerSizeType, 1);
  constantPtrZero = ConstantInt::get(pointerSizeType, 0);

  constantPtrNull = Constant::getNullValue(ptrType); 
  constantPtrLogSize = 
    ConstantInt::get(Type::getInt32Ty(Context), sizeof(void*) == 8 ? 3 : 2);
  arrayPtrType = PointerType::getUnqual(ArrayType::get(Type::getInt8Ty(Context), 0));


  copyDefinitions(module, globalModule); 
    
  printFloatLLVM = module->getFunction("printFloat");
  printDoubleLLVM = module->getFunction("printDouble");
  printLongLLVM = module->getFunction("printLong");
  printIntLLVM = module->getFunction("printInt");
  printObjectLLVM = module->getFunction("printObject");

  unwindResume = module->getFunction("_Unwind_Resume_or_Rethrow");
  
  llvmGetException = module->getFunction("llvm.eh.exception");
  exceptionSelector = module->getFunction("llvm.eh.selector"); 
  
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
  llvm_gc_gcroot = module->getFunction("llvm.gcroot");

  llvm_atomic_lcs_i8 = module->getFunction("llvm.atomic.cmp.swap.i8.p0i8");
  llvm_atomic_lcs_i16 = module->getFunction("llvm.atomic.cmp.swap.i16.p0i16");
  llvm_atomic_lcs_i32 = module->getFunction("llvm.atomic.cmp.swap.i32.p0i32");
  llvm_atomic_lcs_i64 = module->getFunction("llvm.atomic.cmp.swap.i64.p0i64");

  llvm_atomic_lcs_ptr = pointerSizeType == Type::getInt32Ty(Context) ? llvm_atomic_lcs_i32 :
                                                           llvm_atomic_lcs_i64;

  unconditionalSafePoint = module->getFunction("unconditionalSafePoint");
  conditionalSafePoint = module->getFunction("conditionalSafePoint");
  AllocateFunction = module->getFunction("gcmalloc");
  assert(AllocateFunction && "No allocate function");
}


const llvm::PointerType* MvmModule::ptrType;
const llvm::PointerType* MvmModule::ptr32Type;
const llvm::PointerType* MvmModule::ptrPtrType;
const llvm::Type* MvmModule::pointerSizeType;
const llvm::Type* MvmModule::arrayPtrType;

const llvm::TargetData* MvmModule::TheTargetData;
llvm::GCStrategy* MvmModule::GC;
llvm::Module *MvmModule::globalModule;
llvm::ExistingModuleProvider *MvmModule::globalModuleProvider;
llvm::FunctionPassManager* MvmModule::globalFunctionPasses;
llvm::ExecutionEngine* MvmModule::executionEngine;
mvm::LockRecursive MvmModule::protectEngine;
mvm::BumpPtrAllocator MvmModule::Allocator;

uint64 MvmModule::getTypeSize(const llvm::Type* type) {
  return TheTargetData->getTypeAllocSize(type);
}

void MvmModule::runPasses(llvm::Function* func,
                          llvm::FunctionPassManager* pm) {
  // Take the lock because the pass manager will call materializeFunction.
  // Our implementation of materializeFunction requires that the lock is held
  // by the caller. This is due to LLVM's JIT subsystem where the call to
  // materializeFunction is guarded.
  if (executionEngine) executionEngine->lock.acquire();
  pm->run(*func);
  if (executionEngine) executionEngine->lock.release();
}

static void addPass(FunctionPassManager *PM, Pass *P) {
  // Add the pass to the pass manager...
  PM->add(P);
}

// This is equivalent to:
// opt -simplifycfg -mem2reg -instcombine -jump-threading -simplifycfg
//     -scalarrepl -instcombine -condprop -simplifycfg -predsimplify 
//     -reassociate -licm -loop-unswitch -indvars -loop-deletion -loop-unroll 
//     -instcombine -gvn -sccp -simplifycfg -instcombine -condprop -dse -adce 
//     -simplifycfg
//
void MvmModule::AddStandardCompilePasses() {
  // TODO: enable this when
  // - each module will have its declaration of external functions
  // 
  //PM->add(llvm::createVerifierPass());        // Verify that input is correct
 
  FunctionPassManager* PM = globalFunctionPasses;
  PM->add(new TargetData(*MvmModule::TheTargetData));

  addPass(PM, createCFGSimplificationPass()); // Clean up disgusting code
  addPass(PM, createPromoteMemoryToRegisterPass());// Kill useless allocas
  
  addPass(PM, createInstructionCombiningPass()); // Cleanup for scalarrepl.
  addPass(PM, createJumpThreadingPass());        // Thread jumps.
  addPass(PM, createCFGSimplificationPass());    // Merge & remove BBs
  addPass(PM, createScalarReplAggregatesPass()); // Break up aggregate allocas
  addPass(PM, createInstructionCombiningPass()); // Combine silly seq's
  addPass(PM, createCondPropagationPass());      // Propagate conditionals
  
  addPass(PM, createCFGSimplificationPass());    // Merge & remove BBs
  addPass(PM, createReassociatePass());          // Reassociate expressions
  addPass(PM, createLICMPass());                 // Hoist loop invariants
  
  addPass(PM, createLoopUnswitchPass());         // Unswitch loops.
  addPass(PM, createIndVarSimplifyPass());       // Canonicalize indvars
  addPass(PM, createLoopDeletionPass());         // Delete dead loops
  addPass(PM, createLoopUnrollPass());           // Unroll small loops*/
  addPass(PM, createInstructionCombiningPass()); // Clean up after the unroller
  addPass(PM, createGVNPass());                  // Remove redundancies
  addPass(PM, createSCCPPass());                 // Constant prop with SCCP
  addPass(PM, createCFGSimplificationPass());    // Merge & remove BBs
 
  // Run instcombine after redundancy elimination to exploit opportunities
  // opened up by them.
  addPass(PM, createInstructionCombiningPass());
  addPass(PM, createCondPropagationPass());      // Propagate conditionals

  addPass(PM, createDeadStoreEliminationPass()); // Delete dead stores
  addPass(PM, createAggressiveDCEPass());        // Delete dead instructions
  addPass(PM, createCFGSimplificationPass());    // Merge & remove BBs
  
  PM->doInitialization();
  
}

// We protect the creation of IR with the executionEngine lock because
// codegen'ing a function may also create IR objects.
void MvmModule::protectIR() {
  if (executionEngine) {
    protectEngine.lock();
  }
}

void MvmModule::unprotectIR() {
  if (executionEngine) {
    protectEngine.unlock();
  }
}


void MvmModule::copyDefinitions(Module* Dst, Module* Src) {
  // Loop over all of the functions in the src module, mapping them over
  for (Module::const_iterator I = Src->begin(), E = Src->end(); I != E; ++I) {
    const Function *SF = I;   // SrcFunction
    if (SF->isDeclaration()) {
      Function* F = Function::Create(SF->getFunctionType(),
                                     GlobalValue::ExternalLinkage,
                                     SF->getName(), Dst);
      F->setAttributes(SF->getAttributes());
    }
  }
  Function* SF = Src->getFunction("gcmalloc");
  if (SF && !SF->isDeclaration()) {
    Function* F = Function::Create(SF->getFunctionType(),
                                   GlobalValue::ExternalLinkage,
                                   SF->getName(), Dst);
    F->setAttributes(SF->getAttributes());
    if (executionEngine) {
      void* ptr = executionEngine->getPointerToFunction(SF);
      executionEngine->updateGlobalMapping(F, ptr);
    }
  }
}

void JITMethodInfo::scan(void* TL, void* ip, void* addr) {
  if (GCInfo) {
    DEBUG(llvm::errs() << GCInfo->getFunction().getName() << '\n');
    // All safe points have the same informations currently in LLVM.
    llvm::GCFunctionInfo::iterator J = GCInfo->begin();
    //uintptr_t spaddr = (uintptr_t)addr + GFI->getFrameSize() + sizeof(void*);
    uintptr_t spaddr = ((uintptr_t*)addr)[0];
    for (llvm::GCFunctionInfo::live_iterator K = GCInfo->live_begin(J),
         KE = GCInfo->live_end(J); K != KE; ++K) {
      intptr_t obj = *(intptr_t*)(spaddr + K->StackOffset);
      // Verify that obj does not come from a JSR bytecode.
      if (!(obj & 1)) Collector::scanObject((void**)(spaddr + K->StackOffset));
    }
  }
}

void MvmJITMethodInfo::print(void* ip, void* addr) {
  fprintf(stderr, "; %p in %s LLVM method\n", ip, Func->getName().data());
}
