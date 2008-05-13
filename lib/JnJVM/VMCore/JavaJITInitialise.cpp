//===------JavaJITInitialise.cpp - Initialization of LLVM objects ---------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <stddef.h>

#include "llvm/Instructions.h"
#include "llvm/LinkAllPasses.h"
#include "llvm/Module.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Support/MutexGuard.h"


#include "mvm/JIT.h"
#include "mvm/Method.h"

#include "JavaArray.h"
#include "JavaCache.h"
#include "JavaClass.h"
#include "JavaJIT.h"
#include "JavaObject.h"
#include "JavaThread.h"
#include "Jnjvm.h"
#include "JnjvmModuleProvider.h"
#ifdef SERVICE_VM
#include "ServiceDomain.h"
#endif

using namespace jnjvm;
using namespace llvm;

#ifdef WITH_TRACER
const llvm::FunctionType* JavaJIT::markAndTraceLLVMType = 0;
#endif

const llvm::Type* JavaObject::llvmType = 0;
const llvm::Type* JavaArray::llvmType = 0;
const llvm::Type* ArrayUInt8::llvmType = 0;
const llvm::Type* ArraySInt8::llvmType = 0;
const llvm::Type* ArrayUInt16::llvmType = 0;
const llvm::Type* ArraySInt16::llvmType = 0;
const llvm::Type* ArrayUInt32::llvmType = 0;
const llvm::Type* ArraySInt32::llvmType = 0;
const llvm::Type* ArrayFloat::llvmType = 0;
const llvm::Type* ArrayDouble::llvmType = 0;
const llvm::Type* ArrayLong::llvmType = 0;
const llvm::Type* ArrayObject::llvmType = 0;
const llvm::Type* CacheNode::llvmType = 0;
const llvm::Type* Enveloppe::llvmType = 0;

void JavaJIT::initialiseJITIsolateVM(Jnjvm* vm) {
  mvm::jit::protectEngine->lock();
  mvm::jit::executionEngine->addModuleProvider(vm->TheModuleProvider);
  mvm::jit::protectEngine->unlock();
}

