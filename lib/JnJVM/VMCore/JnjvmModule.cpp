//===--------- JnjvmModule.cpp - Definition of a Jnjvm module -------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/CallingConv.h"
#include "llvm/ParameterAttributes.h"
#include "llvm/Support/MutexGuard.h"


#include "mvm/JIT.h"

#include "JavaJIT.h"
#include "JavaTypes.h"
#include "Jnjvm.h"
#include "JnjvmModule.h"
#include "JnjvmModuleProvider.h"
#include "LockedMap.h"


using namespace jnjvm;
using namespace llvm;


#ifdef WITH_TRACER
const llvm::FunctionType* JnjvmModule::MarkAndTraceType = 0;
#endif

const llvm::Type* JnjvmModule::JavaObjectType = 0;
const llvm::Type* JnjvmModule::JavaArrayType = 0;
const llvm::Type* JnjvmModule::JavaArrayUInt8Type = 0;
const llvm::Type* JnjvmModule::JavaArraySInt8Type = 0;
const llvm::Type* JnjvmModule::JavaArrayUInt16Type = 0;
const llvm::Type* JnjvmModule::JavaArraySInt16Type = 0;
const llvm::Type* JnjvmModule::JavaArrayUInt32Type = 0;
const llvm::Type* JnjvmModule::JavaArraySInt32Type = 0;
const llvm::Type* JnjvmModule::JavaArrayFloatType = 0;
const llvm::Type* JnjvmModule::JavaArrayDoubleType = 0;
const llvm::Type* JnjvmModule::JavaArrayLongType = 0;
const llvm::Type* JnjvmModule::JavaArrayObjectType = 0;
const llvm::Type* JnjvmModule::CacheNodeType = 0;
const llvm::Type* JnjvmModule::EnveloppeType = 0;

llvm::Constant*       JnjvmModule::JavaObjectNullConstant;
llvm::Constant*       JnjvmModule::UTF8NullConstant;
llvm::Constant*       JnjvmModule::JavaClassNullConstant;
llvm::Constant*       JnjvmModule::MaxArraySizeConstant;
llvm::Constant*       JnjvmModule::JavaObjectSizeConstant;
llvm::GlobalVariable* JnjvmModule::JavaObjectVirtualTableGV;
llvm::GlobalVariable* JnjvmModule::ArrayObjectVirtualTableGV;
llvm::ConstantInt*    JnjvmModule::OffsetObjectSizeInClassConstant;
llvm::ConstantInt*    JnjvmModule::OffsetVTInClassConstant;
const llvm::Type*     JnjvmModule::JavaClassType;
const llvm::Type*     JnjvmModule::VTType;
llvm::ConstantInt*    JnjvmModule::JavaArrayElementsOffsetConstant;
llvm::ConstantInt*    JnjvmModule::JavaArraySizeOffsetConstant;
llvm::ConstantInt*    JnjvmModule::JavaObjectLockOffsetConstant;
llvm::ConstantInt*    JnjvmModule::JavaObjectClassOffsetConstant;

#ifdef WITH_TRACER
llvm::Function* JnjvmModule::MarkAndTraceFunction = 0;
llvm::Function* JnjvmModule::JavaObjectTracerFunction = 0;
#endif
llvm::Function* JnjvmModule::GetSJLJBufferFunction = 0;
llvm::Function* JnjvmModule::ThrowExceptionFunction = 0;
llvm::Function* JnjvmModule::GetExceptionFunction = 0;
llvm::Function* JnjvmModule::GetJavaExceptionFunction = 0;
llvm::Function* JnjvmModule::ClearExceptionFunction = 0;
llvm::Function* JnjvmModule::CompareExceptionFunction = 0;
llvm::Function* JnjvmModule::NullPointerExceptionFunction = 0;
llvm::Function* JnjvmModule::ClassCastExceptionFunction = 0;
llvm::Function* JnjvmModule::IndexOutOfBoundsExceptionFunction = 0;
llvm::Function* JnjvmModule::NegativeArraySizeExceptionFunction = 0;
llvm::Function* JnjvmModule::OutOfMemoryErrorFunction = 0;
llvm::Function* JnjvmModule::JavaObjectAllocateFunction = 0;
llvm::Function* JnjvmModule::InterfaceLookupFunction = 0;
llvm::Function* JnjvmModule::FieldLookupFunction = 0;
#ifndef WITHOUT_VTABLE
llvm::Function* JnjvmModule::VirtualLookupFunction = 0;
#endif
llvm::Function* JnjvmModule::PrintExecutionFunction = 0;
llvm::Function* JnjvmModule::PrintMethodStartFunction = 0;
llvm::Function* JnjvmModule::PrintMethodEndFunction = 0;
llvm::Function* JnjvmModule::JniProceedPendingExceptionFunction = 0;
llvm::Function* JnjvmModule::InitialisationCheckFunction = 0;
llvm::Function* JnjvmModule::ForceInitialisationCheckFunction = 0;
llvm::Function* JnjvmModule::ClassLookupFunction = 0;
llvm::Function* JnjvmModule::InstanceOfFunction = 0;
llvm::Function* JnjvmModule::AquireObjectFunction = 0;
llvm::Function* JnjvmModule::ReleaseObjectFunction = 0;
llvm::Function* JnjvmModule::MultiCallNewFunction = 0;
llvm::Function* JnjvmModule::RuntimeUTF8ToStrFunction = 0;
llvm::Function* JnjvmModule::GetStaticInstanceFunction = 0;
llvm::Function* JnjvmModule::GetClassDelegateeFunction = 0;
llvm::Function* JnjvmModule::ArrayLengthFunction = 0;
llvm::Function* JnjvmModule::GetVTFunction = 0;
llvm::Function* JnjvmModule::GetClassFunction = 0;
llvm::Function* JnjvmModule::GetVTFromClassFunction = 0;
llvm::Function* JnjvmModule::GetObjectSizeFromClassFunction = 0;

#ifdef MULTIPLE_GC
llvm::Function* JnjvmModule::FetCollectorFunction = 0;
#endif

#ifdef SERVICE_VM
llvm::Function* JnjvmModule::AquireObjectInSharedDomainFunction = 0;
llvm::Function* JnjvmModule::ReleaseObjectInSharedDomainFunction = 0;
llvm::Function* JnjvmModule::ServiceCallStartFunction = 0;
llvm::Function* JnjvmModule::ServiceCallStopFunction = 0;
#endif



