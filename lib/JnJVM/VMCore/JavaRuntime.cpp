//===--------- JavaRuntime.cpp - Definition of a Jnjvm module -------------===//
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
#include "JavaThread.h"
#include "JavaTypes.h"
#include "Jnjvm.h"
#include "JavaRuntime.h"
#include "JnjvmModuleProvider.h"
#include "LockedMap.h"


using namespace jnjvm;
using namespace llvm;


#ifdef WITH_TRACER
const llvm::FunctionType* JavaRuntime::MarkAndTraceType = 0;
#endif

const llvm::Type* JavaRuntime::JavaObjectType = 0;
const llvm::Type* JavaRuntime::JavaArrayType = 0;
const llvm::Type* JavaRuntime::JavaArrayUInt8Type = 0;
const llvm::Type* JavaRuntime::JavaArraySInt8Type = 0;
const llvm::Type* JavaRuntime::JavaArrayUInt16Type = 0;
const llvm::Type* JavaRuntime::JavaArraySInt16Type = 0;
const llvm::Type* JavaRuntime::JavaArrayUInt32Type = 0;
const llvm::Type* JavaRuntime::JavaArraySInt32Type = 0;
const llvm::Type* JavaRuntime::JavaArrayFloatType = 0;
const llvm::Type* JavaRuntime::JavaArrayDoubleType = 0;
const llvm::Type* JavaRuntime::JavaArrayLongType = 0;
const llvm::Type* JavaRuntime::JavaArrayObjectType = 0;
const llvm::Type* JavaRuntime::CacheNodeType = 0;
const llvm::Type* JavaRuntime::EnveloppeType = 0;

llvm::Constant*       JavaRuntime::JavaObjectNullConstant;
llvm::Constant*       JavaRuntime::UTF8NullConstant;
llvm::Constant*       JavaRuntime::JavaClassNullConstant;
llvm::Constant*       JavaRuntime::MaxArraySizeConstant;
llvm::Constant*       JavaRuntime::JavaObjectSizeConstant;
llvm::GlobalVariable* JavaRuntime::JavaObjectVirtualTableGV;
llvm::GlobalVariable* JavaRuntime::ArrayObjectVirtualTableGV;
llvm::ConstantInt*    JavaRuntime::OffsetObjectSizeInClassConstant;
llvm::ConstantInt*    JavaRuntime::OffsetVTInClassConstant;
llvm::ConstantInt*    JavaRuntime::OffsetDepthInClassConstant;
llvm::ConstantInt*    JavaRuntime::OffsetDisplayInClassConstant;
const llvm::Type*     JavaRuntime::JavaClassType;
const llvm::Type*     JavaRuntime::VTType;
llvm::ConstantInt*    JavaRuntime::JavaArrayElementsOffsetConstant;
llvm::ConstantInt*    JavaRuntime::JavaArraySizeOffsetConstant;
llvm::ConstantInt*    JavaRuntime::JavaObjectLockOffsetConstant;
llvm::ConstantInt*    JavaRuntime::JavaObjectClassOffsetConstant;

