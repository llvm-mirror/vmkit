//===------- ServiceDomain.cpp - Service domain description ---------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "mvm/JIT.h"

#include "JavaJIT.h"
#include "JnjvmModuleProvider.h"
#include "ServiceDomain.h"

extern "C" struct JNINativeInterface JNI_JNIEnvTable;
extern "C" const struct JNIInvokeInterface JNI_JavaVMTable;

using namespace jnjvm;

void ServiceDomain::destroyer(size_t sz) {
  mvm::jit::protectEngine->lock();
  mvm::jit::executionEngine->removeModuleProvider(TheModuleProvider);
  mvm::jit::protectEngine->unlock();
  delete TheModuleProvider;
  delete module;
}

void ServiceDomain::print(mvm::PrintBuffer* buf) const {
  buf->write("Service domain: ");
  buf->write(name);
}

ServiceDomain* ServiceDomain::allocateService(Jnjvm* callingVM) {
  ServiceDomain* service = gc_new(ServiceDomain)();
  
  service->functions = FunctionMap::allocate();
  service->module = new llvm::Module("Service Domain");
  service->protectModule = mvm::Lock::allocNormal();
  service->TheModuleProvider = new JnjvmModuleProvider(service->module, 
                                                       service->functions);  
  JavaJIT::initialiseJITIsolateVM(service);
  
  service->name = "service";
  service->jniEnv = &JNI_JNIEnvTable;
  service->javavmEnv = &JNI_JavaVMTable;
  
  // We copy so that bootstrap utf8 such as "<init>" are unique  
  service->hashUTF8 = callingVM->hashUTF8->copy();
  service->hashStr = StringMap::allocate();
  service->bootstrapClasses = callingVM->bootstrapClasses;
  service->loadedMethods = MethodMap::allocate();
  service->loadedFields = FieldMap::allocate();
  service->javaTypes = jnjvm::TypeMap::allocate(); 
  service->globalRefsLock = mvm::Lock::allocNormal();
  service->statics = StaticInstanceMap::allocate();  
  service->delegatees = DelegateeMap::allocate();  
  
  // A service is related to a class loader
  // Here are the classes it loaded
  service->classes = ClassMap::allocate();

  service->started = 0;
  service->executionTime = 0;
  service->memoryUsed = 0;
  service->gcTriggered = 0;
  service->numThreads = 0;
  
  service->loadBootstrap();
  return service;
}

void ServiceDomain::loadBootstrap() {
  // load and initialise math since it is responsible for dlopen'ing 
  // libjavalang.so and we are optimizing some math operations
  loadName(asciizConstructUTF8("java/lang/Math"), 
           CommonClass::jnjvmClassLoader, true, true, true);
}
