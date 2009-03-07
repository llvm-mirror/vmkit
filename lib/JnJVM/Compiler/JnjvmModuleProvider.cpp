//===--- JnjvmModuleProvider.cpp - LLVM Module Provider for Jnjvm ---------===//
//
//                              Jnjvm
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/Constants.h"
#include "llvm/Module.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"

#include "mvm/JIT.h"

#include "JavaAccess.h"
#include "JavaClass.h"
#include "JavaConstantPool.h"
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
  uint16 index;
  bool stat;

  CallbackInfo(Class* c, uint32 i, bool s) : Annotation(JavaCallback_ID), 
    cl(c), index(i), stat(s) {}
};

static JavaMethod* staticLookup(Function* F) {
  CallbackInfo* CI = (CallbackInfo*)F->getAnnotation(JavaCallback_ID);
  assert(CI && "No callback where there should be one");
  Class* caller = CI->cl;
  uint16 index = CI->index; 
  bool isStatic = CI->stat;
  JavaConstantPool* ctpInfo = caller->getConstantPool();

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



Function* JnjvmModuleJIT::addCallback(Class* cl, uint16 index,
                                      Signdef* sign, bool stat) {
  
  Function* func = 0;
  LLVMSignatureInfo* LSI = getSignatureInfo(sign);
  
  const char* name = cl->printString();
  char* key = (char*)alloca(strlen(name) + 16);
  sprintf(key, "%s%d", name, index);
  Function* F = TheModule->getFunction(key);
  if (F) return F;
  
  const FunctionType* type = stat ? LSI->getStaticType() : 
                                    LSI->getVirtualType();
  
  func = Function::Create(type, GlobalValue::GhostLinkage, key, TheModule);
  
  CallbackInfo* A = new CallbackInfo(cl, index, stat);
  func->addAnnotation(A);
  
  return func;
}


bool JnjvmModuleProvider::materializeFunction(Function *F, 
                                              std::string *ErrInfo) {
  
  if (!(F->hasNotBeenReadFromBitcode())) 
    return false;
 
  if (mvm::MvmModule::executionEngine->getPointerToGlobalIfAvailable(F))
    return false;

  JavaMethod* meth = LLVMMethodInfo::get(F);
  
  if (!meth) {
    // It's a callback
    meth = staticLookup(F);
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

JnjvmModuleProvider::JnjvmModuleProvider(JnjvmModuleJIT *m) {
  TheModule = m->getLLVMModule();
  m->protectEngine.lock();
  m->executionEngine->addModuleProvider(this);
  m->protectEngine.unlock();
}

JnjvmModuleProvider::~JnjvmModuleProvider() {
  mvm::MvmModule::protectEngine.lock();
  mvm::MvmModule::executionEngine->removeModuleProvider(this);
  mvm::MvmModule::protectEngine.unlock();
}