#ifdef WITH_TRACER
llvm::Function* JavaRuntime::MarkAndTraceFunction = 0;
llvm::Function* JavaRuntime::JavaObjectTracerFunction = 0;
#endif
llvm::Function* JavaRuntime::GetSJLJBufferFunction = 0;
llvm::Function* JavaRuntime::ThrowExceptionFunction = 0;
llvm::Function* JavaRuntime::GetExceptionFunction = 0;
llvm::Function* JavaRuntime::GetJavaExceptionFunction = 0;
llvm::Function* JavaRuntime::ClearExceptionFunction = 0;
llvm::Function* JavaRuntime::CompareExceptionFunction = 0;
llvm::Function* JavaRuntime::NullPointerExceptionFunction = 0;
llvm::Function* JavaRuntime::ClassCastExceptionFunction = 0;
llvm::Function* JavaRuntime::IndexOutOfBoundsExceptionFunction = 0;
llvm::Function* JavaRuntime::NegativeArraySizeExceptionFunction = 0;
llvm::Function* JavaRuntime::OutOfMemoryErrorFunction = 0;
llvm::Function* JavaRuntime::JavaObjectAllocateFunction = 0;
llvm::Function* JavaRuntime::InterfaceLookupFunction = 0;
llvm::Function* JavaRuntime::FieldLookupFunction = 0;
#ifndef WITHOUT_VTABLE
llvm::Function* JavaRuntime::VirtualLookupFunction = 0;
#endif
llvm::Function* JavaRuntime::PrintExecutionFunction = 0;
llvm::Function* JavaRuntime::PrintMethodStartFunction = 0;
llvm::Function* JavaRuntime::PrintMethodEndFunction = 0;
llvm::Function* JavaRuntime::JniProceedPendingExceptionFunction = 0;
llvm::Function* JavaRuntime::InitialisationCheckFunction = 0;
llvm::Function* JavaRuntime::ForceInitialisationCheckFunction = 0;
llvm::Function* JavaRuntime::ClassLookupFunction = 0;
llvm::Function* JavaRuntime::InstanceOfFunction = 0;
llvm::Function* JavaRuntime::IsAssignableFromFunction = 0;
llvm::Function* JavaRuntime::ImplementsFunction = 0;
llvm::Function* JavaRuntime::InstantiationOfArrayFunction = 0;
llvm::Function* JavaRuntime::GetDepthFunction = 0;
llvm::Function* JavaRuntime::GetDisplayFunction = 0;
llvm::Function* JavaRuntime::GetClassInDisplayFunction = 0;
llvm::Function* JavaRuntime::AquireObjectFunction = 0;
llvm::Function* JavaRuntime::ReleaseObjectFunction = 0;
llvm::Function* JavaRuntime::MultiCallNewFunction = 0;
llvm::Function* JavaRuntime::RuntimeUTF8ToStrFunction = 0;
llvm::Function* JavaRuntime::GetStaticInstanceFunction = 0;
llvm::Function* JavaRuntime::GetClassDelegateeFunction = 0;
llvm::Function* JavaRuntime::ArrayLengthFunction = 0;
llvm::Function* JavaRuntime::GetVTFunction = 0;
llvm::Function* JavaRuntime::GetClassFunction = 0;
llvm::Function* JavaRuntime::GetVTFromClassFunction = 0;
llvm::Function* JavaRuntime::GetObjectSizeFromClassFunction = 0;

#ifdef MULTIPLE_GC
llvm::Function* JavaRuntime::GetCollectorFunction = 0;
#endif

#ifdef SERVICE_VM
llvm::Function* JavaRuntime::AquireObjectInSharedDomainFunction = 0;
llvm::Function* JavaRuntime::ReleaseObjectInSharedDomainFunction = 0;
llvm::Function* JavaRuntime::ServiceCallStartFunction = 0;
llvm::Function* JavaRuntime::ServiceCallStopFunction = 0;
#endif

llvm::Function* JavaRuntime::GetThreadIDFunction = 0;
llvm::Function* JavaRuntime::GetLockFunction = 0;
llvm::Function* JavaRuntime::OverflowThinLockFunction = 0;



Value* LLVMCommonClassInfo::getVar(JavaJIT* jit) {
  if (!varGV) {
#ifdef MULTIPLE_VM
    if (jit->compilingClass->isolate->TheModule == Jnjvm::bootstrapVM->TheModule &&
        classDef->isArray && classDef->isolate != Jnjvm::bootstrapVM) {
      // We know the array class can belong to bootstrap
      CommonClass* cl = Jnjvm::bootstrapVM->constructArray(classDef->name, 0);
      return cl->isolate->TheModule->getClassInfo(cl)->getVar(jit);
    }
#endif
      
    Constant* cons = 
      ConstantExpr::getIntToPtr(ConstantInt::get(Type::Int64Ty,
                                                 uint64_t (classDef)),
                                JavaRuntime::JavaClassType);
      
    varGV = new GlobalVariable(JavaRuntime::JavaClassType, true,
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
                                JavaRuntime::JavaObjectType);
    delegateeGV = new GlobalVariable(JavaRuntime::JavaObjectType, true,
                                    GlobalValue::ExternalLinkage,
                                    cons, "",
                                    classDef->isolate->TheModule);
  }
  return new LoadInst(delegateeGV, "", jit->currentBlock);
#else
  Value* ld = getVar(jit);
  return llvm::CallInst::Create(JavaRuntime::GetClassDelegateeFunction, ld, "",
                                jit->currentBlock);
#endif
}

