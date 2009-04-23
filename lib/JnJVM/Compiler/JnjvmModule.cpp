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
#include "llvm/Instructions.h"
#include "llvm/Module.h"
#include "llvm/PassManager.h"
#include "llvm/Target/TargetData.h"

#include "mvm/JIT.h"

#include "JavaArray.h"
#include "JavaClass.h"
#include "JavaJIT.h"
#include "JavaTypes.h"

#include "jnjvm/JnjvmModule.h"
#include "jnjvm/JnjvmModuleProvider.h"

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
const llvm::Type* JnjvmModule::ConstantPoolType = 0;
const llvm::Type* JnjvmModule::UTF8Type = 0;
const llvm::Type* JnjvmModule::JavaFieldType = 0;
const llvm::Type* JnjvmModule::JavaMethodType = 0;
const llvm::Type* JnjvmModule::AttributType = 0;
const llvm::Type* JnjvmModule::JavaThreadType = 0;

#ifdef ISOLATE_SHARING
const llvm::Type* JnjvmModule::JnjvmType = 0;
#endif

llvm::Constant*     JnjvmModule::JavaObjectNullConstant;
llvm::Constant*     JnjvmModule::MaxArraySizeConstant;
llvm::Constant*     JnjvmModule::JavaArraySizeConstant;
llvm::ConstantInt*  JnjvmModule::OffsetObjectSizeInClassConstant;
llvm::ConstantInt*  JnjvmModule::OffsetVTInClassConstant;
llvm::ConstantInt*  JnjvmModule::OffsetTaskClassMirrorInClassConstant;
llvm::ConstantInt*  JnjvmModule::OffsetStaticInstanceInTaskClassMirrorConstant;
llvm::ConstantInt*  JnjvmModule::OffsetStatusInTaskClassMirrorConstant;
llvm::ConstantInt*  JnjvmModule::OffsetInitializedInTaskClassMirrorConstant;
llvm::ConstantInt*  JnjvmModule::OffsetJavaExceptionInThreadConstant;
llvm::ConstantInt*  JnjvmModule::OffsetCXXExceptionInThreadConstant;
llvm::ConstantInt*  JnjvmModule::ClassReadyConstant;
const llvm::Type*   JnjvmModule::JavaClassType;
const llvm::Type*   JnjvmModule::JavaClassPrimitiveType;
const llvm::Type*   JnjvmModule::JavaClassArrayType;
const llvm::Type*   JnjvmModule::JavaCommonClassType;
const llvm::Type*   JnjvmModule::VTType;
llvm::ConstantInt*  JnjvmModule::JavaArrayElementsOffsetConstant;
llvm::ConstantInt*  JnjvmModule::JavaArraySizeOffsetConstant;
llvm::ConstantInt*  JnjvmModule::JavaObjectLockOffsetConstant;
llvm::ConstantInt*  JnjvmModule::JavaObjectVTOffsetConstant;
llvm::ConstantInt*  JnjvmModule::OffsetClassInVTConstant;
llvm::ConstantInt*  JnjvmModule::OffsetDepthInVTConstant;
llvm::ConstantInt*  JnjvmModule::OffsetDisplayInVTConstant;
llvm::ConstantInt*  JnjvmModule::OffsetBaseClassVTInVTConstant;


JavaLLVMCompiler::JavaLLVMCompiler(const std::string& str) :
  TheModule(new llvm::Module(str)), JavaIntrinsics(TheModule) {

  enabledException = true;
}