Value* LLVMCommonClassInfo::getVar(JavaJIT* jit) {
  if (!varGV) {
#ifdef MULTIPLE_VM
    if (jit->compilingClass->isolate->module == Jnjvm::bootstrapVM->module &&
        isArray && classDef->isolate != Jnjvm::bootstrapVM) {
      // We know the array class can belong to bootstrap
      CommonClass* cl = Jnjvm::bootstrapVM->constructArray(this->name, 0);
      return getVar(cl, jit);
    }
#endif
      
    Constant* cons = 
      ConstantExpr::getIntToPtr(ConstantInt::get(Type::Int64Ty,
                                                 uint64_t (classDef)),
                                JnjvmModule::JavaClassType);
      
    varGV = new GlobalVariable(JnjvmModule::JavaClassType, true,
                               GlobalValue::ExternalLinkage,
                               cons, "",
                               classDef->isolate->TheModule);
  }
  return new LoadInst(varGV, "", jit->currentBlock);
}

Value* LLVMCommonClassInfo::getDelegatee(JavaJIT* jit) {
#ifndef MULTIPLE_VM
  if (!delegateeGV) {
    JavaObject* obj = classDef->getClassDelegatee();
    Constant* cons = 
      ConstantExpr::getIntToPtr(ConstantInt::get(Type::Int64Ty, uint64(obj)),
                                JnjvmModule::JavaObjectType);
    delegateeGV = new GlobalVariable(JnjvmModule::JavaObjectType, true,
                                    GlobalValue::ExternalLinkage,
                                    cons, "",
                                    classDef->isolate->TheModule);
  }
  return new LoadInst(delegateeGV, "", jit->currentBlock);
#else
  Value* ld = llvmVar(jit);
  return llvm::CallInst::Create(JnjvmModule::getClassDelegateeLLVM, ld, "",
                                jit->currentBlock);
#endif
}

#ifndef WITHOUT_VTABLE
VirtualTable* JnjvmModule::allocateVT(Class* cl,
                                      CommonClass::method_iterator meths) {
  if (meths == cl->virtualMethods.end()) {
    uint64 size = cl->virtualTableSize;
    VirtualTable* VT = (VirtualTable*)malloc(size * sizeof(void*));
    if (cl->super) {
      Class* super = (Class*)cl->super;
      memcpy(VT, super->virtualVT, cl->super->virtualTableSize * sizeof(void*));
    } else {
      memcpy(VT, JavaObject::VT, VT_SIZE);
    }
    return VT;
  } else {
    JavaMethod* meth = meths->second;
    JavaMethod* parent = cl->super? 
      cl->super->lookupMethodDontThrow(meth->name, meth->type, false, true) : 0;

    uint64_t offset = 0;
    if (!parent) {
      offset = cl->virtualTableSize++;
      meth->offset = offset;
    } else {
      offset = parent->offset;
      meth->offset = parent->offset;
    }
    VirtualTable* VT = allocateVT(cl, ++meths);
    LLVMMethodInfo* LMI = getMethodInfo(meth);
    Function* func = LMI->getMethod();
    ExecutionEngine* EE = mvm::jit::executionEngine;
    ((void**)VT)[offset] = EE->getPointerToFunctionOrStub(func);
    return VT;
  }
}
#endif


VirtualTable* JnjvmModule::makeVT(Class* cl, bool stat) {
  
  VirtualTable* res = 0;
#ifndef WITHOUT_VTABLE
  if (stat) {
#endif
    res = (VirtualTable*)malloc(VT_SIZE);
    memcpy(res, JavaObject::VT, VT_SIZE);
#ifndef WITHOUT_VTABLE
  } else {
    res = allocateVT(cl, cl->virtualMethods.begin());
  
    if (!(cl->super)) {
      // 12 = number of virtual methods in java/lang/Object (!!!)
      uint32 size = 12 * sizeof(void*);
#define COPY(CLASS) \
    memcpy((void*)((unsigned)CLASS::VT + VT_SIZE), \
           (void*)((unsigned)res + VT_SIZE), size);

      COPY(ArrayUInt8)
      COPY(ArraySInt8)
      COPY(ArrayUInt16)
      COPY(ArraySInt16)
      COPY(ArrayUInt32)
      COPY(ArraySInt32)
      COPY(ArrayLong)
      COPY(ArrayFloat)
      COPY(ArrayDouble)
      COPY(UTF8)
      COPY(ArrayObject)

#undef COPY
    }
  }
#endif
 
#ifdef WITH_TRACER
  LLVMClassInfo* LCI = (LLVMClassInfo*)getClassInfo(cl);
  const Type* type = stat ? LCI->getStaticType() : LCI->getVirtualType();
  CommonClass::field_map fields = stat ? cl->staticFields : cl->virtualFields;
 
  Function* func = Function::Create(JnjvmModule::MarkAndTraceType,
                                    GlobalValue::ExternalLinkage,
                                    "markAndTraceObject",
                                    cl->isolate->TheModule);

  Constant* zero = mvm::jit::constantZero;
  Argument* arg = func->arg_begin();
  BasicBlock* block = BasicBlock::Create("", func);
  llvm::Value* realArg = new BitCastInst(arg, type, "", block);

#ifdef MULTIPLE_GC
  Value* GC = ++func->arg_begin();
  std::vector<Value*> Args;
  Args.push_back(arg);
  Args.push_back(GC);
  if (stat || cl->super == 0) {
    CallInst::Create(JnjvmModule::JavaObjectTracer, Args.begin(), Args.end(),
                     "", block);
  } else {
    CallInst::Create(((Class*)cl->super)->virtualTracer, Args.begin(),
                     Args.end(), "", block);
  }
#else  
  if (stat || cl->super == 0) {
    CallInst::Create(JnjvmModule::JavaObjectTracerFunction, arg, "", block);
  } else {
    LLVMClassInfo* LCP = (LLVMClassInfo*)getClassInfo((Class*)(cl->super));
    CallInst::Create(LCP->getVirtualTracer(), arg, "", block);
  }
#endif
  
  for (CommonClass::field_iterator i = fields.begin(), e = fields.end();
       i!= e; ++i) {
    if (i->second->getSignature()->funcs->doTrace) {
      LLVMFieldInfo* LFI = getFieldInfo(i->second);
      std::vector<Value*> args; //size = 2
      args.push_back(zero);
      args.push_back(LFI->getOffset());
      Value* ptr = GetElementPtrInst::Create(realArg, args.begin(), args.end(), 
                                             "",block);
      Value* val = new LoadInst(ptr, "", block);
      Value* valCast = new BitCastInst(val, JnjvmModule::JavaObjectType, "",
                                       block);
#ifdef MULTIPLE_GC
      std::vector<Value*> Args;
      Args.push_back(valCast);
      Args.push_back(GC);
      CallInst::Create(JnjvmModule::MarkAndTraceFunction, Args.begin(),
                       Args.end(), "", block);
#else
      CallInst::Create(JnjvmModule::MarkAndTraceFunction, valCast, "", block);
#endif
    }
  }

  ReturnInst::Create(block);

  void* codePtr = mvm::jit::executionEngine->getPointerToGlobal(func);
  ((void**)res)[VT_TRACER_OFFSET] = codePtr;
  
  if (!stat) {
    LCI->virtualTracerFunction = func;
  } else {
    LCI->staticTracerFunction = func;
  }
#endif
  return res;
}


