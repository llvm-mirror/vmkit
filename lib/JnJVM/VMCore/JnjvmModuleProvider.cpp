//===--- JnjvmModuleProvider.cpp - LLVM Module Provider for Jnjvm ---------===//
//
//                              Jnjvm
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/LinkAllPasses.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/Support/MutexGuard.h"

#include "mvm/JIT.h"

#include "JavaAccess.h"
#include "JavaClass.h"
#include "JavaConstantPool.h"
#include "JavaJIT.h"
#include "JavaThread.h"
#include "Jnjvm.h"
#include "JnjvmModule.h"
#include "JnjvmModuleProvider.h"

using namespace llvm;
using namespace jnjvm;


static AnnotationID JavaCallback_ID(
  AnnotationManager::getID("Java::Callback"));


class CallbackInfo: public Annotation {
public:
  Class* cl;
  uint32 index;

  CallbackInfo(Class* c, uint32 i) : Annotation(JavaCallback_ID), 
    cl(c), index(i) {}
};

JavaMethod* JnjvmModuleProvider::staticLookup(Class* caller, uint32 index) {
  JavaConstantPool* ctpInfo = caller->getConstantPool();
  

  bool isStatic = ctpInfo->isAStaticCall(index);

  CommonClass* cl = 0;
  const UTF8* utf8 = 0;
  Signdef* sign = 0;

  ctpInfo->resolveMethod(index, cl, utf8, sign);
  JavaMethod* meth = cl->lookupMethod(utf8, sign->keyName, isStatic, true,
                                      0);

#ifndef ISOLATE_SHARING
  // A multi environment would have already initialized the class. Besides,
  // a callback does not involve UserClass, therefore we wouldn't know
  // which class to initialize.
  if (!isVirtual(meth->access))
    cl->initialiseClass(JavaThread::get()->getJVM());
#endif

  meth->compiledPtr();
  
  return meth;
}

bool JnjvmModuleProvider::materializeFunction(Function *F, 
                                              std::string *ErrInfo) {
  
  if (mvm::MvmModule::executionEngine->getPointerToGlobalIfAvailable(F))
    return false;

  if (!(F->hasNotBeenReadFromBitcode())) 
    return false;
  
  JavaMethod* meth = LLVMMethodInfo::get(F);
  
  if (!meth) {
    // It's a callback
    CallbackInfo* CI = (CallbackInfo*)F->getAnnotation(JavaCallback_ID);
    assert(CI && "No callback where there should be one");
    meth = staticLookup(CI->cl, CI->index); 
  }
  
  void* val = meth->compiledPtr();
  if (F->isDeclaration())
    mvm::MvmModule::executionEngine->updateGlobalMapping(F, val);
 
  if (isVirtual(meth->access)) {
    LLVMMethodInfo* LMI = ((JnjvmModule*)TheModule)->getMethodInfo(meth);
    uint64_t offset = LMI->getOffset()->getZExtValue();
    assert(meth->classDef->isResolved() && "Class not resolved");
#ifndef ISOLATE_SHARING
    assert(meth->classDef->isInitializing() && "Class not ready");
#endif
    assert(meth->classDef->virtualVT && "Class has no VT");
    assert(meth->classDef->virtualTableSize > offset && 
        "The method's offset is greater than the virtual table size");
    ((void**)meth->classDef->virtualVT)[offset] = val;
  } else {
#ifndef ISOLATE_SHARING
    meth->classDef->initialiseClass(JavaThread::get()->getJVM());
#endif
  }

  return false;
}

void* JnjvmModuleProvider::materializeFunction(JavaMethod* meth) {
  Function* func = parseFunction(meth);
  
  void* res = mvm::MvmModule::executionEngine->getPointerToGlobal(func);
  func->deleteBody();

  return res;
}

Function* JnjvmModuleProvider::parseFunction(JavaMethod* meth) {
  LLVMMethodInfo* LMI = ((JnjvmModule*)TheModule)->getMethodInfo(meth);
  Function* func = LMI->getMethod();
  if (func->hasNotBeenReadFromBitcode()) {
    // We are jitting. Take the lock.
    llvm::MutexGuard locked(mvm::MvmModule::executionEngine->lock);
    JavaJIT jit(meth, func);
    if (isNative(meth->access)) {
      jit.nativeCompile();
      mvm::MvmModule::runPasses(func, perNativeFunctionPasses);
    } else {
      jit.javaCompile();
      mvm::MvmModule::runPasses(func, perFunctionPasses);
    }
  }
  return func;
}

