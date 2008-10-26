//===-------- JavaInitialise.cpp - Initialization of JnJVM ----------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "mvm/VirtualMachine.h"

#include "JavaArray.h"
#include "JavaObject.h"
#include "JavaThread.h"
#include "Jnjvm.h"

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

#ifdef ISOLATE_SHARING
void mvm::VirtualMachine::initialiseJVM() {
  initialiseVT();
  if (!JnjvmSharedLoader::sharedLoader) {
    JnjvmSharedLoader::sharedLoader = JnjvmSharedLoader::createSharedLoader();
  }
}

mvm::VirtualMachine* mvm::VirtualMachine::createJVM() {
  JnjvmBootstraLoader* bootstrapLoader = gc_new(JnjvmBootstrapLoader)(0);
  Jnjvm* vm = gc_new(Jnjvm)(bootstrapLoader);
  return vm;
}
#else
  
void mvm::VirtualMachine::initialiseJVM() {
  initialiseVT();
  if (!JnjvmClassLoader::bootstrapLoader) {
    JnjvmClassLoader::bootstrapLoader = gc_new(JnjvmBootstrapLoader)(0);
  }
}

mvm::VirtualMachine* mvm::VirtualMachine::createJVM() {
  Jnjvm* vm = gc_new(Jnjvm)(JnjvmClassLoader::bootstrapLoader);
  return vm;
}

#endif
