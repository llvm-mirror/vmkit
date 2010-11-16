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
  j3::llvm_runtime::makeLLVMModuleContents(module);
  
  VTType = PointerType::getUnqual(module->getTypeByName("VT"));
  LLVMContext& Context = module->getContext();

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
  
  JavaObjectNullConstant =
    Constant::getNullValue(J3Intrinsics::JavaObjectType);
  MaxArraySizeConstant = ConstantInt::get(Type::getInt32Ty(Context),
                                          JavaArray::MaxArraySize);
  JavaArraySizeConstant = ConstantInt::get(Type::getInt32Ty(Context),
                                          sizeof(JavaObject) + sizeof(ssize_t));
  
  
  JavaArrayElementsOffsetConstant = constantTwo;
  JavaArraySizeOffsetConstant = constantOne;
  JavaObjectLockOffsetConstant = constantOne;
  JavaObjectVTOffsetConstant = constantZero;

  OffsetClassInVTConstant =
    ConstantInt::get(Type::getInt32Ty(Context),
                     JavaVirtualTable::getClassIndex());
  OffsetDepthInVTConstant =
    ConstantInt::get(Type::getInt32Ty(Context),
                     JavaVirtualTable::getDepthIndex());
  OffsetDisplayInVTConstant =
    ConstantInt::get(Type::getInt32Ty(Context),
                     JavaVirtualTable::getDisplayIndex());
  OffsetBaseClassVTInVTConstant =
    ConstantInt::get(Type::getInt32Ty(Context),
                     JavaVirtualTable::getBaseClassIndex());
  OffsetIMTInVTConstant =
    ConstantInt::get(Type::getInt32Ty(Context),
                     JavaVirtualTable::getIMTIndex());
  
  OffsetAccessInCommonClassConstant = constantOne;
  IsArrayConstant = ConstantInt::get(Type::getInt16Ty(Context),
                                     JNJVM_ARRAY);
  
  IsPrimitiveConstant = ConstantInt::get(Type::getInt16Ty(Context),
                                         JNJVM_PRIMITIVE);
 
  OffsetBaseClassInArrayClassConstant = constantOne;
  OffsetLogSizeInPrimitiveClassConstant = constantOne;

  OffsetObjectSizeInClassConstant = constantOne;
  OffsetVTInClassConstant = ConstantInt::get(Type::getInt32Ty(Context), 7);
  OffsetTaskClassMirrorInClassConstant = constantThree;
  OffsetStaticInstanceInTaskClassMirrorConstant = constantThree;
  OffsetStatusInTaskClassMirrorConstant = constantZero;
  OffsetInitializedInTaskClassMirrorConstant = constantOne;
  
  OffsetIsolateIDInThreadConstant =         ConstantInt::get(Type::getInt32Ty(Context), 1);
  OffsetVMInThreadConstant =                ConstantInt::get(Type::getInt32Ty(Context), 2);
  OffsetDoYieldInThreadConstant =           ConstantInt::get(Type::getInt32Ty(Context), 4);
  OffsetCXXExceptionInThreadConstant =      ConstantInt::get(Type::getInt32Ty(Context), 11);
	OffsetThreadInMutatorThreadConstant =     ConstantInt::get(Type::getInt32Ty(Context), 0);
  OffsetJNIInJavaThreadConstant =           ConstantInt::get(Type::getInt32Ty(Context), 1);
  OffsetJavaExceptionInJavaThreadConstant = ConstantInt::get(Type::getInt32Ty(Context), 2);
  
  ClassReadyConstant = ConstantInt::get(Type::getInt8Ty(Context), ready);
  
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

#ifndef WITHOUT_VTABLE
  VirtualLookupFunction = module->getFunction("j3VirtualTableLookup");
#endif

  GetLockFunction = module->getFunction("getLock");
  ThrowExceptionFromJITFunction =
    module->getFunction("j3ThrowExceptionFromJIT");
 
}
