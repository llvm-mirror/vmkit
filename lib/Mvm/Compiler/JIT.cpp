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
#include <llvm/PassManager.h>
#include <llvm/Type.h>
#include <llvm/Analysis/DebugInfo.h>
#include <llvm/Analysis/LoopPass.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Assembly/Parser.h>
#include <llvm/CodeGen/GCStrategy.h>
#include <llvm/CodeGen/JITCodeEmitter.h>
#include <llvm/Config/config.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include "llvm/ExecutionEngine/JITEventListener.h"
#include "llvm/Support/CommandLine.h"
#include <llvm/Support/Debug.h>
#include <llvm/Support/IRReader.h>
#include <llvm/Support/MutexGuard.h>
#include <llvm/Support/PassNameParser.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Target/TargetData.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Target/TargetSelect.h>
#include <../lib/ExecutionEngine/JIT/JIT.h>

#include "mvm/JIT.h"
#include "mvm/Threads/Locks.h"
#include "mvm/Threads/Thread.h"
#include "mvm/VirtualMachine.h"
#include "mvm/GC/GC.h"
#include "MutatorThread.h"
#include "MvmGC.h"

#include <dlfcn.h>
#include <sys/mman.h>

#if defined(__MACH__)
#define SELF_HANDLE RTLD_DEFAULT
#else
#define SELF_HANDLE 0
#endif

using namespace mvm;
using namespace llvm;


static cl::list<std::string> 
LoadBytecodeFiles("load-bc", cl::desc("Load bytecode file"), cl::ZeroOrMore,
                  cl::CommaSeparated);

cl::opt<bool> EmitDebugInfo("emit-debug-info", 
                  cl::desc("Emit debugging information"),
                  cl::init(false));

namespace mvm {
  namespace llvm_runtime {
    #include "LLVMRuntime.inc"
  }
  
#ifdef WITH_MMTK
  namespace mmtk_runtime {
    #include "MMTkInline.inc"
  }
#endif
  void linkVmkitGC();
}

const char* MvmModule::getHostTriple() {
  return LLVM_HOSTTRIPLE;
}

void MvmJITMethodInfo::print(void* ip, void* addr) {
  fprintf(stderr, "; %p in %s LLVM method\n", ip,
      ((llvm::Function*)MetaInfo)->getName().data());
}

class MvmJITListener : public llvm::JITEventListener {
public:
  virtual void NotifyFunctionEmitted(const Function &F,
                                     void *Code, size_t Size,
                                     const EmittedFunctionDetails &Details) {
    assert(F.getParent() == MvmModule::globalModule);
    assert(F.hasGC());
    // We know the last GC info is for this method.
    GCStrategy::iterator I = mvm::MvmModule::TheGCStrategy->end();
    I--;
    DEBUG(errs() << (*I)->getFunction().getName() << '\n');
    DEBUG(errs() << F.getName() << '\n');
    assert(&(*I)->getFunction() == &F &&
        "GC Info and method do not correspond");
    llvm::GCFunctionInfo* GFI = *I;
    JITMethodInfo* MI = new(*MvmModule::Allocator, "MvmJITMethodInfo")
        MvmJITMethodInfo(GFI, &F, MvmModule::executionEngine);
    MI->addToVM(mvm::Thread::get()->MyVM, (JIT*)MvmModule::executionEngine);
  }
};

void JITMethodInfo::addToVM(VirtualMachine* VM, JIT* jit) {
  JITCodeEmitter* JCE = jit->getCodeEmitter();
  assert(GCInfo != NULL);
  for (GCFunctionInfo::iterator I = GCInfo->begin(), E = GCInfo->end();
       I != E;
       I++) {
    uintptr_t address = JCE->getLabelAddress(I->Label);
    assert(address != 0);
    VM->FunctionsCache.addMethodInfo(this, (void*)address);
  }
}

static MvmJITListener JITListener;

