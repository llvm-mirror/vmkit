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
#include "JavaThread.h"
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
const llvm::Type* JnjvmModule::JnjvmType = 0;
const llvm::Type* JnjvmModule::ConstantPoolType = 0;

llvm::Constant*       JnjvmModule::JavaObjectNullConstant;
llvm::Constant*       JnjvmModule::UTF8NullConstant;
llvm::Constant*       JnjvmModule::JavaClassNullConstant;
llvm::Constant*       JnjvmModule::MaxArraySizeConstant;
llvm::Constant*       JnjvmModule::JavaObjectSizeConstant;
llvm::GlobalVariable* JnjvmModule::JavaObjectVirtualTableGV;
llvm::GlobalVariable* JnjvmModule::ArrayObjectVirtualTableGV;
llvm::ConstantInt*    JnjvmModule::OffsetObjectSizeInClassConstant;
llvm::ConstantInt*    JnjvmModule::OffsetVTInClassConstant;
llvm::ConstantInt*    JnjvmModule::OffsetDepthInClassConstant;
llvm::ConstantInt*    JnjvmModule::OffsetDisplayInClassConstant;
llvm::ConstantInt*    JnjvmModule::OffsetStatusInClassConstant;
llvm::ConstantInt*    JnjvmModule::OffsetCtpInClassConstant;
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
llvm::Function* JnjvmModule::StaticFieldLookupFunction = 0;
llvm::Function* JnjvmModule::VirtualFieldLookupFunction = 0;
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
llvm::Function* JnjvmModule::IsAssignableFromFunction = 0;
llvm::Function* JnjvmModule::ImplementsFunction = 0;
llvm::Function* JnjvmModule::InstantiationOfArrayFunction = 0;
llvm::Function* JnjvmModule::GetDepthFunction = 0;
llvm::Function* JnjvmModule::GetDisplayFunction = 0;
llvm::Function* JnjvmModule::GetClassInDisplayFunction = 0;
llvm::Function* JnjvmModule::AquireObjectFunction = 0;
llvm::Function* JnjvmModule::ReleaseObjectFunction = 0;
llvm::Function* JnjvmModule::MultiCallNewFunction = 0;
llvm::Function* JnjvmModule::GetConstantPoolAtFunction = 0;

#ifdef MULTIPLE_VM
llvm::Function* JnjvmModule::StringLookupFunction = 0;
llvm::Function* JnjvmModule::GetCtpCacheNodeFunction = 0;
llvm::Function* JnjvmModule::GetCtpClassFunction = 0;
llvm::Function* JnjvmModule::EnveloppeLookupFunction = 0;
llvm::Function* JnjvmModule::GetJnjvmExceptionClassFunction = 0;
llvm::Function* JnjvmModule::GetJnjvmArrayClassFunction = 0;
llvm::Function* JnjvmModule::StaticCtpLookupFunction = 0;
llvm::Function* JnjvmModule::GetArrayClassFunction = 0;
#endif
llvm::Function* JnjvmModule::GetClassDelegateeFunction = 0;
llvm::Function* JnjvmModule::ArrayLengthFunction = 0;
llvm::Function* JnjvmModule::GetVTFunction = 0;
llvm::Function* JnjvmModule::GetClassFunction = 0;
llvm::Function* JnjvmModule::GetVTFromClassFunction = 0;
llvm::Function* JnjvmModule::GetObjectSizeFromClassFunction = 0;

#ifdef MULTIPLE_GC
llvm::Function* JnjvmModule::GetCollectorFunction = 0;
#endif

#ifdef SERVICE_VM
llvm::Function* JnjvmModule::AquireObjectInSharedDomainFunction = 0;
llvm::Function* JnjvmModule::ReleaseObjectInSharedDomainFunction = 0;
llvm::Function* JnjvmModule::ServiceCallStartFunction = 0;
llvm::Function* JnjvmModule::ServiceCallStopFunction = 0;
#endif

llvm::Function* JnjvmModule::GetThreadIDFunction = 0;
llvm::Function* JnjvmModule::GetLockFunction = 0;
llvm::Function* JnjvmModule::OverflowThinLockFunction = 0;



