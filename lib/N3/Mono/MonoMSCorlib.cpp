//===----- MonoMSCorlib.cpp - The Mono MSCorlib implementation ------------===//
//
//                              N3
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//


#include "mvm/JIT.h"

#include "Assembly.h"
#include "MSCorlib.h"
#include "N3.h"
#include "VMClass.h"
#include "VMObject.h"
#include "VMThread.h"

using namespace n3;

void MSCorlib::loadStringClass(N3* vm) {
  VMClass* type = (VMClass*)vm->coreAssembly->loadTypeFromName(
                                           vm->asciizToUTF8("String"),
                                           vm->asciizToUTF8("System"),
                                           false, false, false, true);
  MSCorlib::pString = type;
  MSCorlib::pObject->resolveType(true, false, NULL);
  MSCorlib::pObject->resolveVT();
  type->resolveType(true, false, NULL);
  type->resolveVT();

  uint64 size = mvm::MvmModule::getTypeSize(type->virtualType->getContainedType(0)) + sizeof(const UTF8*) + sizeof(llvm::GlobalVariable*);
  type->virtualInstance = 
    (VMObject*)gc::operator new(size, type->virtualInstance->getVirtualTable());
  type->virtualInstance->initialise(type);
}


void MSCorlib::initialise(N3* vm) {
  
  VMClass* runtimeTypeHandle = 0;
  VMClass* realClrType = 0;
  #define INIT(var, nameSpace, name, type, prim) {\
  var = (VMClass*)vm->coreAssembly->loadTypeFromName( \
                                           vm->asciizToUTF8(name),     \
                                           vm->asciizToUTF8(nameSpace),\
                                           false, false, false, true); \
  var->isPrimitive = prim; \
  if (type) { \
    var->naturalType = type;  \
    var->virtualType = type;  \
  }}

  INIT(MSCorlib::clrType,   "System", "MonoType", 0, false);
  INIT(realClrType,   "System", "Type", 0, false);
  INIT(runtimeTypeHandle,   "System", "RuntimeTypeHandle", 0, false);
  /*
  INIT(MSCorlib::assemblyReflection,   "System.Reflection", "Assembly", 0, false);
  INIT(MSCorlib::typedReference,   "System", "TypedReference", 0, false);
  INIT(MSCorlib::propertyType,   "System.Reflection", "ClrProperty", 0, false);
  INIT(MSCorlib::methodType,   "System.Reflection", "ClrMethod", 0, false);
  INIT(MSCorlib::resourceStreamType,   "System.Reflection", "ClrResourceStream", 0, false);*/
#undef INIT
  
  {
  MSCorlib::clrType->resolveType(false, false, NULL);
  MSCorlib::typeClrType = realClrType->lookupField(vm->asciizToUTF8("_impl"), runtimeTypeHandle, false, false);
  }

