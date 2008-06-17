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
#include "JavaThread.h"
#include "JavaUpcalls.h"
#include "JnjvmModuleProvider.h"
#include "ServiceDomain.h"

extern "C" struct JNINativeInterface JNI_JNIEnvTable;
extern "C" const struct JNIInvokeInterface JNI_JavaVMTable;


using namespace jnjvm;

JavaMethod* ServiceDomain::ServiceErrorInit;
Class* ServiceDomain::ServiceErrorClass;
ServiceDomain* ServiceDomain::bootstrapDomain;

// OSGi specific fields
JavaField* ServiceDomain::OSGiFramework;
JavaMethod* ServiceDomain::uninstallBundle;


ServiceDomain::~ServiceDomain() {
  delete lock;
}

void ServiceDomain::print(mvm::PrintBuffer* buf) const {
  buf->write("Service domain: ");
  buf->write(name);
}

ServiceDomain* ServiceDomain::allocateService(JavaIsolate* callingVM) {
  ServiceDomain* service = vm_new(callingVM, ServiceDomain)();
  service->threadSystem = callingVM->threadSystem;
#ifdef MULTIPLE_GC
  service->GC = Collector::allocate();
#endif
  

  service->classpath = callingVM->classpath;
  service->bootClasspathEnv = callingVM->bootClasspathEnv;
  service->libClasspathEnv = callingVM->libClasspathEnv;
  service->bootClasspath = callingVM->bootClasspath;
  service->functions = vm_new(service, FunctionMap)();
  service->functionDefs = vm_new(service, FunctionDefMap)();
  service->module = new JnjvmModule("Service Domain");
  std::string str = 
    mvm::jit::executionEngine->getTargetData()->getStringRepresentation();
  service->module->setDataLayout(str);
  service->protectModule = mvm::Lock::allocNormal();
  service->TheModuleProvider = new JnjvmModuleProvider(service->module,
                                                       service->functions,
                                                       service->functionDefs); 
  

#ifdef MULTIPLE_GC
  mvm::jit::memoryManager->addGCForModule(service->module, service->GC);
#endif
  JavaJIT::initialiseJITIsolateVM(service);
  
  service->name = "service";
  service->jniEnv = &JNI_JNIEnvTable;
  service->javavmEnv = &JNI_JavaVMTable;
  service->appClassLoader = callingVM->appClassLoader;

  // We copy so that bootstrap utf8 such as "<init>" are unique  
  service->hashUTF8 = vm_new(service, UTF8Map)();
  callingVM->hashUTF8->copy(service->hashUTF8);
  service->hashStr = vm_new(service, StringMap)();
  service->bootstrapClasses = callingVM->bootstrapClasses;
  service->loadedMethods = vm_new(service, MethodMap)();
  service->loadedFields = vm_new(service, FieldMap)();
  service->javaTypes = vm_new(service, TypeMap)(); 
  service->globalRefsLock = mvm::Lock::allocNormal();
#ifdef MULTIPLE_VM
  service->statics = vm_new(service, StaticInstanceMap)();  
  service->delegatees = vm_new(service, DelegateeMap)();  
#endif
  
  // A service is related to a class loader
  // Here are the classes it loaded
  service->classes = vm_new(service, ClassMap)();

  service->executionTime = 0;
  service->numThreads = 0;
  
  service->lock = mvm::Lock::allocNormal();
  service->state = DomainLoaded;
  return service;
}

void ServiceDomain::serviceError(ServiceDomain* errorDomain,
                                 const char* str) {
  if (ServiceErrorClass) {
    JavaObject* obj = (*ServiceErrorClass)(bootstrapDomain);
    ServiceErrorInit->invokeIntVirtual(bootstrapDomain, 
                                       bootstrapDomain->asciizToStr(str),
                                       errorDomain);
    JavaThread::throwException(obj);
  } else {
    fprintf(stderr, str);
    abort();
  }
}

ServiceDomain* ServiceDomain::getDomainFromLoader(JavaObject* loader) {
  ServiceDomain* vm = 
    (ServiceDomain*)(*Classpath::vmdataClassLoader)(loader).PointerVal;
  return vm;
}

