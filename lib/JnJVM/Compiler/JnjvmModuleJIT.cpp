//===--------- JnjvmModuleJIT.cpp - Support for JIT compiling -------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/Constants.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Function.h"
#include "llvm/Instructions.h"
#include "llvm/Module.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"

#include "JavaConstantPool.h"
#include "JavaThread.h"

#include "jnjvm/JnjvmModule.h"
#include "jnjvm/JnjvmModuleProvider.h"

using namespace jnjvm;
using namespace llvm;

extern void* JavaArrayVT[];
extern void* ArrayObjectVT[];

Constant* JnjvmModuleJIT::getNativeClass(CommonClass* classDef) {
  const llvm::Type* Ty = classDef->isClass() ? JavaClassType : 
                                               JavaCommonClassType;
  
  ConstantInt* CI = ConstantInt::get(Type::Int64Ty, uint64_t(classDef));
  return ConstantExpr::getIntToPtr(CI, Ty);
}

Constant* JnjvmModuleJIT::getConstantPool(JavaConstantPool* ctp) {
  void* ptr = ctp->ctpRes;
  assert(ptr && "No constant pool found");
  ConstantInt* CI = ConstantInt::get(Type::Int64Ty, uint64_t(ptr));
  return ConstantExpr::getIntToPtr(CI, ConstantPoolType);
}

Constant* JnjvmModuleJIT::getMethodInClass(JavaMethod* meth) {
  ConstantInt* CI = ConstantInt::get(Type::Int64Ty, (int64_t)meth);
  return ConstantExpr::getIntToPtr(CI, JavaMethodType);
}

Constant* JnjvmModuleJIT::getString(JavaString* str) {
  assert(str && "No string given");
  ConstantInt* CI = ConstantInt::get(Type::Int64Ty, uint64(str));
  return ConstantExpr::getIntToPtr(CI, JavaObjectType);
}

Constant* JnjvmModuleJIT::getEnveloppe(Enveloppe* enveloppe) {
  assert(enveloppe && "No enveloppe given");
  ConstantInt* CI = ConstantInt::get(Type::Int64Ty, uint64(enveloppe));
  return ConstantExpr::getIntToPtr(CI, EnveloppeType);
}

Constant* JnjvmModuleJIT::getJavaClass(CommonClass* cl) {
  JavaObject* obj = cl->getClassDelegatee(JavaThread::get()->getJVM());
  assert(obj && "Delegatee not created");
  Constant* CI = ConstantInt::get(Type::Int64Ty, uint64(obj));
  return ConstantExpr::getIntToPtr(CI, JavaObjectType);
}

JavaObject* JnjvmModuleJIT::getFinalObject(llvm::Value* obj) {
  if (ConstantExpr* CE = dyn_cast<ConstantExpr>(obj)) {
    if (ConstantInt* C = dyn_cast<ConstantInt>(CE->getOperand(0))) {
      return (JavaObject*)C->getZExtValue();
    }
  }
  return 0;
}

Constant* JnjvmModuleJIT::getFinalObject(JavaObject* obj) {
  Constant* CI = ConstantInt::get(Type::Int64Ty, uint64(obj));
  return ConstantExpr::getIntToPtr(CI, JavaObjectType);
}

Constant* JnjvmModuleJIT::getStaticInstance(Class* classDef) {
#ifdef ISOLATE
  assert(0 && "Should not be here");
  abort();
#endif
  void* obj = ((Class*)classDef)->getStaticInstance();
  if (!obj) {
    Class* cl = (Class*)classDef;
    classDef->acquire();
    obj = cl->getStaticInstance();
    if (!obj) {
      // Allocate now so that compiled code can reference it.
      obj = cl->allocateStaticInstance(JavaThread::get()->getJVM());
    }
    classDef->release();
  }
  Constant* CI = ConstantInt::get(Type::Int64Ty, (uint64_t(obj)));
  return ConstantExpr::getIntToPtr(CI, ptrType);
}

Constant* JnjvmModuleJIT::getVirtualTable(Class* classDef) {
  LLVMClassInfo* LCI = getClassInfo((Class*)classDef);
  LCI->getVirtualType();
  
  assert(classDef->virtualVT && "Virtual VT not created");
  void* ptr = classDef->virtualVT;
  ConstantInt* CI = ConstantInt::get(Type::Int64Ty, uint64_t(ptr));
  return ConstantExpr::getIntToPtr(CI, VTType);
}

