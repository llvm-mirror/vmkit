//===-------- LLVMMaterializer.cpp - LLVM Materializer for J3 -------------===//
//
//                            The VMKit project
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

#include "j3/J3Intrinsics.h"
#include "j3/LLVMMaterializer.h"

using namespace llvm;
using namespace j3;


JavaLLVMLazyJITCompiler::JavaLLVMLazyJITCompiler(const std::string& ModuleID)
    : JavaJITCompiler(ModuleID) {
  TheMaterializer = new LLVMMaterializer(TheModule, this);
  executionEngine->DisableLazyCompilation(false);   
}

JavaLLVMLazyJITCompiler::~JavaLLVMLazyJITCompiler() {
  // The module already destroys the materializer.
}

void* JavaLLVMLazyJITCompiler::loadMethod(void* handle, const char* symbol) {
  Function* F = mvm::MvmModule::globalModule->getFunction(symbol);
  if (F) {
    void* res = executionEngine->getPointerToFunctionOrStub(F);
    return res;
  }

  return JavaCompiler::loadMethod(handle, symbol);
}

uintptr_t JavaLLVMLazyJITCompiler::getPointerOrStub(JavaMethod& meth,
                                                    int side) {
  LLVMMethodInfo* LMI = getMethodInfo(&meth);
  Function* func = LMI->getMethod();
  return (uintptr_t)executionEngine->getPointerToFunctionOrStub(func);
}

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


Value* JavaLLVMLazyJITCompiler::addCallback(Class* cl, uint16 index,
                                            Signdef* sign, bool stat,
                                            BasicBlock* insert) {
  
  Function* F = 0;
  LLVMSignatureInfo* LSI = getSignatureInfo(sign);
  mvm::ThreadAllocator allocator;
  
  const UTF8* name = cl->name;
  char* key = (char*)allocator.Allocate(name->size + 16);
  for (sint32 i = 0; i < name->size; ++i)
    key[i] = name->elements[i];
  sprintf(key + name->size, "%d", index);
  F = TheModule->getFunction(key);
  if (F) return F;
  
  const FunctionType* type = stat ? LSI->getStaticType() : 
                                    LSI->getVirtualType();
  
  F = Function::Create(type, GlobalValue::ExternalWeakLinkage, key, TheModule);
  
  CallbackInfo A(cl, index, stat);
  callbacks.insert(std::make_pair(F, A));
  
  return F;
}


bool LLVMMaterializer::Materialize(GlobalValue *GV, std::string *ErrInfo) {
  
  Function* F = dyn_cast<Function>(GV);
  assert(F && "Not a function");
  if (F->getLinkage() == GlobalValue::ExternalLinkage) return false;

  // The caller of materializeFunction *must* always hold the JIT lock.
  // Because we are materializing a function here, we must release the
  // JIT lock and get the global vmkit lock to be thread-safe.
  // This prevents jitting the function while someone else is doing it.
  Comp->executionEngine->lock.release(); 
  mvm::MvmModule::protectIR();

  // Don't use hasNotBeenReadFromBitcode: materializeFunction is called
  // by the pass manager, and we don't want another thread to JIT the
  // function while all passes have not been run.
  if (!(F->isDeclaration())) {
    mvm::MvmModule::unprotectIR();
    // TODO: Is this still valid?
    // Reacquire and go back to the JIT function.
    // mvm::MvmModule::executionEngine->lock.acquire();
    return false;
  }

  if (Comp->executionEngine->getPointerToGlobalIfAvailable(F)) {
    mvm::MvmModule::unprotectIR(); 
    // TODO: Is this still valid?
    // Reacquire and go back to the JIT function.
    // mvm::MvmModule::executionEngine->lock.acquire();
    return false;
  }
  
  JavaMethod* meth = Comp->getJavaMethod(*F);
  
  if (!meth) {
    // It's a callback
    JavaLLVMLazyJITCompiler::callback_iterator I = Comp->callbacks.find(F);
    assert(I != Comp->callbacks.end() && "No callbacks found");
    meth = staticLookup(I->second);
  }
  
  void* val = meth->compiledPtr();

  if (isVirtual(meth->access)) {
    LLVMMethodInfo* LMI = Comp->getMethodInfo(meth);
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
  
  // TODO: Is this still valid?
  // Reacquire to go back to the JIT function.
  // mvm::MvmModule::executionEngine->lock.acquire();
  
  if (F->isDeclaration())
    Comp->executionEngine->updateGlobalMapping(F, val);
  
  return false;
}

bool LLVMMaterializer::isMaterializable(const llvm::GlobalValue* GV) const {
  return GV->getLinkage() == GlobalValue::ExternalWeakLinkage;
}


LLVMMaterializer::LLVMMaterializer(
    llvm::Module* m, JavaLLVMLazyJITCompiler* C) {
  Comp = C;
  TheModule = m;
  m->setMaterializer(this);
}
