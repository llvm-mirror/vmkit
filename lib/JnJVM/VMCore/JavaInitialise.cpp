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
#include "Reader.h"
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

  INIT(JavaArray);
  INIT(CommonClass);
  INIT(Class);
  INIT(ClassArray);
  INIT(LockObj);
  INIT(JavaObject);
  INIT(JavaThread);
  INIT(Jnjvm);
  INIT(Reader);
  INIT(ZipFile);
  INIT(ZipArchive);
  INIT(ClassMap);
  INIT(ZipFileMap);
  INIT(StringMap);
  INIT(JavaIsolate);
  INIT(JavaString);
#ifdef SERVICE_VM
  INIT(ServiceDomain);
#endif
#undef INIT

#define INIT(X) { \
  X fake; \
  void* V = ((void**)(void*)(&fake))[0]; \
  X::VT = (VirtualTable*)malloc(12 * sizeof(void*) + VT_SIZE); \
  memcpy(X::VT, V, VT_SIZE); }

  INIT(ArrayUInt8);
  INIT(ArraySInt8);
  INIT(ArrayUInt16);
  INIT(ArraySInt16);
  INIT(ArrayUInt32);
  INIT(ArraySInt32);
  INIT(ArrayLong);
  INIT(ArrayFloat);
  INIT(ArrayDouble);
  INIT(ArrayObject);
#undef INIT
}

static void initialiseStatics() {
  JavaObject::globalLock = mvm::Lock::allocNormal();
  //mvm::Object::pushRoot((mvm::Object*)JavaObject::globalLock);

  Jnjvm* vm = JavaIsolate::bootstrapVM = JavaIsolate::allocateBootstrap();
  mvm::Object::pushRoot((mvm::Object*)JavaIsolate::bootstrapVM);
  
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

#undef DEF_UTF8
 
}

extern "C" void ClasspathBoot();

void handler(int val, siginfo_t* info, void* addr) {
  printf("Crash in JnJVM at %p\n", addr);
  assert(0);
}

extern "C" int boot() {
  
  struct sigaction sa;
  
  sigaction(SIGINT, 0, &sa);
  sa.sa_sigaction = handler;
  sa.sa_flags |= (SA_RESTART | SA_SIGINFO | SA_NODEFER);
  sigaction(SIGINT, &sa, 0);
  
  sigaction(SIGILL, 0, &sa);
  sa.sa_sigaction = handler;
  sa.sa_flags |= (SA_RESTART | SA_SIGINFO | SA_NODEFER);
  sigaction(SIGILL, &sa, 0);
  
  sigaction(SIGSEGV, 0, &sa);
  sa.sa_sigaction = handler;
  sa.sa_flags |= (SA_RESTART | SA_SIGINFO | SA_NODEFER);
  sigaction(SIGSEGV, &sa, 0);

  initialiseVT();
  initialiseStatics();
  
  ClasspathBoot();
  Classpath::initialiseClasspath(JavaIsolate::bootstrapVM);
  return 0; 
}

extern "C" int start_app(int argc, char** argv) {
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
  vm->runMain(argc, argv);
  return 0;
}