const Type* LLVMClassInfo::getVirtualType() {
  if (!virtualType) {
    std::vector<const llvm::Type*> fields;
    JavaField* array[classDef->virtualFields.size()];
    
    if (classDef->super) {
      LLVMClassInfo* CLI = 
        (LLVMClassInfo*)module->getClassInfo(classDef->super);
      fields.push_back(CLI->getVirtualType()->getContainedType(0));
    } else {
      fields.push_back(JnjvmModule::JavaObjectType->getContainedType(0));
    }
    
    for (CommonClass::field_iterator i = classDef->virtualFields.begin(),
         e = classDef->virtualFields.end(); i!= e; ++i) {
      JavaField* field = i->second;
      array[field->num] = field;
    }
    
    
    for (uint32 index = 0; index < classDef->virtualFields.size(); ++index) {
      uint8 id = array[index]->getSignature()->funcs->numId;
      LLVMAssessorInfo& LAI = LLVMAssessorInfo::AssessorInfo[id];
      fields.push_back(LAI.llvmType);
    }
    
    StructType* structType = StructType::get(fields, false);
    virtualType = PointerType::getUnqual(structType);
    const TargetData* targetData = mvm::jit::executionEngine->getTargetData();
    const StructLayout* sl = targetData->getStructLayout(structType);
    
    for (CommonClass::field_iterator i = classDef->virtualFields.begin(),
         e = classDef->virtualFields.end(); i!= e; ++i) {
      JavaField* field = i->second;
      field->ptrOffset = sl->getElementOffset(field->num + 1);
    }
    
    VirtualTable* VT = module->makeVT((Class*)classDef, false);
  
    uint64 size = mvm::jit::getTypeSize(structType);
    classDef->virtualSize = (uint32)size;
    classDef->virtualVT = VT;
    virtualSizeConstant = ConstantInt::get(Type::Int32Ty, size);

  }

  return virtualType;
}

const Type* LLVMClassInfo::getStaticType() {
  
  if (!staticType) {
    Class* cl = (Class*)classDef;
    std::vector<const llvm::Type*> fields;
    JavaField* array[classDef->staticFields.size() + 1];
    fields.push_back(JnjvmModule::JavaObjectType->getContainedType(0));

    for (CommonClass::field_iterator i = classDef->staticFields.begin(),
         e = classDef->staticFields.end(); i!= e; ++i) {
      JavaField* field = i->second;
      array[field->num] = field;
    }

    for (uint32 index = 0; index < classDef->staticFields.size(); ++index) {
      JavaField* field = array[index];
      uint8 id = field->getSignature()->funcs->numId;
      LLVMAssessorInfo& LAI = LLVMAssessorInfo::AssessorInfo[id];
      fields.push_back(LAI.llvmType);
    }
  
    StructType* structType = StructType::get(fields, false);
    staticType = PointerType::getUnqual(structType);
    const TargetData* targetData = mvm::jit::executionEngine->getTargetData();
    const StructLayout* sl = targetData->getStructLayout(structType);
    
    for (CommonClass::field_iterator i = classDef->staticFields.begin(),
         e = classDef->staticFields.end(); i!= e; ++i) {
      JavaField* field = i->second;
      field->ptrOffset = sl->getElementOffset(field->num + 1);
    }
    

    VirtualTable* VT = module->makeVT((Class*)classDef, true);

    uint64 size = mvm::jit::getTypeSize(structType);
    cl->staticSize = size;
    cl->staticVT = VT;

#ifndef MULTIPLE_VM
    JavaObject* val = 
      (JavaObject*)classDef->isolate->allocateObject(cl->staticSize,
                                                     cl->staticVT);
    val->initialise(classDef);
    for (CommonClass::field_iterator i = cl->staticFields.begin(),
         e = cl->staticFields.end(); i!= e; ++i) {
    
      i->second->initField(val);
    }
  
    cl->_staticInstance = val;
#endif
  }
  return staticType;
}

Value* LLVMClassInfo::getStaticVar(JavaJIT* jit) {
#ifndef MULTIPLE_VM
  if (!staticVarGV) {
    getStaticType();
    JavaObject* obj = ((Class*)classDef)->staticInstance();
    Constant* cons = 
      ConstantExpr::getIntToPtr(ConstantInt::get(Type::Int64Ty,
                                uint64_t (obj)), JnjvmModule::JavaObjectType);
      
      staticVarGV = new GlobalVariable(JnjvmModule::JavaObjectType, true,
                                       GlobalValue::ExternalLinkage,
                                       cons, "",
                                       classDef->isolate->TheModule);
  }

  return new LoadInst(staticVarGV, "", jit->currentBlock);

#else
  Value* var = getVar(jit->compilingClass->isolate->module);
  Value* ld = new LoadInst(var, "", jit->currentBlock);
  ld = jit->invoke(JnjvmModule::InitialisationCheckFunction, ld, "",
                   jit->currentBlock);
  return jit->invoke(JnjvmModule::GetStaticInstanceFunction, ld,
                     jit->isolateLocal, "", jit->currentBlock);
#endif
}