#ifndef WITHOUT_VTABLE
VirtualTable* JavaRuntime::allocateVT(Class* cl,
                                      CommonClass::method_iterator meths) {
  if (meths == cl->virtualMethods.end()) {
    uint64 size = cl->virtualTableSize;
    VirtualTable* VT = (VirtualTable*)malloc(size * sizeof(void*));
    if (!VT) JavaThread::get()->isolate->outOfMemoryError(size * sizeof(void*));
    if (cl->super) {
      Class* super = (Class*)cl->super;
      assert(cl->virtualTableSize >= cl->super->virtualTableSize &&
        "Super VT bigger than own VT");
      assert(super->virtualVT && "Super does not have a VT!");
      memcpy(VT, super->virtualVT, cl->super->virtualTableSize * sizeof(void*));
    } else {
      memcpy(VT, JavaObject::VT, VT_SIZE);
    }
    return VT;
  } else {
    JavaMethod* meth = meths->second;
    VirtualTable* VT = 0;
    if (meth->name->equals(Jnjvm::finalize)) {
      VT = allocateVT(cl, ++meths);
      meth->offset = 0;
      Function* func = cl->isolate->TheModuleProvider->parseFunction(meth);
      if (!cl->super) meth->canBeInlined = true;
      Function::iterator BB = func->begin();
      BasicBlock::iterator I = BB->begin();
      if (isa<ReturnInst>(I)) {
        ((void**)VT)[0] = 0;
      } else {
        ExecutionEngine* EE = mvm::jit::executionEngine;
        // LLVM does not allow recursive compilation. Create the code now.
        ((void**)VT)[0] = EE->getPointerToFunction(func);
      }
    } else {
    
      JavaMethod* parent = cl->super? 
        cl->super->lookupMethodDontThrow(meth->name, meth->type, false, true) :
        0;

      uint64_t offset = 0;
      if (!parent) {
        offset = cl->virtualTableSize++;
        meth->offset = offset;
      } else {
        offset = parent->offset;
        meth->offset = parent->offset;
      }
      VT = allocateVT(cl, ++meths);
      LLVMMethodInfo* LMI = getMethodInfo(meth);
      Function* func = LMI->getMethod();
      ExecutionEngine* EE = mvm::jit::executionEngine;
      ((void**)VT)[offset] = EE->getPointerToFunctionOrStub(func);
    }

    return VT;
  }
}
#endif


VirtualTable* JavaRuntime::makeVT(Class* cl, bool stat) {
  
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

      COPY(JavaObject)
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
 
  Function* func = Function::Create(JavaRuntime::MarkAndTraceType,
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
    CallInst::Create(JavaRuntime::JavaObjectTracer, Args.begin(), Args.end(),
                     "", block);
  } else {
    CallInst::Create(((Class*)cl->super)->virtualTracer, Args.begin(),
                     Args.end(), "", block);
  }
#else  
  if (stat || cl->super == 0) {
    CallInst::Create(JavaRuntime::JavaObjectTracerFunction, arg, "", block);
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
      Value* valCast = new BitCastInst(val, JavaRuntime::JavaObjectType, "",
                                       block);
#ifdef MULTIPLE_GC
      std::vector<Value*> Args;
      Args.push_back(valCast);
      Args.push_back(GC);
      CallInst::Create(JavaRuntime::MarkAndTraceFunction, Args.begin(),
                       Args.end(), "", block);
#else
      CallInst::Create(JavaRuntime::MarkAndTraceFunction, valCast, "", block);
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
    JavaField** array = 
      (JavaField**)alloca(sizeof(JavaField*) * classDef->virtualFields.size());
    
    if (classDef->super) {
      LLVMClassInfo* CLI = 
        (LLVMClassInfo*)JavaRuntime::getClassInfo(classDef->super);
      fields.push_back(CLI->getVirtualType()->getContainedType(0));
    } else {
      fields.push_back(JavaRuntime::JavaObjectType->getContainedType(0));
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
    
    VirtualTable* VT = JavaRuntime::makeVT((Class*)classDef, false);
  
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
    JavaField** array = (JavaField**)
      alloca(sizeof(JavaField*) * (classDef->staticFields.size() + 1));
    fields.push_back(JavaRuntime::JavaObjectType->getContainedType(0));

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
    

    VirtualTable* VT = JavaRuntime::makeVT((Class*)classDef, true);

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
                                uint64_t (obj)), JavaRuntime::JavaObjectType);
      
      staticVarGV = new GlobalVariable(JavaRuntime::JavaObjectType, true,
                                       GlobalValue::ExternalLinkage,
                                       cons, "",
                                       classDef->isolate->TheModule);
  }

  return new LoadInst(staticVarGV, "", jit->currentBlock);

