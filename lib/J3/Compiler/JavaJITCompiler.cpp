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
public:
  virtual void print(void* ip, void* addr);
  
  JavaJITMethodInfo(llvm::GCFunctionInfo* GFI, JavaMethod* m) : 
    mvm::JITMethodInfo(GFI) {
    MetaInfo = m;
    MethodType = 1;
  }
};

void JavaJITMethodInfo::print(void* ip, void* addr) {
  void* new_ip = NULL;
  if (ip) new_ip = isStub(ip, addr);
  JavaMethod* meth = (JavaMethod*)MetaInfo;
  CodeLineInfo* info = meth->lookupCodeLineInfo((uintptr_t)ip);
  fprintf(stderr, "; %p in %s.%s (line %d, bytecode %d, code start %p)", new_ip,
          UTF8Buffer(meth->classDef->name).cString(),
          UTF8Buffer(meth->name).cString(), info->lineNumber,
          info->bytecodeIndex, meth->code);
  if (ip != new_ip) fprintf(stderr, " (from stub)");
  fprintf(stderr, "\n");
}


void JavaJITListener::NotifyFunctionEmitted(const Function &F,
                                     void *Code, size_t Size,
                                     const EmittedFunctionDetails &Details) {

  // The following could be changed to an assert when -load-bc supports
  // the verifier.
  if (F.getParent() != TheCompiler->getLLVMModule()) return;

  Jnjvm* vm = JavaThread::get()->getJVM();
  mvm::BumpPtrAllocator& Alloc = TheCompiler->allocator;
  llvm::GCFunctionInfo* GFI = 0;

  if (F.hasGC()) {
    if (TheCompiler->TheGCStrategy == NULL) {
      assert(mvm::MvmModule::TheGCStrategy != NULL && "No GC strategy");
      TheCompiler->TheGCStrategy = mvm::MvmModule::TheGCStrategy;
      mvm::MvmModule::TheGCStrategy = NULL;
    }
    GCStrategy::iterator I = TheCompiler->TheGCStrategy->end();
    I--;
    while (&(*I)->getFunction() != &F) {
      // This happens when the compilation of a function was post-poned.
      assert(I != TheCompiler->TheGCStrategy->begin() && "No GC info");
      I--;
    }
    assert(&(*I)->getFunction() == &F &&
        "GC Info and method do not correspond");
    GFI = *I;
  }

  JavaMethod* meth = TheCompiler->getJavaMethod(F);
  if (meth == NULL) {
    // This is a stub.
    mvm::MvmJITMethodInfo* MI = new(Alloc, "JITMethodInfo")
      mvm::MvmJITMethodInfo(GFI, &F);
    vm->RuntimeFunctions.addMethodInfo(MI, Code,
                                       (void*)((uintptr_t)Code + Size));
  } else {
    JavaJITMethodInfo* MI = new(Alloc, "JavaJITMethodInfo")
      JavaJITMethodInfo(GFI, meth);
    vm->RuntimeFunctions.addMethodInfo(MI, Code,
                                       (void*)((uintptr_t)Code + Size));
    uint32 infoLength = Details.LineStarts.size();
    meth->codeInfoLength = infoLength;
    if (infoLength > 0) {
      mvm::BumpPtrAllocator& JavaAlloc = meth->classDef->classLoader->allocator;
      CodeLineInfo* infoTable =
        new(JavaAlloc, "CodeLineInfo") CodeLineInfo[infoLength];
      for (uint32 i = 0; i < infoLength; ++i) {
        DebugLoc DL = Details.LineStarts[i].Loc;
        uint32_t first = DL.getLine();
        uint32_t second = DL.getCol();
        assert(second == 0 && "Wrong column number");
        infoTable[i].address = Details.LineStarts[i].Address;
        infoTable[i].lineNumber = meth->codeInfo[first].lineNumber;
        infoTable[i].bytecodeIndex = meth->codeInfo[first].bytecodeIndex;
        infoTable[i].ctpIndex = meth->codeInfo[first].ctpIndex;
        infoTable[i].bytecode = meth->codeInfo[first].bytecode;
      }
      delete[] meth->codeInfo;
      meth->codeInfo = infoTable;
    } else {
      if (meth->codeInfo != NULL) {
        delete[] meth->codeInfo;
        meth->codeInfo = NULL;
      }
    }
  }
}

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
  llvm_gcroot(str, 0);
  fprintf(stderr, "Should not be here\n");
  abort();
}