void JavaJIT::initialiseJITBootstrapVM(Jnjvm* vm) {
  //llvm::PrintMachineCode = true;
  Module* module = vm->module;
  mvm::jit::protectEngine->lock();
  mvm::jit::executionEngine->addModuleProvider(vm->TheModuleProvider); 
  mvm::jit::protectEngine->unlock();



  mvm::jit::protectTypes();//->lock();
  // Create JavaObject::llvmType
  const llvm::Type* Pty = mvm::jit::ptrType;
  VTType = PointerType::getUnqual(PointerType::getUnqual(Type::Int32Ty));
  
  std::vector<const llvm::Type*> objectFields;
  objectFields.push_back(VTType); // VT
  objectFields.push_back(Pty); // Class
  objectFields.push_back(Pty); // Lock
  JavaObject::llvmType = 
    llvm::PointerType::getUnqual(llvm::StructType::get(objectFields, false));

  // Create JavaArray::llvmType
  {
  std::vector<const llvm::Type*> arrayFields;
  arrayFields.push_back(JavaObject::llvmType->getContainedType(0));
  arrayFields.push_back(llvm::Type::Int32Ty);
  JavaArray::llvmType =
    llvm::PointerType::getUnqual(llvm::StructType::get(arrayFields, false));
  }

#define ARRAY_TYPE(name, type)                                            \
  {                                                                       \
  std::vector<const Type*> arrayFields;                                   \
  arrayFields.push_back(JavaObject::llvmType->getContainedType(0));       \
  arrayFields.push_back(Type::Int32Ty);                                   \
  arrayFields.push_back(ArrayType::get(type, 0));                         \
  name::llvmType = PointerType::getUnqual(StructType::get(arrayFields, false)); \
  }

  ARRAY_TYPE(ArrayUInt8, Type::Int8Ty);
  ARRAY_TYPE(ArraySInt8, Type::Int8Ty);
  ARRAY_TYPE(ArrayUInt16, Type::Int16Ty);
  ARRAY_TYPE(ArraySInt16, Type::Int16Ty);
  ARRAY_TYPE(ArrayUInt32, Type::Int32Ty);
  ARRAY_TYPE(ArraySInt32, Type::Int32Ty);
  ARRAY_TYPE(ArrayLong, Type::Int64Ty);
  ARRAY_TYPE(ArrayDouble, Type::DoubleTy);
  ARRAY_TYPE(ArrayFloat, Type::FloatTy);
  ARRAY_TYPE(ArrayObject, JavaObject::llvmType);

#undef ARRAY_TYPE

  // Create CacheNode::llvmType
  {
  std::vector<const llvm::Type*> arrayFields;
  arrayFields.push_back(mvm::jit::ptrType); // VT
  arrayFields.push_back(mvm::jit::ptrType); // methPtr
  arrayFields.push_back(mvm::jit::ptrType); // lastCible
  arrayFields.push_back(mvm::jit::ptrType); // next
  arrayFields.push_back(mvm::jit::ptrType); // enveloppe
  CacheNode::llvmType =
    PointerType::getUnqual(StructType::get(arrayFields, false));
  }
  
  // Create Enveloppe::llvmType
  {
  std::vector<const llvm::Type*> arrayFields;
  arrayFields.push_back(mvm::jit::ptrType); // VT
  arrayFields.push_back(CacheNode::llvmType); // firstCache
  arrayFields.push_back(mvm::jit::ptrType); // ctpInfo
  arrayFields.push_back(mvm::jit::ptrType); // cacheLock
  arrayFields.push_back(Type::Int32Ty); // index
  Enveloppe::llvmType =
    PointerType::getUnqual(StructType::get(arrayFields, false));
  }

  
  // Create javaObjectTracerLLVM
  {
  std::vector<const Type*> args;
  args.push_back(JavaObject::llvmType);
#ifdef MULTIPLE_GC
  args.push_back(mvm::jit::ptrType);
#endif
  const FunctionType* type = FunctionType::get(Type::VoidTy, args, false);
  javaObjectTracerLLVM = Function::Create(type,
                                      GlobalValue::ExternalLinkage,
#ifdef MULTIPLE_GC
                                      "_ZN5jnjvm10JavaObject6tracerEPv",
#else
                                      "_ZN5jnjvm10JavaObject6tracerEv",
#endif
                                      module);
  }
  
  // Create virtualLookupLLVM
  {
  std::vector<const Type*> args;
  //args.push_back(JavaObject::llvmType);
  //args.push_back(mvm::jit::ptrType);
  //args.push_back(llvm::Type::Int32Ty);
  args.push_back(CacheNode::llvmType);
  args.push_back(JavaObject::llvmType);
  const FunctionType* type =
    FunctionType::get(mvm::jit::ptrType, args, false);

  virtualLookupLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "virtualLookup",
                     module);
  }
  
  // Create multiCallNewLLVM
  {
  std::vector<const Type*> args;
  args.push_back(mvm::jit::ptrType);
  args.push_back(Type::Int32Ty);
  const FunctionType* type = FunctionType::get(JavaObject::llvmType, args,
                                               true);

  multiCallNewLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "_ZN5jnjvm9JavaArray12multiCallNewEPNS_10ClassArrayEjz",
                     module);
  }

  // Create initialisationCheckLLVM
  {
  std::vector<const Type*> args;
  args.push_back(mvm::jit::ptrType);
  const FunctionType* type = FunctionType::get(mvm::jit::ptrType, args,
                                               false);

  initialisationCheckLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "initialisationCheck",
                     module);
  PAListPtr func_toto_PAL;
  SmallVector<ParamAttrsWithIndex, 4> Attrs;
  ParamAttrsWithIndex PAWI;
  PAWI.Index = 0; PAWI.Attrs = 0  | ParamAttr::ReadNone;
  Attrs.push_back(PAWI);
  func_toto_PAL = PAListPtr::get(Attrs.begin(), Attrs.end());
  initialisationCheckLLVM->setParamAttrs(func_toto_PAL);
  } 
  // Create forceInitialisationCheckLLVM
  {
  std::vector<const Type*> args;
  args.push_back(mvm::jit::ptrType);
  const FunctionType* type = FunctionType::get(Type::VoidTy, args,
                                               false);

  forceInitialisationCheckLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "forceInitialisationCheck",
                     module);
  } 
  
  
  // Create arrayLengthLLVM
  {
  std::vector<const Type*> args;
  args.push_back(JavaObject::llvmType);
  const FunctionType* type = FunctionType::get(Type::Int32Ty, args, false);

  arrayLengthLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "arrayLength",
                     module);
  PAListPtr func_toto_PAL;
  SmallVector<ParamAttrsWithIndex, 4> Attrs;
  ParamAttrsWithIndex PAWI;
  PAWI.Index = 0; PAWI.Attrs = 0  | ParamAttr::ReadNone;
  Attrs.push_back(PAWI);
  func_toto_PAL = PAListPtr::get(Attrs.begin(), Attrs.end());
  arrayLengthLLVM->setParamAttrs(func_toto_PAL);
  }
  
  // Create getVTLLVM
  {
  std::vector<const Type*> args;
  args.push_back(JavaObject::llvmType);
  const FunctionType* type = FunctionType::get(
    PointerType::getUnqual(PointerType::getUnqual(Type::Int32Ty)),
    args, false);

  getVTLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "getVT",
                     module);
  PAListPtr func_toto_PAL;
  SmallVector<ParamAttrsWithIndex, 4> Attrs;
  ParamAttrsWithIndex PAWI;
  PAWI.Index = 0; PAWI.Attrs = 0  | ParamAttr::ReadNone;
  Attrs.push_back(PAWI);
  func_toto_PAL = PAListPtr::get(Attrs.begin(), Attrs.end());
  getVTLLVM->setParamAttrs(func_toto_PAL);
  }
  
  // Create getClassLLVM
  {
  std::vector<const Type*> args;
  args.push_back(JavaObject::llvmType);
  const FunctionType* type = FunctionType::get(mvm::jit::ptrType, args, false);

  getClassLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "getClass",
                     module);
  PAListPtr func_toto_PAL;
  SmallVector<ParamAttrsWithIndex, 4> Attrs;
  ParamAttrsWithIndex PAWI;
  PAWI.Index = 0; PAWI.Attrs = 0  | ParamAttr::ReadNone;
  Attrs.push_back(PAWI);
  func_toto_PAL = PAListPtr::get(Attrs.begin(), Attrs.end());
  getClassLLVM->setParamAttrs(func_toto_PAL);
  }
  
  // Create newLookupLLVM
  {
  std::vector<const Type*> args;
  args.push_back(mvm::jit::ptrType);
  args.push_back(Type::Int32Ty);
  args.push_back(PointerType::getUnqual(mvm::jit::ptrType));
  args.push_back(Type::Int32Ty);
  const FunctionType* type = FunctionType::get(mvm::jit::ptrType, args,
                                               false);

  newLookupLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "newLookup",
                     module);
  PAListPtr func_toto_PAL;
  SmallVector<ParamAttrsWithIndex, 4> Attrs;
  ParamAttrsWithIndex PAWI;
  PAWI.Index = 0; PAWI.Attrs = 0  | ParamAttr::ReadNone;
  Attrs.push_back(PAWI);
  func_toto_PAL = PAListPtr::get(Attrs.begin(), Attrs.end());
  newLookupLLVM->setParamAttrs(func_toto_PAL);
  }
 
