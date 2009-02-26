//===--------- JnjvmModule.cpp - Definition of a Jnjvm module -------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/BasicBlock.h"
#include "llvm/CallingConv.h"
#include "llvm/Constants.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/Instructions.h"
#include "llvm/Support/MutexGuard.h"
#include "llvm/Target/TargetData.h"


#include "mvm/JIT.h"

#include "JavaCache.h"
#include "JavaConstantPool.h"
#include "JavaJIT.h"
#include "JavaString.h"
#include "JavaThread.h"
#include "JavaTypes.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"
#include "Reader.h"

#include "jnjvm/JnjvmModule.h"
#include "jnjvm/JnjvmModuleProvider.h"

#include <cstdio>

using namespace jnjvm;
using namespace llvm;

const Type* LLVMClassInfo::getVirtualType() {
  if (!virtualType) {
    std::vector<const llvm::Type*> fields;
    
    if (classDef->super && classDef->super->super) {
      LLVMClassInfo* CLI = 
        JnjvmModule::getClassInfo((Class*)classDef->super);
      fields.push_back(CLI->getVirtualType()->getContainedType(0));
    } else {
      fields.push_back(JnjvmModule::JavaObjectType->getContainedType(0));
    }
    
    for (uint32 i = 0; i < classDef->nbVirtualFields; ++i) {
      JavaField& field = classDef->virtualFields[i];
      field.num = i + 1;
      Typedef* type = field.getSignature();
      LLVMAssessorInfo& LAI = JnjvmModule::getTypedefInfo(type);
      fields.push_back(LAI.llvmType);
    }
    
    
    JnjvmModule* Mod = classDef->classLoader->getModule();
    StructType* structType = StructType::get(fields, false);
    virtualType = PointerType::getUnqual(structType);
    const TargetData* targetData = Mod->TheTargetData;
    const StructLayout* sl = targetData->getStructLayout(structType);
    
    for (uint32 i = 0; i < classDef->nbVirtualFields; ++i) {
      JavaField& field = classDef->virtualFields[i];
      field.ptrOffset = sl->getElementOffset(i + 1);
    }
    
    uint64 size = Mod->getTypeSize(structType);
    classDef->virtualSize = (uint32)size;
    virtualSizeConstant = ConstantInt::get(Type::Int32Ty, size);
    
    if (!Mod->isStaticCompiling()) {
      if (!classDef->virtualVT) {
        Mod->makeVT((Class*)classDef);
      } else {
#ifdef WITH_TRACER
        // So the class is vmjc'ed. Create the virtual tracer.
        Function* func = Function::Create(JnjvmModule::MarkAndTraceType,
                                          GlobalValue::ExternalLinkage,
                                          "markAndTraceObject",
                                          Mod->getLLVMModule());
       
        void* ptr = ((void**)classDef->virtualVT)[VT_TRACER_OFFSET];
        Mod->executionEngine->addGlobalMapping(func, ptr);
        virtualTracerFunction = func;
#endif
      }
    } else {
      Mod->makeVT(classDef);
    }
  
  }

  return virtualType;
}

const Type* LLVMClassInfo::getStaticType() {
  
  if (!staticType) {
    Class* cl = (Class*)classDef;
    std::vector<const llvm::Type*> fields;

    for (uint32 i = 0; i < classDef->nbStaticFields; ++i) {
      JavaField& field = classDef->staticFields[i];
      field.num = i;
      Typedef* type = field.getSignature();
      LLVMAssessorInfo& LAI = JnjvmModule::getTypedefInfo(type);
      fields.push_back(LAI.llvmType);
    }
  
    JnjvmModule* Mod = cl->classLoader->getModule();
    StructType* structType = StructType::get(fields, false);
    staticType = PointerType::getUnqual(structType);
    const TargetData* targetData = Mod->TheTargetData;
    const StructLayout* sl = targetData->getStructLayout(structType);
    
    for (uint32 i = 0; i < classDef->nbStaticFields; ++i) {
      JavaField& field = classDef->staticFields[i];
      field.ptrOffset = sl->getElementOffset(i);
    }
    
    uint64 size = Mod->getTypeSize(structType);
    cl->staticSize = size;
#ifdef WITH_TRACER
    if (!Mod->isStaticCompiling()) {
      Function* F = Mod->makeTracer(cl, true);
      cl->staticTracer = (void (*)(void*)) (uintptr_t)
        Mod->executionEngine->getPointerToFunction(F);
      F->deleteBody();
    }
#endif
  }
  return staticType;
}