Value* LLVMClassInfo::getVirtualTable(JavaJIT* jit) {
  if (!virtualTableGV) {
    getVirtualType();
    Constant* cons = 
      ConstantExpr::getIntToPtr(ConstantInt::get(Type::Int64Ty,
                                                 uint64_t(classDef->virtualVT)),
                                JnjvmModule::VTType);
    virtualTableGV = new GlobalVariable(JnjvmModule::VTType, true,
                                        GlobalValue::ExternalLinkage,
                                        cons, "",
                                        classDef->isolate->TheModule);
  }
  return new LoadInst(virtualTableGV, "", jit->currentBlock);
}

Value* LLVMClassInfo::getVirtualSize(JavaJIT* jit) {
  if (!virtualSizeConstant) {
    getVirtualType();
    virtualSizeConstant = 
      ConstantInt::get(Type::Int32Ty, classDef->virtualSize);
  }
  return virtualSizeConstant;
}

Function* LLVMClassInfo::getStaticTracer() {
  if (!staticTracerFunction) {
    getStaticType();
  }
  return staticTracerFunction;
}

Function* LLVMClassInfo::getVirtualTracer() {
  if (!virtualTracerFunction) {
    getVirtualType();
  }
  return virtualTracerFunction;
}

Function* LLVMMethodInfo::getMethod() {
  if (!methodFunction) {
    Jnjvm* vm = methodDef->classDef->isolate;
    methodFunction = Function::Create(getFunctionType(), 
                                      GlobalValue::GhostLinkage,
                                      methodDef->printString(),
                                      vm->TheModule);
    vm->TheModuleProvider->addFunction(methodFunction, methodDef);
  }
  return methodFunction;
}

const FunctionType* LLVMMethodInfo::getFunctionType() {
  if (!functionType) {
    LLVMSignatureInfo* LSI = module->getSignatureInfo(methodDef->getSignature());
    assert(LSI);
    if (isStatic(methodDef->access)) {
      functionType = LSI->getStaticType();
    } else {
      functionType = LSI->getVirtualType();
    }
  }
  return functionType;
}

ConstantInt* LLVMMethodInfo::getOffset() {
  if (!offsetConstant) {
    module->resolveVirtualClass(methodDef->classDef);
    offsetConstant = ConstantInt::get(Type::Int32Ty, methodDef->offset);
  }
  return offsetConstant;
}

ConstantInt* LLVMFieldInfo::getOffset() {
  if (!offsetConstant) {
    if (isStatic(fieldDef->access)) {
      module->resolveStaticClass(fieldDef->classDef); 
    } else {
      module->resolveVirtualClass(fieldDef->classDef); 
    }
    // Increment by one because zero is JavaObject
    offsetConstant = ConstantInt::get(Type::Int32Ty, fieldDef->num + 1);
  }
  return offsetConstant;
}

const llvm::FunctionType* LLVMSignatureInfo::getVirtualType() {
 if (!virtualType) {
    // Lock here because we are called by arbitrary code
    llvm::MutexGuard locked(mvm::jit::executionEngine->lock);
    std::vector<const llvm::Type*> llvmArgs;
    unsigned int size = signature->args.size();

    llvmArgs.push_back(JnjvmModule::JavaObjectType);

    for (uint32 i = 0; i < size; ++i) {
      uint8 id = signature->args.at(i)->funcs->numId;
      LLVMAssessorInfo& LAI = LLVMAssessorInfo::AssessorInfo[id];
      llvmArgs.push_back(LAI.llvmType);
    }

#if defined(MULTIPLE_VM) || defined(MULTIPLE_GC)
    llvmArgs.push_back(mvm::jit::ptrType); // domain
#endif

    uint8 id = signature->ret->funcs->numId;
    LLVMAssessorInfo& LAI = LLVMAssessorInfo::AssessorInfo[id];
    virtualType = FunctionType::get(LAI.llvmType, llvmArgs, false);
  }
  return virtualType;
}

const llvm::FunctionType* LLVMSignatureInfo::getStaticType() {
 if (!staticType) {
    // Lock here because we are called by arbitrary code
    llvm::MutexGuard locked(mvm::jit::executionEngine->lock);
    std::vector<const llvm::Type*> llvmArgs;
    unsigned int size = signature->args.size();

    for (uint32 i = 0; i < size; ++i) {
      uint8 id = signature->args.at(i)->funcs->numId;
      LLVMAssessorInfo& LAI = LLVMAssessorInfo::AssessorInfo[id];
      llvmArgs.push_back(LAI.llvmType);
    }

#if defined(MULTIPLE_VM) || defined(MULTIPLE_GC)
    llvmArgs.push_back(mvm::jit::ptrType); // domain
#endif

    uint8 id = signature->ret->funcs->numId;
    LLVMAssessorInfo& LAI = LLVMAssessorInfo::AssessorInfo[id];
    staticType = FunctionType::get(LAI.llvmType, llvmArgs, false);
  }
  return staticType;
}

const llvm::FunctionType* LLVMSignatureInfo::getNativeType() {
  if (!nativeType) {
    // Lock here because we are called by arbitrary code
    llvm::MutexGuard locked(mvm::jit::executionEngine->lock);
    std::vector<const llvm::Type*> llvmArgs;
    unsigned int size = signature->args.size();
    
    llvmArgs.push_back(mvm::jit::ptrType); // JNIEnv
    llvmArgs.push_back(JnjvmModule::JavaObjectType); // Class

    for (uint32 i = 0; i < size; ++i) {
      uint8 id = signature->args.at(i)->funcs->numId;
      LLVMAssessorInfo& LAI = LLVMAssessorInfo::AssessorInfo[id];
      llvmArgs.push_back(LAI.llvmType);
    }

#if defined(MULTIPLE_VM) || defined(MULTIPLE_GC)
    llvmArgs.push_back(mvm::jit::ptrType); // domain
#endif

    uint8 id = signature->ret->funcs->numId;
    LLVMAssessorInfo& LAI = LLVMAssessorInfo::AssessorInfo[id];
    nativeType = FunctionType::get(LAI.llvmType, llvmArgs, false);
  }
  return nativeType;
}