Constant* JnjvmModuleJIT::getNativeFunction(JavaMethod* meth, void* ptr) {
  LLVMSignatureInfo* LSI = getSignatureInfo(meth->getSignature());
  const llvm::Type* valPtrType = LSI->getNativePtrType();
  
  assert(ptr && "No native function given");

  Constant* CI = ConstantInt::get(Type::Int64Ty, uint64_t(ptr));
  return ConstantExpr::getIntToPtr(CI, valPtrType);
}

JnjvmModuleJIT::JnjvmModuleJIT(const std::string &ModuleID) :
  JnjvmModule(ModuleID) {
   
  ConstantInt* CI = ConstantInt::get(Type::Int64Ty, uint64(JavaArrayVT));
  PrimitiveArrayVT = ConstantExpr::getIntToPtr(CI, VTType);
 
  CI = ConstantInt::get(Type::Int64Ty, uint64(ArrayObjectVT));
  ReferenceArrayVT = ConstantExpr::getIntToPtr(CI, VTType);

  TheModuleProvider = new JnjvmModuleProvider(this);
  addJavaPasses();
}

#ifdef SERVICE
Value* JnjvmModuleJIT::getIsolate(Jnjvm* isolate, Value* Where) {
  ConstantInt* CI = ConstantInt::get(Type::Int64Ty, uint64_t(isolate));
  return ConstantExpr::getIntToPtr(CI, ptrType);
}
#endif

void JnjvmModuleJIT::makeVT(Class* cl) { 
  internalMakeVT(cl);

#ifndef WITHOUT_VTABLE
  VirtualTable* VT = cl->virtualVT;
  
  // Fill the virtual table with function pointers.
  ExecutionEngine* EE = mvm::MvmModule::executionEngine;
  for (uint32 i = 0; i < cl->nbVirtualMethods; ++i) {
    JavaMethod& meth = cl->virtualMethods[i];
    LLVMMethodInfo* LMI = getMethodInfo(&meth);
    Function* func = LMI->getMethod();

    // Special handling for finalize method. Don't put a finalizer
    // if there is none, or if it is empty.
    if (meth.offset == 0) {
#if defined(ISOLATE_SHARING) || defined(USE_GC_BOEHM)
      ((void**)VT)[0] = 0;
#else
      JnjvmClassLoader* loader = cl->classLoader;
      Function* func = loader->getModule()->parseFunction(&meth);
      if (!cl->super) {
        meth.canBeInlined = true;
        ((void**)VT)[0] = 0;
      } else {
        Function::iterator BB = func->begin();
        BasicBlock::iterator I = BB->begin();
        if (isa<ReturnInst>(I)) {
          ((void**)VT)[0] = 0;
        } else {
          // LLVM does not allow recursive compilation. Create the code now.
          ((void**)VT)[0] = EE->getPointerToFunction(func);
        }
      }
#endif
    } else {
      ((void**)VT)[meth.offset] = EE->getPointerToFunctionOrStub(func);
    }
  }

#ifdef WITH_TRACER
  Function* func = makeTracer(cl, false);
  
  void* codePtr = mvm::MvmModule::executionEngine->getPointerToFunction(func);
  ((void**)VT)[VT_TRACER_OFFSET] = codePtr;
  func->deleteBody();
#endif
    
  // If there is no super, then it's the first VT that we allocate. Assign
  // this VT to native types.
  if (!(cl->super)) {
    ClassArray::initialiseVT(cl);
  }
#endif 
}

void JnjvmModuleJIT::setMethod(JavaMethod* meth, void* ptr, const char* name) {
  Function* func = getMethodInfo(meth)->getMethod();
  func->setName(name);
  assert(ptr && "No value given");
  executionEngine->addGlobalMapping(func, ptr);
  func->setLinkage(GlobalValue::ExternalLinkage);
}

void* JnjvmModuleJIT::materializeFunction(JavaMethod* meth) {
  Function* func = parseFunction(meth);
  
  void* res = mvm::MvmModule::executionEngine->getPointerToGlobal(func);
  func->deleteBody();

  return res;
}