/*
  {
  MSCorlib::assemblyReflection->resolveType(false, false);
  std::vector<VMCommonClass*> args;
  args.push_back(MSCorlib::pVoid);
  args.push_back(MSCorlib::assemblyReflection);
  MSCorlib::ctorAssemblyReflection = MSCorlib::assemblyReflection->lookupMethod(vm->asciizToUTF8(".ctor"), args, false, false);
  MSCorlib::assemblyAssemblyReflection = MSCorlib::assemblyReflection->lookupField(vm->asciizToUTF8("privateData"), MSCorlib::pIntPtr, false, false);
  }
  
  {
  MSCorlib::propertyType->resolveType(false, false);
  std::vector<VMCommonClass*> args;
  args.push_back(MSCorlib::pVoid);
  args.push_back(MSCorlib::propertyType);
  MSCorlib::ctorPropertyType = MSCorlib::propertyType->lookupMethod(vm->asciizToUTF8(".ctor"), args, false, false);
  MSCorlib::propertyPropertyType = MSCorlib::propertyType->lookupField(vm->asciizToUTF8("privateData"), MSCorlib::pIntPtr, false, false);
  }
  
  {
  MSCorlib::methodType->resolveType(false, false);
  std::vector<VMCommonClass*> args;
  args.push_back(MSCorlib::pVoid);
  args.push_back(MSCorlib::methodType);
  MSCorlib::ctorMethodType = MSCorlib::methodType->lookupMethod(vm->asciizToUTF8(".ctor"), args, false, false);
  MSCorlib::methodMethodType = MSCorlib::methodType->lookupField(vm->asciizToUTF8("privateData"), MSCorlib::pIntPtr, false, false);
  }
  
  {
  MSCorlib::resourceStreamType->resolveType(false, false);
  std::vector<VMCommonClass*> args;
  args.push_back(MSCorlib::pVoid);
  args.push_back(MSCorlib::resourceStreamType);
  args.push_back(MSCorlib::pIntPtr);
  args.push_back(MSCorlib::pSInt64);
  args.push_back(MSCorlib::pSInt64);
  MSCorlib::ctorResourceStreamType = MSCorlib::resourceStreamType->lookupMethod(vm->asciizToUTF8(".ctor"), args, false, false);
  }
  
  VMCommonClass* voidPtr = vm->coreAssembly->constructPointer(MSCorlib::pVoid, 1);
#define INIT(var, cl, type) {\
    cl->resolveType(false, false); \
    var = cl->lookupField(vm->asciizToUTF8("value_"), type, false, false); \
  }
  
  INIT(MSCorlib::ctorBoolean,  MSCorlib::pBoolean, MSCorlib::pBoolean);
  INIT(MSCorlib::ctorUInt8, MSCorlib::pUInt8, MSCorlib::pUInt8);
  INIT(MSCorlib::ctorSInt8, MSCorlib::pSInt8, MSCorlib::pSInt8);
  INIT(MSCorlib::ctorChar,  MSCorlib::pChar, MSCorlib::pChar);
  INIT(MSCorlib::ctorSInt16, MSCorlib::pSInt16, MSCorlib::pSInt16);
  INIT(MSCorlib::ctorUInt16, MSCorlib::pUInt16, MSCorlib::pUInt16);
  INIT(MSCorlib::ctorSInt32, MSCorlib::pSInt32, MSCorlib::pSInt32);
  INIT(MSCorlib::ctorUInt32, MSCorlib::pUInt32, MSCorlib::pUInt32);
  INIT(MSCorlib::ctorSInt64, MSCorlib::pSInt64, MSCorlib::pSInt64);
  INIT(MSCorlib::ctorUInt64, MSCorlib::pUInt64, MSCorlib::pUInt64);
  INIT(MSCorlib::ctorIntPtr, MSCorlib::pIntPtr, voidPtr);
  INIT(MSCorlib::ctorUIntPtr, MSCorlib::pUIntPtr, voidPtr);
  INIT(MSCorlib::ctorDouble, MSCorlib::pDouble, MSCorlib::pDouble);
  INIT(MSCorlib::ctorFloat, MSCorlib::pFloat, MSCorlib::pFloat);

#undef INIT

  */
}

VMObject* Property::getPropertyDelegatee() {
  if (!delegatee) {
    VMThread::get()->getVM()->error("implement me");  
  }
  return delegatee;
}

VMObject* VMMethod::getMethodDelegatee() {
  if (!delegatee) {
    VMThread::get()->getVM()->error("implement me");  
  }
  return delegatee;
}

VMObject* VMCommonClass::getClassDelegatee() {
  if (!delegatee) {
    delegatee = (*MSCorlib::clrType)();
    (*MSCorlib::typeClrType)(delegatee, (VMObject*)this);
  }
  return delegatee;
}

VMObject* Assembly::getAssemblyDelegatee() {
  if (!delegatee) {
    VMThread::get()->getVM()->error("implement me");  
  }
  return delegatee;
}

void MSCorlib::loadBootstrap(N3* vm) {
  VMClass* cl = (VMClass*)vm->coreAssembly->loadTypeFromName(
                                        vm->asciizToUTF8("Thread"),
                                        vm->asciizToUTF8("System.Threading"),
                                        true, true, true, true);
  VMObject* th = (*cl)();
  declare_gcroot(VMThread*, myth) = VMThread::get();
  myth->vmThread = th;

}