#ifdef MULTIPLE_GC
  // Create getCollectorLLVM
  {
  std::vector<const Type*> args;
  args.push_back(mvm::jit::ptrType);
  const FunctionType* type = FunctionType::get(mvm::jit::ptrType, args,
                                               false);

  getCollectorLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "getCollector",
                     module);
  PAListPtr func_toto_PAL;
  SmallVector<ParamAttrsWithIndex, 4> Attrs;
  ParamAttrsWithIndex PAWI;
  PAWI.Index = 0; PAWI.Attrs = 0  | ParamAttr::ReadNone;
  Attrs.push_back(PAWI);
  func_toto_PAL = PAListPtr::get(Attrs.begin(), Attrs.end());
  getCollectorLLVM->setParamAttrs(func_toto_PAL);
  }
#endif
  
  // Create getVTFromClassLLVM
  {
  std::vector<const Type*> args;
  args.push_back(mvm::jit::ptrType);
  const FunctionType* type = FunctionType::get(VTType, args,
                                               false);

  getVTFromClassLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "getVTFromClass",
                     module);
  PAListPtr func_toto_PAL;
  SmallVector<ParamAttrsWithIndex, 4> Attrs;
  ParamAttrsWithIndex PAWI;
  PAWI.Index = 0; PAWI.Attrs = 0  | ParamAttr::ReadNone;
  Attrs.push_back(PAWI);
  func_toto_PAL = PAListPtr::get(Attrs.begin(), Attrs.end());
  getVTFromClassLLVM->setParamAttrs(func_toto_PAL);
  }
  
  // Create getSizeFromClassLLVM
  {
  std::vector<const Type*> args;
  args.push_back(mvm::jit::ptrType);
  const FunctionType* type = FunctionType::get(Type::Int32Ty, args,
                                               false);

  getObjectSizeFromClassLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "getObjectSizeFromClass",
                     module);
  PAListPtr func_toto_PAL;
  SmallVector<ParamAttrsWithIndex, 4> Attrs;
  ParamAttrsWithIndex PAWI;
  PAWI.Index = 0; PAWI.Attrs = 0  | ParamAttr::ReadNone;
  Attrs.push_back(PAWI);
  func_toto_PAL = PAListPtr::get(Attrs.begin(), Attrs.end());
  getObjectSizeFromClassLLVM->setParamAttrs(func_toto_PAL);
  }
 
