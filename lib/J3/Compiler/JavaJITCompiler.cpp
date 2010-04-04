//===--------- JavaJITCompiler.cpp - Support for JIT compiling -------------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/Constants.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Function.h"
#include "llvm/Instructions.h"
#include "llvm/LLVMContext.h"
#include "llvm/Module.h"
#include "llvm/Analysis/DebugInfo.h"
#include "llvm/CodeGen/GCStrategy.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/JITEventListener.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

#include "MvmGC.h"
#include "mvm/VirtualMachine.h"

#include "JavaClass.h"
#include "JavaConstantPool.h"
#include "JavaThread.h"
#include "JavaTypes.h"
#include "Jnjvm.h"

#include "j3/JavaJITCompiler.h"
#include "j3/J3Intrinsics.h"
#include "j3/LLVMMaterializer.h"

using namespace j3;
using namespace llvm;


class JavaJITMethodInfo : public mvm::JITMethodInfo {
protected:
  JavaMethod* meth;
public:
  virtual void print(void* ip, void* addr);
  
  JavaJITMethodInfo(llvm::GCFunctionInfo* GFI, JavaMethod* m) : 
    mvm::JITMethodInfo(GFI) {
    meth = m;
    MethodType = 1;
  }
  
  virtual void* getMetaInfo() {
    return meth;
  }

};

void JavaJITMethodInfo::print(void* ip, void* addr) {
  void* new_ip = NULL;
  if (ip) new_ip = isStub(ip, addr);
  uint16 line = meth->lookupLineNumber((uintptr_t)ip);
  fprintf(stderr, "; %p in %s.%s (line %d)", new_ip,
          UTF8Buffer(meth->classDef->name).cString(),
          UTF8Buffer(meth->name).cString(), line);
  if (ip != new_ip) fprintf(stderr, " (from stub)");
  fprintf(stderr, "\n");
}


class JavaJITListener : public llvm::JITEventListener {
  JavaMethod* currentCompiledMethod;
  llvm::Function* currentCompiledFunction;
public:
  virtual void NotifyFunctionEmitted(const Function &F,
                                     void *Code, size_t Size,
                                     const EmittedFunctionDetails &Details) {
    if (currentCompiledMethod && currentCompiledFunction == &F) {
      Jnjvm* vm = JavaThread::get()->getJVM();
      mvm::BumpPtrAllocator& Alloc = 
        currentCompiledMethod->classDef->classLoader->allocator;
      llvm::GCFunctionInfo* GFI = 0;
      if (F.hasGC()) {
        GCStrategy::iterator I = mvm::MvmModule::TheGCStrategy->end();
        I--;
        // TODO: see why this is not true in a non- LLVM lazy mode.
        while (&(*I)->getFunction() != &F) {
          assert(I != mvm::MvmModule::TheGCStrategy->begin() && "No GC info");
          I--;
        }
        assert(&(*I)->getFunction() == &F &&
           "GC Info and method do not correspond");
        GFI = *I;
      }
      JavaJITMethodInfo* MI = new(Alloc, "JavaJITMethodInfo")
        JavaJITMethodInfo(GFI, currentCompiledMethod);
      vm->RuntimeFunctions.addMethodInfo(MI, Code,
                                         (void*)((uintptr_t)Code + Size));
      uint32 infoLength = Details.LineStarts.size();
      currentCompiledMethod->codeInfoLength = infoLength;
      if (infoLength) {
        CodeLineInfo* infoTable =
          new(Alloc, "CodeLineInfo") CodeLineInfo[infoLength];
        for (uint32 i = 0; i < infoLength; ++i) {
          DebugLoc DL = Details.LineStarts[i].Loc;
          uint32_t first = DL.getLine();
          uint32_t second = DL.getCol();
          assert(second == 0 && "Wrong column number");
          infoTable[i].address = Details.LineStarts[i].Address;
          infoTable[i].lineNumber =
            currentCompiledMethod->codeInfo[first].lineNumber;
          infoTable[i].bytecodeIndex =
            currentCompiledMethod->codeInfo[first].bytecodeIndex;
          infoTable[i].ctpIndex =
            currentCompiledMethod->codeInfo[first].ctpIndex;
        }
        delete[] currentCompiledMethod->codeInfo;
        currentCompiledMethod->codeInfo = infoTable;
      } else {
        if (currentCompiledMethod->codeInfo) {
          delete[] currentCompiledMethod->codeInfo;
        }
      }
    }
  }