Value* LLVMCommonClassInfo::getVar(JavaJIT* jit) {
  if (!varGV) {
      
    Constant* cons = 
      ConstantExpr::getIntToPtr(ConstantInt::get(Type::Int64Ty,
                                                 uint64_t (classDef)),
                                JnjvmModule::JavaClassType);
      
    varGV = new GlobalVariable(JnjvmModule::JavaClassType, true,
                               GlobalValue::ExternalLinkage,
                               cons, "",
                               classDef->classLoader->TheModule);
  }
  return new LoadInst(varGV, "", jit->currentBlock);
}

Value* LLVMConstantPoolInfo::getDelegatee(JavaJIT* jit) {
  if (!delegateeGV) {
    void* ptr = ctp->ctpRes;
    Constant* cons = 
      ConstantExpr::getIntToPtr(ConstantInt::get(Type::Int64Ty, uint64(ptr)),
                                mvm::jit::ptrPtrType);
    delegateeGV = new GlobalVariable(mvm::jit::ptrPtrType, true,
                                     GlobalValue::ExternalLinkage,
                                     cons, "",
                                     ctp->classDef->classLoader->TheModule);
  }
  return new LoadInst(delegateeGV, "", jit->currentBlock);
}

Value* LLVMCommonClassInfo::getDelegatee(JavaJIT* jit) {
#ifndef MULTIPLE_VM
  if (!delegateeGV) {
    JavaObject* obj = classDef->getClassDelegatee(JavaThread::get()->isolate);
    Constant* cons = 
      ConstantExpr::getIntToPtr(ConstantInt::get(Type::Int64Ty, uint64(obj)),
                                JnjvmModule::JavaObjectType);
    delegateeGV = new GlobalVariable(JnjvmModule::JavaObjectType, true,
                                    GlobalValue::ExternalLinkage,
                                    cons, "",
                                    classDef->classLoader->TheModule);
  }
  return new LoadInst(delegateeGV, "", jit->currentBlock);
#else
  fprintf(stderr, "implement me\n");
  abort();
#endif
}

#ifndef WITHOUT_VTABLE
VirtualTable* JnjvmModule::allocateVT(Class* cl,
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
      Function* func = cl->classLoader->TheModuleProvider->parseFunction(meth);
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
    
      Class* methodCl = 0;
      JavaMethod* parent = cl->super? 
        cl->super->lookupMethodDontThrow(meth->name, meth->type, false, true,
                                         methodCl) :
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


VirtualTable* JnjvmModule::makeVT(Class* cl, bool stat) {
  
  VirtualTable* res = 0;
#ifndef WITHOUT_VTABLE
  if (stat) {
#endif
    res = (VirtualTable*)malloc(VT_SIZE);
    memcpy(res, JavaObject::VT, VT_SIZE);
#ifndef WITHOUT_VTABLE
  } else {
    if (cl->super) {
      cl->virtualTableSize = cl->super->virtualTableSize;
    } else {
      cl->virtualTableSize = VT_NB_FUNCS;
    }
    res = allocateVT(cl, cl->virtualMethods.begin());
  
    if (!(cl->super)) {
      uint32 size =  (cl->virtualTableSize - VT_NB_FUNCS) * sizeof(void*);
#define COPY(CLASS) \
    memcpy((void*)((unsigned)CLASS::VT + VT_SIZE), \
           (void*)((unsigned)res + VT_SIZE), size);

      COPY(JavaArray)
      COPY(JavaObject)
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
                                    cl->classLoader->TheModule);

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
    JavaField** array = 
      (JavaField**)alloca(sizeof(JavaField*) * classDef->virtualFields.size());
    
    if (classDef->super) {
      LLVMClassInfo* CLI = 
        (LLVMClassInfo*)JnjvmModule::getClassInfo(classDef->super);
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
    
    VirtualTable* VT = JnjvmModule::makeVT((Class*)classDef, false);
  
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
    

    VirtualTable* VT = JnjvmModule::makeVT((Class*)classDef, true);

    uint64 size = mvm::jit::getTypeSize(structType);
    cl->staticSize = size;
    cl->staticVT = VT;
  }
  return staticType;
}

#ifndef MULTIPLE_VM
Value* LLVMClassInfo::getStaticVar(JavaJIT* jit) {
  if (!staticVarGV) {
    getStaticType();
    JavaObject* obj = ((Class*)classDef)->getStaticInstance();
    Constant* cons = 
      ConstantExpr::getIntToPtr(ConstantInt::get(Type::Int64Ty,
                                uint64_t (obj)), JnjvmModule::JavaObjectType);
      
      staticVarGV = new GlobalVariable(JnjvmModule::JavaObjectType, true,
                                       GlobalValue::ExternalLinkage,
                                       cons, "",
                                       classDef->classLoader->TheModule);
  }

  return new LoadInst(staticVarGV, "", jit->currentBlock);
}
#endif

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
                                        classDef->classLoader->TheModule);
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
    JnjvmClassLoader* JCL = methodDef->classDef->classLoader;
    methodFunction = Function::Create(getFunctionType(), 
                                      GlobalValue::GhostLinkage,
                                      methodDef->printString(),
                                      JCL->TheModule);
    JCL->TheModuleProvider->addFunction(methodFunction, methodDef);
  }
  return methodFunction;
}