#ifndef WITHOUT_VTABLE
  // Create vtableLookupLLVM
  {
  std::vector<const Type*> args;
  args.push_back(JavaObject::llvmType); // obj
  args.push_back(mvm::jit::ptrType); // cl
  args.push_back(Type::Int32Ty); // index
  args.push_back(PointerType::getUnqual(Type::Int32Ty)); // vtable index
  const FunctionType* type = FunctionType::get(Type::Int32Ty, args, false);

  vtableLookupLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                                      "vtableLookup", module);
  PAListPtr func_toto_PAL;
  SmallVector<ParamAttrsWithIndex, 4> Attrs;
  ParamAttrsWithIndex PAWI;
  PAWI.Index = 0; PAWI.Attrs = 0  | ParamAttr::ReadNone;
  Attrs.push_back(PAWI);
  func_toto_PAL = PAListPtr::get(Attrs.begin(), Attrs.end());
  vtableLookupLLVM->setParamAttrs(func_toto_PAL);
  }
#endif

  // Create fieldLookupLLVM
  {
  std::vector<const Type*> args;
  args.push_back(JavaObject::llvmType);
  args.push_back(mvm::jit::ptrType);
  args.push_back(llvm::Type::Int32Ty);
  args.push_back(llvm::Type::Int32Ty);
  args.push_back(PointerType::getUnqual(mvm::jit::ptrType));
  args.push_back(PointerType::getUnqual(llvm::Type::Int32Ty));
  const FunctionType* type =
    FunctionType::get(mvm::jit::ptrType, args,
                      false);

  fieldLookupLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "fieldLookup",
                     module);
  PAListPtr func_toto_PAL;
  SmallVector<ParamAttrsWithIndex, 4> Attrs;
  ParamAttrsWithIndex PAWI;
  PAWI.Index = 0; PAWI.Attrs = 0  | ParamAttr::ReadNone;
  Attrs.push_back(PAWI);
  func_toto_PAL = PAListPtr::get(Attrs.begin(), Attrs.end());
  fieldLookupLLVM->setParamAttrs(func_toto_PAL);
  }
  
  // Create nullPointerExceptionLLVM
  {
  std::vector<const Type*> args;
  const FunctionType* type = FunctionType::get(Type::VoidTy, args, false);

  nullPointerExceptionLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "nullPointerException",
                     module);
  }
  
  // Create classCastExceptionLLVM
  {
  std::vector<const Type*> args;
  args.push_back(JavaObject::llvmType);
  args.push_back(mvm::jit::ptrType);
  const FunctionType* type = FunctionType::get(Type::VoidTy, args, false);

  classCastExceptionLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "classCastException",
                     module);
  }
  
  // Create indexOutOfBoundsExceptionLLVM
  {
  std::vector<const Type*> args;
  args.push_back(JavaObject::llvmType);
  args.push_back(Type::Int32Ty);
  const FunctionType* type = FunctionType::get(Type::VoidTy, args, false);

  indexOutOfBoundsExceptionLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "indexOutOfBoundsException",
                     module);
  }
  {
  std::vector<const Type*> args;
  args.push_back(Type::Int32Ty);
  const FunctionType* type = FunctionType::get(Type::VoidTy, args, false);


  negativeArraySizeExceptionLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "negativeArraySizeException",
                     module);
  
  outOfMemoryErrorLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "outOfMemoryError",
                     module);
  }
  
  {
  std::vector<const Type*> args;
  args.push_back(Type::Int32Ty);
  args.push_back(VTType);
#ifdef MULTIPLE_GC
  args.push_back(mvm::jit::ptrType);
#endif
  const FunctionType* type = FunctionType::get(JavaObject::llvmType, args, false);


  javaObjectAllocateLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "gcmalloc",
                     module);
  }
  
  // Create proceedPendingExceptionLLVM
  {
  std::vector<const Type*> args;
  const FunctionType* type = FunctionType::get(Type::VoidTy, args, false);

  jniProceedPendingExceptionLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "jniProceedPendingException",
                     module);
  }
  
  // Create printExecutionLLVM
  {
  std::vector<const Type*> args;
  args.push_back(Type::Int32Ty);
  args.push_back(Type::Int32Ty);
  args.push_back(Type::Int32Ty);
  const FunctionType* type = FunctionType::get(Type::VoidTy, args, false);

  printExecutionLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "printExecution",
                     module);
  }
  
  // Create printMethodStartLLVM
  {
  std::vector<const Type*> args;
  args.push_back(Type::Int32Ty);
  const FunctionType* type = FunctionType::get(Type::VoidTy, args, false);

  printMethodStartLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "printMethodStart",
                     module);
  }
  
  // Create printMethodEndLLVM
  {
  std::vector<const Type*> args;
  args.push_back(Type::Int32Ty);
  const FunctionType* type = FunctionType::get(Type::VoidTy, args, false);

  printMethodEndLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "printMethodEnd",
                     module);
  }
    
  // Create throwExceptionLLVM
  {
  std::vector<const Type*> args;
  args.push_back(JavaObject::llvmType);
  const FunctionType* type = FunctionType::get(Type::VoidTy, args, false);

  throwExceptionLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "_ZN5jnjvm10JavaThread14throwExceptionEPNS_10JavaObjectE",
                     module);
  }
  
  // Create clearExceptionLLVM
  {
  std::vector<const Type*> args;
  const FunctionType* type = FunctionType::get(Type::VoidTy, args, false);

  clearExceptionLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "_ZN5jnjvm10JavaThread14clearExceptionEv",
                     module);
  }
  
  
  
  // Create getExceptionLLVM
  {
  std::vector<const Type*> args;
  const FunctionType* type = FunctionType::get(mvm::jit::ptrType, 
                                               args, false);

  getExceptionLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "_ZN5jnjvm10JavaThread12getExceptionEv",
                     module);
  }
  
  // Create getJavaExceptionLLVM
  {
  std::vector<const Type*> args;
  const FunctionType* type = FunctionType::get(JavaObject::llvmType, 
                                               args, false);

  getJavaExceptionLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "_ZN5jnjvm10JavaThread16getJavaExceptionEv",
                     module);
  }
  
  // Create compareExceptionLLVM
  {
  std::vector<const Type*> args;
  args.push_back(mvm::jit::ptrType);
  const FunctionType* type = FunctionType::get(Type::Int1Ty, args, false);

  compareExceptionLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "_ZN5jnjvm10JavaThread16compareExceptionEPNS_5ClassE",
                     module);
  }
  
  // Create getStaticInstanceLLVM
  {
  std::vector<const Type*> args;
  args.push_back(mvm::jit::ptrType); // cl
  args.push_back(mvm::jit::ptrType); // vm
  const FunctionType* type = FunctionType::get(JavaObject::llvmType, args, false);

  getStaticInstanceLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "getStaticInstance",
                     module);
  PAListPtr func_toto_PAL;
  SmallVector<ParamAttrsWithIndex, 4> Attrs;
  ParamAttrsWithIndex PAWI;
  PAWI.Index = 0; PAWI.Attrs = 0  | ParamAttr::ReadNone;
  Attrs.push_back(PAWI);
  func_toto_PAL = PAListPtr::get(Attrs.begin(), Attrs.end());
  getStaticInstanceLLVM->setParamAttrs(func_toto_PAL);
  }
  
  // Create getClassDelegateeLLVM
  {
  std::vector<const Type*> args;
  args.push_back(mvm::jit::ptrType);
  const FunctionType* type = FunctionType::get(JavaObject::llvmType, args, false);

  getClassDelegateeLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "getClassDelegatee",
                     module);
  PAListPtr func_toto_PAL;
  SmallVector<ParamAttrsWithIndex, 4> Attrs;
  ParamAttrsWithIndex PAWI;
  PAWI.Index = 0; PAWI.Attrs = 0  | ParamAttr::ReadNone;
  Attrs.push_back(PAWI);
  func_toto_PAL = PAListPtr::get(Attrs.begin(), Attrs.end());
  getClassDelegateeLLVM->setParamAttrs(func_toto_PAL);
  }
  
  // Create instanceOfLLVM
  {
  std::vector<const Type*> args;
  args.push_back(JavaObject::llvmType);
  args.push_back(mvm::jit::ptrType);
  const FunctionType* type = FunctionType::get(Type::Int32Ty, args, false);

  instanceOfLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "_ZN5jnjvm10JavaObject10instanceOfEPNS_11CommonClassE",
                     module);
  PAListPtr func_toto_PAL;
  SmallVector<ParamAttrsWithIndex, 4> Attrs;
  ParamAttrsWithIndex PAWI;
  PAWI.Index = 0; PAWI.Attrs = 0  | ParamAttr::ReadNone;
  Attrs.push_back(PAWI);
  func_toto_PAL = PAListPtr::get(Attrs.begin(), Attrs.end());
  instanceOfLLVM->setParamAttrs(func_toto_PAL);
  }
  
  // Create aquireObjectLLVM
  {
  std::vector<const Type*> args;
  args.push_back(JavaObject::llvmType);
  const FunctionType* type = FunctionType::get(Type::VoidTy, args, false);

  aquireObjectLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "_ZN5jnjvm10JavaObject6aquireEv",
                     module);
