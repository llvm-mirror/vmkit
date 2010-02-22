//===------------- J3Intrinsics.cpp - Intrinsics for J3 -------------------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/Constants.h"
#include "llvm/DerivedTypes.h"
#include "llvm/LLVMContext.h"
#include "llvm/Module.h"

#include "mvm/JIT.h"

#include "JavaAccess.h"
#include "JavaArray.h"
#include "JavaClass.h"
#include "JavaTypes.h"

#include "j3/J3Intrinsics.h"
#include "j3/LLVMInfo.h"

using namespace j3;
using namespace llvm;

namespace j3 { 
  namespace llvm_runtime { 
    #include "LLVMRuntime.inc"
  }
}

J3Intrinsics::J3Intrinsics(llvm::Module* module) :
  BaseIntrinsics(module) {
  
  llvm::Module* globalModule = mvm::MvmModule::globalModule;

  if (!globalModule->getTypeByName("JavaThread")) {
    j3::llvm_runtime::makeLLVMModuleContents(globalModule);
    mvm::MvmModule::copyDefinitions(module, globalModule);   
  }
  

  if (!(LLVMAssessorInfo::AssessorInfo[I_VOID].llvmType)) {
    LLVMAssessorInfo::initialise();
  }
  
  VTType = PointerType::getUnqual(globalModule->getTypeByName("VT"));

#ifdef ISOLATE_SHARING
  JnjvmType = 
    PointerType::getUnqual(globalModule->getTypeByName("Jnjvm"));
#endif
  ConstantPoolType = ptrPtrType;
  
  JavaObjectType = 
    PointerType::getUnqual(globalModule->getTypeByName("JavaObject"));

  JavaArrayType =
    PointerType::getUnqual(globalModule->getTypeByName("JavaArray"));
  
  JavaCommonClassType =
    PointerType::getUnqual(globalModule->getTypeByName("JavaCommonClass"));
  JavaClassPrimitiveType =
    PointerType::getUnqual(globalModule->getTypeByName("JavaClassPrimitive"));
  JavaClassArrayType =
    PointerType::getUnqual(globalModule->getTypeByName("JavaClassArray"));
  JavaClassType =
    PointerType::getUnqual(globalModule->getTypeByName("JavaClass"));
  
  JavaArrayUInt8Type =
    PointerType::getUnqual(globalModule->getTypeByName("ArrayUInt8"));
  JavaArraySInt8Type =
    PointerType::getUnqual(globalModule->getTypeByName("ArraySInt8"));
  JavaArrayUInt16Type =
    PointerType::getUnqual(globalModule->getTypeByName("ArrayUInt16"));
  JavaArraySInt16Type =
    PointerType::getUnqual(globalModule->getTypeByName("ArraySInt16"));
  JavaArrayUInt32Type =
    PointerType::getUnqual(globalModule->getTypeByName("ArrayUInt32"));
  JavaArraySInt32Type =
    PointerType::getUnqual(globalModule->getTypeByName("ArraySInt32"));
  JavaArrayLongType =
    PointerType::getUnqual(globalModule->getTypeByName("ArrayLong"));
  JavaArrayFloatType =
    PointerType::getUnqual(globalModule->getTypeByName("ArrayFloat"));
  JavaArrayDoubleType =
    PointerType::getUnqual(globalModule->getTypeByName("ArrayDouble"));
  JavaArrayObjectType =
    PointerType::getUnqual(globalModule->getTypeByName("ArrayObject"));

  JavaFieldType =
    PointerType::getUnqual(globalModule->getTypeByName("JavaField"));
  JavaMethodType =
    PointerType::getUnqual(globalModule->getTypeByName("JavaMethod"));
  UTF8Type =
    PointerType::getUnqual(globalModule->getTypeByName("UTF8"));
  AttributType =
    PointerType::getUnqual(globalModule->getTypeByName("Attribut"));
  JavaThreadType =
    PointerType::getUnqual(globalModule->getTypeByName("JavaThread"));
  MutatorThreadType =
    PointerType::getUnqual(globalModule->getTypeByName("MutatorThread"));
  
  CodeLineInfoType =
    PointerType::getUnqual(globalModule->getTypeByName("CodeLineInfo"));
  
  JavaObjectNullConstant =
    Constant::getNullValue(J3Intrinsics::JavaObjectType);
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
  
  globalModule->addTypeName("JavaObject", JavaObjectType->getContainedType(0));
  globalModule->addTypeName("JavaArray", JavaArrayType->getContainedType(0));
  globalModule->addTypeName("JavaCommonClass",
                      JavaCommonClassType->getContainedType(0));
  globalModule->addTypeName("JavaClass", JavaClassType->getContainedType(0));
  globalModule->addTypeName("JavaClassPrimitive",
                      JavaClassPrimitiveType->getContainedType(0));
  globalModule->addTypeName("JavaClassArray",
                      JavaClassArrayType->getContainedType(0));
  globalModule->addTypeName("ArrayUInt8", JavaArrayUInt8Type->getContainedType(0));
  globalModule->addTypeName("ArraySInt8", JavaArraySInt8Type->getContainedType(0));
  globalModule->addTypeName("ArrayUInt16", JavaArrayUInt16Type->getContainedType(0));
  globalModule->addTypeName("ArraySInt16", JavaArraySInt16Type->getContainedType(0));
  globalModule->addTypeName("ArraySInt32", JavaArraySInt32Type->getContainedType(0));
  globalModule->addTypeName("ArrayLong", JavaArrayLongType->getContainedType(0));
  globalModule->addTypeName("ArrayFloat", JavaArrayFloatType->getContainedType(0));
  globalModule->addTypeName("ArrayDouble", JavaArrayDoubleType->getContainedType(0));
  globalModule->addTypeName("ArrayObject", JavaArrayObjectType->getContainedType(0));
   
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
  
  ResolveVirtualStubFunction = module->getFunction("j3ResolveVirtualStub");
  ResolveStaticStubFunction = module->getFunction("j3ResolveStaticStub");
  ResolveSpecialStubFunction = module->getFunction("j3ResolveSpecialStub");
  
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
