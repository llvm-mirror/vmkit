//===--------- JnjvmModule.cpp - Definition of a Jnjvm module -------------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/BasicBlock.h"
#include "llvm/CallingConv.h"
#include "llvm/Constants.h"
#include "llvm/Instructions.h"
#include "llvm/LLVMContext.h"
#include "llvm/Module.h"
#include "llvm/PassManager.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Target/TargetData.h"

#include "mvm/JIT.h"

#include "JavaArray.h"
#include "JavaClass.h"
#include "JavaJIT.h"
#include "JavaTypes.h"

#include "j3/JnjvmModule.h"
#include "j3/LLVMMaterializer.h"

using namespace j3;
using namespace llvm;

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
const llvm::Type* JnjvmModule::CodeLineInfoType = 0;
const llvm::Type* JnjvmModule::ConstantPoolType = 0;
const llvm::Type* JnjvmModule::UTF8Type = 0;
const llvm::Type* JnjvmModule::JavaFieldType = 0;
const llvm::Type* JnjvmModule::JavaMethodType = 0;
const llvm::Type* JnjvmModule::AttributType = 0;
const llvm::Type* JnjvmModule::JavaThreadType = 0;
const llvm::Type* JnjvmModule::MutatorThreadType = 0;

#ifdef ISOLATE_SHARING
const llvm::Type* JnjvmModule::JnjvmType = 0;
#endif

const llvm::Type*   JnjvmModule::JavaClassType;
const llvm::Type*   JnjvmModule::JavaClassPrimitiveType;
const llvm::Type*   JnjvmModule::JavaClassArrayType;
const llvm::Type*   JnjvmModule::JavaCommonClassType;
const llvm::Type*   JnjvmModule::VTType;


JavaLLVMCompiler::JavaLLVMCompiler(const std::string& str) :
  TheModule(new llvm::Module(str, getGlobalContext())),
  JavaIntrinsics(TheModule) {

  enabledException = true;
#ifdef WITH_LLVM_GCC
  cooperativeGC = true;
#else
  cooperativeGC = false;
#endif
}
  
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


namespace j3 { 
  namespace llvm_runtime { 
    #include "LLVMRuntime.inc"
  }
}