#ifdef WITH_TRACER
llvm::Function* JavaLLVMCompiler::internalMakeTracer(Class* cl, bool stat) {
  
  LLVMClassInfo* LCI = getClassInfo(cl);
  const Type* type = stat ? LCI->getStaticType() : LCI->getVirtualType();
  JavaField* fields = 0;
  uint32 nbFields = 0;
  if (stat) {
    fields = cl->getStaticFields();
    nbFields = cl->nbStaticFields;
  } else {
    fields = cl->getVirtualFields();
    nbFields = cl->nbVirtualFields;
  }
  
  Function* func = Function::Create(JnjvmModule::MarkAndTraceType,
                                    GlobalValue::InternalLinkage,
                                    "", getLLVMModule());

  Constant* zero = mvm::MvmModule::constantZero;
  Argument* arg = func->arg_begin();
  BasicBlock* block = BasicBlock::Create("", func);
  llvm::Value* realArg = new BitCastInst(arg, type, "", block);

  std::vector<Value*> Args;
  Args.push_back(arg);
#ifdef MULTIPLE_GC
  Value* GC = ++func->arg_begin();
  Args.push_back(GC);
#endif
  if (!stat) {
    if (cl->super == 0) {
      CallInst::Create(JavaIntrinsics.JavaObjectTracerFunction, Args.begin(),
                       Args.end(), "", block);

    } else {
      LLVMClassInfo* LCP = (LLVMClassInfo*)getClassInfo((Class*)(cl->super));
      Function* F = LCP->virtualTracerFunction;
      if (!F) {
        if (isStaticCompiling()) {
          F = internalMakeTracer(cl->super, false);
        } else {
          F = LCP->getVirtualTracer();
        }
        assert(F && "Still no virtual tracer for super");
      }
      CallInst::Create(F, Args.begin(), Args.end(), "", block);
    }
  }
  
  for (uint32 i = 0; i < nbFields; ++i) {
    JavaField& cur = fields[i];
    if (cur.getSignature()->trace()) {
      LLVMFieldInfo* LFI = getFieldInfo(&cur);
      std::vector<Value*> args; //size = 2
      args.push_back(zero);
      args.push_back(LFI->getOffset());
      Value* ptr = GetElementPtrInst::Create(realArg, args.begin(), args.end(), 
                                             "",block);
      Value* val = new LoadInst(ptr, "", block);
      Value* valCast = new BitCastInst(val, JnjvmModule::JavaObjectType, "",
                                       block);
      std::vector<Value*> Args;
      Args.push_back(valCast);
#ifdef MULTIPLE_GC
      Args.push_back(GC);
#endif
      CallInst::Create(JavaIntrinsics.MarkAndTraceFunction, Args.begin(),
                       Args.end(), "", block);
    }
  }

  ReturnInst::Create(block);
  
  if (!stat) {
    LCI->virtualTracerFunction = func;
  } else {
    LCI->staticTracerFunction = func;
  }

  return func;
}
#endif

void JavaLLVMCompiler::resolveVirtualClass(Class* cl) {
  // Lock here because we may be called by a class resolver
  mvm::MvmModule::protectIR();
  LLVMClassInfo* LCI = (LLVMClassInfo*)getClassInfo(cl);
  LCI->getVirtualType();
  mvm::MvmModule::unprotectIR();
}

void JavaLLVMCompiler::resolveStaticClass(Class* cl) {
  // Lock here because we may be called by a class initializer
  mvm::MvmModule::protectIR();
  LLVMClassInfo* LCI = (LLVMClassInfo*)getClassInfo(cl);
  LCI->getStaticType();
  mvm::MvmModule::unprotectIR();
}


namespace jnjvm { 
  namespace llvm_runtime { 
    #include "LLVMRuntime.inc"
  }
}

void JnjvmModule::initialise() {
  Module* module = globalModule;
  jnjvm::llvm_runtime::makeLLVMModuleContents(module);

  VTType = PointerType::getUnqual(module->getTypeByName("VT"));

#ifdef ISOLATE_SHARING
  JnjvmType = 
    PointerType::getUnqual(module->getTypeByName("Jnjvm"));
#endif
  ConstantPoolType = ptrPtrType;
  
  JavaObjectType = 
    PointerType::getUnqual(module->getTypeByName("JavaObject"));

  JavaArrayType =
    PointerType::getUnqual(module->getTypeByName("JavaArray"));
  
  JavaCommonClassType =
    PointerType::getUnqual(module->getTypeByName("JavaCommonClass"));
  JavaClassPrimitiveType =
    PointerType::getUnqual(module->getTypeByName("JavaClassPrimitive"));
  JavaClassArrayType =
    PointerType::getUnqual(module->getTypeByName("JavaClassArray"));
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
  
  JavaFieldType =
    PointerType::getUnqual(module->getTypeByName("JavaField"));
  JavaMethodType =
    PointerType::getUnqual(module->getTypeByName("JavaMethod"));
  UTF8Type =
    PointerType::getUnqual(module->getTypeByName("UTF8"));
  AttributType =
    PointerType::getUnqual(module->getTypeByName("Attribut"));
  JavaThreadType =
    PointerType::getUnqual(module->getTypeByName("JavaThread"));

#ifdef WITH_TRACER
  MarkAndTraceType = module->getFunction("MarkAndTrace")->getFunctionType();
#endif
 
  JavaObjectNullConstant = Constant::getNullValue(JnjvmModule::JavaObjectType);
  MaxArraySizeConstant = ConstantInt::get(Type::Int32Ty,
                                          JavaArray::MaxArraySize);
  JavaArraySizeConstant = 
    ConstantInt::get(Type::Int32Ty, sizeof(JavaObject) + sizeof(ssize_t));
  
  
  JavaArrayElementsOffsetConstant = mvm::MvmModule::constantTwo;
  JavaArraySizeOffsetConstant = mvm::MvmModule::constantOne;
  JavaObjectLockOffsetConstant = mvm::MvmModule::constantOne;
  JavaObjectVTOffsetConstant = mvm::MvmModule::constantZero;
  OffsetClassInVTConstant = mvm::MvmModule::constantThree;
  OffsetDepthInVTConstant = mvm::MvmModule::constantFour;
  OffsetDisplayInVTConstant = mvm::MvmModule::constantSeven;
  OffsetBaseClassVTInVTConstant = ConstantInt::get(Type::Int32Ty, 17);
  
  OffsetObjectSizeInClassConstant = mvm::MvmModule::constantOne;
  OffsetVTInClassConstant = ConstantInt::get(Type::Int32Ty, 7);
  OffsetTaskClassMirrorInClassConstant = mvm::MvmModule::constantTwo;
  OffsetStaticInstanceInTaskClassMirrorConstant = mvm::MvmModule::constantTwo;
  OffsetStatusInTaskClassMirrorConstant = mvm::MvmModule::constantZero;
  OffsetInitializedInTaskClassMirrorConstant = mvm::MvmModule::constantOne;
  
  OffsetJavaExceptionInThreadConstant = ConstantInt::get(Type::Int32Ty, 9);
  OffsetCXXExceptionInThreadConstant = ConstantInt::get(Type::Int32Ty, 10);
  
  ClassReadyConstant = ConstantInt::get(Type::Int8Ty, ready);
 
  LLVMAssessorInfo::initialise();
}

