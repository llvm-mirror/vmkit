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

#include "Jnjvm.h"

namespace jnjvm {

class ClassMap;

class ServiceDomain : public Jnjvm {
public:
  static VirtualTable* VT;

  virtual void print(mvm::PrintBuffer* buf) const;
  virtual void tracer(size_t sz);
  virtual void destroyer(size_t sz);
  void loadBootstrap();
  static ServiceDomain* allocateService(Jnjvm* callingVM);

  ClassMap* classes;
  time_t started;
  uint64 executionTime;
  uint64 memoryUsed;
  uint64 gcTriggered;
  uint64 numThreads;
};

} // end namespace jnjvm

#endif //JNJVM_SERVICE_DOMAIN_H