void JnjvmModule::initialise() {
  Module* module = globalModule;
  
  if (!module->getTypeByName("JavaThread"))
    j3::llvm_runtime::makeLLVMModuleContents(module);

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
  MutatorThreadType =
    PointerType::getUnqual(module->getTypeByName("MutatorThread"));
  
  CodeLineInfoType =
    PointerType::getUnqual(module->getTypeByName("CodeLineInfo"));
 
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
  
  JavaObjectNullConstant =
    Constant::getNullValue(JnjvmModule::JavaObjectType);
  MaxArraySizeConstant = ConstantInt::get(Type::getInt32Ty(getGlobalContext()),
                                          JavaArray::MaxArraySize);
  JavaArraySizeConstant = ConstantInt::get(Type::getInt32Ty(getGlobalContext()),
                                          sizeof(JavaObject) + sizeof(ssize_t));
  
  
  JavaArrayElementsOffsetConstant = constantTwo;
  JavaArraySizeOffsetConstant = constantOne;
  JavaObjectLockOffsetConstant = constantOne;
  JavaObjectVTOffsetConstant = constantZero;
  OffsetClassInVTConstant = constantThree;
  OffsetDepthInVTConstant = constantFour;
  OffsetDisplayInVTConstant = constantSeven;
  OffsetBaseClassVTInVTConstant =
    ConstantInt::get(Type::getInt32Ty(getGlobalContext()), 17);
  OffsetIMTInVTConstant =
    ConstantInt::get(Type::getInt32Ty(getGlobalContext()), 18);
  
  OffsetAccessInCommonClassConstant = constantOne;
  IsArrayConstant = ConstantInt::get(Type::getInt16Ty(getGlobalContext()),
                                     JNJVM_ARRAY);
  
  IsPrimitiveConstant = ConstantInt::get(Type::getInt16Ty(getGlobalContext()),
                                         JNJVM_PRIMITIVE);
 
  OffsetBaseClassInArrayClassConstant = constantOne;
  OffsetLogSizeInPrimitiveClassConstant = constantOne;

  OffsetObjectSizeInClassConstant = constantOne;
  OffsetVTInClassConstant =
    ConstantInt::get(Type::getInt32Ty(getGlobalContext()), 7);
  OffsetTaskClassMirrorInClassConstant = constantThree;
  OffsetVirtualMethodsInClassConstant =
    ConstantInt::get(Type::getInt32Ty(getGlobalContext()), 9);
  OffsetStaticInstanceInTaskClassMirrorConstant = constantThree;
  OffsetStatusInTaskClassMirrorConstant = constantZero;
  OffsetInitializedInTaskClassMirrorConstant = constantOne;
  
  OffsetIsolateInThreadConstant =
    ConstantInt::get(Type::getInt32Ty(getGlobalContext()), 3);
  OffsetDoYieldInThreadConstant =
    ConstantInt::get(Type::getInt32Ty(getGlobalContext()), 6);
  OffsetJNIInThreadConstant =
    ConstantInt::get(Type::getInt32Ty(getGlobalContext()), 1);
  OffsetJavaExceptionInThreadConstant =
    ConstantInt::get(Type::getInt32Ty(getGlobalContext()), 2);
  OffsetCXXExceptionInThreadConstant =
    ConstantInt::get(Type::getInt32Ty(getGlobalContext()), 3);
  
  ClassReadyConstant =
    ConstantInt::get(Type::getInt8Ty(getGlobalContext()), ready);
  
  module->addTypeName("JavaObject", JavaObjectType->getContainedType(0));
  module->addTypeName("JavaArray", JavaArrayType->getContainedType(0));
  module->addTypeName("JavaCommonClass",
                      JavaCommonClassType->getContainedType(0));
  module->addTypeName("JavaClass", JavaClassType->getContainedType(0));
  module->addTypeName("JavaClassPrimitive",
                      JavaClassPrimitiveType->getContainedType(0));
  module->addTypeName("JavaClassArray",
                      JavaClassArrayType->getContainedType(0));
  module->addTypeName("ArrayUInt8", JavaArrayUInt8Type->getContainedType(0));
  module->addTypeName("ArraySInt8", JavaArraySInt8Type->getContainedType(0));
  module->addTypeName("ArrayUInt16", JavaArrayUInt16Type->getContainedType(0));
  module->addTypeName("ArraySInt16", JavaArraySInt16Type->getContainedType(0));
  module->addTypeName("ArraySInt32", JavaArraySInt32Type->getContainedType(0));
  module->addTypeName("ArrayLong", JavaArrayLongType->getContainedType(0));
  module->addTypeName("ArrayFloat", JavaArrayFloatType->getContainedType(0));
  module->addTypeName("ArrayDouble", JavaArrayDoubleType->getContainedType(0));
  module->addTypeName("ArrayObject", JavaArrayObjectType->getContainedType(0));
   
  InterfaceLookupFunction = module->getFunction("j3InterfaceLookup");
  MultiCallNewFunction = module->getFunction("j3MultiCallNew");
  ForceLoadedCheckFunction = module->getFunction("forceLoadedCheck");
  InitialisationCheckFunction = module->getFunction("initialisationCheck");
  ForceInitialisationCheckFunction = 
    module->getFunction("forceInitialisationCheck");
  InitialiseClassFunction = module->getFunction("j3RuntimeInitialiseClass");
  
  GetConstantPoolAtFunction = module->getFunction("getConstantPoolAt");
  ArrayLengthFunction = module->getFunction("arrayLength");
  GetVTFunction = module->getFunction("getVT");
  GetIMTFunction = module->getFunction("getIMT");
  GetClassFunction = module->getFunction("getClass");
  ClassLookupFunction = module->getFunction("j3ClassLookup");
  GetVTFromClassFunction = module->getFunction("getVTFromClass");
  GetVTFromClassArrayFunction = module->getFunction("getVTFromClassArray");
  GetVTFromCommonClassFunction = module->getFunction("getVTFromCommonClass");
  GetBaseClassVTFromVTFunction = module->getFunction("getBaseClassVTFromVT");
  GetObjectSizeFromClassFunction = 
    module->getFunction("getObjectSizeFromClass");
 
  GetClassDelegateeFunction = module->getFunction("getClassDelegatee");
  RuntimeDelegateeFunction = module->getFunction("j3RuntimeDelegatee");
  IsAssignableFromFunction = module->getFunction("isAssignableFrom");
  IsSecondaryClassFunction = module->getFunction("isSecondaryClass");
  GetDepthFunction = module->getFunction("getDepth");
  GetStaticInstanceFunction = module->getFunction("getStaticInstance");
  GetDisplayFunction = module->getFunction("getDisplay");
  GetVTInDisplayFunction = module->getFunction("getVTInDisplay");
  AquireObjectFunction = module->getFunction("j3JavaObjectAquire");
  ReleaseObjectFunction = module->getFunction("j3JavaObjectRelease");
  OverflowThinLockFunction = module->getFunction("j3OverflowThinLock");

  VirtualFieldLookupFunction = module->getFunction("j3VirtualFieldLookup");
  StaticFieldLookupFunction = module->getFunction("j3StaticFieldLookup");
  StringLookupFunction = module->getFunction("j3StringLookup");
  StartJNIFunction = module->getFunction("j3StartJNI");
  EndJNIFunction = module->getFunction("j3EndJNI");
  
  NullPointerExceptionFunction =
    module->getFunction("j3NullPointerException");
  ClassCastExceptionFunction = module->getFunction("j3ClassCastException");
  IndexOutOfBoundsExceptionFunction = 
    module->getFunction("j3IndexOutOfBoundsException");
  NegativeArraySizeExceptionFunction = 
    module->getFunction("j3NegativeArraySizeException");
  OutOfMemoryErrorFunction = module->getFunction("j3OutOfMemoryError");
  StackOverflowErrorFunction = module->getFunction("j3StackOverflowError");
  ArrayStoreExceptionFunction = module->getFunction("j3ArrayStoreException");
  ArithmeticExceptionFunction = module->getFunction("j3ArithmeticException");

  PrintExecutionFunction = module->getFunction("j3PrintExecution");
  PrintMethodStartFunction = module->getFunction("j3PrintMethodStart");
  PrintMethodEndFunction = module->getFunction("j3PrintMethodEnd");

  ThrowExceptionFunction = module->getFunction("j3ThrowException");

  GetArrayClassFunction = module->getFunction("j3GetArrayClass");
 
  GetFinalInt8FieldFunction = module->getFunction("getFinalInt8Field");
  GetFinalInt16FieldFunction = module->getFunction("getFinalInt16Field");
  GetFinalInt32FieldFunction = module->getFunction("getFinalInt32Field");
  GetFinalLongFieldFunction = module->getFunction("getFinalLongField");
  GetFinalFloatFieldFunction = module->getFunction("getFinalFloatField");
  GetFinalDoubleFieldFunction = module->getFunction("getFinalDoubleField");
  GetFinalObjectFieldFunction = module->getFunction("getFinalObjectField");

#ifdef ISOLATE_SHARING
  GetCtpClassFunction = module->getFunction("getCtpClass");
  GetJnjvmExceptionClassFunction = 
    module->getFunction("getJnjvmExceptionClass");
  GetJnjvmArrayClassFunction = module->getFunction("getJnjvmArrayClass");
  StaticCtpLookupFunction = module->getFunction("j3StaticCtpLookup");
  SpecialCtpLookupFunction = module->getFunction("j3SpecialCtpLookup");
#endif
 
#ifdef SERVICE
  ServiceCallStartFunction = module->getFunction("j3ServiceCallStart");
  ServiceCallStopFunction = module->getFunction("j3ServiceCallStop");
#endif

  JavaObjectTracerFunction = module->getFunction("JavaObjectTracer");
  EmptyTracerFunction = module->getFunction("EmptyTracer");
  JavaArrayTracerFunction = module->getFunction("JavaArrayTracer");
  ArrayObjectTracerFunction = module->getFunction("ArrayObjectTracer");
  RegularObjectTracerFunction = module->getFunction("RegularObjectTracer");

#ifndef WITHOUT_VTABLE
  VirtualLookupFunction = module->getFunction("j3VirtualTableLookup");
#endif

  GetLockFunction = module->getFunction("getLock");
  ThrowExceptionFromJITFunction =
    module->getFunction("j3ThrowExceptionFromJIT");
 
}