#ifdef SERVICE_VM
  aquireObjectInSharedDomainLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "aquireObjectInSharedDomain",
                     module);
#endif
  }
  
  // Create releaseObjectLLVM
  {
  std::vector<const Type*> args;
  args.push_back(JavaObject::llvmType);
  const FunctionType* type = FunctionType::get(Type::VoidTy, args, false);

  releaseObjectLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "_ZN5jnjvm10JavaObject6unlockEv",
                     module);
#ifdef SERVICE_VM
  releaseObjectInSharedDomainLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "releaseObjectInSharedDomain",
                     module);
#endif
  }
  
  
  {
    std::vector<const Type*> args;
    args.push_back(ArrayUInt16::llvmType);
    FunctionType* FuncTy = FunctionType::get(
      /*Result=*/JavaObject::llvmType,
      /*Params=*/args,
      /*isVarArg=*/false);
    runtimeUTF8ToStrLLVM = Function::Create(FuncTy, GlobalValue::ExternalLinkage,
      "runtimeUTF8ToStr", module);
  }

   
  // Create getSJLJBufferLLVM
  {
    std::vector<const Type*> args;
    const FunctionType* type = FunctionType::get(mvm::jit::ptrType, args,
                                               false);

    getSJLJBufferLLVM = Function::Create(type, GlobalValue::ExternalLinkage,
                     "getSJLJBuffer",
                     module);
    
  }

