//===--------- ServiceDomain.h - Service domain description ---------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef JNJVM_SERVICE_DOMAIN_H
#define JNJVM_SERVICE_DOMAIN_H

#include <sys/time.h>
#include <time.h>

#include <map>

#include "JavaIsolate.h"

namespace mvm {
  class Lock;
}

namespace jnjvm {

class ClassMap;

typedef enum DomainState {
  DomainLoaded, DomainUnloading, DomainUnloaded
}DomainState;

class ServiceDomain : public JavaIsolate {
public:
  static VirtualTable* VT;

  virtual void print(mvm::PrintBuffer* buf) const;
  virtual void TRACER;
  virtual void destroyer(size_t sz);
  static ServiceDomain* allocateService(JavaIsolate* callingVM);
  static void serviceError(ServiceDomain* error, const char* str);
   
  mvm::Lock* lock;
  ClassMap* classes;
  uint64 executionTime;
  uint64 numThreads;
  std::map<ServiceDomain*, uint64> interactions;
  DomainState state;

  static ServiceDomain* getDomainFromLoader(JavaObject* loader);
  static JavaMethod* ServiceErrorInit;
  static Class* ServiceErrorClass;

  static ServiceDomain* bootstrapDomain;

  static bool isLockableDomain(Jnjvm* vm) {
    return (vm == Jnjvm::bootstrapVM || vm == bootstrapDomain);
  }

  static void initialise(ServiceDomain* boot);
  void startExecution();
  
  // OSGi specific fields
  static JavaField* OSGiFramework;
  static JavaMethod* uninstallBundle;

};

} // end namespace jnjvm

#endif //JNJVM_SERVICE_DOMAIN_H