Function* JavaLLVMCompiler::parseFunction(JavaMethod* meth) {
  LLVMMethodInfo* LMI = getMethodInfo(meth);
  Function* func = LMI->getMethod();
  
  // We are jitting. Take the lock.
  JnjvmModule::protectIR();
  if (func->getLinkage() == GlobalValue::ExternalWeakLinkage) {
    JavaJIT jit(this, meth, func);
    if (isNative(meth->access)) {
      jit.nativeCompile();
      JnjvmModule::runPasses(func, JavaNativeFunctionPasses);
    } else {
      jit.javaCompile();
      JnjvmModule::runPasses(func, JnjvmModule::globalFunctionPasses);
      JnjvmModule::runPasses(func, JavaFunctionPasses);
    }
    func->setLinkage(GlobalValue::ExternalLinkage);
  }
  JnjvmModule::unprotectIR();

  return func;
}

JavaMethod* JavaLLVMCompiler::getJavaMethod(llvm::Function* F) {
  function_iterator E = functions.end();
  function_iterator I = functions.find(F);
  if (I == E) return 0;
  return I->second;
}

MDNode* JavaLLVMCompiler::GetDbgSubprogram(JavaMethod* meth) {
  if (getMethodInfo(meth)->getDbgSubprogram() == NULL) {
    MDNode* node =
      JavaIntrinsics.DebugFactory->CreateSubprogram(DIDescriptor(), "", "",
                                                    "", DICompileUnit(), 0,
                                                    DIType(), false,
                                                    false).getNode();
    DbgInfos.insert(std::make_pair(node, meth));
    getMethodInfo(meth)->setDbgSubprogram(node);
  }
  return getMethodInfo(meth)->getDbgSubprogram();
}

