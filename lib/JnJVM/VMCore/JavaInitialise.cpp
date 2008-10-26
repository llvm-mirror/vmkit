//===-------- JavaInitialise.cpp - Initialization of JnJVM ----------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "mvm/Allocator.h"
#include "mvm/VirtualMachine.h"
#include "mvm/Threads/Locks.h"
#include "mvm/Threads/Thread.h"

#include "JavaArray.h"
#include "JavaCache.h"
#include "JavaClass.h"
#include "JavaConstantPool.h"
#include "JavaJIT.h"
#include "JavaObject.h"
#include "JavaString.h"
#include "JavaThread.h"
#include "JavaTypes.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"
#include "JnjvmModuleProvider.h"
#include "NativeUtil.h"
#include "LockedMap.h"
#include "Zip.h"

#ifdef SERVICE_VM
#include "ServiceDomain.h"
#endif

#ifdef ISOLATE_SHARING
#include "SharedMaps.h"
#include "IsolateSharedLoader.h"
#endif

using namespace jnjvm;

static void initialiseVT() {

# define INIT(X) { \
  X fake; \
  X::VT = ((void**)(void*)(&fake))[0]; }

  INIT(JavaThread);
  INIT(Jnjvm);
  INIT(JnjvmBootstrapLoader);
  INIT(JnjvmClassLoader);
  INIT(LockObj);
#ifdef ISOLATE_SHARING
  INIT(JnjvmSharedLoader);
  INIT(SharedClassByteMap);
  INIT(UserClass);
  INIT(UserClassArray);
  INIT(UserConstantPool);
#endif
#ifdef SERVICE_VM
  INIT(ServiceDomain);
#endif
#undef INIT

#define INIT(X) { \
  X fake; \
  void* V = ((void**)(void*)(&fake))[0]; \
  X::VT = (VirtualTable*)malloc(12 * sizeof(void*) + VT_SIZE); \
  memcpy(X::VT, V, VT_SIZE); \
  ((void**)X::VT)[0] = 0; }

  INIT(JavaObject);
  INIT(JavaArray);
  INIT(ArrayObject);
#undef INIT
}

