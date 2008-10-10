//===----- PNetMSCorlib.cpp - The Pnet MSCorlib implementation ------------===//
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
                                           vm->asciizConstructUTF8("String"),
                                           vm->asciizConstructUTF8("System"),
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
  #define INIT(var, nameSpace, name, type, prim) {\
  var = (VMClass*)vm->coreAssembly->loadTypeFromName( \
                                           vm->asciizConstructUTF8(name),     \
                                           vm->asciizConstructUTF8(nameSpace),\
                                           false, false, false, true); \
  var->isPrimitive = prim; \
  if (type) { \
    var->naturalType = type;  \
    var->virtualType = type;  \
  }}

  INIT(MSCorlib::clrType,   "System.Reflection", "ClrType", 0, false);
  INIT(MSCorlib::assemblyReflection,   "System.Reflection", "Assembly", 0, false);
  INIT(MSCorlib::typedReference,   "System", "TypedReference", 0, false);
  INIT(MSCorlib::propertyType,   "System.Reflection", "ClrProperty", 0, false);
  INIT(MSCorlib::methodType,   "System.Reflection", "ClrMethod", 0, false);
  INIT(MSCorlib::resourceStreamType,   "System.Reflection", "ClrResourceStream", 0, false);

#undef INIT
  
  {
  MSCorlib::clrType->resolveType(false, false, NULL);
  std::vector<VMCommonClass*> args;
  args.push_back(MSCorlib::pVoid);
  args.push_back(MSCorlib::clrType);
  MSCorlib::ctorClrType = MSCorlib::clrType->lookupMethod(vm->asciizConstructUTF8(".ctor"), args, false, false);
  MSCorlib::typeClrType = MSCorlib::clrType->lookupField(vm->asciizConstructUTF8("privateData"), MSCorlib::pIntPtr, false, false);
  }

  {
  MSCorlib::assemblyReflection->resolveType(false, false, NULL);
  std::vector<VMCommonClass*> args;
  args.push_back(MSCorlib::pVoid);
  args.push_back(MSCorlib::assemblyReflection);
  MSCorlib::ctorAssemblyReflection = MSCorlib::assemblyReflection->lookupMethod(vm->asciizConstructUTF8(".ctor"), args, false, false);
  MSCorlib::assemblyAssemblyReflection = MSCorlib::assemblyReflection->lookupField(vm->asciizConstructUTF8("privateData"), MSCorlib::pIntPtr, false, false);
  }
  
  {
  MSCorlib::propertyType->resolveType(false, false, NULL);
  std::vector<VMCommonClass*> args;
  args.push_back(MSCorlib::pVoid);
  args.push_back(MSCorlib::propertyType);
  MSCorlib::ctorPropertyType = MSCorlib::propertyType->lookupMethod(vm->asciizConstructUTF8(".ctor"), args, false, false);
  MSCorlib::propertyPropertyType = MSCorlib::propertyType->lookupField(vm->asciizConstructUTF8("privateData"), MSCorlib::pIntPtr, false, false);
  }
  
  {
  MSCorlib::methodType->resolveType(false, false, NULL);
  std::vector<VMCommonClass*> args;
  args.push_back(MSCorlib::pVoid);
  args.push_back(MSCorlib::methodType);
  MSCorlib::ctorMethodType = MSCorlib::methodType->lookupMethod(vm->asciizConstructUTF8(".ctor"), args, false, false);
  MSCorlib::methodMethodType = MSCorlib::methodType->lookupField(vm->asciizConstructUTF8("privateData"), MSCorlib::pIntPtr, false, false);
  }
  
  {
  MSCorlib::resourceStreamType->resolveType(false, false, NULL);
  std::vector<VMCommonClass*> args;
  args.push_back(MSCorlib::pVoid);
  args.push_back(MSCorlib::resourceStreamType);
  args.push_back(MSCorlib::pIntPtr);
  args.push_back(MSCorlib::pSInt64);
  args.push_back(MSCorlib::pSInt64);
  MSCorlib::ctorResourceStreamType = MSCorlib::resourceStreamType->lookupMethod(vm->asciizConstructUTF8(".ctor"), args, false, false);
  }
  
  VMCommonClass* voidPtr = vm->coreAssembly->constructPointer(MSCorlib::pVoid, 1);
#define INIT(var, cl, type) {\
    cl->resolveType(false, false, NULL); \
    var = cl->lookupField(vm->asciizConstructUTF8("value_"), type, false, false); \
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


}

VMObject* Property::getPropertyDelegatee() {
  if (!delegatee) {
    delegatee = (*MSCorlib::propertyType)();
    (*MSCorlib::ctorPropertyType)(delegatee);
    (*MSCorlib::propertyPropertyType)(delegatee, (VMObject*)this);
  }
  return delegatee;
}

VMObject* VMMethod::getMethodDelegatee() {
  if (!delegatee) {
    delegatee = (*MSCorlib::methodType)();
    (*MSCorlib::ctorMethodType)(delegatee);
    (*MSCorlib::methodMethodType)(delegatee, (VMObject*)this);
  }
  return delegatee;
}

VMObject* VMCommonClass::getClassDelegatee() {
  if (!delegatee) {
    delegatee = (*MSCorlib::clrType)();
    (*MSCorlib::ctorClrType)(delegatee);
    (*MSCorlib::typeClrType)(delegatee, (VMObject*)this);
  }
  return delegatee;
}

VMObject* Assembly::getAssemblyDelegatee() {
  if (!delegatee) {
    delegatee = (*MSCorlib::assemblyReflection)();
    (*MSCorlib::ctorAssemblyReflection)(delegatee);
    (*MSCorlib::assemblyAssemblyReflection)(delegatee, (VMObject*)this);
  }
  return delegatee;
}

static void mapInitialThread(N3* vm) {
  VMClass* cl = (VMClass*)vm->coreAssembly->loadTypeFromName(
                                        vm->asciizConstructUTF8("Thread"),
                                        vm->asciizConstructUTF8("System.Threading"),
                                        true, true, true, true);
  VMObject* th = (*cl)();
  std::vector<VMCommonClass*> args;
  args.push_back(MSCorlib::pVoid);
  args.push_back(cl);
  args.push_back(MSCorlib::pIntPtr);
  VMMethod* meth = cl->lookupMethod(vm->asciizConstructUTF8(".ctor"), args, 
                                    false, false);
  VMThread* myth = VMThread::get();
  (*meth)(th, myth);
  myth->vmThread = th;
}

void MSCorlib::loadBootstrap(N3* vm) {
  mapInitialThread(vm);
}