void MvmModule::loadBytecodeFile(const std::string& str) {
  SMDiagnostic Err;
  Module* M = ParseIRFile(str, Err, globalModule->getContext());
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
  llvm::JITEmitDebugInfo = EmitDebugInfo;
#if DWARF_EXCEPTIONS
  llvm::JITExceptionHandling = true;
#else
  llvm::JITExceptionHandling = false;
#endif
  
  // Disable branch fold for accurate line numbers.
  const char* commands[2] = { "vmkit", "-disable-branch-fold" };
  llvm::cl::ParseCommandLineOptions(2, const_cast<char**>(commands));

  if (!M) {
    globalModule = new Module("bootstrap module", *(new LLVMContext()));

    InitializeNativeTarget();

    executionEngine = ExecutionEngine::createJIT(globalModule, 0,
                                                 0, level, false);

    Allocator = new BumpPtrAllocator();
    executionEngine->RegisterJITEventListener(&JITListener);    
    std::string str = 
      executionEngine->getTargetData()->getStringRepresentation();
    globalModule->setDataLayout(str);
    globalModule->setTargetTriple(getHostTriple());
  
    TheTargetData = executionEngine->getTargetData();
  } else {
    globalModule = M;
    TheTargetData = T->getTargetData();
  }

  //LLVMContext& Context = globalModule->getContext();
  //MetadataTypeKind = Context.getMDKindID("HighLevelType");
 
  for (std::vector<std::string>::iterator i = LoadBytecodeFiles.begin(),
       e = LoadBytecodeFiles.end(); i != e; ++i) {
    loadBytecodeFile(*i); 
  }
}

BaseIntrinsics::BaseIntrinsics(llvm::Module* module) {

  module->setDataLayout(MvmModule::globalModule->getDataLayout());
  module->setTargetTriple(MvmModule::globalModule->getTargetTriple());
  LLVMContext& Context = module->getContext();
  
#ifdef WITH_MMTK
  static const char* MMTkSymbol =
    "JnJVM_org_j3_bindings_Bindings_gcmalloc__"
    "ILorg_vmmagic_unboxed_ObjectReference_2";
  if (dlsym(SELF_HANDLE, MMTkSymbol)) {
    // If we have found MMTk, read the gcmalloc function.
    mvm::mmtk_runtime::makeLLVMFunction(module);
  }
#endif
  mvm::llvm_runtime::makeLLVMModuleContents(module);
  
  
  // Type declaration
  ptrType = PointerType::getUnqual(Type::getInt8Ty(Context));
  ptr32Type = PointerType::getUnqual(Type::getInt32Ty(Context));
  ptrPtrType = PointerType::getUnqual(ptrType);
  pointerSizeType = module->getPointerSize() == Module::Pointer32 ?
    Type::getInt32Ty(Context) : Type::getInt64Ty(Context);
  
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
  AllocateFunction->setGC("vmkit");
  assert(AllocateFunction && "No allocate function");
  AllocateUnresolvedFunction = module->getFunction("gcmallocUnresolved");
  assert(AllocateUnresolvedFunction && "No allocateUnresolved function");
  AddFinalizationCandidate = module->getFunction("addFinalizationCandidate");
  assert(AddFinalizationCandidate && "No addFinalizationCandidate function");
}

const llvm::TargetData* MvmModule::TheTargetData;
llvm::GCStrategy* MvmModule::TheGCStrategy;
llvm::Module *MvmModule::globalModule;
llvm::ExecutionEngine* MvmModule::executionEngine;
mvm::LockRecursive MvmModule::protectEngine;
mvm::BumpPtrAllocator* MvmModule::Allocator;
//unsigned MvmModule::MetadataTypeKind;

uint64 MvmModule::getTypeSize(const llvm::Type* type) {
  return TheTargetData->getTypeAllocSize(type);
}