Function* LLVMSignatureInfo::createFunctionCallBuf(bool virt) {
  
  ConstantInt* CI = mvm::jit::constantZero;
  std::vector<Value*> Args;

  Function* res = Function::Create(virt ? getVirtualBufType() : 
                                          getStaticBufType(),
                                   GlobalValue::ExternalLinkage,
                                   signature->printString(),
                                   signature->isolate->TheModule);
  
  BasicBlock* currentBlock = BasicBlock::Create("enter", res);
  Function::arg_iterator i = res->arg_begin();
  Value *obj, *ptr, *func;
#if defined(MULTIPLE_VM) || defined(MULTIPLE_GC)
  Value* vm = i;
#endif
  ++i;
  func = i;
  ++i;
  if (virt) {
    obj = i;
    ++i;
    Args.push_back(obj);
  }
  ptr = i;

  for (std::vector<Typedef*>::iterator i = signature->args.begin(), 
            e = signature->args.end(); i!= e; ++i) {
  
    const AssessorDesc* funcs = (*i)->funcs;
    ptr = GetElementPtrInst::Create(ptr, CI, "", currentBlock);
    LLVMAssessorInfo& LAI = LLVMAssessorInfo::AssessorInfo[funcs->numId];
    Value* val = new BitCastInst(ptr, LAI.llvmTypePtr, "", currentBlock);
    Value* arg = new LoadInst(val, "", currentBlock);
    Args.push_back(arg);
    if (funcs == AssessorDesc::dLong || funcs == AssessorDesc::dDouble) {
      CI = mvm::jit::constantTwo;
    } else {
      CI = mvm::jit::constantOne;
    }
  }

#if defined(MULTIPLE_VM) || defined(MULTIPLE_GC)
  Args.push_back(vm);
#endif

  Value* val = CallInst::Create(func, Args.begin(), Args.end(), "", currentBlock);
  if (signature->ret->funcs != AssessorDesc::dVoid)
    ReturnInst::Create(val, currentBlock);
  else
    ReturnInst::Create(currentBlock);
  
  return res;
}

Function* LLVMSignatureInfo::createFunctionCallAP(bool virt) {
  
  std::vector<Value*> Args;

  Function* res = Function::Create(virt ? getVirtualBufType() :
                                          getStaticBufType(),
                                      GlobalValue::ExternalLinkage,
                                      signature->printString(),
                                      signature->isolate->TheModule);
  
  BasicBlock* currentBlock = BasicBlock::Create("enter", res);
  Function::arg_iterator i = res->arg_begin();
  Value *obj, *ap, *func;
#if defined(MULTIPLE_VM) || defined(MULTIPLE_GC)
  Value* vm = i;
#endif
  ++i;
  func = i;
  ++i;
  if (virt) {
    obj = i;
    Args.push_back(obj);
    ++i;
  }
  ap = i;

  for (std::vector<Typedef*>::iterator i = signature->args.begin(), 
       e = signature->args.end(); i!= e; i++) {

    LLVMAssessorInfo& LAI = LLVMAssessorInfo::AssessorInfo[(*i)->funcs->numId];
    Args.push_back(new VAArgInst(ap, LAI.llvmType, "", currentBlock));
  }

#if defined(MULTIPLE_VM) || defined(MULTIPLE_GC)
  Args.push_back(vm);
#endif

  Value* val = CallInst::Create(func, Args.begin(), Args.end(), "", currentBlock);
  if (signature->ret->funcs != AssessorDesc::dVoid)
    ReturnInst::Create(val, currentBlock);
  else
    ReturnInst::Create(currentBlock);
  
  return res;
}

const PointerType* LLVMSignatureInfo::getStaticPtrType() {
  if (!staticPtrType) {
    staticPtrType = PointerType::getUnqual(getStaticType());
  }
  return staticPtrType;
}

const PointerType* LLVMSignatureInfo::getVirtualPtrType() {
  if (!virtualPtrType) {
    virtualPtrType = PointerType::getUnqual(getVirtualType());
  }
  return virtualPtrType;
}

const PointerType* LLVMSignatureInfo::getNativePtrType() {
  if (!nativePtrType) {
    nativePtrType = PointerType::getUnqual(getNativeType());
  }
  return nativePtrType;
}


const FunctionType* LLVMSignatureInfo::getVirtualBufType() {
  if (!virtualBufType) {
    // Lock here because we are called by arbitrary code
    llvm::MutexGuard locked(mvm::jit::executionEngine->lock);
    std::vector<const llvm::Type*> Args2;
    Args2.push_back(mvm::jit::ptrType); // vm
    Args2.push_back(getVirtualPtrType());
    Args2.push_back(JnjvmModule::JavaObjectType);
    Args2.push_back(PointerType::getUnqual(Type::Int32Ty));
    uint8 id = signature->ret->funcs->numId;
    LLVMAssessorInfo& LAI = LLVMAssessorInfo::AssessorInfo[id];
    virtualBufType = FunctionType::get(LAI.llvmType, Args2, false);
  }
  return virtualBufType;
}

const FunctionType* LLVMSignatureInfo::getStaticBufType() {
  if (!staticBufType) {
    // Lock here because we are called by arbitrary code
    llvm::MutexGuard locked(mvm::jit::executionEngine->lock);
    std::vector<const llvm::Type*> Args;
    Args.push_back(mvm::jit::ptrType); // vm
    Args.push_back(getStaticPtrType());
    Args.push_back(PointerType::getUnqual(Type::Int32Ty));
    uint8 id = signature->ret->funcs->numId;
    LLVMAssessorInfo& LAI = LLVMAssessorInfo::AssessorInfo[id];
    staticBufType = FunctionType::get(LAI.llvmType, Args, false);
  }
  return staticBufType;
}

Function* LLVMSignatureInfo::getVirtualBuf() {
  if (!virtualBufFunction) {
    // Lock here because we are called by arbitrary code
    llvm::MutexGuard locked(mvm::jit::executionEngine->lock);
    virtualBufFunction = createFunctionCallBuf(true);
    signature->setVirtualCallBuf((mvm::Code*)
      mvm::jit::executionEngine->getPointerToGlobal(virtualBufFunction));
  }
  return virtualBufFunction;
}

