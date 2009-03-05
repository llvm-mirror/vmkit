//===--- JnjvmModuleProvider.cpp - LLVM Module Provider for Jnjvm ---------===//
//
//                              Jnjvm
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/LinkAllPasses.h"
#include "llvm/PassManager.h"
#include "llvm/Module.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/Support/MutexGuard.h"
#include "llvm/Target/TargetData.h"

#include "mvm/JIT.h"

#include "JavaAccess.h"
#include "JavaClass.h"
#include "JavaConstantPool.h"
#include "JavaJIT.h"
#include "JavaThread.h"
#include "JavaTypes.h"
#include "Jnjvm.h"

#include "jnjvm/JnjvmModule.h"
#include "jnjvm/JnjvmModuleProvider.h"

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
  UserClass* lookup = cl->isArray() ? cl->super : cl->asClass();
  JavaMethod* meth = lookup->lookupMethod(utf8, sign->keyName, isStatic, true,
                                          0);

  assert(lookup->isInitializing() && "Class not ready");

  
  return meth;
}

bool JnjvmModuleProvider::materializeFunction(Function *F, 
                                              std::string *ErrInfo) {
  
  if (!(F->hasNotBeenReadFromBitcode())) 
    return false;
 
  assert(mvm::MvmModule::executionEngine && "No execution engine");
  if (mvm::MvmModule::executionEngine->getPointerToGlobalIfAvailable(F))
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
#if !defined(ISOLATE_SHARING) && !defined(SERVICE)
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

llvm::Function* JnjvmModuleProvider::addCallback(Class* cl, uint32 index,
                                                 Signdef* sign, bool stat) {
  
  JnjvmModule* M = cl->classLoader->getModule();
  Function* func = 0;
  LLVMSignatureInfo* LSI = M->getSignatureInfo(sign);
  if (!stat) {
    const char* name = cl->printString();
    char* key = (char*)alloca(strlen(name) + 16);
    sprintf(key, "%s%d", name, index);
    Function* F = TheModule->getFunction(key);
    if (F) return F;
  
    const FunctionType* type = LSI->getVirtualType();
  
    func = Function::Create(type, GlobalValue::GhostLinkage, key, TheModule);
  } else {
    const llvm::FunctionType* type = LSI->getStaticType();
    if (M->isStaticCompiling()) {
      func = Function::Create(type, GlobalValue::ExternalLinkage, "staticCallback",
                              TheModule);
    } else {
      func = Function::Create(type, GlobalValue::GhostLinkage, "staticCallback",
                              TheModule);
    }
  }
  
  ++nbCallbacks;
  CallbackInfo* A = new CallbackInfo(cl, index);
  func->addAnnotation(A);
  
  return func;
}

namespace mvm {
  llvm::FunctionPass* createEscapeAnalysisPass(llvm::Function*);
}

namespace jnjvm {
  llvm::FunctionPass* createLowerConstantCallsPass();
}

JnjvmModuleProvider::JnjvmModuleProvider(JnjvmModule *m) {
  TheModule = m->getLLVMModule();
  if (m->executionEngine) {
    m->protectEngine.lock();
    m->executionEngine->addModuleProvider(this);
    m->protectEngine.unlock();
  }
    
  m->JavaNativeFunctionPasses = new llvm::FunctionPassManager(this);
  m->JavaNativeFunctionPasses->add(new llvm::TargetData(TheModule));
  // Lower constant calls to lower things like getClass used
  // on synchronized methods.
  m->JavaNativeFunctionPasses->add(createLowerConstantCallsPass());
  
  m->JavaFunctionPasses = new llvm::FunctionPassManager(this);
  m->JavaFunctionPasses->add(new llvm::TargetData(TheModule));
  Function* func = m->JavaObjectAllocateFunction;
  m->JavaFunctionPasses->add(mvm::createEscapeAnalysisPass(func));
  m->JavaFunctionPasses->add(createLowerConstantCallsPass());
  nbCallbacks = 0;
}

JnjvmModuleProvider::~JnjvmModuleProvider() {
  if (mvm::MvmModule::executionEngine) {
    mvm::MvmModule::protectEngine.lock();
    mvm::MvmModule::executionEngine->removeModuleProvider(this);
    mvm::MvmModule::protectEngine.unlock();
  }
  delete TheModule;
}

void JnjvmModuleProvider::printStats() {
  fprintf(stderr, "------------ Info from the module provider -------------\n");
  fprintf(stderr, "Number of callbacks        : %d\n", nbCallbacks);
}