Value* LLVMClassInfo::getVirtualSize() {
  if (!virtualSizeConstant) {
    getVirtualType();
    assert(classDef->virtualSize && "Zero size for a class?");
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
    JnjvmModule* Mod = JCL->getModule();
    if (Mod->isStaticCompiling()) {

      const UTF8* jniConsClName = methodDef->classDef->name;
      const UTF8* jniConsName = methodDef->name;
      const UTF8* jniConsType = methodDef->type;
      sint32 clen = jniConsClName->size;
      sint32 mnlen = jniConsName->size;
      sint32 mtlen = jniConsType->size;

      char* buf = (char*)alloca(3 + JNI_NAME_PRE_LEN + 1 +
                                ((mnlen + clen + mtlen) << 1));
      
      bool jnjvm = false;
      if (isNative(methodDef->access)) {
        // Verify if it's defined by JnJVM
        JCL->nativeLookup(methodDef, jnjvm, buf);
      }

      if (!jnjvm) {
        methodDef->jniConsFromMethOverloaded(buf + 1);
        memcpy(buf, "JnJVM", 5);
      }

      methodFunction = Function::Create(getFunctionType(), 
                                        GlobalValue::GhostLinkage, buf,
                                        Mod->getLLVMModule());

    } else {

      methodFunction = Function::Create(getFunctionType(), 
                                        GlobalValue::GhostLinkage,
                                        "", Mod->getLLVMModule());

    }
    methodFunction->addAnnotation(this);
  }
  return methodFunction;
}

const FunctionType* LLVMMethodInfo::getFunctionType() {
  if (!functionType) {
    Signdef* sign = methodDef->getSignature();
    LLVMSignatureInfo* LSI = JnjvmModule::getSignatureInfo(sign);
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
    
    offsetConstant = ConstantInt::get(Type::Int32Ty, fieldDef->num);
  }
  return offsetConstant;
}

const llvm::FunctionType* LLVMSignatureInfo::getVirtualType() {
 if (!virtualType) {
    // Lock here because we are called by arbitrary code
    mvm::MvmModule::protectIR();
    std::vector<const llvm::Type*> llvmArgs;
    uint32 size = signature->nbArguments;
    Typedef* const* arguments = signature->getArgumentsType();

    llvmArgs.push_back(JnjvmModule::JavaObjectType);

    for (uint32 i = 0; i < size; ++i) {
      Typedef* type = arguments[i];
      LLVMAssessorInfo& LAI = JnjvmModule::getTypedefInfo(type);
      llvmArgs.push_back(LAI.llvmType);
    }

#if defined(ISOLATE_SHARING)
    llvmArgs.push_back(JnjvmModule::ConstantPoolType); // cached constant pool
#endif

    LLVMAssessorInfo& LAI = 
      JnjvmModule::getTypedefInfo(signature->getReturnType());
    virtualType = FunctionType::get(LAI.llvmType, llvmArgs, false);
    mvm::MvmModule::unprotectIR();
  }
  return virtualType;
}

const llvm::FunctionType* LLVMSignatureInfo::getStaticType() {
 if (!staticType) {
    // Lock here because we are called by arbitrary code
    mvm::MvmModule::protectIR();
    std::vector<const llvm::Type*> llvmArgs;
    uint32 size = signature->nbArguments;
    Typedef* const* arguments = signature->getArgumentsType();

    for (uint32 i = 0; i < size; ++i) {
      Typedef* type = arguments[i];
      LLVMAssessorInfo& LAI = JnjvmModule::getTypedefInfo(type);
      llvmArgs.push_back(LAI.llvmType);
    }

#if defined(ISOLATE_SHARING)
    llvmArgs.push_back(JnjvmModule::ConstantPoolType); // cached constant pool
#endif

    LLVMAssessorInfo& LAI = 
      JnjvmModule::getTypedefInfo(signature->getReturnType());
    staticType = FunctionType::get(LAI.llvmType, llvmArgs, false);
    mvm::MvmModule::unprotectIR();
  }
  return staticType;
}

const llvm::FunctionType* LLVMSignatureInfo::getNativeType() {
  if (!nativeType) {
    // Lock here because we are called by arbitrary code
    mvm::MvmModule::protectIR();
    std::vector<const llvm::Type*> llvmArgs;
    uint32 size = signature->nbArguments;
    Typedef* const* arguments = signature->getArgumentsType();
    
    llvmArgs.push_back(mvm::MvmModule::ptrType); // JNIEnv
    llvmArgs.push_back(JnjvmModule::JavaObjectType); // Class

    for (uint32 i = 0; i < size; ++i) {
      Typedef* type = arguments[i];
      LLVMAssessorInfo& LAI = JnjvmModule::getTypedefInfo(type);
      llvmArgs.push_back(LAI.llvmType);
    }

#if defined(ISOLATE_SHARING)
    llvmArgs.push_back(JnjvmModule::ConstantPoolType); // cached constant pool
#endif

    LLVMAssessorInfo& LAI = 
      JnjvmModule::getTypedefInfo(signature->getReturnType());
    nativeType = FunctionType::get(LAI.llvmType, llvmArgs, false);
    mvm::MvmModule::unprotectIR();
  }
  return nativeType;
}