Function* LLVMSignatureInfo::getVirtualAP() {
  if (!virtualAPFunction) {
    // Lock here because we are called by arbitrary code
    llvm::MutexGuard locked(mvm::jit::executionEngine->lock);
    virtualAPFunction = createFunctionCallAP(true);
    signature->setVirtualCallAP((mvm::Code*)
      mvm::jit::executionEngine->getPointerToGlobal(virtualAPFunction));
  }
  return virtualAPFunction;
}

Function* LLVMSignatureInfo::getStaticBuf() {
  if (!staticBufFunction) {
    // Lock here because we are called by arbitrary code
    llvm::MutexGuard locked(mvm::jit::executionEngine->lock);
    staticBufFunction = createFunctionCallBuf(false);
    signature->setStaticCallBuf((mvm::Code*)
      mvm::jit::executionEngine->getPointerToGlobal(staticBufFunction));
  }
  return staticBufFunction;
}

Function* LLVMSignatureInfo::getStaticAP() {
  if (!staticAPFunction) {
    // Lock here because we are called by arbitrary code
    llvm::MutexGuard locked(mvm::jit::executionEngine->lock);
    staticAPFunction = createFunctionCallAP(false);
    signature->setStaticCallAP((mvm::Code*)
      mvm::jit::executionEngine->getPointerToGlobal(staticAPFunction));
  }
  return staticAPFunction;
}

void JnjvmModule::resolveVirtualClass(Class* cl) {
  // Lock here because we may be called by a class resolver
  llvm::MutexGuard locked(mvm::jit::executionEngine->lock);
  LLVMClassInfo* LCI = (LLVMClassInfo*)getClassInfo(cl);
  LCI->getVirtualType();
}

void JnjvmModule::resolveStaticClass(Class* cl) {
  // Lock here because we may be called by a class initializer
  llvm::MutexGuard locked(mvm::jit::executionEngine->lock);
  LLVMClassInfo* LCI = (LLVMClassInfo*)getClassInfo(cl);
  LCI->getStaticType();
}

LLVMCommonClassInfo* JnjvmModule::getClassInfo(CommonClass* cl) {
  class_iterator CI = classMap.find(cl);
  if (CI != classMap.end()) {
    return CI->second;
  } else {
    if (cl->isArray) {
      LLVMCommonClassInfo* LCI = new LLVMCommonClassInfo(cl, this);
      classMap.insert(std::make_pair(cl, LCI));
      return LCI;
    } else {
      LLVMClassInfo* LCI = new LLVMClassInfo((Class*)cl, this);
      classMap.insert(std::make_pair(cl, LCI));
      return LCI;
    }
  }
}

LLVMMethodInfo* JnjvmModule::getMethodInfo(JavaMethod* meth) {
  method_iterator MI = methodMap.find(meth);
  if (MI != methodMap.end()) {
    return MI->second;
  } else {
    LLVMMethodInfo* LMI = new LLVMMethodInfo(meth, this);
    methodMap.insert(std::make_pair(meth, LMI));
    return LMI;
  }
}

LLVMFieldInfo* JnjvmModule::getFieldInfo(JavaField* field) {
  field_iterator FI = fieldMap.find(field);
  if (FI != fieldMap.end()) {
    return FI->second;
  } else {
    LLVMFieldInfo* LFI = new LLVMFieldInfo(field, this);
    fieldMap.insert(std::make_pair(field, LFI));
    return LFI;
  }
}

LLVMSignatureInfo* JnjvmModule::getSignatureInfo(Signdef* sign) {
  signature_iterator SI = signatureMap.find(sign);
  if (SI != signatureMap.end()) {
    return SI->second;
  } else {
    LLVMSignatureInfo* LSI = new LLVMSignatureInfo(sign);
    signatureMap.insert(std::make_pair(sign, LSI));
    return LSI;
  }
}

#ifdef SERVICE_VM
LLVMServiceInfo* JnjvmModule::getServiceInfo(ServiceDomain* S) {
  service_iterator SI = serviceMap.find(S);
  if (SI != serviceMap.end()) {
    return SI->second;
  } else {
    LLVMServiceInfo* LSI = new LLVMServiceInfo(sign);
    serviceMap.insert(std::make_pair(S, LSI));
    return LSI;
  }
}

Value* LLVMServiceInfo::getDelegatee(JavaJIT* jit) {
  if (!delegateeGV) {
    Constant* cons = 
      ConstantExpr::getIntToPtr(ConstantInt::get(Type::Int64Ty, uint64(vm)),
                                mvm::jit::ptrType);
    delegateeGV = new GlobalVariable(mvm::jit::ptrType, true,
                                    GlobalValue::ExternalLinkage,
                                    cons, "",
                                    vm->module);
  }
  return new LoadInst(delegateeGV, "", jit->currentBlock);
}

#endif


#include "LLVMRuntime.cpp"

