//===-------- JavaInitialise.cpp - Initialization of JnJVM ----------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <signal.h>
#include <vector>

#include "mvm/VirtualMachine.h"
#include "mvm/Threads/Locks.h"

#include "JavaArray.h"
#include "JavaCache.h"
#include "JavaClass.h"
#include "JavaConstantPool.h"
#include "JavaIsolate.h"
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

using namespace jnjvm;

static void initialiseVT() {

# define INIT(X) { \
  X fake; \
  X::VT = ((void**)(void*)(&fake))[0]; }

  INIT(CommonClass);
  INIT(Class);
  INIT(ClassArray);
  INIT(JavaThread);
  INIT(Jnjvm);
  INIT(ClassMap);
  INIT(StaticInstanceMap);
  INIT(DelegateeMap);
  INIT(JavaIsolate);
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

static void initialiseStatics() {
  
  Jnjvm* vm = JavaIsolate::bootstrapVM = JavaIsolate::allocateBootstrap();
  
  // Array initialization
  const UTF8* utf8OfChar = vm->asciizConstructUTF8("[C");
  JavaArray::ofChar = vm->constructArray(utf8OfChar,
                                         CommonClass::jnjvmClassLoader);
  ((UTF8*)utf8OfChar)->classOf = JavaArray::ofChar;
  


  
  ClassArray::InterfacesArray.push_back(
    vm->constructClass(vm->asciizConstructUTF8("java/lang/Cloneable"),
                       CommonClass::jnjvmClassLoader));
  
  ClassArray::InterfacesArray.push_back(
    vm->constructClass(vm->asciizConstructUTF8("java/io/Serializable"),
                       CommonClass::jnjvmClassLoader));
  
  ClassArray::SuperArray = 
    vm->constructClass(vm->asciizConstructUTF8("java/lang/Object"),
                       CommonClass::jnjvmClassLoader);
  
  JavaArray::ofChar->interfaces = ClassArray::InterfacesArray;
  JavaArray::ofChar->super = ClassArray::SuperArray;
  
  JavaArray::ofByte = vm->constructArray(vm->asciizConstructUTF8("[B"),
                                         CommonClass::jnjvmClassLoader);
  JavaArray::ofString = 
    vm->constructArray(vm->asciizConstructUTF8("[Ljava/lang/String;"),
                       CommonClass::jnjvmClassLoader);
  
  JavaArray::ofObject = 
    vm->constructArray(vm->asciizConstructUTF8("[Ljava/lang/Object;"),
                       CommonClass::jnjvmClassLoader);
  
  JavaArray::ofInt = vm->constructArray(vm->asciizConstructUTF8("[I"), 
                                        CommonClass::jnjvmClassLoader);
  
  JavaArray::ofBool = vm->constructArray(vm->asciizConstructUTF8("[Z"), 
                                        CommonClass::jnjvmClassLoader);
  
  JavaArray::ofLong = vm->constructArray(vm->asciizConstructUTF8("[J"), 
                                        CommonClass::jnjvmClassLoader);
  
  JavaArray::ofFloat = vm->constructArray(vm->asciizConstructUTF8("[F"), 
                                        CommonClass::jnjvmClassLoader);
  
  JavaArray::ofDouble = vm->constructArray(vm->asciizConstructUTF8("[D"), 
                                        CommonClass::jnjvmClassLoader);
  
  JavaArray::ofShort = vm->constructArray(vm->asciizConstructUTF8("[S"), 
                                        CommonClass::jnjvmClassLoader);
  
  // End array initialization
  
  AssessorDesc::initialise(vm);

  Attribut::codeAttribut = vm->asciizConstructUTF8("Code");
  Attribut::exceptionsAttribut = vm->asciizConstructUTF8("Exceptions");
  Attribut::constantAttribut = vm->asciizConstructUTF8("ConstantValue");
  Attribut::lineNumberTableAttribut =
    vm->asciizConstructUTF8("LineNumberTable");
  Attribut::innerClassesAttribut = vm->asciizConstructUTF8("InnerClasses");
  Attribut::sourceFileAttribut = vm->asciizConstructUTF8("SourceFile");
  
  Jnjvm::initName = vm->asciizConstructUTF8("<init>");
  Jnjvm::clinitName = vm->asciizConstructUTF8("<clinit>");
  Jnjvm::clinitType = vm->asciizConstructUTF8("()V");
  Jnjvm::runName = vm->asciizConstructUTF8("run");
  Jnjvm::prelib = vm->asciizConstructUTF8("lib");
#if defined(__MACH__)
  Jnjvm::postlib = vm->asciizConstructUTF8(".dylib");
#else 
  Jnjvm::postlib = vm->asciizConstructUTF8(".so");
#endif
  Jnjvm::mathName = vm->asciizConstructUTF8("java/lang/Math");

#define DEF_UTF8(var) \
  Jnjvm::var = vm->asciizConstructUTF8(#var)
  
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

extern "C" void ClasspathBoot();

void mvm::VirtualMachine::initialiseJVM() {
  if (!JavaIsolate::bootstrapVM) {
    initialiseVT();
    initialiseStatics();
  
    ClasspathBoot();
    Classpath::initialiseClasspath(JavaIsolate::bootstrapVM);
  }
}

void Jnjvm::runApplication(int argc, char** argv) {
  mvm::Thread::threadKey->set(((JavaIsolate*)this)->bootstrapThread);
  ((JavaIsolate*)this)->runMain(argc, argv);
}

mvm::VirtualMachine* mvm::VirtualMachine::createJVM() {
#if !defined(MULTIPLE_VM)
  JavaIsolate* vm = (JavaIsolate*)JavaIsolate::bootstrapVM;
#else
#ifdef SERVICE_VM
  ServiceDomain* vm = ServiceDomain::allocateService((JavaIsolate*)Jnjvm::bootstrapVM);
  vm->startExecution();
#else
  JavaIsolate* vm = JavaIsolate::allocateIsolate(JavaIsolate::bootstrapVM);
#endif
#endif
  return vm;
}