Function* LLVMSignatureInfo::createFunctionCallBuf(bool virt) {
  
  std::vector<Value*> Args;

  JnjvmModule* Mod = signature->initialLoader->getModule();
  const char* name = 0;
  if (Mod->isStaticCompiling()) {
    name = virt ? signature->printString("virtual_buf") :
                  signature->printString("static_buf");
  } else {
    name = "";
  }

  Function* res = Function::Create(virt ? getVirtualBufType() : 
                                          getStaticBufType(),
                                   GlobalValue::InternalLinkage, name,
                                   Mod->getLLVMModule());
  
  BasicBlock* currentBlock = BasicBlock::Create("enter", res);
  Function::arg_iterator i = res->arg_begin();
  Value *obj, *ptr, *func;
#if defined(ISOLATE_SHARING)
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
  
  Typedef* const* arguments = signature->getArgumentsType();
  for (uint32 i = 0; i < signature->nbArguments; ++i) {
  
    LLVMAssessorInfo& LAI = JnjvmModule::getTypedefInfo(arguments[i]);
    Value* val = new BitCastInst(ptr, LAI.llvmTypePtr, "", currentBlock);
    Value* arg = new LoadInst(val, "", currentBlock);
    Args.push_back(arg);
    ptr = GetElementPtrInst::Create(ptr, JnjvmModule::constantEight, "",
                                    currentBlock);
  }

#if defined(ISOLATE_SHARING)
  Args.push_back(ctp);
#endif

  Value* val = CallInst::Create(func, Args.begin(), Args.end(), "",
                                currentBlock);
  if (res->getFunctionType()->getReturnType() != Type::VoidTy)
    ReturnInst::Create(val, currentBlock);
  else
    ReturnInst::Create(currentBlock);
  
  return res;
}

Function* LLVMSignatureInfo::createFunctionCallAP(bool virt) {
  
  std::vector<Value*> Args;
  
  JnjvmModule* Mod = signature->initialLoader->getModule();
  const char* name = 0;
  if (Mod->isStaticCompiling()) {
    name = virt ? signature->printString("virtual_ap") :
                  signature->printString("static_ap");
  } else {
    name = "";
  }

  Function* res = Function::Create(virt ? getVirtualBufType() :
                                          getStaticBufType(),
                                   GlobalValue::InternalLinkage, name,
                                   Mod->getLLVMModule());
  
  BasicBlock* currentBlock = BasicBlock::Create("enter", res);
  Function::arg_iterator i = res->arg_begin();
  Value *obj, *ap, *func;
#if defined(ISOLATE_SHARING)
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

  Typedef* const* arguments = signature->getArgumentsType();
  for (uint32 i = 0; i < signature->nbArguments; ++i) {
    LLVMAssessorInfo& LAI = JnjvmModule::getTypedefInfo(arguments[i]);
    Args.push_back(new VAArgInst(ap, LAI.llvmType, "", currentBlock));
  }

#if defined(ISOLATE_SHARING)
  Args.push_back(ctp);
#endif

  Value* val = CallInst::Create(func, Args.begin(), Args.end(), "",
                                currentBlock);
  if (res->getFunctionType()->getReturnType() != Type::VoidTy)
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
    mvm::MvmModule::protectIR();
    std::vector<const llvm::Type*> Args2;
    Args2.push_back(JnjvmModule::ConstantPoolType); // ctp
    Args2.push_back(getVirtualPtrType());
    Args2.push_back(JnjvmModule::JavaObjectType);
    Args2.push_back(JnjvmModule::ptrType);
    LLVMAssessorInfo& LAI = 
      JnjvmModule::getTypedefInfo(signature->getReturnType());
    virtualBufType = FunctionType::get(LAI.llvmType, Args2, false);
    mvm::MvmModule::unprotectIR();
  }
  return virtualBufType;
}