llvm::Function* JnjvmModuleProvider::addCallback(Class* cl, uint32 index,
                                                 Signdef* sign, bool stat) {
  
  JnjvmModule* M = cl->classLoader->getModule();
  Function* func = 0;
  LLVMSignatureInfo* LSI = M->getSignatureInfo(sign);
  if (!stat) {
    char* name = cl->printString();
    char* key = (char*)alloca(strlen(name) + 2);
    sprintf(key, "%s%d", name, index);
    Function* F = TheModule->getFunction(key);
    if (F) return F;
  
    const FunctionType* type = LSI->getVirtualType();
  
    func = Function::Create(type, GlobalValue::GhostLinkage, key, TheModule);
  } else {
    const llvm::FunctionType* type = LSI->getStaticType();
    func = Function::Create(type, GlobalValue::GhostLinkage, "staticCallback",
                            TheModule);
  }
  
  CallbackInfo* A = new CallbackInfo(cl, index);
  func->addAnnotation(A);
  
  return func;
}

namespace mvm {
  llvm::FunctionPass* createEscapeAnalysisPass(llvm::Function*);
  llvm::FunctionPass* createLowerConstantCallsPass();
  llvm::FunctionPass* createLowerForcedCallsPass();
}

static void addPass(FunctionPassManager *PM, Pass *P) {
  // Add the pass to the pass manager...
  PM->add(P);
}

// This is equivalent to:
// opt -simplifycfg -mem2reg -instcombine -jump-threading -scalarrepl -instcombine 
//     -condprop -simplifycfg -reassociate -licm essai.bc -loop-unswitch 
//     -indvars -loop-unroll -instcombine -gvn -sccp -simplifycfg
//     -instcombine -condprop -dse -adce -simplifycfg
//
static void AddStandardCompilePasses(JnjvmModule* mod, FunctionPassManager *PM) {
  llvm::MutexGuard locked(mvm::MvmModule::executionEngine->lock);
  
  // TODO: enable this when
  // - we can call multiple times the makeLLVMModuleContents function generated 
  //   by llc -march=cpp -cppgen=contents
  // - intrinsics won't be in the .ll files
  // - each module will have its declaration of external functions
  // 
  //PM->add(llvm::createVerifierPass());        // Verify that input is correct
  
  addPass(PM, llvm::createCFGSimplificationPass()); // Clean up disgusting code
  addPass(PM, llvm::createPromoteMemoryToRegisterPass());// Kill useless allocas
  
  addPass(PM, createInstructionCombiningPass()); // Cleanup for scalarrepl.
  addPass(PM, createJumpThreadingPass());        // Thread jumps.
  addPass(PM, createCFGSimplificationPass());    // Merge & remove BBs
  addPass(PM, createScalarReplAggregatesPass()); // Break up aggregate allocas
  addPass(PM, createInstructionCombiningPass()); // Combine silly seq's
  addPass(PM, createCondPropagationPass());      // Propagate conditionals
  
  addPass(PM, createCFGSimplificationPass());    // Merge & remove BBs
  addPass(PM, createPredicateSimplifierPass());
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
 
  Function* func = mod->JavaObjectAllocateFunction;
  addPass(PM, mvm::createEscapeAnalysisPass(func));

  // Do not do GVN after this pass: initialization checks could be removed.
  addPass(PM, mvm::createLowerConstantCallsPass());
  
  // Run instcombine after redundancy elimination to exploit opportunities
  // opened up by them.
  addPass(PM, createInstructionCombiningPass());
  addPass(PM, createCondPropagationPass());      // Propagate conditionals

  addPass(PM, createDeadStoreEliminationPass()); // Delete dead stores
  addPass(PM, createAggressiveDCEPass());        // Delete dead instructions
  addPass(PM, createCFGSimplificationPass());    // Merge & remove BBs

#ifndef ISOLATE
  if (mod->isStaticCompiling())
#endif
    addPass(PM, mvm::createLowerForcedCallsPass());    // Remove forced initialization
  
}

JnjvmModuleProvider::JnjvmModuleProvider(JnjvmModule *m) {
  TheModule = (Module*)m;
  mvm::MvmModule::protectEngine.lock();
  mvm::MvmModule::executionEngine->addModuleProvider(this);
  mvm::MvmModule::protectEngine.unlock();
  perFunctionPasses = new llvm::FunctionPassManager(this);
  perFunctionPasses->add(new llvm::TargetData(m));
  AddStandardCompilePasses(m, perFunctionPasses);
  
  perNativeFunctionPasses = new llvm::FunctionPassManager(this);
  perNativeFunctionPasses->add(new llvm::TargetData(m));
  addPass(perNativeFunctionPasses, createInstructionCombiningPass()); // Cleanup for scalarrepl.
  addPass(perNativeFunctionPasses, mvm::createLowerConstantCallsPass());
}

JnjvmModuleProvider::~JnjvmModuleProvider() {
  mvm::MvmModule::protectEngine.lock();
  mvm::MvmModule::executionEngine->removeModuleProvider(this);
  mvm::MvmModule::protectEngine.unlock();
  delete TheModule;
  delete perFunctionPasses;
  delete perNativeFunctionPasses;
}
