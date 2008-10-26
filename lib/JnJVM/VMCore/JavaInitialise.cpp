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
 
  bootstrapLoader = gc_new(JnjvmBootstrapLoader)(0);
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
