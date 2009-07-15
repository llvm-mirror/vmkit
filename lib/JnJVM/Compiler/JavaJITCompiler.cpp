//===--------- JavaJITCompiler.cpp - Support for JIT compiling -------------===//
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
#include "llvm/Support/ManagedStatic.h"

#include "MvmGC.h"
#include "mvm/VirtualMachine.h"

#include "JavaClass.h"
#include "JavaConstantPool.h"
#include "JavaThread.h"

#include "jnjvm/JnjvmModule.h"
#include "jnjvm/JnjvmModuleProvider.h"

using namespace jnjvm;
using namespace llvm;

Constant* JavaJITCompiler::getNativeClass(CommonClass* classDef) {
  LLVMContext& Context = getLLVMModule()->getContext();
  const llvm::Type* Ty = classDef->isClass() ? JnjvmModule::JavaClassType :
                                               JnjvmModule::JavaCommonClassType;
  
  ConstantInt* CI = Context.getConstantInt(Type::Int64Ty, uint64_t(classDef));
  return ConstantExpr::getIntToPtr(CI, Ty);
}

Constant* JavaJITCompiler::getConstantPool(JavaConstantPool* ctp) {
  void* ptr = ctp->ctpRes;
  assert(ptr && "No constant pool found");
  LLVMContext& Context = getLLVMModule()->getContext();
  ConstantInt* CI = Context.getConstantInt(Type::Int64Ty, uint64_t(ptr));
  return ConstantExpr::getIntToPtr(CI, JnjvmModule::ConstantPoolType);
}

Constant* JavaJITCompiler::getMethodInClass(JavaMethod* meth) {
  LLVMContext& Context = getLLVMModule()->getContext();
  ConstantInt* CI = Context.getConstantInt(Type::Int64Ty, (int64_t)meth);
  return ConstantExpr::getIntToPtr(CI, JnjvmModule::JavaMethodType);
}

Constant* JavaJITCompiler::getString(JavaString* str) {
  assert(str && "No string given");
  LLVMContext& Context = getLLVMModule()->getContext();
  ConstantInt* CI = Context.getConstantInt(Type::Int64Ty, uint64(str));
  return ConstantExpr::getIntToPtr(CI, JnjvmModule::JavaObjectType);
}

Constant* JavaJITCompiler::getEnveloppe(Enveloppe* enveloppe) {
  assert(enveloppe && "No enveloppe given");
  LLVMContext& Context = getLLVMModule()->getContext();
  ConstantInt* CI = Context.getConstantInt(Type::Int64Ty, uint64(enveloppe));
  return ConstantExpr::getIntToPtr(CI, JnjvmModule::EnveloppeType);
}

Constant* JavaJITCompiler::getJavaClass(CommonClass* cl) {
  JavaObject* obj = cl->getClassDelegatee(JavaThread::get()->getJVM());
  assert(obj && "Delegatee not created");
  LLVMContext& Context = getLLVMModule()->getContext();
  Constant* CI = Context.getConstantInt(Type::Int64Ty, uint64(obj));
  return ConstantExpr::getIntToPtr(CI, JnjvmModule::JavaObjectType);
}

JavaObject* JavaJITCompiler::getFinalObject(llvm::Value* obj) {
  if (ConstantExpr* CE = dyn_cast<ConstantExpr>(obj)) {
    if (ConstantInt* C = dyn_cast<ConstantInt>(CE->getOperand(0))) {
      return (JavaObject*)C->getZExtValue();
    }
  }
  return 0;
}

Constant* JavaJITCompiler::getFinalObject(JavaObject* obj) {
  LLVMContext& Context = getLLVMModule()->getContext();
  Constant* CI = Context.getConstantInt(Type::Int64Ty, uint64(obj));
  return ConstantExpr::getIntToPtr(CI, JnjvmModule::JavaObjectType);
}

Constant* JavaJITCompiler::getStaticInstance(Class* classDef) {
  LLVMContext& Context = getLLVMModule()->getContext();
#ifdef ISOLATE
  assert(0 && "Should not be here");
  abort();
#endif
  void* obj = classDef->getStaticInstance();
  if (!obj) {
    classDef->acquire();
    obj = classDef->getStaticInstance();
    if (!obj) {
      // Allocate now so that compiled code can reference it.
      obj = classDef->allocateStaticInstance(JavaThread::get()->getJVM());
    }
    classDef->release();
  }
  Constant* CI = Context.getConstantInt(Type::Int64Ty, (uint64_t(obj)));
  return ConstantExpr::getIntToPtr(CI, JnjvmModule::ptrType);
}