#else
  Value* ld = getVar(jit);
  ld = jit->invoke(JavaRuntime::InitialisationCheckFunction, ld, "",
                   jit->currentBlock);
  return jit->invoke(JavaRuntime::GetStaticInstanceFunction, ld,
                     jit->isolateLocal, "", jit->currentBlock);
#endif
}

Value* LLVMClassInfo::getVirtualTable(JavaJIT* jit) {
  if (!virtualTableGV) {
    getVirtualType();
    Constant* cons = 
      ConstantExpr::getIntToPtr(ConstantInt::get(Type::Int64Ty,
                                                 uint64_t(classDef->virtualVT)),
                                JavaRuntime::VTType);
    virtualTableGV = new GlobalVariable(JavaRuntime::VTType, true,
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
    LLVMSignatureInfo* LSI = JavaRuntime::getSignatureInfo(methodDef->getSignature());
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
    JavaRuntime::resolveVirtualClass(methodDef->classDef);
    offsetConstant = ConstantInt::get(Type::Int32Ty, methodDef->offset);
  }
  return offsetConstant;
}

ConstantInt* LLVMFieldInfo::getOffset() {
  if (!offsetConstant) {
    if (isStatic(fieldDef->access)) {
      JavaRuntime::resolveStaticClass(fieldDef->classDef); 
    } else {
      JavaRuntime::resolveVirtualClass(fieldDef->classDef); 
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

    llvmArgs.push_back(JavaRuntime::JavaObjectType);

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
    llvmArgs.push_back(JavaRuntime::JavaObjectType); // Class

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
    Args2.push_back(JavaRuntime::JavaObjectType);
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
    signature->setVirtualCallBuf((intptr_t)
      mvm::jit::executionEngine->getPointerToGlobal(virtualBufFunction));
  }
  return virtualBufFunction;
}

Function* LLVMSignatureInfo::getVirtualAP() {
  if (!virtualAPFunction) {
    // Lock here because we are called by arbitrary code
    llvm::MutexGuard locked(mvm::jit::executionEngine->lock);
    virtualAPFunction = createFunctionCallAP(true);
    signature->setVirtualCallAP((intptr_t)
      mvm::jit::executionEngine->getPointerToGlobal(virtualAPFunction));
  }
  return virtualAPFunction;
}

Function* LLVMSignatureInfo::getStaticBuf() {
  if (!staticBufFunction) {
    // Lock here because we are called by arbitrary code
    llvm::MutexGuard locked(mvm::jit::executionEngine->lock);
    staticBufFunction = createFunctionCallBuf(false);
    signature->setStaticCallBuf((intptr_t)
      mvm::jit::executionEngine->getPointerToGlobal(staticBufFunction));
  }
  return staticBufFunction;
}

Function* LLVMSignatureInfo::getStaticAP() {
  if (!staticAPFunction) {
    // Lock here because we are called by arbitrary code
    llvm::MutexGuard locked(mvm::jit::executionEngine->lock);
    staticAPFunction = createFunctionCallAP(false);
    signature->setStaticCallAP((intptr_t)
      mvm::jit::executionEngine->getPointerToGlobal(staticAPFunction));
  }
  return staticAPFunction;
}

void JavaRuntime::resolveVirtualClass(Class* cl) {
  // Lock here because we may be called by a class resolver
  llvm::MutexGuard locked(mvm::jit::executionEngine->lock);
  LLVMClassInfo* LCI = (LLVMClassInfo*)getClassInfo(cl);
  LCI->getVirtualType();
}

void JavaRuntime::resolveStaticClass(Class* cl) {
  // Lock here because we may be called by a class initializer
  llvm::MutexGuard locked(mvm::jit::executionEngine->lock);
  LLVMClassInfo* LCI = (LLVMClassInfo*)getClassInfo(cl);
  LCI->getStaticType();
}

#ifdef SERVICE_VM
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


namespace jnjvm { 
  namespace llvm_runtime { 
    #include "LLVMRuntime.inc"
  }
}

void JavaRuntime::initialise(llvm::Module* module) {
  
  jnjvm::llvm_runtime::makeLLVMModuleContents(module);
  
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

  InterfaceLookupFunction = module->getFunction("jnjvmVirtualLookup");
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
  InstanceOfFunction = module->getFunction("instanceOf");
  IsAssignableFromFunction = module->getFunction("isAssignableFrom");
  ImplementsFunction = module->getFunction("implements");
  InstantiationOfArrayFunction = module->getFunction("instantiationOfArray");
  GetDepthFunction = module->getFunction("getDepth");
  GetDisplayFunction = module->getFunction("getDisplay");
  GetClassInDisplayFunction = module->getFunction("getClassInDisplay");
  AquireObjectFunction = module->getFunction("JavaObjectAquire");
  ReleaseObjectFunction = module->getFunction("JavaObjectRelease");
  OverflowThinLockFunction = module->getFunction("overflowThinLock");

  FieldLookupFunction = module->getFunction("fieldLookup");
  
  GetExceptionFunction = module->getFunction("JavaThreadGetException");
  GetJavaExceptionFunction = module->getFunction("JavaThreadGetJavaException");
  CompareExceptionFunction = module->getFunction("JavaThreadCompareException");
  JniProceedPendingExceptionFunction = 
    module->getFunction("jniProceedPendingException");
  GetSJLJBufferFunction = module->getFunction("getSJLJBuffer");
  
  NullPointerExceptionFunction = module->getFunction("jnjvmNullPointerException");
  ClassCastExceptionFunction = module->getFunction("jnjvmClassCastException");
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
  
  GetThreadIDFunction = module->getFunction("getThreadID");
  GetLockFunction = module->getFunction("getLock");
  
  UTF8NullConstant = Constant::getNullValue(JavaArrayUInt16Type); 
  JavaClassNullConstant = Constant::getNullValue(JavaClassType); 
  JavaObjectNullConstant = Constant::getNullValue(JavaRuntime::JavaObjectType);
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
  
  JavaArrayElementsOffsetConstant = mvm::jit::constantTwo;
  JavaArraySizeOffsetConstant = mvm::jit::constantOne;
  JavaObjectLockOffsetConstant = mvm::jit::constantTwo;
  JavaObjectClassOffsetConstant = mvm::jit::constantOne; 
  
  OffsetObjectSizeInClassConstant = mvm::jit::constantOne;
  OffsetVTInClassConstant = mvm::jit::constantTwo;
  OffsetDisplayInClassConstant = mvm::jit::constantThree;
  OffsetDepthInClassConstant = mvm::jit::constantFour;

  LLVMAssessorInfo::initialise();
}

void JavaRuntime::InitField(JavaField* field, JavaObject* obj, uint64 val) {
  
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

void JavaRuntime::InitField(JavaField* field, JavaObject* obj, JavaObject* val) {
  ((JavaObject**)((uint64)obj + field->ptrOffset))[0] = val;
}

void JavaRuntime::InitField(JavaField* field, JavaObject* obj, double val) {
  ((double*)((uint64)obj + field->ptrOffset))[0] = val;
}

void JavaRuntime::InitField(JavaField* field, JavaObject* obj, float val) {
  ((float*)((uint64)obj + field->ptrOffset))[0] = val;
}

void JavaRuntime::setMethod(JavaMethod* meth, const char* name) {
  llvm::Function* func = getMethodInfo(meth)->getMethod();
  func->setName(name);
  func->setLinkage(llvm::GlobalValue::ExternalLinkage);
}

void* JavaRuntime::getMethod(JavaMethod* meth) {
  return getMethodInfo(meth)->getMethod();
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
  
  AssessorInfo[ARRAY_ID].llvmType = JavaRuntime::JavaObjectType;
  AssessorInfo[ARRAY_ID].llvmTypePtr =
    PointerType::getUnqual(JavaRuntime::JavaObjectType);
  AssessorInfo[ARRAY_ID].llvmNullConstant =
    JavaRuntime::JavaObjectNullConstant;
  AssessorInfo[ARRAY_ID].sizeInBytesConstant = mvm::jit::constantPtrSize;
  
  AssessorInfo[OBJECT_ID].llvmType = JavaRuntime::JavaObjectType;
  AssessorInfo[OBJECT_ID].llvmTypePtr =
    PointerType::getUnqual(JavaRuntime::JavaObjectType);
  AssessorInfo[OBJECT_ID].llvmNullConstant =
    JavaRuntime::JavaObjectNullConstant;
  AssessorInfo[OBJECT_ID].sizeInBytesConstant = mvm::jit::constantPtrSize;
}

LLVMAssessorInfo LLVMAssessorInfo::AssessorInfo[NUM_ASSESSORS];