#include "gccollector.h"
#include "gcthread.h"

extern "C" void serviceCallStart(ServiceDomain* caller,
                                 ServiceDomain* callee) {
  assert(caller && "No caller in service start?");
  assert(callee && "No callee in service start?");
  assert(caller->getVirtualTable() == ServiceDomain::VT && 
         "Caller not a service domain?");
  assert(callee->getVirtualTable() == ServiceDomain::VT && 
         "Callee not a service domain?");
  if (callee->state != DomainLoaded) {
    ServiceDomain::serviceError(callee, "calling a stopped bundle");
  }
  JavaThread* th = JavaThread::get();
  th->isolate = callee;
  caller->lock->lock();
  caller->interactions[callee]++;
  caller->lock->unlock();
  mvm::GCThreadCollector* cur = mvm::GCCollector::threads->myloc();
  cur->meta = callee;
}

extern "C" void serviceCallStop(ServiceDomain* caller,
                                ServiceDomain* callee) {
  assert(caller && "No caller in service stop?");
  assert(callee && "No callee in service stop?");
  assert(caller->getVirtualTable() == ServiceDomain::VT && 
         "Caller not a service domain?");
  assert(callee->getVirtualTable() == ServiceDomain::VT && 
         "Callee not a service domain?");
  if (caller->state != DomainLoaded) {
    ServiceDomain::serviceError(caller, "Returning to a stopped bundle");
  }
  JavaThread* th = JavaThread::get();
  th->isolate = caller;
  mvm::GCThreadCollector* cur = mvm::GCCollector::threads->myloc();
  cur->meta = caller;
}


static int updateCPUUsage(void* unused) {
  mvm::GCThreadCollector *cur;
  while (true) {
    sleep(1);
    for(cur=(mvm::GCThreadCollector *)mvm::GCCollector::threads->base.next();
        cur!=&(mvm::GCCollector::threads->base); 
        cur=(mvm::GCThreadCollector *)cur->next()) {
      ServiceDomain* executingDomain = (ServiceDomain*)cur->meta;
      if (executingDomain)
        ++executingDomain->executionTime;
    }
  }
}

void ServiceDomain::initialise(ServiceDomain* boot) {
  ServiceDomain::bootstrapDomain = boot;
  Jnjvm::bootstrapVM->appClassLoader = boot->appClassLoader;
  (*Classpath::vmdataClassLoader)(boot->appClassLoader, (JavaObject*)boot);
  int tid = 0;
  mvm::Thread::start(&tid, (int (*)(void *))updateCPUUsage, 0);
  ServiceErrorClass = UPCALL_CLASS(Jnjvm::bootstrapVM, "JnJVMBundleException");
  ServiceErrorInit = UPCALL_METHOD(Jnjvm::bootstrapVM, "JnjvmBundleException", 
                                   "<init>", 
                                   "(Ljava/lang/String;Ljava/lang/Object;)V",
                                   ACC_VIRTUAL);
  Class* MainClass = bootstrapDomain->constructClass(bootstrapDomain->asciizConstructUTF8("Main"), 
                                                     Jnjvm::bootstrapVM->appClassLoader);
  OSGiFramework = bootstrapDomain->constructField(MainClass, bootstrapDomain->asciizConstructUTF8("m_felix"),
                                            bootstrapDomain->asciizConstructUTF8("Lorg/apache/felix/framework/Felix;"),
                                            ACC_STATIC);
  uninstallBundle = bootstrapDomain->constructMethod(OSGiFramework->classDef, bootstrapDomain->asciizConstructUTF8("uninstallBundle"),
                                                     bootstrapDomain->asciizConstructUTF8("(Lorg/apache/felix/framework/FelixBundle;)V"),
                                                     ACC_VIRTUAL);
}

void ServiceDomain::startExecution() {
  JavaThread::get()->isolate = this;
  mvm::GCThreadCollector* cur = mvm::GCCollector::threads->myloc();
  cur->meta = this;
}
