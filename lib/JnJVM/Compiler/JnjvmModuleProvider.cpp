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


static JavaMethod* staticLookup(CallbackInfo& F) {
  Class* caller = F.cl;
  uint16 index = F.index; 
  bool isStatic = F.stat;
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


Value* JavaJITCompiler::addCallback(Class* cl, uint16 index,
                                    Signdef* sign, bool stat) {
  
  Function* F = 0;
  LLVMSignatureInfo* LSI = getSignatureInfo(sign);
  
  const UTF8* name = cl->name;
  char* key = (char*)alloca(name->size + 16);
  for (sint32 i = 0; i < name->size; ++i)
    key[i] = name->elements[i];
  sprintf(key + name->size, "%d", index);
  F = TheModule->getFunction(key);
  if (F) return F;
  
  const FunctionType* type = stat ? LSI->getStaticType() : 
                                    LSI->getVirtualType();
  
  F = Function::Create(type, GlobalValue::GhostLinkage, key, TheModule);
  
  CallbackInfo A(cl, index, stat);
  callbacks.insert(std::make_pair(F, A));
  
  return F;
}


bool JnjvmModuleProvider::materializeFunction(Function *F, 
                                              std::string *ErrInfo) {
  
  // The caller of materializeFunction *must* always hold the JIT lock.
  // Because we are materializing a function here, we must release the
  // JIT lock and get the global vmkit lock to be thread-safe.
  // This prevents jitting the function while someone else is doing it.
  mvm::MvmModule::executionEngine->lock.release(); 
  mvm::MvmModule::protectIR();

  // Don't use hasNotBeenReadFromBitcode: materializeFunction is called
  // by the pass manager, and we don't want another thread to JIT the
  // function while all passes have not been run.
  if (!(F->isDeclaration())) {
    mvm::MvmModule::unprotectIR(); 
    // Reacquire and go back to the JIT function.
    mvm::MvmModule::executionEngine->lock.acquire();
    return false;
  }

  if (mvm::MvmModule::executionEngine->getPointerToGlobalIfAvailable(F)) {
    mvm::MvmModule::unprotectIR(); 
    // Reacquire and go back to the JIT function.
    mvm::MvmModule::executionEngine->lock.acquire();
    return false;
  }
  

  JavaMethod* meth = Comp->getJavaMethod(F);
  
  if (!meth) {
    // It's a callback
    JavaJITCompiler::callback_iterator I = Comp->callbacks.find(F);
    assert(I != Comp->callbacks.end() && "No callbacks found");
    meth = staticLookup(I->second);
  }
  
  void* val = meth->compiledPtr();

  if (isVirtual(meth->access)) {
    LLVMMethodInfo* LMI = JavaLLVMCompiler::getMethodInfo(meth);
    uint64_t offset = dyn_cast<ConstantInt>(LMI->getOffset())->getZExtValue();
    assert(meth->classDef->isResolved() && "Class not resolved");
#if !defined(ISOLATE_SHARING) && !defined(SERVICE)
    assert(meth->classDef->isInitializing() && "Class not ready");
#endif
    assert(meth->classDef->virtualVT && "Class has no VT");
    assert(meth->classDef->virtualTableSize > offset && 
        "The method's offset is greater than the virtual table size");
    ((void**)meth->classDef->virtualVT)[offset] = val;
  } else {
    assert(meth->classDef->isInitializing() && "Class not ready");
  }

  mvm::MvmModule::unprotectIR();
  
  // Reacquire to go back to the JIT function.
  mvm::MvmModule::executionEngine->lock.acquire();
  
  if (F->isDeclaration())
    mvm::MvmModule::executionEngine->updateGlobalMapping(F, val);
  
  return false;
}

JnjvmModuleProvider::JnjvmModuleProvider(llvm::Module* m,
                                         JavaJITCompiler* C) {
  Comp = C;
  TheModule = m;
  JnjvmModule::protectEngine.lock();
  JnjvmModule::executionEngine->addModuleProvider(this);
  JnjvmModule::protectEngine.unlock();
}

JnjvmModuleProvider::~JnjvmModuleProvider() {
  JnjvmModule::protectEngine.lock();
  JnjvmModule::executionEngine->removeModuleProvider(this);
  JnjvmModule::protectEngine.unlock();
}