  void setCurrentCompiledMethod(JavaMethod* meth, llvm::Function* func) {
    assert(currentCompiledMethod == NULL && "Recursive compilation detected");
    currentCompiledMethod = meth;
    currentCompiledFunction = func;
  }
  
  void clearCurrentCompiledMethod() {
    currentCompiledMethod = NULL;
    currentCompiledFunction = NULL;
  }

  JavaJITListener() {
    currentCompiledMethod = NULL;
    currentCompiledFunction = NULL;
  }
};

static JavaJITListener* JITListener = 0;

Constant* JavaJITCompiler::getNativeClass(CommonClass* classDef) {
  const llvm::Type* Ty = classDef->isClass() ? JavaIntrinsics.JavaClassType :
                                               JavaIntrinsics.JavaCommonClassType;
  
  ConstantInt* CI = ConstantInt::get(Type::getInt64Ty(getLLVMContext()),
                                     uint64_t(classDef));
  return ConstantExpr::getIntToPtr(CI, Ty);
}

Constant* JavaJITCompiler::getConstantPool(JavaConstantPool* ctp) {
  void* ptr = ctp->ctpRes;
  assert(ptr && "No constant pool found");
  ConstantInt* CI = ConstantInt::get(Type::getInt64Ty(getLLVMContext()),
                                     uint64_t(ptr));
  return ConstantExpr::getIntToPtr(CI, JavaIntrinsics.ConstantPoolType);
}

Constant* JavaJITCompiler::getMethodInClass(JavaMethod* meth) {
  ConstantInt* CI = ConstantInt::get(Type::getInt64Ty(getLLVMContext()),
                                     (int64_t)meth);
  return ConstantExpr::getIntToPtr(CI, JavaIntrinsics.JavaMethodType);
}

Constant* JavaJITCompiler::getString(JavaString* str) {
  assert(str && "No string given");
  ConstantInt* CI = ConstantInt::get(Type::getInt64Ty(getLLVMContext()),
                                     uint64(str));
  return ConstantExpr::getIntToPtr(CI, JavaIntrinsics.JavaObjectType);
}

Constant* JavaJITCompiler::getStringPtr(JavaString** str) {
  assert(str && "No string given");
  const llvm::Type* Ty = PointerType::getUnqual(JavaIntrinsics.JavaObjectType);
  ConstantInt* CI = ConstantInt::get(Type::getInt64Ty(getLLVMContext()),
                                     uint64(str));
  return ConstantExpr::getIntToPtr(CI, Ty);
}

Constant* JavaJITCompiler::getJavaClass(CommonClass* cl) {
  JavaObject* obj = cl->getClassDelegatee(JavaThread::get()->getJVM());
  assert(obj && "Delegatee not created");
  Constant* CI = ConstantInt::get(Type::getInt64Ty(getLLVMContext()),
                                  uint64(obj));
  return ConstantExpr::getIntToPtr(CI, JavaIntrinsics.JavaObjectType);
}

Constant* JavaJITCompiler::getJavaClassPtr(CommonClass* cl) {
  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaObject* const* obj = cl->getClassDelegateePtr(vm);
  assert(obj && "Delegatee not created");
  Constant* CI = ConstantInt::get(Type::getInt64Ty(getLLVMContext()),
                                  uint64(obj));
  const Type* Ty = PointerType::getUnqual(JavaIntrinsics.JavaObjectType);
  return ConstantExpr::getIntToPtr(CI, Ty);
}

JavaObject* JavaJITCompiler::getFinalObject(llvm::Value* obj) {
  if (ConstantExpr* CE = dyn_cast<ConstantExpr>(obj)) {
    if (ConstantInt* C = dyn_cast<ConstantInt>(CE->getOperand(0))) {
      return (JavaObject*)C->getZExtValue();
    }
  }
  return 0;
}

Constant* JavaJITCompiler::getFinalObject(JavaObject* obj, CommonClass* cl) {
  Constant* CI = ConstantInt::get(Type::getInt64Ty(getLLVMContext()),
                                  uint64(obj));
  return ConstantExpr::getIntToPtr(CI, JavaIntrinsics.JavaObjectType);
}

Constant* JavaJITCompiler::getStaticInstance(Class* classDef) {
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
  Constant* CI = ConstantInt::get(Type::getInt64Ty(getLLVMContext()),
                                  (uint64_t(obj)));
  return ConstantExpr::getIntToPtr(CI, JavaIntrinsics.ptrType);
}

