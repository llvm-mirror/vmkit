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
    cl->initialiseClass(JavaThread::get()->isolate);
#endif

  meth->compiledPtr();
  
  return meth;
}

std::pair<Class*, uint32>* JnjvmModuleProvider::lookupCallback(Function* F) {
  callback_iterator CI = callbacks.find(F);
  if (CI != callbacks.end()) {
    return &(CI->second);
  } else {
    return 0;
  }
}

JavaMethod* JnjvmModuleProvider::lookupFunction(Function* F) {
  function_iterator CI = functions.find(F);
  if (CI != functions.end()) {
    return CI->second;
  } else {
    return 0;
  }
}

bool JnjvmModuleProvider::materializeFunction(Function *F, 
                                              std::string *ErrInfo) {
  
  if (mvm::MvmModule::executionEngine->getPointerToGlobalIfAvailable(F))
    return false;

  if (!(F->hasNotBeenReadFromBitcode())) 
    return false;
  
  JavaMethod* meth = lookupFunction(F);
  
  if (!meth) {
    // It's a callback
    std::pair<Class*, uint32> * p = lookupCallback(F);
    assert(p && "No callback where there should be one");
    meth = staticLookup(p->first, p->second); 
  }
  
  void* val = meth->compiledPtr();
  if (F->isDeclaration())
    mvm::MvmModule::executionEngine->updateGlobalMapping(F, val);
 
  if (isVirtual(meth->access)) {
    LLVMMethodInfo* LMI = ((JnjvmModule*)TheModule)->getMethodInfo(meth);
    uint64_t offset = LMI->getOffset()->getZExtValue();
    assert(meth->classDef->isResolved() && "Class not resolved");
#ifndef ISOLATE_SHARING
    assert(meth->classDef->isReady() && "Class not ready");
#endif
    assert(meth->classDef->virtualVT && "Class has no VT");
    assert(meth->classDef->virtualTableSize > offset && 
        "The method's offset is greater than the virtual table size");
    ((void**)meth->classDef->virtualVT)[offset] = val;
  } else {
#ifndef ISOLATE_SHARING
    meth->classDef->initialiseClass(JavaThread::get()->isolate);
#endif
  }

  return false;
}
void* JnjvmModuleProvider::materializeFunction(JavaMethod* meth) {
  Function* func = parseFunction(meth);
  
  void* res = mvm::MvmModule::executionEngine->getPointerToGlobal(func);
  mvm::Code* m = mvm::MvmModule::getCodeFromPointer(res);
  if (m) m->setMetaInfo(meth);
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
    } else {
      jit.javaCompile();
      mvm::MvmModule::runPasses(func, perFunctionPasses);
    }
  }
  return func;
}

llvm::Function* JnjvmModuleProvider::addCallback(Class* cl, uint32 index,
                                                 Signdef* sign, bool stat) {
  
  void* key = &(cl->getConstantPool()->ctpRes[index]);
   
  reverse_callback_iterator CI = reverseCallbacks.find(key);
  if (CI != reverseCallbacks.end()) {
    return CI->second;
  }
  
  const llvm::FunctionType* type = 0;
  JnjvmModule* M = cl->classLoader->getModule();
  LLVMSignatureInfo* LSI = M->getSignatureInfo(sign);
  
  if (stat) {
    type = LSI->getStaticType();
  } else {
    type = LSI->getVirtualType();
  }
  
  Function* func = llvm::Function::Create(type, 
                                          llvm::GlobalValue::GhostLinkage,
                                          "callback",
                                          TheModule);

  callbacks.insert(std::make_pair(func, std::make_pair(cl, index)));
  reverseCallbacks.insert(std::make_pair(key, func));

  return func;
}

void JnjvmModuleProvider::addFunction(Function* F, JavaMethod* meth) {
  functions.insert(std::make_pair(F, meth));
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
  //addPass(PM, createPredicateSimplifierPass());
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
  addPass(PM, mvm::createLowerConstantCallsPass());
  
  // Run instcombine after redundancy elimination to exploit opportunities
  // opened up by them.
  addPass(PM, createInstructionCombiningPass());
  addPass(PM, createCondPropagationPass());      // Propagate conditionals

  addPass(PM, createDeadStoreEliminationPass()); // Delete dead stores
  addPass(PM, createAggressiveDCEPass());        // Delete dead instructions
  addPass(PM, createCFGSimplificationPass());    // Merge & remove BBs
  
  addPass(PM, mvm::createLowerForcedCallsPass());
  
}

JnjvmModuleProvider::JnjvmModuleProvider(JnjvmModule *m) {
  TheModule = (Module*)m;
  mvm::MvmModule::protectEngine->lock();
  mvm::MvmModule::executionEngine->addModuleProvider(this);
  mvm::MvmModule::protectEngine->unlock();
  perFunctionPasses = new llvm::FunctionPassManager(this);
  perFunctionPasses->add(new llvm::TargetData(m));
  AddStandardCompilePasses(m, perFunctionPasses);
}

JnjvmModuleProvider::~JnjvmModuleProvider() {
  mvm::MvmModule::protectEngine->lock();
  mvm::MvmModule::executionEngine->removeModuleProvider(this);
  mvm::MvmModule::protectEngine->unlock();
  delete TheModule;
}