Function* JavaLLVMCompiler::getMethod(JavaMethod* meth) {
  return getMethodInfo(meth)->getMethod();
}

JnjvmModule::JnjvmModule(llvm::Module* module) :
  MvmModule(module) {
  
  if (!VTType) {
    initialise();
    copyDefinitions(module, globalModule);
  }
  
  module->addTypeName("JavaObject", JavaObjectType);
  module->addTypeName("JavaArray", JavaArrayType);
  module->addTypeName("JavaCommonClass", JavaCommonClassType);
  module->addTypeName("JavaClass", JavaClassType);
  module->addTypeName("JavaClassPrimitive", JavaClassPrimitiveType);
  module->addTypeName("JavaClassArray", JavaClassArrayType);
  module->addTypeName("ArrayUInt8", JavaArrayUInt8Type);
  module->addTypeName("ArraySInt8", JavaArraySInt8Type);
  module->addTypeName("ArrayUInt16", JavaArrayUInt16Type);
  module->addTypeName("ArraySInt16", JavaArraySInt16Type);
  module->addTypeName("ArraySInt32", JavaArraySInt32Type);
  module->addTypeName("ArrayLong", JavaArrayLongType);
  module->addTypeName("ArrayFloat", JavaArrayFloatType);
  module->addTypeName("ArrayDouble", JavaArrayDoubleType);
  module->addTypeName("ArrayObject", JavaArrayObjectType);
  module->addTypeName("CacheNode", CacheNodeType); 
  module->addTypeName("Enveloppe", EnveloppeType);
   
  InterfaceLookupFunction = module->getFunction("jnjvmVirtualLookup");
  MultiCallNewFunction = module->getFunction("multiCallNew");
  ForceLoadedCheckFunction = module->getFunction("forceLoadedCheck");
  InitialisationCheckFunction = module->getFunction("initialisationCheck");
  ForceInitialisationCheckFunction = 
    module->getFunction("forceInitialisationCheck");
  InitialiseClassFunction = module->getFunction("jnjvmRuntimeInitialiseClass");
  
  GetConstantPoolAtFunction = module->getFunction("getConstantPoolAt");
  ArrayLengthFunction = module->getFunction("arrayLength");
  GetVTFunction = module->getFunction("getVT");
  GetClassFunction = module->getFunction("getClass");
  ClassLookupFunction = module->getFunction("classLookup");
  GetVTFromClassFunction = module->getFunction("getVTFromClass");
  GetVTFromClassArrayFunction = module->getFunction("getVTFromClassArray");
  GetVTFromCommonClassFunction = module->getFunction("getVTFromCommonClass");
  GetBaseClassVTFromVTFunction = module->getFunction("getBaseClassVTFromVT");
  GetObjectSizeFromClassFunction = 
    module->getFunction("getObjectSizeFromClass");
 
  GetClassDelegateeFunction = module->getFunction("getClassDelegatee");
  RuntimeDelegateeFunction = module->getFunction("jnjvmRuntimeDelegatee");
  IsAssignableFromFunction = module->getFunction("isAssignableFrom");
  IsSecondaryClassFunction = module->getFunction("isSecondaryClass");
  GetDepthFunction = module->getFunction("getDepth");
  GetStaticInstanceFunction = module->getFunction("getStaticInstance");
  GetDisplayFunction = module->getFunction("getDisplay");
  GetVTInDisplayFunction = module->getFunction("getVTInDisplay");
  AquireObjectFunction = module->getFunction("JavaObjectAquire");
  ReleaseObjectFunction = module->getFunction("JavaObjectRelease");
  OverflowThinLockFunction = module->getFunction("overflowThinLock");

  VirtualFieldLookupFunction = module->getFunction("virtualFieldLookup");
  StaticFieldLookupFunction = module->getFunction("staticFieldLookup");
  
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
  ArrayStoreExceptionFunction = module->getFunction("jnjvmArrayStoreException");

  JavaObjectAllocateFunction = module->getFunction("gcmalloc");

  PrintExecutionFunction = module->getFunction("printExecution");
  PrintMethodStartFunction = module->getFunction("printMethodStart");
  PrintMethodEndFunction = module->getFunction("printMethodEnd");

  ThrowExceptionFunction = module->getFunction("JavaThreadThrowException");

  GetArrayClassFunction = module->getFunction("getArrayClass");
 
  GetFinalInt8FieldFunction = module->getFunction("getFinalInt8Field");
  GetFinalInt16FieldFunction = module->getFunction("getFinalInt16Field");
  GetFinalInt32FieldFunction = module->getFunction("getFinalInt32Field");
  GetFinalLongFieldFunction = module->getFunction("getFinalLongField");
  GetFinalFloatFieldFunction = module->getFunction("getFinalFloatField");
  GetFinalDoubleFieldFunction = module->getFunction("getFinalDoubleField");
  GetFinalObjectFieldFunction = module->getFunction("getFinalObjectField");

#ifdef ISOLATE
  StringLookupFunction = module->getFunction("stringLookup");
#ifdef ISOLATE_SHARING
  EnveloppeLookupFunction = module->getFunction("enveloppeLookup");
  GetCtpCacheNodeFunction = module->getFunction("getCtpCacheNode");
  GetCtpClassFunction = module->getFunction("getCtpClass");
  GetJnjvmExceptionClassFunction = 
    module->getFunction("getJnjvmExceptionClass");
  GetJnjvmArrayClassFunction = module->getFunction("getJnjvmArrayClass");
  StaticCtpLookupFunction = module->getFunction("staticCtpLookup");
  SpecialCtpLookupFunction = module->getFunction("specialCtpLookup");
#endif
#endif
 
#ifdef SERVICE
  ServiceCallStartFunction = module->getFunction("serviceCallStart");
  ServiceCallStopFunction = module->getFunction("serviceCallStop");
#endif

#ifdef WITH_TRACER
  MarkAndTraceFunction = module->getFunction("MarkAndTrace");
  JavaObjectTracerFunction = module->getFunction("JavaObjectTracer");
  JavaArrayTracerFunction = module->getFunction("JavaArrayTracer");
  ArrayObjectTracerFunction = module->getFunction("ArrayObjectTracer");
#endif

#ifndef WITHOUT_VTABLE
  VirtualLookupFunction = module->getFunction("vtableLookup");
#endif

  GetLockFunction = module->getFunction("getLock");
 
}