JavaLLVMCompiler::~JavaLLVMCompiler() {
  delete JavaFunctionPasses;
  delete JavaNativeFunctionPasses;
  delete TheModuleProvider;
}

namespace mvm {
  llvm::FunctionPass* createEscapeAnalysisPass();
  llvm::LoopPass* createLoopSafePointsPass();
}

namespace j3 {
  llvm::FunctionPass* createLowerConstantCallsPass(JnjvmModule* M);
}

void JavaLLVMCompiler::addJavaPasses() {
  JavaNativeFunctionPasses = new FunctionPassManager(TheModule);
  JavaNativeFunctionPasses->add(new TargetData(TheModule));
  // Lower constant calls to lower things like getClass used
  // on synchronized methods.
  JavaNativeFunctionPasses->add(createLowerConstantCallsPass(getIntrinsics()));
  
  JavaFunctionPasses = new FunctionPassManager(TheModule);
  JavaFunctionPasses->add(new TargetData(TheModule));
  if (cooperativeGC)
    JavaFunctionPasses->add(mvm::createLoopSafePointsPass());

  // Re-enable this when the pointers in stack-allocated objects can
  // be given to the GC.
  //JavaFunctionPasses->add(mvm::createEscapeAnalysisPass());
  JavaFunctionPasses->add(createLowerConstantCallsPass(getIntrinsics()));
}

void JavaJITMethodInfo::print(void* ip, void* addr) {
  void* new_ip = NULL;
  if (ip) new_ip = isStub(ip, addr);
  fprintf(stderr, "; %p in %s.%s", new_ip,
          UTF8Buffer(meth->classDef->name).cString(),
          UTF8Buffer(meth->name).cString());
  if (ip != new_ip) fprintf(stderr, " (from stub)");
  fprintf(stderr, "\n");
}
