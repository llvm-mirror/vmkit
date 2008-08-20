//===-------- JavaInitialise.cpp - Initialization of JnJVM ----------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <vector>

#include "mvm/VirtualMachine.h"
#include "mvm/Threads/Locks.h"

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

using namespace jnjvm;

static void initialiseVT() {

# define INIT(X) { \
  X fake; \
  X::VT = ((void**)(void*)(&fake))[0]; }

  INIT(Class);
  INIT(ClassArray);
  INIT(JavaThread);
  INIT(Jnjvm);
  INIT(ClassMap);
  INIT(StaticInstanceMap);
  INIT(DelegateeMap);
  INIT(JnjvmBootstrapLoader);
  INIT(JnjvmClassLoader);
#ifdef MULTIPLE_VM
  INIT(JnjvmSharedLoader);
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

static void initialiseStatics() {
  
  JnjvmClassLoader* JCL = JnjvmClassLoader::bootstrapLoader = 
    JnjvmBootstrapLoader::createBootstrapLoader();
  
  // Array initialization
  const UTF8* utf8OfChar = JCL->asciizConstructUTF8("[C");
  JavaArray::ofChar = JCL->constructArray(utf8OfChar);
  ((UTF8*)utf8OfChar)->classOf = JavaArray::ofChar;
  

 
  ClassArray::InterfacesArray.push_back(
    JCL->constructClass(JCL->asciizConstructUTF8("java/lang/Cloneable")));
  
  ClassArray::InterfacesArray.push_back(
    JCL->constructClass(JCL->asciizConstructUTF8("java/io/Serializable")));
  
  ClassArray::SuperArray = 
    JCL->constructClass(JCL->asciizConstructUTF8("java/lang/Object"));
  
  JavaArray::ofChar->interfaces = ClassArray::InterfacesArray;
  JavaArray::ofChar->super = ClassArray::SuperArray;
  
  JavaArray::ofByte = JCL->constructArray(JCL->asciizConstructUTF8("[B"));
  JavaArray::ofString = 
    JCL->constructArray(JCL->asciizConstructUTF8("[Ljava/lang/String;"));
  
  JavaArray::ofObject = 
    JCL->constructArray(JCL->asciizConstructUTF8("[Ljava/lang/Object;"));
  
  JavaArray::ofInt = JCL->constructArray(JCL->asciizConstructUTF8("[I"));
  
  JavaArray::ofBool = JCL->constructArray(JCL->asciizConstructUTF8("[Z"));
  
  JavaArray::ofLong = JCL->constructArray(JCL->asciizConstructUTF8("[J"));
  
  JavaArray::ofFloat = JCL->constructArray(JCL->asciizConstructUTF8("[F"));
  
  JavaArray::ofDouble = JCL->constructArray(JCL->asciizConstructUTF8("[D"));
  
  JavaArray::ofShort = JCL->constructArray(JCL->asciizConstructUTF8("[S"));
  
  // End array initialization
  
  AssessorDesc::initialise(JCL);

  Attribut::codeAttribut = JCL->asciizConstructUTF8("Code");
  Attribut::exceptionsAttribut = JCL->asciizConstructUTF8("Exceptions");
  Attribut::constantAttribut = JCL->asciizConstructUTF8("ConstantValue");
  Attribut::lineNumberTableAttribut =
    JCL->asciizConstructUTF8("LineNumberTable");
  Attribut::innerClassesAttribut = JCL->asciizConstructUTF8("InnerClasses");
  Attribut::sourceFileAttribut = JCL->asciizConstructUTF8("SourceFile");
  
  Jnjvm::initName = JCL->asciizConstructUTF8("<init>");
  Jnjvm::clinitName = JCL->asciizConstructUTF8("<clinit>");
  Jnjvm::clinitType = JCL->asciizConstructUTF8("()V");
  Jnjvm::runName = JCL->asciizConstructUTF8("run");
  Jnjvm::prelib = JCL->asciizConstructUTF8("lib");
#if defined(__MACH__)
  Jnjvm::postlib = JCL->asciizConstructUTF8(".dylib");
#else 
  Jnjvm::postlib = JCL->asciizConstructUTF8(".so");
#endif
  Jnjvm::mathName = JCL->asciizConstructUTF8("java/lang/Math");
  Jnjvm::NoClassDefFoundError = 
    JCL->asciizConstructUTF8("java/lang/NoClassDefFoundError");

#define DEF_UTF8(var) \
  Jnjvm::var = JCL->asciizConstructUTF8(#var)
  
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
  if (!JnjvmClassLoader::bootstrapLoader) {
    initialiseVT();
    initialiseStatics();
  
    ClasspathBoot();
    Classpath::initialiseClasspath(JnjvmClassLoader::bootstrapLoader);
  }
}

void Jnjvm::runApplication(int argc, char** argv) {
  mvm::Thread::threadKey->set(this->bootstrapThread);
  this->runMain(argc, argv);
}

mvm::VirtualMachine* mvm::VirtualMachine::createJVM() {
#ifdef SERVICE_VM
  ServiceDomain* vm = ServiceDomain::allocateService();
  vm->startExecution();
#else
  Jnjvm* vm = Jnjvm::allocateIsolate();
#endif
  return vm;
}