void MvmModule::runPasses(llvm::Function* func,
                          llvm::FunctionPassManager* pm) {
  // Take the lock because the pass manager will call materializeFunction.
  // Our implementation of materializeFunction requires that the lock is held
  // by the caller. This is due to LLVM's JIT subsystem where the call to
  // materializeFunction is guarded.
  pm->run(*func);
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
static void AddStandardCompilePasses(FunctionPassManager* PM) { 
   
  addPass(PM, createCFGSimplificationPass()); // Clean up disgusting code
  addPass(PM, createPromoteMemoryToRegisterPass());// Kill useless allocas
  
  addPass(PM, createInstructionCombiningPass()); // Cleanup for scalarrepl.
  addPass(PM, createScalarReplAggregatesPass()); // Break up aggregate allocas
  addPass(PM, createInstructionCombiningPass()); // Cleanup for scalarrepl.
  addPass(PM, createJumpThreadingPass());        // Thread jumps.
  addPass(PM, createCFGSimplificationPass());    // Merge & remove BBs
  addPass(PM, createInstructionCombiningPass()); // Combine silly seq's
  
  addPass(PM, createCFGSimplificationPass());    // Merge & remove BBs
  addPass(PM, createReassociatePass());          // Reassociate expressions
  addPass(PM, createLoopRotatePass());           // Rotate loops.
  addPass(PM, createLICMPass());                 // Hoist loop invariants
  addPass(PM, createLoopUnswitchPass());         // Unswitch loops.
  addPass(PM, createInstructionCombiningPass()); 
  addPass(PM, createIndVarSimplifyPass());       // Canonicalize indvars
  addPass(PM, createLoopDeletionPass());         // Delete dead loops
  addPass(PM, createLoopUnrollPass());           // Unroll small loops*/
  addPass(PM, createInstructionCombiningPass()); // Clean up after the unroller
  addPass(PM, createGVNPass());                  // Remove redundancies
  addPass(PM, createMemCpyOptPass());             // Remove memcpy / form memset  
  addPass(PM, createSCCPPass());                 // Constant prop with SCCP

  // Run instcombine after redundancy elimination to exploit opportunities
  // opened up by them.
  addPass(PM, createInstructionCombiningPass());
  addPass(PM, createJumpThreadingPass());         // Thread jumps
  addPass(PM, createDeadStoreEliminationPass());  // Delete dead stores
  addPass(PM, createAggressiveDCEPass());         // Delete dead instructions
  addPass(PM, createCFGSimplificationPass());     // Merge & remove BBs
}

static cl::opt<bool> 
DisableOptimizations("disable-opt", 
                     cl::desc("Do not run any optimization passes"));

cl::opt<bool>
StandardCompileOpts("std-compile-opts", 
                   cl::desc("Include the standard compile time optimizations"));

// The OptimizationList is automatically populated with registered Passes by the
// PassNameParser.
//
static llvm::cl::list<const llvm::PassInfo*, bool, llvm::PassNameParser>
PassList(llvm::cl::desc("Optimizations available:"));

namespace mvm {
  llvm::FunctionPass* createInlineMallocPass();
}

void MvmModule::addCommandLinePasses(FunctionPassManager* PM) {
  addPass(PM, new TargetData(*MvmModule::TheTargetData));

  addPass(PM, createVerifierPass());        // Verify that input is correct

#ifdef WITH_MMTK
  addPass(PM, createCFGSimplificationPass()); // Clean up disgusting code
  addPass(PM, createInlineMallocPass());
#endif
  
  // Create a new optimization pass for each one specified on the command line
  for (unsigned i = 0; i < PassList.size(); ++i) {
    // Check to see if -std-compile-opts was specified before this option.  If
    // so, handle it.
    if (StandardCompileOpts && 
        StandardCompileOpts.getPosition() < PassList.getPosition(i)) {
      if (!DisableOptimizations) AddStandardCompilePasses(PM);
      StandardCompileOpts = false;
    }
      
    const PassInfo *PassInf = PassList[i];
    Pass *P = 0;
    if (PassInf->getNormalCtor())
      P = PassInf->getNormalCtor()();
    else
      errs() << "cannot create pass: "
           << PassInf->getPassName() << "\n";
    if (P) {
        bool isModulePass = (P->getPassKind() == PT_Module);
        if (isModulePass) 
          errs() << "vmkit does not support module pass: "
             << PassInf->getPassName() << "\n";
        else addPass(PM, P);

    }
  }
    
  // If -std-compile-opts was specified at the end of the pass list, add them.
  if (StandardCompileOpts) {
    AddStandardCompilePasses(PM);
  }
  PM->doInitialization();
}

// We protect the creation of IR with the executionEngine lock because
// codegen'ing a function may also create IR objects.
void MvmModule::protectIR() {
  protectEngine.lock();
}

void MvmModule::unprotectIR() {
  protectEngine.unlock();
}

void JITMethodInfo::scan(uintptr_t closure, void* ip, void* addr) {
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
      if (!(obj & 1)) {
        Collector::scanObject((void**)(spaddr + K->StackOffset), closure);
      }
    }
  }
}