void JnjvmModule::initialise() {
  Module* module = this;
  makeLLVMModuleContents(module);
  
  VTType = module->getTypeByName("VT");
  
  JavaObjectType = 
    PointerType::getUnqual(module->getTypeByName("JavaObject"));

  JavaArrayType =
    PointerType::getUnqual(module->getTypeByName("JavaArray"));
  
  JavaClassType =
    PointerType::getUnqual(module->getTypeByName("JavaClass"));
  
  JavaArrayUInt8Type =
    PointerType::getUnqual(module->getTypeByName("ArrayUInt8"));
  JavaArraySInt8Type =
    PointerType::getUnqual(module->getTypeByName("ArraySInt8"));
  JavaArrayUInt16Type =
    PointerType::getUnqual(module->getTypeByName("ArrayUInt16"));
  JavaArraySInt16Type =
    PointerType::getUnqual(module->getTypeByName("ArraySInt16"));
  JavaArrayUInt32Type =
    PointerType::getUnqual(module->getTypeByName("ArrayUInt32"));
  JavaArraySInt32Type =
    PointerType::getUnqual(module->getTypeByName("ArraySInt32"));
  JavaArrayLongType =
    PointerType::getUnqual(module->getTypeByName("ArrayLong"));
  JavaArrayFloatType =
    PointerType::getUnqual(module->getTypeByName("ArrayFloat"));
  JavaArrayDoubleType =
    PointerType::getUnqual(module->getTypeByName("ArrayDouble"));
  JavaArrayObjectType =
    PointerType::getUnqual(module->getTypeByName("ArrayObject"));

  CacheNodeType =
    PointerType::getUnqual(module->getTypeByName("CacheNode"));
  
  EnveloppeType =
    PointerType::getUnqual(module->getTypeByName("Enveloppe"));

  InterfaceLookupFunction = module->getFunction("virtualLookup");
  MultiCallNewFunction = module->getFunction("multiCallNew");
  InitialisationCheckFunction = module->getFunction("initialisationCheck");
  ForceInitialisationCheckFunction = 
    module->getFunction("forceInitialisationCheck");

  ArrayLengthFunction = module->getFunction("arrayLength");
  GetVTFunction = module->getFunction("getVT");
  GetClassFunction = module->getFunction("getClass");
  ClassLookupFunction = module->getFunction("newLookup");
  GetVTFromClassFunction = module->getFunction("getVTFromClass");
  GetObjectSizeFromClassFunction = module->getFunction("getObjectSizeFromClass");
 
  GetClassDelegateeFunction = module->getFunction("getClassDelegatee");
  InstanceOfFunction = module->getFunction("JavaObjectInstanceOf");
  AquireObjectFunction = module->getFunction("JavaObjectAquire");
  ReleaseObjectFunction = module->getFunction("JavaObjectRelease");

  FieldLookupFunction = module->getFunction("fieldLookup");
  
  GetExceptionFunction = module->getFunction("JavaThreadGetException");
  GetJavaExceptionFunction = module->getFunction("JavaThreadGetJavaException");
  CompareExceptionFunction = module->getFunction("JavaThreadCompareException");
  JniProceedPendingExceptionFunction = 
    module->getFunction("jniProceedPendingException");
  GetSJLJBufferFunction = module->getFunction("getSJLJBuffer");
  
  NullPointerExceptionFunction = module->getFunction("nullPointerException");
  ClassCastExceptionFunction = module->getFunction("classCastException");
  IndexOutOfBoundsExceptionFunction = 
    module->getFunction("indexOutOfBoundsException");
  NegativeArraySizeExceptionFunction = 
    module->getFunction("negativeArraySizeException");
  OutOfMemoryErrorFunction = module->getFunction("outOfMemoryError");

  JavaObjectAllocateFunction = module->getFunction("gcmalloc");

  PrintExecutionFunction = module->getFunction("printExecution");
  PrintMethodStartFunction = module->getFunction("printMethodStart");
  PrintMethodEndFunction = module->getFunction("printMethodEnd");

  ThrowExceptionFunction = module->getFunction("JavaThreadThrowException");

  ClearExceptionFunction = module->getFunction("JavaThreadClearException");
  

#ifdef MULTIPLE_VM
  GetStaticInstanceFunction = module->getFunction("getStaticInstance");
  RuntimeUTF8ToStrFunction = module->getFunction("runtimeUTF8ToStr");
#endif
  
#ifdef SERVICE_VM
  AquireObjectInSharedDomainFunction = 
    module->getFunction("JavaObjectAquireInSharedDomain");
  ReleaseObjectInSharedDomainfunction = 
    module->getFunction("JavaObjectReleaseInSharedDomain");
  ServiceCallStartFunction = module->getFunction("serviceCallStart");
  ServiceCallStopFunction = module->getFunction("serviceCallStop");
#endif
    
#ifdef WITH_TRACER
  MarkAndTraceFunction = module->getFunction("MarkAndTrace");
  MarkAndTraceType = MarkAndTraceFunction->getFunctionType();
  JavaObjectTracerFunction = module->getFunction("JavaObjectTracer");
#endif

#ifndef WITHOUT_VTABLE
  VirtualLookupFunction = module->getFunction("vtableLookup");
#endif

#ifdef MULTIPLE_GC
  GetCollectorFunction = module->getFunction("getCollector");
#endif

  
  UTF8NullConstant = Constant::getNullValue(JavaArrayUInt16Type); 
  JavaClassNullConstant = Constant::getNullValue(JavaClassType); 
  JavaObjectNullConstant = Constant::getNullValue(JnjvmModule::JavaObjectType);
  MaxArraySizeConstant = ConstantInt::get(Type::Int32Ty,
                                          JavaArray::MaxArraySize);
  JavaObjectSizeConstant = ConstantInt::get(Type::Int32Ty, sizeof(JavaObject));
  
  
  Constant* cons = 
    ConstantExpr::getIntToPtr(ConstantInt::get(Type::Int64Ty,
                              uint64_t (JavaObject::VT)), VTType);
      
  JavaObjectVirtualTableGV = new GlobalVariable(VTType, true,
                                                GlobalValue::ExternalLinkage,
                                                cons, "",
                                                module);
  
  cons = 
    ConstantExpr::getIntToPtr(ConstantInt::get(Type::Int64Ty,
                              uint64_t (ArrayObject::VT)), VTType);
  
  ArrayObjectVirtualTableGV = new GlobalVariable(VTType, true,
                                          GlobalValue::ExternalLinkage,
                                          cons, "",
                                          module);
  
  OffsetObjectSizeInClassConstant = mvm::jit::constantOne;
  OffsetVTInClassConstant = mvm::jit::constantTwo;
  JavaArrayElementsOffsetConstant = mvm::jit::constantTwo;
  JavaArraySizeOffsetConstant = mvm::jit::constantOne;
  JavaObjectLockOffsetConstant = mvm::jit::constantTwo;
  JavaObjectClassOffsetConstant = mvm::jit::constantOne;
  
  LLVMAssessorInfo::initialise();
}

void JnjvmModule::InitField(JavaField* field, JavaObject* obj, uint64 val) {
  
  const AssessorDesc* funcs = field->getSignature()->funcs;
  if (funcs == AssessorDesc::dLong) {
    ((sint64*)((uint64)obj + field->ptrOffset))[0] = val;
  } else if (funcs == AssessorDesc::dInt) {
    ((sint32*)((uint64)obj + field->ptrOffset))[0] = (sint32)val;
  } else if (funcs == AssessorDesc::dChar) {
    ((uint16*)((uint64)obj + field->ptrOffset))[0] = (uint16)val;
  } else if (funcs == AssessorDesc::dShort) {
    ((sint16*)((uint64)obj + field->ptrOffset))[0] = (sint16)val;
  } else if (funcs == AssessorDesc::dByte) {
    ((sint8*)((uint64)obj + field->ptrOffset))[0] = (sint8)val;
  } else if (funcs == AssessorDesc::dBool) {
    ((uint8*)((uint64)obj + field->ptrOffset))[0] = (uint8)val;
  } else {
    // 0 value for everything else
    ((sint32*)((uint64)obj + field->ptrOffset))[0] = (sint32)val;
  }
}