Function* JavaLLVMCompiler::parseFunction(JavaMethod* meth) {
  LLVMMethodInfo* LMI = getMethodInfo(meth);
  Function* func = LMI->getMethod();
  if (func->hasNotBeenReadFromBitcode()) {
    // We are jitting. Take the lock.
    JnjvmModule::protectIR();
    if (func->hasNotBeenReadFromBitcode()) {
      JavaJIT jit(this, meth, func);
      if (isNative(meth->access)) {
        jit.nativeCompile();
        JnjvmModule::runPasses(func, JavaNativeFunctionPasses);
      } else {
        jit.javaCompile();
        JnjvmModule::runPasses(func, JnjvmModule::globalFunctionPasses);
        JnjvmModule::runPasses(func, JavaFunctionPasses);
      }
    }
    JnjvmModule::unprotectIR();
  }
  return func;
}

JavaLLVMCompiler::~JavaLLVMCompiler() {
  delete JavaFunctionPasses;
  delete JavaNativeFunctionPasses;
  delete TheModuleProvider;
}

namespace mvm {
  llvm::FunctionPass* createEscapeAnalysisPass();
}

namespace jnjvm {
  llvm::FunctionPass* createLowerConstantCallsPass();
}

void JavaLLVMCompiler::addJavaPasses() {
  JavaNativeFunctionPasses = new FunctionPassManager(TheModuleProvider);
  JavaNativeFunctionPasses->add(new TargetData(TheModule));
  // Lower constant calls to lower things like getClass used
  // on synchronized methods.
  JavaNativeFunctionPasses->add(createLowerConstantCallsPass());
  
  JavaFunctionPasses = new FunctionPassManager(TheModuleProvider);
  JavaFunctionPasses->add(new TargetData(TheModule));
  JavaFunctionPasses->add(mvm::createEscapeAnalysisPass());
  JavaFunctionPasses->add(createLowerConstantCallsPass());
}