void Jnjvm::initialiseStatics() {

#ifdef ISOLATE_SHARING
  if (!JnjvmSharedLoader::sharedLoader) {
    JnjvmSharedLoader::sharedLoader = JnjvmSharedLoader::createSharedLoader();
  }
#endif
 
  JnjvmBootstrapLoader* JCL = bootstrapLoader = gc_new(JnjvmBootstrapLoader)(0);
  
  // Create the name of char arrays.
  const UTF8* utf8OfChar = JCL->asciizConstructUTF8("[C");

  // Create the base class of char arrays.
  JCL->upcalls->OfChar = UPCALL_PRIMITIVE_CLASS(JCL, "char", 2);
  
  // Create the char array.
  JCL->upcalls->ArrayOfChar = JCL->constructArray(utf8OfChar,
                                                  JCL->upcalls->OfChar);

  // Alright, now we can repair the damage: set the class to the UTF8s created
  // and set the array class of UTF8s.
  ((UTF8*)utf8OfChar)->classOf = JCL->upcalls->ArrayOfChar;
  ((UTF8*)JCL->upcalls->OfChar->name)->classOf = JCL->upcalls->ArrayOfChar;
  JCL->hashUTF8->array = JCL->upcalls->ArrayOfChar;
 
  // Create the byte array, so that bytes for classes can be created.
  JCL->upcalls->OfByte = UPCALL_PRIMITIVE_CLASS(JCL, "byte", 1);
  JCL->upcalls->ArrayOfByte = 
    JCL->constructArray(JCL->asciizConstructUTF8("[B"), JCL->upcalls->OfByte);

  JCL->InterfacesArray = 
    (Class**)JCL->allocator.Allocate(2 * sizeof(UserClass*));

  // Now we can create the super and interfaces of arrays.
  JCL->InterfacesArray[0] = 
    JCL->loadName(JCL->asciizConstructUTF8("java/lang/Cloneable"), false,
                  false);
  
  JCL->InterfacesArray[1] = 
    JCL->loadName(JCL->asciizConstructUTF8("java/io/Serializable"), false,
                  false);
  
  JCL->SuperArray = 
    JCL->loadName(JCL->asciizConstructUTF8("java/lang/Object"), false,
                  false);
  
#ifdef ISOLATE_SHARING
  if (!ClassArray::SuperArray) {
    ClassArray::SuperArray = JCL->SuperArray->classDef;
    ClassArray::InterfacesArray[0] = ((Class*)JCL->InterfacesArray[0]->classDef);
    ClassArray::InterfacesArray[1] = ((Class*)JCL->InterfacesArray[1]->classDef);
  }
#else
  ClassArray::SuperArray = JCL->SuperArray;
  ClassArray::InterfacesArray = JCL->InterfacesArray;
#endif
  
  // And repair the damage: set the interfaces and super of array classes already
  // created.
  JCL->upcalls->ArrayOfChar->setInterfaces(JCL->InterfacesArray);
  JCL->upcalls->ArrayOfChar->setSuper(JCL->SuperArray);
  JCL->upcalls->ArrayOfByte->setInterfaces(JCL->InterfacesArray);
  JCL->upcalls->ArrayOfByte->setSuper(JCL->SuperArray);
  
  // Yay, create the other primitive types.
  JCL->upcalls->OfBool = UPCALL_PRIMITIVE_CLASS(JCL, "boolean", 1);
  JCL->upcalls->OfShort = UPCALL_PRIMITIVE_CLASS(JCL, "short", 2);
  JCL->upcalls->OfInt = UPCALL_PRIMITIVE_CLASS(JCL, "int", 4);
  JCL->upcalls->OfLong = UPCALL_PRIMITIVE_CLASS(JCL, "long", 8);
  JCL->upcalls->OfFloat = UPCALL_PRIMITIVE_CLASS(JCL, "float", 4);
  JCL->upcalls->OfDouble = UPCALL_PRIMITIVE_CLASS(JCL, "double", 8);
  JCL->upcalls->OfVoid = UPCALL_PRIMITIVE_CLASS(JCL, "void", 0);
  
  // And finally create the primitive arrays.
  JCL->upcalls->ArrayOfInt = 
    JCL->constructArray(JCL->asciizConstructUTF8("[I"), JCL->upcalls->OfInt);
  
  JCL->upcalls->ArrayOfBool = 
    JCL->constructArray(JCL->asciizConstructUTF8("[Z"), JCL->upcalls->OfBool);
  
  JCL->upcalls->ArrayOfLong = 
    JCL->constructArray(JCL->asciizConstructUTF8("[J"), JCL->upcalls->OfLong);
  
  JCL->upcalls->ArrayOfFloat = 
    JCL->constructArray(JCL->asciizConstructUTF8("[F"), JCL->upcalls->OfFloat);
  
  JCL->upcalls->ArrayOfDouble = 
    JCL->constructArray(JCL->asciizConstructUTF8("[D"), JCL->upcalls->OfDouble);
  
  JCL->upcalls->ArrayOfShort = 
    JCL->constructArray(JCL->asciizConstructUTF8("[S"), JCL->upcalls->OfShort);
  
  JCL->upcalls->ArrayOfString = 
    JCL->constructArray(JCL->asciizConstructUTF8("[Ljava/lang/String;"));
  
  JCL->upcalls->ArrayOfObject = 
    JCL->constructArray(JCL->asciizConstructUTF8("[Ljava/lang/Object;"));
  
  
  Attribut::codeAttribut = JCL->asciizConstructUTF8("Code");
  Attribut::exceptionsAttribut = JCL->asciizConstructUTF8("Exceptions");
  Attribut::constantAttribut = JCL->asciizConstructUTF8("ConstantValue");
  Attribut::lineNumberTableAttribut =
    JCL->asciizConstructUTF8("LineNumberTable");
  Attribut::innerClassesAttribut = JCL->asciizConstructUTF8("InnerClasses");
  Attribut::sourceFileAttribut = JCL->asciizConstructUTF8("SourceFile");
  
  JCL->initName = JCL->asciizConstructUTF8("<init>");
  JCL->clinitName = JCL->asciizConstructUTF8("<clinit>");
  JCL->clinitType = JCL->asciizConstructUTF8("()V");
  JCL->runName = JCL->asciizConstructUTF8("run");
  JCL->prelib = JCL->asciizConstructUTF8("lib");
#if defined(__MACH__)
  JCL->postlib = JCL->asciizConstructUTF8(".dylib");
#else 
  JCL->postlib = JCL->asciizConstructUTF8(".so");
#endif
  JCL->mathName = JCL->asciizConstructUTF8("java/lang/Math");
  JCL->stackWalkerName = JCL->asciizConstructUTF8("gnu/classpath/VMStackWalker");
  JCL->NoClassDefFoundError = 
    JCL->asciizConstructUTF8("java/lang/NoClassDefFoundError");

#define DEF_UTF8(var) \
  JCL->var = JCL->asciizConstructUTF8(#var)
  
  DEF_UTF8(abs);
  DEF_UTF8(sqrt);
  DEF_UTF8(sin);
  DEF_UTF8(cos);
  DEF_UTF8(tan);
  DEF_UTF8(asin);
  DEF_UTF8(acos);
  DEF_UTF8(atan);
  DEF_UTF8(atan2);
  DEF_UTF8(exp);
  DEF_UTF8(log);
  DEF_UTF8(pow);
  DEF_UTF8(ceil);
  DEF_UTF8(floor);
  DEF_UTF8(rint);
  DEF_UTF8(cbrt);
  DEF_UTF8(cosh);
  DEF_UTF8(expm1);
  DEF_UTF8(hypot);
  DEF_UTF8(log10);
  DEF_UTF8(log1p);
  DEF_UTF8(sinh);
  DEF_UTF8(tanh);
  DEF_UTF8(finalize);

#undef DEF_UTF8
}

void mvm::VirtualMachine::initialiseJVM() {
#ifndef ISOLATE_SHARING
  if (!JnjvmClassLoader::bootstrapLoader) {
    initialiseVT();
    Jnjvm::initialiseStatics();
    JnjvmClassLoader::bootstrapLoader = Jnjvm::bootstrapLoader;
  }
#else
  initialiseVT(); 
#endif
}

mvm::VirtualMachine* mvm::VirtualMachine::createJVM() {
#ifdef SERVICE_VM
  ServiceDomain* vm = ServiceDomain::allocateService();
  vm->startExecution();
#else
  Jnjvm* vm = gc_new(Jnjvm)(0);
#endif
  return vm;
}