#ifdef WITH_TRACER
  // Create markAndTraceLLVM
  {
  std::vector<const Type*> args;
  args.push_back(JavaObject::llvmType);
#ifdef MULTIPLE_GC
  args.push_back(mvm::jit::ptrType);
#endif
  markAndTraceLLVMType = FunctionType::get(llvm::Type::VoidTy, args, false);
  markAndTraceLLVM = Function::Create(markAndTraceLLVMType,
                                  GlobalValue::ExternalLinkage,
#ifdef MULTIPLE_GC
                                  "_ZNK2gc12markAndTraceEP9Collector",
#else
                                  "_ZNK2gc12markAndTraceEv",
#endif
                                  module);
  }
#endif
#ifdef SERVICE_VM
  // Create serviceCallStart/Stop
  {
  std::vector<const Type*> args;
  args.push_back(mvm::jit::ptrType);
  args.push_back(mvm::jit::ptrType);
  const FunctionType* type = FunctionType::get(llvm::Type::VoidTy, args, false);
  ServiceDomain::serviceCallStartLLVM = 
    Function::Create(type, GlobalValue::ExternalLinkage, "serviceCallStart",
                     module);
  
  ServiceDomain::serviceCallStopLLVM = 
    Function::Create(type, GlobalValue::ExternalLinkage, "serviceCallStop",
                     module);
  }
