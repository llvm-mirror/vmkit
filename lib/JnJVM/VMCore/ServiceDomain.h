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

namespace llvm {
  class GlobalVariable;
  class Function;
}

namespace mvm {
  class Lock;
}

namespace jnjvm {

class ClassMap;

class ServiceDomain : public JavaIsolate {
private:
  llvm::GlobalVariable* _llvmDelegatee;
public:
  static VirtualTable* VT;

  virtual void print(mvm::PrintBuffer* buf) const;
  virtual void TRACER;
  virtual void destroyer(size_t sz);
  void loadBootstrap();
  static ServiceDomain* allocateService(JavaIsolate* callingVM);
  llvm::GlobalVariable* llvmDelegatee();
  void serviceError(const char* str);
  
  mvm::Lock* lock;
  ClassMap* classes;
  struct timeval started;
  uint64 executionTime;
  uint64 memoryUsed;
  uint64 gcTriggered;
  uint64 numThreads;
  std::map<ServiceDomain*, uint64> interactions;

  static ServiceDomain* getDomainFromLoader(JavaObject* loader);
  static llvm::Function* serviceCallStartLLVM;
  static llvm::Function* serviceCallStopLLVM;
};

} // end namespace jnjvm

#endif //JNJVM_SERVICE_DOMAIN_H