void JnjvmModule::InitField(JavaField* field, JavaObject* obj, JavaObject* val) {
  ((JavaObject**)((uint64)obj + field->ptrOffset))[0] = val;
}

void JnjvmModule::InitField(JavaField* field, JavaObject* obj, double val) {
  ((double*)((uint64)obj + field->ptrOffset))[0] = val;
}

void JnjvmModule::InitField(JavaField* field, JavaObject* obj, float val) {
  ((float*)((uint64)obj + field->ptrOffset))[0] = val;
}

void JnjvmModule::setMethod(JavaMethod* meth, const char* name) {
  llvm::Function* func = getMethodInfo(meth)->getMethod();
  func->setName(name);
  func->setLinkage(llvm::GlobalValue::ExternalLinkage);
}

void* JnjvmModule::getMethod(JavaMethod* meth) {
  return getMethodInfo(meth)->getMethod();
}

JnjvmModule::JnjvmModule(const std::string &ModuleID) : llvm::Module(ModuleID) {
 std::string str = 
    mvm::jit::executionEngine->getTargetData()->getStringRepresentation();
  setDataLayout(str);
}
void LLVMAssessorInfo::initialise() {
  AssessorInfo[VOID_ID].llvmType = Type::VoidTy;
  AssessorInfo[VOID_ID].llvmTypePtr = 0;
  AssessorInfo[VOID_ID].llvmNullConstant = 0;
  AssessorInfo[VOID_ID].sizeInBytesConstant = 0;
  
  AssessorInfo[BOOL_ID].llvmType = Type::Int8Ty;
  AssessorInfo[BOOL_ID].llvmTypePtr = PointerType::getUnqual(Type::Int8Ty);
  AssessorInfo[BOOL_ID].llvmNullConstant = 
    Constant::getNullValue(Type::Int8Ty);
  AssessorInfo[BOOL_ID].sizeInBytesConstant = mvm::jit::constantOne;
  
  AssessorInfo[BYTE_ID].llvmType = Type::Int8Ty;
  AssessorInfo[BYTE_ID].llvmTypePtr = PointerType::getUnqual(Type::Int8Ty);
  AssessorInfo[BYTE_ID].llvmNullConstant = 
    Constant::getNullValue(Type::Int8Ty);
  AssessorInfo[BYTE_ID].sizeInBytesConstant = mvm::jit::constantOne;
  
  AssessorInfo[SHORT_ID].llvmType = Type::Int16Ty;
  AssessorInfo[SHORT_ID].llvmTypePtr = PointerType::getUnqual(Type::Int16Ty);
  AssessorInfo[SHORT_ID].llvmNullConstant = 
    Constant::getNullValue(Type::Int16Ty);
  AssessorInfo[SHORT_ID].sizeInBytesConstant = mvm::jit::constantTwo;
  
  AssessorInfo[CHAR_ID].llvmType = Type::Int16Ty;
  AssessorInfo[CHAR_ID].llvmTypePtr = PointerType::getUnqual(Type::Int16Ty);
  AssessorInfo[CHAR_ID].llvmNullConstant = 
    Constant::getNullValue(Type::Int16Ty);
  AssessorInfo[CHAR_ID].sizeInBytesConstant = mvm::jit::constantTwo;
  
  AssessorInfo[INT_ID].llvmType = Type::Int32Ty;
  AssessorInfo[INT_ID].llvmTypePtr = PointerType::getUnqual(Type::Int32Ty);
  AssessorInfo[INT_ID].llvmNullConstant = 
    Constant::getNullValue(Type::Int32Ty);
  AssessorInfo[INT_ID].sizeInBytesConstant = mvm::jit::constantFour;
  
  AssessorInfo[FLOAT_ID].llvmType = Type::FloatTy;
  AssessorInfo[FLOAT_ID].llvmTypePtr = PointerType::getUnqual(Type::FloatTy);
  AssessorInfo[FLOAT_ID].llvmNullConstant = 
    Constant::getNullValue(Type::FloatTy);
  AssessorInfo[FLOAT_ID].sizeInBytesConstant = mvm::jit::constantFour;
  
  AssessorInfo[LONG_ID].llvmType = Type::Int64Ty;
  AssessorInfo[LONG_ID].llvmTypePtr = PointerType::getUnqual(Type::Int64Ty);
  AssessorInfo[LONG_ID].llvmNullConstant = 
    Constant::getNullValue(Type::Int64Ty);
  AssessorInfo[LONG_ID].sizeInBytesConstant = mvm::jit::constantEight;
  
  AssessorInfo[DOUBLE_ID].llvmType = Type::DoubleTy;
  AssessorInfo[DOUBLE_ID].llvmTypePtr = PointerType::getUnqual(Type::DoubleTy);
  AssessorInfo[DOUBLE_ID].llvmNullConstant = 
    Constant::getNullValue(Type::DoubleTy);
  AssessorInfo[DOUBLE_ID].sizeInBytesConstant = mvm::jit::constantEight;
  
  AssessorInfo[ARRAY_ID].llvmType = JnjvmModule::JavaObjectType;
  AssessorInfo[ARRAY_ID].llvmTypePtr =
    PointerType::getUnqual(JnjvmModule::JavaObjectType);
  AssessorInfo[ARRAY_ID].llvmNullConstant =
    JnjvmModule::JavaObjectNullConstant;
  AssessorInfo[ARRAY_ID].sizeInBytesConstant = mvm::jit::constantPtrSize;
  
  AssessorInfo[OBJECT_ID].llvmType = JnjvmModule::JavaObjectType;
  AssessorInfo[OBJECT_ID].llvmTypePtr =
    PointerType::getUnqual(JnjvmModule::JavaObjectType);
  AssessorInfo[OBJECT_ID].llvmNullConstant =
    JnjvmModule::JavaObjectNullConstant;
  AssessorInfo[OBJECT_ID].sizeInBytesConstant = mvm::jit::constantPtrSize;
}

LLVMAssessorInfo LLVMAssessorInfo::AssessorInfo[NUM_ASSESSORS];
