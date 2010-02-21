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
    mvm::MvmModule::copyDefinitions(module, mvm::MvmModule::globalModule);
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
   
  InterfaceLookupFunction = globalModule->getFunction("j3InterfaceLookup");
  MultiCallNewFunction = globalModule->getFunction("j3MultiCallNew");
  ForceLoadedCheckFunction = globalModule->getFunction("forceLoadedCheck");
  InitialisationCheckFunction = globalModule->getFunction("initialisationCheck");
  ForceInitialisationCheckFunction = 
    globalModule->getFunction("forceInitialisationCheck");
  InitialiseClassFunction = globalModule->getFunction("j3RuntimeInitialiseClass");
  
  GetConstantPoolAtFunction = globalModule->getFunction("getConstantPoolAt");
  ArrayLengthFunction = globalModule->getFunction("arrayLength");
  GetVTFunction = globalModule->getFunction("getVT");
  GetIMTFunction = globalModule->getFunction("getIMT");
  GetClassFunction = globalModule->getFunction("getClass");
  ClassLookupFunction = globalModule->getFunction("j3ClassLookup");
  GetVTFromClassFunction = globalModule->getFunction("getVTFromClass");
  GetVTFromClassArrayFunction = globalModule->getFunction("getVTFromClassArray");
  GetVTFromCommonClassFunction = globalModule->getFunction("getVTFromCommonClass");
  GetBaseClassVTFromVTFunction = globalModule->getFunction("getBaseClassVTFromVT");
  GetObjectSizeFromClassFunction = 
    globalModule->getFunction("getObjectSizeFromClass");
 
  GetClassDelegateeFunction = globalModule->getFunction("getClassDelegatee");
  RuntimeDelegateeFunction = globalModule->getFunction("j3RuntimeDelegatee");
  IsAssignableFromFunction = globalModule->getFunction("isAssignableFrom");
  IsSecondaryClassFunction = globalModule->getFunction("isSecondaryClass");
  GetDepthFunction = globalModule->getFunction("getDepth");
  GetStaticInstanceFunction = globalModule->getFunction("getStaticInstance");
  GetDisplayFunction = globalModule->getFunction("getDisplay");
  GetVTInDisplayFunction = globalModule->getFunction("getVTInDisplay");
  AquireObjectFunction = globalModule->getFunction("j3JavaObjectAquire");
  ReleaseObjectFunction = globalModule->getFunction("j3JavaObjectRelease");
  OverflowThinLockFunction = globalModule->getFunction("j3OverflowThinLock");

  VirtualFieldLookupFunction = globalModule->getFunction("j3VirtualFieldLookup");
  StaticFieldLookupFunction = globalModule->getFunction("j3StaticFieldLookup");
  StringLookupFunction = globalModule->getFunction("j3StringLookup");
  StartJNIFunction = globalModule->getFunction("j3StartJNI");
  EndJNIFunction = globalModule->getFunction("j3EndJNI");
  
  ResolveVirtualStubFunction = globalModule->getFunction("j3ResolveVirtualStub");
  ResolveStaticStubFunction = globalModule->getFunction("j3ResolveStaticStub");
  ResolveSpecialStubFunction = globalModule->getFunction("j3ResolveSpecialStub");
  
  NullPointerExceptionFunction =
    globalModule->getFunction("j3NullPointerException");
  ClassCastExceptionFunction = globalModule->getFunction("j3ClassCastException");
  IndexOutOfBoundsExceptionFunction = 
    globalModule->getFunction("j3IndexOutOfBoundsException");
  NegativeArraySizeExceptionFunction = 
    globalModule->getFunction("j3NegativeArraySizeException");
  OutOfMemoryErrorFunction = globalModule->getFunction("j3OutOfMemoryError");
  StackOverflowErrorFunction = globalModule->getFunction("j3StackOverflowError");
  ArrayStoreExceptionFunction = globalModule->getFunction("j3ArrayStoreException");
  ArithmeticExceptionFunction = globalModule->getFunction("j3ArithmeticException");

  PrintExecutionFunction = globalModule->getFunction("j3PrintExecution");
  PrintMethodStartFunction = globalModule->getFunction("j3PrintMethodStart");
  PrintMethodEndFunction = globalModule->getFunction("j3PrintMethodEnd");

  ThrowExceptionFunction = globalModule->getFunction("j3ThrowException");

  GetArrayClassFunction = globalModule->getFunction("j3GetArrayClass");
 
  GetFinalInt8FieldFunction = globalModule->getFunction("getFinalInt8Field");
  GetFinalInt16FieldFunction = globalModule->getFunction("getFinalInt16Field");
  GetFinalInt32FieldFunction = globalModule->getFunction("getFinalInt32Field");
  GetFinalLongFieldFunction = globalModule->getFunction("getFinalLongField");
  GetFinalFloatFieldFunction = globalModule->getFunction("getFinalFloatField");
  GetFinalDoubleFieldFunction = globalModule->getFunction("getFinalDoubleField");
  GetFinalObjectFieldFunction = globalModule->getFunction("getFinalObjectField");

#ifdef ISOLATE_SHARING
  GetCtpClassFunction = globalModule->getFunction("getCtpClass");
  GetJnjvmExceptionClassFunction = 
    globalModule->getFunction("getJnjvmExceptionClass");
  GetJnjvmArrayClassFunction = globalModule->getFunction("getJnjvmArrayClass");
  StaticCtpLookupFunction = globalModule->getFunction("j3StaticCtpLookup");
  SpecialCtpLookupFunction = globalModule->getFunction("j3SpecialCtpLookup");
#endif
 
#ifdef SERVICE
  ServiceCallStartFunction = globalModule->getFunction("j3ServiceCallStart");
  ServiceCallStopFunction = globalModule->getFunction("j3ServiceCallStop");
#endif

  JavaObjectTracerFunction = globalModule->getFunction("JavaObjectTracer");
  EmptyTracerFunction = globalModule->getFunction("EmptyTracer");
  JavaArrayTracerFunction = globalModule->getFunction("JavaArrayTracer");
  ArrayObjectTracerFunction = globalModule->getFunction("ArrayObjectTracer");
  RegularObjectTracerFunction = globalModule->getFunction("RegularObjectTracer");

#ifndef WITHOUT_VTABLE
  VirtualLookupFunction = globalModule->getFunction("j3VirtualTableLookup");
#endif

  GetLockFunction = globalModule->getFunction("getLock");
  ThrowExceptionFromJITFunction =
    globalModule->getFunction("j3ThrowExceptionFromJIT");
 
}
