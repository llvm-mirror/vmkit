//===-------- JavaInitialise.cpp - Initialization of JnJVM ----------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "mvm/VirtualMachine.h"
#include "jnjvm/JavaCompiler.h"

#include "JavaArray.h"
#include "JavaClass.h"
#include "JavaObject.h"
#include "Jnjvm.h"
#include "JnjvmClassLoader.h"

#ifdef ISOLATE_SHARING
#include "SharedMaps.h"
#include "IsolateSharedLoader.h"
#endif

using namespace jnjvm;


#ifdef ISOLATE_SHARING
JnjvmClassLoader* mvm::VirtualMachine::initialiseJVM(JavaCompiler* Comp) {
  JnjvmSharedLoader::sharedLoader = JnjvmSharedLoader::createSharedLoader(Comp);
  return JnjvmSharedLoader::sharedLoader;
}

mvm::VirtualMachine* mvm::VirtualMachine::createJVM(JnjvClassLoader* JCL) {
  mvm::BumpPtrAllocator* A = new mvm::BumpPtrAllocator();
  mvm::BumpPtrAllocator* C = new mvm::BumpPtrAllocator();
  JnjvmBootstraLoader* bootstrapLoader = 
    new(*C) JnjvmBootstrapLoader(*C, JCL->getCompiler());
  Jnjvm* vm = new(*A, "VM") Jnjvm(*A, bootstrapLoader);
  vm->scanner = JCL->getCompiler()->createStackScanner();
  return vm;
}
#else
  
JnjvmClassLoader*
mvm::VirtualMachine::initialiseJVM(JavaCompiler* Comp, bool dlLoad) {
  mvm::BumpPtrAllocator* A = new mvm::BumpPtrAllocator();
  return new(*A, "Bootstrap loader") JnjvmBootstrapLoader(*A, Comp, dlLoad);
}

mvm::VirtualMachine* mvm::VirtualMachine::createJVM(JnjvmClassLoader* C) {
  mvm::BumpPtrAllocator* A = new mvm::BumpPtrAllocator();
  Jnjvm* vm = new(*A, "VM") Jnjvm(*A, (JnjvmBootstrapLoader*)C);
  vm->scanner = C->getCompiler()->createStackScanner();
  return vm;
}

#endif