Constant* JavaJITCompiler::getStringPtr(JavaString** str) {
  assert(str && "No string given");
  const llvm::Type* Ty = PointerType::getUnqual(JavaIntrinsics.JavaObjectType);
  ConstantInt* CI = ConstantInt::get(Type::getInt64Ty(getLLVMContext()),
                                     uint64(str));
  return ConstantExpr::getIntToPtr(CI, Ty);
}

Constant* JavaJITCompiler::getJavaClass(CommonClass* cl) {
  fprintf(stderr, "Should not be here\n");
  abort();
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
  // obj can not encode direclty an object.
  return NULL;
}

Constant* JavaJITCompiler::getFinalObject(JavaObject* obj, CommonClass* cl) {
  llvm_gcroot(obj, 0);
  return NULL;
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

JavaJITCompiler::JavaJITCompiler(const std::string &ModuleID, bool trusted) :
  JavaLLVMCompiler(ModuleID), listener(this) {

  EmitFunctionName = false;

  // Protect IR for the GC.
  mvm::MvmModule::protectIR();
  executionEngine = ExecutionEngine::createJIT(TheModule, 0,
                                               0, llvm::CodeGenOpt::Default, false);
  TheGCStrategy = mvm::MvmModule::TheGCStrategy;
  mvm::MvmModule::TheGCStrategy = NULL;
  mvm::MvmModule::unprotectIR();
 
  executionEngine->RegisterJITEventListener(&listener);

  addJavaPasses(trusted);
}

JavaJITCompiler::~JavaJITCompiler() {
  executionEngine->removeModule(TheModule);
  delete executionEngine;
  // ~JavaLLVMCompiler will delete the module.
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
    // The VT has already been filled by the AOT compiler so there
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

void JavaJITCompiler::setMethod(Function* func, void* ptr, const char* name) {
  func->setLinkage(GlobalValue::ExternalLinkage);
  func->setName(name);
  assert(ptr && "No value given");
  executionEngine->updateGlobalMapping(func, ptr);
}

void* JavaJITCompiler::materializeFunction(JavaMethod* meth) {
  mvm::MvmModule::protectIR();
  Function* func = parseFunction(meth);
  void* res = executionEngine->getPointerToGlobal(func);
  // Now that it's compiled, we don't need the IR anymore
  func->deleteBody();
  mvm::MvmModule::unprotectIR();
  return res;
}

void* JavaJITCompiler::GenerateStub(llvm::Function* F) {
  mvm::MvmModule::protectIR();
  void* res = executionEngine->getPointerToGlobal(F);
  // Now that it's compiled, we don't need the IR anymore
  F->deleteBody();
  mvm::MvmModule::unprotectIR();
  return res;
}

// Helper function to run an executable with a JIT
extern "C" int StartJnjvmWithJIT(int argc, char** argv, char* mainClass) {
  llvm::llvm_shutdown_obj X; 
   
  mvm::MvmModule::initialise();
  mvm::Collector::initialise();
 
  mvm::ThreadAllocator allocator;
  char** newArgv = (char**)allocator.Allocate((argc + 1) * sizeof(char*));
  memcpy(newArgv + 1, argv, argc * sizeof(void*));
  newArgv[0] = newArgv[1];
  newArgv[1] = mainClass;

  JavaJITCompiler* Comp = JavaJITCompiler::CreateCompiler("JITModule");
  JnjvmClassLoader* JCL = mvm::VirtualMachine::initialiseJVM(Comp);
  mvm::VirtualMachine* vm = mvm::VirtualMachine::createJVM(JCL);
  vm->runApplication(argc + 1, newArgv);
  vm->waitForExit();
  
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

JavaJ3LazyJITCompiler::JavaJ3LazyJITCompiler(const std::string& ModuleID,
                                             bool trusted)
    : JavaJITCompiler(ModuleID, trusted) {}


static llvm::cl::opt<bool> LLVMLazy("llvm-lazy", 
                     cl::desc("Use LLVM lazy stubs"),
                     cl::init(false));

JavaJITCompiler* JavaJITCompiler::CreateCompiler(const std::string& ModuleID) {
  // This is called for the first compiler.
  if (LLVMLazy) {
    return new JavaLLVMLazyJITCompiler(ModuleID, true);
  }
  return new JavaJ3LazyJITCompiler(ModuleID, true);
}
