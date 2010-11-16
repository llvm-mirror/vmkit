//===-------------- J3Intrinsics.h - Intrinsics of J3 ---------------------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef J3_INTRINSICS_H
#define J3_INTRINSICS_H

#include "mvm/JIT.h"

namespace j3 {

class J3Intrinsics : public mvm::BaseIntrinsics {

public:
  const llvm::Type* JavaArrayUInt8Type;
  const llvm::Type* JavaArraySInt8Type;
  const llvm::Type* JavaArrayUInt16Type;
  const llvm::Type* JavaArraySInt16Type;
  const llvm::Type* JavaArrayUInt32Type;
  const llvm::Type* JavaArraySInt32Type;
  const llvm::Type* JavaArrayLongType;
  const llvm::Type* JavaArrayFloatType;
  const llvm::Type* JavaArrayDoubleType;
  const llvm::Type* JavaArrayObjectType;
  
  const llvm::Type* VTType;
  const llvm::Type* JavaObjectType;
  const llvm::Type* JavaArrayType;
  const llvm::Type* JavaCommonClassType;
  const llvm::Type* JavaClassType;
  const llvm::Type* JavaClassArrayType;
  const llvm::Type* JavaClassPrimitiveType;
  const llvm::Type* ConstantPoolType;
  const llvm::Type* CodeLineInfoType;
  const llvm::Type* UTF8Type;
  const llvm::Type* JavaMethodType;
  const llvm::Type* JavaFieldType;
  const llvm::Type* AttributType;
  const llvm::Type* JavaThreadType;
  const llvm::Type* MutatorThreadType;
  
#ifdef ISOLATE_SHARING
  const llvm::Type* JnjvmType;
#endif
  
  llvm::Function* StartJNIFunction;
  llvm::Function* EndJNIFunction;
  llvm::Function* InterfaceLookupFunction;
  llvm::Function* VirtualFieldLookupFunction;
  llvm::Function* StaticFieldLookupFunction;
  llvm::Function* PrintExecutionFunction;
  llvm::Function* PrintMethodStartFunction;
  llvm::Function* PrintMethodEndFunction;
  llvm::Function* InitialiseClassFunction;
  llvm::Function* InitialisationCheckFunction;
  llvm::Function* ForceInitialisationCheckFunction;
  llvm::Function* ForceLoadedCheckFunction;
  llvm::Function* ClassLookupFunction;
  llvm::Function* StringLookupFunction;
  
  llvm::Function* ResolveVirtualStubFunction;
  llvm::Function* ResolveSpecialStubFunction;
  llvm::Function* ResolveStaticStubFunction;

#ifndef WITHOUT_VTABLE
  llvm::Function* VirtualLookupFunction;
#endif
  llvm::Function* IsAssignableFromFunction;
  llvm::Function* IsSecondaryClassFunction;
  llvm::Function* GetDepthFunction;
  llvm::Function* GetDisplayFunction;
  llvm::Function* GetVTInDisplayFunction;
  llvm::Function* GetStaticInstanceFunction;
  llvm::Function* AquireObjectFunction;
  llvm::Function* ReleaseObjectFunction;
  llvm::Function* GetConstantPoolAtFunction;
  llvm::Function* MultiCallNewFunction;
  llvm::Function* GetArrayClassFunction;

#ifdef ISOLATE_SHARING
  llvm::Function* GetCtpClassFunction;
  llvm::Function* GetJnjvmExceptionClassFunction;
  llvm::Function* GetJnjvmArrayClassFunction;
  llvm::Function* StaticCtpLookupFunction;
  llvm::Function* SpecialCtpLookupFunction;
#endif

#ifdef SERVICE
  llvm::Function* ServiceCallStartFunction;
  llvm::Function* ServiceCallStopFunction;
#endif

  llvm::Function* GetClassDelegateeFunction;
  llvm::Function* RuntimeDelegateeFunction;
  llvm::Function* ArrayLengthFunction;
  llvm::Function* GetVTFunction;
  llvm::Function* GetIMTFunction;
  llvm::Function* GetClassFunction;
  llvm::Function* GetVTFromClassFunction;
  llvm::Function* GetVTFromClassArrayFunction;
  llvm::Function* GetVTFromCommonClassFunction;
  llvm::Function* GetObjectSizeFromClassFunction;
  llvm::Function* GetBaseClassVTFromVTFunction;

  llvm::Function* GetLockFunction;
  
  llvm::Function* GetFinalInt8FieldFunction;
  llvm::Function* GetFinalInt16FieldFunction;
  llvm::Function* GetFinalInt32FieldFunction;
  llvm::Function* GetFinalLongFieldFunction;
  llvm::Function* GetFinalFloatFieldFunction;
  llvm::Function* GetFinalDoubleFieldFunction;
  
  llvm::Constant* JavaArraySizeOffsetConstant;
  llvm::Constant* JavaArrayElementsOffsetConstant;
  llvm::Constant* JavaObjectLockOffsetConstant;
  llvm::Constant* JavaObjectVTOffsetConstant;

  llvm::Constant* OffsetAccessInCommonClassConstant;
  llvm::Constant* IsArrayConstant;
  llvm::Constant* IsPrimitiveConstant;
  llvm::Constant* OffsetObjectSizeInClassConstant;
  llvm::Constant* OffsetVTInClassConstant;
  llvm::Constant* OffsetTaskClassMirrorInClassConstant;
  llvm::Constant* OffsetStaticInstanceInTaskClassMirrorConstant;
  llvm::Constant* OffsetInitializedInTaskClassMirrorConstant;
  llvm::Constant* OffsetStatusInTaskClassMirrorConstant;
  
  llvm::Constant* OffsetDoYieldInThreadConstant;
  llvm::Constant* OffsetIsolateIDInThreadConstant;
  llvm::Constant* OffsetVMInThreadConstant;
  llvm::Constant* OffsetCXXExceptionInThreadConstant;
	llvm::Constant* OffsetThreadInMutatorThreadConstant;
  llvm::Constant* OffsetJNIInJavaThreadConstant;
  llvm::Constant* OffsetJavaExceptionInJavaThreadConstant;
  
  llvm::Constant* OffsetClassInVTConstant;
  llvm::Constant* OffsetDepthInVTConstant;
  llvm::Constant* OffsetDisplayInVTConstant;
  llvm::Constant* OffsetBaseClassVTInVTConstant;
  llvm::Constant* OffsetIMTInVTConstant;
  
  llvm::Constant* OffsetBaseClassInArrayClassConstant;
  llvm::Constant* OffsetLogSizeInPrimitiveClassConstant;
  
  llvm::Constant* ClassReadyConstant;

  llvm::Constant* JavaObjectNullConstant;
  llvm::Constant* MaxArraySizeConstant;
  llvm::Constant* JavaArraySizeConstant;

  llvm::Function* ThrowExceptionFunction;
  llvm::Function* NullPointerExceptionFunction;
  llvm::Function* IndexOutOfBoundsExceptionFunction;
  llvm::Function* ClassCastExceptionFunction;
  llvm::Function* OutOfMemoryErrorFunction;
  llvm::Function* StackOverflowErrorFunction;
  llvm::Function* NegativeArraySizeExceptionFunction;
  llvm::Function* ArrayStoreExceptionFunction;
  llvm::Function* ArithmeticExceptionFunction;
  llvm::Function* ThrowExceptionFromJITFunction;
  

  J3Intrinsics(llvm::Module*);
  
  static void initialise();

};

}

#endif