const FunctionType* LLVMMethodInfo::getFunctionType() {
  if (!functionType) {
    LLVMSignatureInfo* LSI = JnjvmModule::getSignatureInfo(methodDef->getSignature());
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
    JnjvmModule::resolveVirtualClass(methodDef->classDef);
    offsetConstant = ConstantInt::get(Type::Int32Ty, methodDef->offset);
  }
  return offsetConstant;
}

ConstantInt* LLVMFieldInfo::getOffset() {
  if (!offsetConstant) {
    if (isStatic(fieldDef->access)) {
      JnjvmModule::resolveStaticClass(fieldDef->classDef); 
    } else {
      JnjvmModule::resolveVirtualClass(fieldDef->classDef); 
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

#if defined(MULTIPLE_VM)
    llvmArgs.push_back(JnjvmModule::JnjvmType); // vm
    llvmArgs.push_back(JnjvmModule::ConstantPoolType); // cached constant pool
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

#if defined(MULTIPLE_VM)
    llvmArgs.push_back(JnjvmModule::JnjvmType); // vm
    llvmArgs.push_back(JnjvmModule::ConstantPoolType); // cached constant pool
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

#if defined(MULTIPLE_VM)
    llvmArgs.push_back(JnjvmModule::JnjvmType); // vm
    llvmArgs.push_back(JnjvmModule::ConstantPoolType); // cached constant pool
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
                                   signature->initialLoader->TheModule);
  
  BasicBlock* currentBlock = BasicBlock::Create("enter", res);
  Function::arg_iterator i = res->arg_begin();
  Value *obj, *ptr, *func;
#if defined(MULTIPLE_VM)
  Value* vm = i;
#endif
  ++i;
#if defined(MULTIPLE_VM)
  Value* ctp = i;
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

#if defined(MULTIPLE_VM)
  Args.push_back(vm);
  Args.push_back(ctp);
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
                                      signature->initialLoader->TheModule);
  
  BasicBlock* currentBlock = BasicBlock::Create("enter", res);
  Function::arg_iterator i = res->arg_begin();
  Value *obj, *ap, *func;
#if defined(MULTIPLE_VM)
  Value* vm = i;
#endif
  ++i;
#if defined(MULTIPLE_VM)
  Value* ctp = i;
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

#if defined(MULTIPLE_VM)
  Args.push_back(vm);
  Args.push_back(ctp);
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
    Args2.push_back(JnjvmModule::JnjvmType); // vm
    Args2.push_back(JnjvmModule::ConstantPoolType); // ctp
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
    Args.push_back(JnjvmModule::JnjvmType); // vm
    Args.push_back(JnjvmModule::ConstantPoolType); // ctp
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


LLVMStringInfo* JnjvmModule::getStringInfo(JavaString* str) {
  string_iterator SI = stringMap.find(str);
  if (SI != stringMap.end()) {
    return SI->second;
  } else {
    LLVMStringInfo* LSI = new LLVMStringInfo(str);
    stringMap.insert(std::make_pair(str, LSI));
    return LSI; 
  }
}

Value* LLVMStringInfo::getDelegatee(JavaJIT* jit) {
  if (!delegateeGV) {
    void* ptr = str;
    Constant* cons = 
      ConstantExpr::getIntToPtr(ConstantInt::get(Type::Int64Ty, uint64(ptr)),
                                JnjvmModule::JavaObjectType);
    delegateeGV = new GlobalVariable(JnjvmModule::JavaObjectType, true,
                                     GlobalValue::ExternalLinkage,
                                     cons, "",
                                     jit->module);
  }
  return new LoadInst(delegateeGV, "", jit->currentBlock);
}


namespace jnjvm { 
  namespace llvm_runtime { 
    #include "LLVMRuntime.inc"
  }
}

void JnjvmModule::initialise() {
  Module* module = this;
  jnjvm::llvm_runtime::makeLLVMModuleContents(module);
  
  VTType = module->getTypeByName("VT");

  JnjvmType = 
    PointerType::getUnqual(module->getTypeByName("Jnjvm"));
  ConstantPoolType = 
    PointerType::getUnqual(module->getTypeByName("ConstantPool"));
  
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
  
  GetConstantPoolAtFunction = module->getFunction("getConstantPoolAt");
  ArrayLengthFunction = module->getFunction("arrayLength");
  GetVTFunction = module->getFunction("getVT");
  GetClassFunction = module->getFunction("getClass");
  ClassLookupFunction = module->getFunction("classLookup");
  GetVTFromClassFunction = module->getFunction("getVTFromClass");
  GetObjectSizeFromClassFunction = 
    module->getFunction("getObjectSizeFromClass");
 
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

  VirtualFieldLookupFunction = module->getFunction("virtualFieldLookup");
  StaticFieldLookupFunction = module->getFunction("staticFieldLookup");
  
  GetExceptionFunction = module->getFunction("JavaThreadGetException");
  GetJavaExceptionFunction = module->getFunction("JavaThreadGetJavaException");
  CompareExceptionFunction = module->getFunction("JavaThreadCompareException");
  JniProceedPendingExceptionFunction = 
    module->getFunction("jniProceedPendingException");
  GetSJLJBufferFunction = module->getFunction("getSJLJBuffer");
  
  NullPointerExceptionFunction =
    module->getFunction("jnjvmNullPointerException");
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
  StringLookupFunction = module->getFunction("stringLookup");
  EnveloppeLookupFunction = module->getFunction("enveloppeLookup");
  GetCtpCacheNodeFunction = module->getFunction("getCtpCacheNode");
  GetCtpClassFunction = module->getFunction("getCtpClass");
  GetJnjvmExceptionClassFunction = 
    module->getFunction("getJnjvmExceptionClass");
  GetJnjvmArrayClassFunction = module->getFunction("getJnjvmArrayClass");
  StaticCtpLookupFunction = module->getFunction("staticCtpLookup");
  GetArrayClassFunction = module->getFunction("getArrayClass");
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
  
  JavaArrayElementsOffsetConstant = mvm::jit::constantTwo;
  JavaArraySizeOffsetConstant = mvm::jit::constantOne;
  JavaObjectLockOffsetConstant = mvm::jit::constantTwo;
  JavaObjectClassOffsetConstant = mvm::jit::constantOne; 
  
  OffsetObjectSizeInClassConstant = mvm::jit::constantOne;
  OffsetVTInClassConstant = mvm::jit::constantTwo;
  OffsetDisplayInClassConstant = mvm::jit::constantThree;
  OffsetDepthInClassConstant = mvm::jit::constantFour;
  OffsetStatusInClassConstant = mvm::jit::constantFive;
  OffsetCtpInClassConstant = mvm::jit::constantSix;

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