#endif
  mvm::jit::unprotectTypes();//->unlock();
  mvm::jit::protectConstants();//->lock();
  constantUTF8Null = Constant::getNullValue(ArrayUInt16::llvmType); 
  constantJavaObjectNull = Constant::getNullValue(JavaObject::llvmType);
  constantMaxArraySize = ConstantInt::get(Type::Int32Ty,
                                          JavaArray::MaxArraySize);
  constantJavaObjectSize = ConstantInt::get(Type::Int32Ty, sizeof(JavaObject));
  
  
  Constant* cons = 
    ConstantExpr::getIntToPtr(ConstantInt::get(Type::Int64Ty,
                              uint64_t (JavaObject::VT)), VTType);
      
  JavaObjectVT = new GlobalVariable(VTType, true,
                                    GlobalValue::ExternalLinkage,
                                    cons, "",
                                    module);
  
  cons = 
    ConstantExpr::getIntToPtr(ConstantInt::get(Type::Int64Ty,
                              uint64_t (ArrayObject::VT)), VTType);
  
  ArrayObjectVT = new GlobalVariable(VTType, true,
                                    GlobalValue::ExternalLinkage,
                                    cons, "",
                                    module);
  {
    Class fake;
    int offset = (intptr_t)&(fake.virtualSize) - (intptr_t)&fake;
    constantOffsetObjectSizeInClass = ConstantInt::get(Type::Int32Ty, offset);
    offset = (intptr_t)&(fake.virtualVT) - (intptr_t)&fake;
    constantOffsetVTInClass = ConstantInt::get(Type::Int32Ty, offset);
  }

  mvm::jit::unprotectConstants();//->unlock();
}