const FunctionType* LLVMSignatureInfo::getStaticBufType() {
  if (!staticBufType) {
    // Lock here because we are called by arbitrary code
    mvm::MvmModule::protectIR();
    std::vector<const llvm::Type*> Args;
    Args.push_back(JnjvmModule::ConstantPoolType); // ctp
    Args.push_back(getStaticPtrType());
    Args.push_back(JnjvmModule::ptrType);
    LLVMAssessorInfo& LAI = 
      JnjvmModule::getTypedefInfo(signature->getReturnType());
    staticBufType = FunctionType::get(LAI.llvmType, Args, false);
    mvm::MvmModule::unprotectIR();
  }
  return staticBufType;
}

Function* LLVMSignatureInfo::getVirtualBuf() {
  if (!virtualBufFunction) {
    // Lock here because we are called by arbitrary code
    mvm::MvmModule::protectIR();
    virtualBufFunction = createFunctionCallBuf(true);
    if (!signature->initialLoader->getModule()->isStaticCompiling()) {
      signature->setVirtualCallBuf((intptr_t)
        mvm::MvmModule::executionEngine->getPointerToGlobal(virtualBufFunction));
      // Now that it's compiled, we don't need the IR anymore
      virtualBufFunction->deleteBody();
    }
    mvm::MvmModule::unprotectIR();
  }
  return virtualBufFunction;
}

Function* LLVMSignatureInfo::getVirtualAP() {
  if (!virtualAPFunction) {
    // Lock here because we are called by arbitrary code
    mvm::MvmModule::protectIR();
    virtualAPFunction = createFunctionCallAP(true);
    if (!signature->initialLoader->getModule()->isStaticCompiling()) {
      signature->setVirtualCallAP((intptr_t)
        mvm::MvmModule::executionEngine->getPointerToGlobal(virtualAPFunction));
      // Now that it's compiled, we don't need the IR anymore
      virtualAPFunction->deleteBody();
    }
    mvm::MvmModule::unprotectIR();
  }
  return virtualAPFunction;
}

Function* LLVMSignatureInfo::getStaticBuf() {
  if (!staticBufFunction) {
    // Lock here because we are called by arbitrary code
    mvm::MvmModule::protectIR();
    staticBufFunction = createFunctionCallBuf(false);
    if (!signature->initialLoader->getModule()->isStaticCompiling()) {
      signature->setStaticCallBuf((intptr_t)
        mvm::MvmModule::executionEngine->getPointerToGlobal(staticBufFunction));
      // Now that it's compiled, we don't need the IR anymore
      staticBufFunction->deleteBody();
    }
    mvm::MvmModule::unprotectIR();
  }
  return staticBufFunction;
}

Function* LLVMSignatureInfo::getStaticAP() {
  if (!staticAPFunction) {
    // Lock here because we are called by arbitrary code
    mvm::MvmModule::protectIR();
    staticAPFunction = createFunctionCallAP(false);
    if (!signature->initialLoader->getModule()->isStaticCompiling()) {
      signature->setStaticCallAP((intptr_t)
        mvm::MvmModule::executionEngine->getPointerToGlobal(staticAPFunction));
      // Now that it's compiled, we don't need the IR anymore
      staticAPFunction->deleteBody();
    }
    mvm::MvmModule::unprotectIR();
  }
  return staticAPFunction;
}