Constant* JavaJITCompiler::getVirtualTable(JavaVirtualTable* VT) {
  LLVMContext& Context = getLLVMModule()->getContext();
  if (VT->cl->isClass()) {
    LLVMClassInfo* LCI = getClassInfo(VT->cl->asClass());
    LCI->getVirtualType();
  }
  
  ConstantInt* CI = Context.getConstantInt(Type::Int64Ty, uint64_t(VT));
  return ConstantExpr::getIntToPtr(CI, JnjvmModule::VTType);
}

Constant* JavaJITCompiler::getNativeFunction(JavaMethod* meth, void* ptr) {
  LLVMContext& Context = getLLVMModule()->getContext();
  LLVMSignatureInfo* LSI = getSignatureInfo(meth->getSignature());
  const llvm::Type* valPtrType = LSI->getNativePtrType();
  
  assert(ptr && "No native function given");

  Constant* CI = Context.getConstantInt(Type::Int64Ty, uint64_t(ptr));
  return ConstantExpr::getIntToPtr(CI, valPtrType);
}

JavaJITCompiler::JavaJITCompiler(const std::string &ModuleID) :
  JavaLLVMCompiler(ModuleID) {

#if DEBUG
  EmitFunctionName = true;
#else
  EmitFunctionName = false;
#endif
  TheModuleProvider = new JnjvmModuleProvider(TheModule);
  addJavaPasses();
}

#ifdef SERVICE
Value* JavaJITCompiler::getIsolate(Jnjvm* isolate, Value* Where) {
  LLVMContext& Context = getLLVMModule()->getContext();
  ConstantInt* CI = Context.getConstantInt(Type::Int64Ty, uint64_t(isolate));
  return ConstantExpr::getIntToPtr(CI, JnjvmModule::ptrType);
}
#endif

void JavaJITCompiler::makeVT(Class* cl) { 
  JavaVirtualTable* VT = cl->virtualVT; 
  assert(VT && "No VT was allocated!");
    
  if (VT->init) {
    // The VT hash already been filled by the AOT compiler so there
    // is nothing left to do!
    return;
  }
  
  if (cl->super) {
    // Copy the super VT into the current VT.
    uint32 size = cl->super->virtualTableSize - 
        JavaVirtualTable::getFirstJavaMethodIndex();
    memcpy(VT->getFirstJavaMethod(), cl->super->virtualVT->getFirstJavaMethod(),
           size * sizeof(uintptr_t));
    VT->destructor = cl->super->virtualVT->destructor;
  }


  // Fill the virtual table with function pointers.
  ExecutionEngine* EE = mvm::MvmModule::executionEngine;
  for (uint32 i = 0; i < cl->nbVirtualMethods; ++i) {
    JavaMethod& meth = cl->virtualMethods[i];
    LLVMMethodInfo* LMI = getMethodInfo(&meth);
    Function* func = LMI->getMethod();

    // Special handling for finalize method. Don't put a finalizer
    // if there is none, or if it is empty.
    if (meth.offset == 0) {
      if (!cl->super) {
        meth.canBeInlined = true;
      } else {
        VT->destructor = (uintptr_t)EE->getPointerToFunctionOrStub(func);
      }
    } else {
      VT->getFunctions()[meth.offset] = 
        (uintptr_t)EE->getPointerToFunctionOrStub(func);
    }
  }

}

void JavaJITCompiler::setMethod(JavaMethod* meth, void* ptr, const char* name) {
  Function* func = getMethodInfo(meth)->getMethod();
  func->setName(name);
  assert(ptr && "No value given");
  JnjvmModule::executionEngine->addGlobalMapping(func, ptr);
  func->setLinkage(GlobalValue::ExternalLinkage);
}

void* JavaJITCompiler::materializeFunction(JavaMethod* meth) {
  Function* func = parseFunction(meth);
 
  mvm::MvmModule::protectIR();
  void* res = mvm::MvmModule::executionEngine->getPointerToGlobal(func);
  func->deleteBody();
  mvm::MvmModule::unprotectIR();

  return res;
}

// Helper function to run an executable with a JIT
extern "C" int StartJnjvmWithJIT(int argc, char** argv, char* mainClass) {
  llvm::llvm_shutdown_obj X;  
   
  mvm::MvmModule::initialise();
  mvm::Collector::initialise();
 
  char** newArgv = new char*[argc + 1];
  memcpy(newArgv + 1, argv, argc * sizeof(void*));
  newArgv[0] = newArgv[1];
  newArgv[1] = mainClass;

  JavaJITCompiler* Comp = new JavaJITCompiler("JITModule");
  mvm::MvmModule::AddStandardCompilePasses();
  JnjvmClassLoader* JCL = mvm::VirtualMachine::initialiseJVM(Comp);
  mvm::VirtualMachine* vm = mvm::VirtualMachine::createJVM(JCL);
  vm->runApplication(argc + 1, newArgv);
  vm->waitForExit();
  
  delete[] newArgv;
  return 0;
}