Constant* JavaJITCompiler::getVirtualTable(JavaVirtualTable* VT) {
  if (VT->cl->isClass()) {
    LLVMClassInfo* LCI = getClassInfo(VT->cl->asClass());
    LCI->getVirtualType();
  }
  
  ConstantInt* CI = ConstantInt::get(Type::getInt64Ty(getLLVMContext()),
                                     uint64_t(VT));
  return ConstantExpr::getIntToPtr(CI, JavaIntrinsics.VTType);
}

Constant* JavaJITCompiler::getNativeFunction(JavaMethod* meth, void* ptr) {
  LLVMSignatureInfo* LSI = getSignatureInfo(meth->getSignature());
  const llvm::Type* valPtrType = LSI->getNativePtrType();
  
  assert(ptr && "No native function given");

  Constant* CI = ConstantInt::get(Type::getInt64Ty(getLLVMContext()),
                                  uint64_t(ptr));
  return ConstantExpr::getIntToPtr(CI, valPtrType);
}

JavaJITCompiler::JavaJITCompiler(const std::string &ModuleID) :
  JavaLLVMCompiler(ModuleID) {

#if DEBUG
  EmitFunctionName = true;
#else
  EmitFunctionName = false;
#endif
  
  mvm::MvmModule::protectEngine.lock();
  mvm::MvmModule::executionEngine->addModule(TheModule);
  mvm::MvmModule::protectEngine.unlock();
  
  addJavaPasses();

  if (!JITListener) {
    JITListener = new JavaJITListener();
    mvm::MvmModule::executionEngine->RegisterJITEventListener(JITListener);
  }
}

#ifdef SERVICE
Value* JavaJITCompiler::getIsolate(Jnjvm* isolate, Value* Where) {
  ConstantInt* CI = ConstantInt::get(Type::getInt64Ty(getLLVMContext()),
                                     uint64_t(isolate));
  return ConstantExpr::getIntToPtr(CI, JavaIntrinsics.ptrType);
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
  for (uint32 i = 0; i < cl->nbVirtualMethods; ++i) {
    JavaMethod& meth = cl->virtualMethods[i];

    // Special handling for finalize method. Don't put a finalizer
    // if there is none, or if it is empty.
    if (meth.offset == 0) {
      if (!cl->super) {
        meth.canBeInlined = true;
      } else {
        VT->destructor = getPointerOrStub(meth, JavaMethod::Virtual);
      }
    } else {
      VT->getFunctions()[meth.offset] = getPointerOrStub(meth,
                                                         JavaMethod::Virtual);
    }
  }

}

extern "C" void ThrowUnfoundInterface() {
  abort();
}

void JavaJITCompiler::makeIMT(Class* cl) {
  InterfaceMethodTable* IMT = cl->virtualVT->IMT;
  if (!IMT) return;
 
  std::set<JavaMethod*> contents[InterfaceMethodTable::NumIndexes];
  cl->fillIMT(contents);
  
  for (uint32_t i = 0; i < InterfaceMethodTable::NumIndexes; ++i) {
    std::set<JavaMethod*>& atIndex = contents[i];
    uint32_t size = atIndex.size();
    if (size == 1) {
      JavaMethod* Imeth = *(atIndex.begin());
      JavaMethod* meth = cl->lookupMethodDontThrow(Imeth->name,
                                                   Imeth->type,
                                                   false, true, 0);
      if (meth) {
        IMT->contents[i] = getPointerOrStub(*meth, JavaMethod::Interface);
      } else {
        IMT->contents[i] = (uintptr_t)ThrowUnfoundInterface;
      }
    } else if (size > 1) {
      std::vector<JavaMethod*> methods;
      bool SameMethod = true;
      JavaMethod* OldMethod = 0;
      
      for (std::set<JavaMethod*>::iterator it = atIndex.begin(),
           et = atIndex.end(); it != et; ++it) {
        JavaMethod* Imeth = *it;
        JavaMethod* Cmeth = cl->lookupMethodDontThrow(Imeth->name, Imeth->type,
                                                      false, true, 0);
       
        if (OldMethod && OldMethod != Cmeth) SameMethod = false;
        else OldMethod = Cmeth;
       
        if (Cmeth) methods.push_back(Cmeth);
      }

      if (SameMethod) {
        if (methods[0]) {
          IMT->contents[i] = getPointerOrStub(*(methods[0]),
                                              JavaMethod::Interface);
        } else {
          IMT->contents[i] = (uintptr_t)ThrowUnfoundInterface;
        }
      } else {

        uint32_t length = 2 * size * sizeof(uintptr_t);
      
        uintptr_t* table = (uintptr_t*)
          cl->classLoader->allocator.Allocate(length, "IMT");
      
        IMT->contents[i] = (uintptr_t)table | 1;

        int j = 0;
        std::set<JavaMethod*>::iterator Interf = atIndex.begin();
        for (std::vector<JavaMethod*>::iterator it = methods.begin(),
             et = methods.end(); it != et; ++it, j += 2, ++Interf) {
          JavaMethod* Imeth = *Interf;
          JavaMethod* Cmeth = *it;
         
          table[j] = (uintptr_t)Imeth;
          if (Cmeth) {
             table[j + 1] = getPointerOrStub(*Cmeth, JavaMethod::Interface);
          } else {
            table[j + 1] = (uintptr_t)ThrowUnfoundInterface;
          }
        }
      }
    }
  }
}