void LLVMAssessorInfo::initialise() {
  AssessorInfo[I_VOID].llvmType = Type::VoidTy;
  AssessorInfo[I_VOID].llvmTypePtr = 0;
  AssessorInfo[I_VOID].llvmNullConstant = 0;
  AssessorInfo[I_VOID].sizeInBytesConstant = 0;
  
  AssessorInfo[I_BOOL].llvmType = Type::Int8Ty;
  AssessorInfo[I_BOOL].llvmTypePtr = PointerType::getUnqual(Type::Int8Ty);
  AssessorInfo[I_BOOL].llvmNullConstant = 
    Constant::getNullValue(Type::Int8Ty);
  AssessorInfo[I_BOOL].sizeInBytesConstant = mvm::MvmModule::constantOne;
  
  AssessorInfo[I_BYTE].llvmType = Type::Int8Ty;
  AssessorInfo[I_BYTE].llvmTypePtr = PointerType::getUnqual(Type::Int8Ty);
  AssessorInfo[I_BYTE].llvmNullConstant = 
    Constant::getNullValue(Type::Int8Ty);
  AssessorInfo[I_BYTE].sizeInBytesConstant = mvm::MvmModule::constantOne;
  
  AssessorInfo[I_SHORT].llvmType = Type::Int16Ty;
  AssessorInfo[I_SHORT].llvmTypePtr = PointerType::getUnqual(Type::Int16Ty);
  AssessorInfo[I_SHORT].llvmNullConstant = 
    Constant::getNullValue(Type::Int16Ty);
  AssessorInfo[I_SHORT].sizeInBytesConstant = mvm::MvmModule::constantTwo;
  
  AssessorInfo[I_CHAR].llvmType = Type::Int16Ty;
  AssessorInfo[I_CHAR].llvmTypePtr = PointerType::getUnqual(Type::Int16Ty);
  AssessorInfo[I_CHAR].llvmNullConstant = 
    Constant::getNullValue(Type::Int16Ty);
  AssessorInfo[I_CHAR].sizeInBytesConstant = mvm::MvmModule::constantTwo;
  
  AssessorInfo[I_INT].llvmType = Type::Int32Ty;
  AssessorInfo[I_INT].llvmTypePtr = PointerType::getUnqual(Type::Int32Ty);
  AssessorInfo[I_INT].llvmNullConstant = 
    Constant::getNullValue(Type::Int32Ty);
  AssessorInfo[I_INT].sizeInBytesConstant = mvm::MvmModule::constantFour;
  
  AssessorInfo[I_FLOAT].llvmType = Type::FloatTy;
  AssessorInfo[I_FLOAT].llvmTypePtr = PointerType::getUnqual(Type::FloatTy);
  AssessorInfo[I_FLOAT].llvmNullConstant = 
    Constant::getNullValue(Type::FloatTy);
  AssessorInfo[I_FLOAT].sizeInBytesConstant = mvm::MvmModule::constantFour;
  
  AssessorInfo[I_LONG].llvmType = Type::Int64Ty;
  AssessorInfo[I_LONG].llvmTypePtr = PointerType::getUnqual(Type::Int64Ty);
  AssessorInfo[I_LONG].llvmNullConstant = 
    Constant::getNullValue(Type::Int64Ty);
  AssessorInfo[I_LONG].sizeInBytesConstant = mvm::MvmModule::constantEight;
  
  AssessorInfo[I_DOUBLE].llvmType = Type::DoubleTy;
  AssessorInfo[I_DOUBLE].llvmTypePtr = PointerType::getUnqual(Type::DoubleTy);
  AssessorInfo[I_DOUBLE].llvmNullConstant = 
    Constant::getNullValue(Type::DoubleTy);
  AssessorInfo[I_DOUBLE].sizeInBytesConstant = mvm::MvmModule::constantEight;
  
  AssessorInfo[I_TAB].llvmType = JnjvmModule::JavaObjectType;
  AssessorInfo[I_TAB].llvmTypePtr =
    PointerType::getUnqual(JnjvmModule::JavaObjectType);
  AssessorInfo[I_TAB].llvmNullConstant =
    JnjvmModule::JavaObjectNullConstant;
  AssessorInfo[I_TAB].sizeInBytesConstant = mvm::MvmModule::constantPtrSize;
  
  AssessorInfo[I_REF].llvmType = JnjvmModule::JavaObjectType;
  AssessorInfo[I_REF].llvmTypePtr =
    PointerType::getUnqual(JnjvmModule::JavaObjectType);
  AssessorInfo[I_REF].llvmNullConstant =
    JnjvmModule::JavaObjectNullConstant;
  AssessorInfo[I_REF].sizeInBytesConstant = mvm::MvmModule::constantPtrSize;
}

std::map<const char, LLVMAssessorInfo> LLVMAssessorInfo::AssessorInfo;

LLVMAssessorInfo& JnjvmModule::getTypedefInfo(const Typedef* type) {
  return LLVMAssessorInfo::AssessorInfo[type->getKey()->elements[0]];
}

static AnnotationID JavaMethod_ID(
  AnnotationManager::getID("Java::JavaMethod"));


LLVMMethodInfo::LLVMMethodInfo(JavaMethod* M) : 
  llvm::Annotation(JavaMethod_ID), methodDef(M), methodFunction(0),
  offsetConstant(0), functionType(0) {}

JavaMethod* LLVMMethodInfo::get(const llvm::Function* F) {
  LLVMMethodInfo *MI = (LLVMMethodInfo*)F->getAnnotation(JavaMethod_ID);
  if (MI) return MI->methodDef;
  return 0;
}

LLVMSignatureInfo* JnjvmModule::getSignatureInfo(Signdef* sign) {
  return sign->getInfo<LLVMSignatureInfo>();
}
  
LLVMClassInfo* JnjvmModule::getClassInfo(Class* cl) {
  return cl->getInfo<LLVMClassInfo>();
}

LLVMFieldInfo* JnjvmModule::getFieldInfo(JavaField* field) {
  return field->getInfo<LLVMFieldInfo>();
}
  
LLVMMethodInfo* JnjvmModule::getMethodInfo(JavaMethod* method) {
  return method->getInfo<LLVMMethodInfo>();
}