llvm::Constant*    JavaJIT::constantJavaObjectNull;
llvm::Constant*    JavaJIT::constantUTF8Null;
llvm::Constant*    JavaJIT::constantMaxArraySize;
llvm::Constant*    JavaJIT::constantJavaObjectSize;
llvm::GlobalVariable*    JavaJIT::JavaObjectVT;
llvm::GlobalVariable*    JavaJIT::ArrayObjectVT;
llvm::ConstantInt* JavaJIT::constantOffsetObjectSizeInClass;
llvm::ConstantInt* JavaJIT::constantOffsetVTInClass;

namespace mvm {

llvm::FunctionPass* createEscapeAnalysisPass(llvm::Function*, llvm::Function*);
llvm::FunctionPass* createLowerConstantCallsPass();
//llvm::FunctionPass* createArrayChecksPass();

}

static void addPass(FunctionPassManager *PM, Pass *P) {
  // Add the pass to the pass manager...
  PM->add(P);
}

void AddStandardCompilePasses(FunctionPassManager *PM) {
  llvm::MutexGuard locked(mvm::jit::executionEngine->lock);
  // LLVM does not allow calling functions from other modules in verifier
  //PM->add(llvm::createVerifierPass());                  // Verify that input is correct
  
  addPass(PM, llvm::createCFGSimplificationPass());    // Clean up disgusting code
  addPass(PM, llvm::createPromoteMemoryToRegisterPass());// Kill useless allocas
  addPass(PM, llvm::createInstructionCombiningPass()); // Clean up after IPCP & DAE
  addPass(PM, llvm::createCFGSimplificationPass());    // Clean up after IPCP & DAE
  
  addPass(PM, createTailDuplicationPass());      // Simplify cfg by copying code
  addPass(PM, createSimplifyLibCallsPass());     // Library Call Optimizations
  addPass(PM, createInstructionCombiningPass()); // Cleanup for scalarrepl.
  addPass(PM, createJumpThreadingPass());        // Thread jumps.
  addPass(PM, createCFGSimplificationPass());    // Merge & remove BBs
  addPass(PM, createScalarReplAggregatesPass()); // Break up aggregate allocas
  addPass(PM, createInstructionCombiningPass()); // Combine silly seq's
  addPass(PM, createCondPropagationPass());      // Propagate conditionals
  
  addPass(PM, createTailCallEliminationPass());  // Eliminate tail calls
  addPass(PM, createCFGSimplificationPass());    // Merge & remove BBs
  addPass(PM, createReassociatePass());          // Reassociate expressions
  addPass(PM, createLoopRotatePass());
  addPass(PM, createLICMPass());                 // Hoist loop invariants
  addPass(PM, createLoopUnswitchPass());         // Unswitch loops.
  addPass(PM, createLoopIndexSplitPass());       // Index split loops.
  addPass(PM, createInstructionCombiningPass()); // Clean up after LICM/reassoc
  addPass(PM, createIndVarSimplifyPass());       // Canonicalize indvars
  addPass(PM, createLoopDeletionPass());         // Delete dead loops
  addPass(PM, createLoopUnrollPass());           // Unroll small loops
  addPass(PM, createInstructionCombiningPass()); // Clean up after the unroller
  addPass(PM, createGVNPass());                  // Remove redundancies
  addPass(PM, createMemCpyOptPass());            // Remove memcpy / form memset
  addPass(PM, createSCCPPass());                 // Constant prop with SCCP
  
  addPass(PM, mvm::createLowerConstantCallsPass());

  // Run instcombine after redundancy elimination to exploit opportunities
  // opened up by them.
  addPass(PM, createInstructionCombiningPass());
  addPass(PM, createCondPropagationPass());      // Propagate conditionals

  addPass(PM, createDeadStoreEliminationPass()); // Delete dead stores
  addPass(PM, createAggressiveDCEPass());        // SSA based 'Aggressive DCE'
  addPass(PM, createCFGSimplificationPass());    // Merge & remove BBs

}