void JavaJITCompiler::setMethod(JavaMethod* meth, void* ptr, const char* name) {
  Function* func = getMethodInfo(meth)->getMethod();
  func->setName(name);
  assert(ptr && "No value given");
  mvm::MvmModule::executionEngine->updateGlobalMapping(func, ptr);
  func->setLinkage(GlobalValue::ExternalLinkage);
}

void* JavaJITCompiler::materializeFunction(JavaMethod* meth) {
  mvm::MvmModule::protectIR();
  Function* func = parseFunction(meth);
  JITListener->setCurrentCompiledMethod(meth, func);
  void* res = mvm::MvmModule::executionEngine->getPointerToGlobal(func);
  JITListener->clearCurrentCompiledMethod();
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

  JavaJITCompiler* Comp = JavaJITCompiler::CreateCompiler("JITModule");
  JnjvmClassLoader* JCL = mvm::VirtualMachine::initialiseJVM(Comp);
  mvm::VirtualMachine* vm = mvm::VirtualMachine::createJVM(JCL);
  vm->runApplication(argc + 1, newArgv);
  vm->waitForExit();
  
  delete[] newArgv;
  return 0;
}

uintptr_t JavaJ3LazyJITCompiler::getPointerOrStub(JavaMethod& meth,
                                                  int side) {
  return meth.getSignature()->getVirtualCallStub();
}

Value* JavaJ3LazyJITCompiler::addCallback(Class* cl, uint16 index,
                                          Signdef* sign, bool stat,
                                          llvm::BasicBlock* insert) {
  LLVMSignatureInfo* LSI = getSignatureInfo(sign);
  // Set the stub in the constant pool.
  JavaConstantPool* ctpInfo = cl->ctpInfo;
  uintptr_t stub = stat ? sign->getStaticCallStub() : sign->getSpecialCallStub();
  if (!ctpInfo->ctpRes[index]) {
    // Do a compare and swap, so that we do not overwrtie what a stub might
    // have just updated.
    uintptr_t val = (uintptr_t)
      __sync_val_compare_and_swap(&(ctpInfo->ctpRes[index]), NULL, stub);
    // If there is something in the the constant pool that is not NULL nor
    // the stub, then it's the method.
    if (val != 0 && val != stub) {
      return ConstantExpr::getIntToPtr(
          ConstantInt::get(Type::getInt64Ty(insert->getContext()), val),
          stat ? LSI->getStaticPtrType() : LSI->getVirtualPtrType());
    }
  }
  // Load the constant pool.
  Value* CTP = getConstantPool(ctpInfo);
  Value* Index = ConstantInt::get(Type::getInt32Ty(insert->getContext()),
                                  index);
  Value* func = GetElementPtrInst::Create(CTP, Index, "", insert);
  func = new LoadInst(func, "", false, insert);
  // Bitcast it to the LLVM function.
  func = new BitCastInst(func, stat ? LSI->getStaticPtrType() :
                                      LSI->getVirtualPtrType(),
                         "", insert);
  return func;
}

bool JavaJ3LazyJITCompiler::needsCallback(JavaMethod* meth, bool* needsInit) {
  *needsInit = true;
  return (meth == NULL || meth->code == NULL);
}

JavaJ3LazyJITCompiler::JavaJ3LazyJITCompiler(const std::string& ModuleID)
    : JavaJITCompiler(ModuleID) {}


static llvm::cl::opt<bool> LLVMLazy("llvm-lazy", 
                     cl::desc("Use LLVM lazy stubs"),
                     cl::init(false));

JavaJITCompiler* JavaJITCompiler::CreateCompiler(const std::string& ModuleID) {
  if (LLVMLazy) {
    mvm::MvmModule::executionEngine->DisableLazyCompilation(false); 
    return new JavaLLVMLazyJITCompiler(ModuleID);
  }
  return new JavaJ3LazyJITCompiler(ModuleID);
}
