//===--- VirtualTables.cpp - Virtual methods for JnJVM objects ------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "mvm/Object.h"

#include "JavaArray.h"
#include "JavaCache.h"
#include "JavaClass.h"
#include "JavaIsolate.h"
#include "JavaObject.h"
#include "JavaThread.h"
#include "Jnjvm.h"
#include "JnjvmClassLoader.h"
#include "LockedMap.h"
#ifdef SERVICE_VM
#include "ServiceDomain.h"
#endif

using namespace jnjvm;

#define INIT(X) VirtualTable* X::VT = 0

  INIT(JavaArray);
  INIT(ArrayObject);
  INIT(CommonClass);
  INIT(Class);
  INIT(ClassArray);
  INIT(JavaObject);
  INIT(JavaThread);
  INIT(Jnjvm);
  INIT(ClassMap);
  INIT(StaticInstanceMap);
  INIT(JavaIsolate);
  INIT(DelegateeMap);
  INIT(JnjvmBootstrapLoader);
  INIT(JnjvmClassLoader);
#ifdef SERVICE_VM
  INIT(ServiceDomain);
#endif

#undef INIT

void ArrayObject::TRACER {
  classOf->MARK_AND_TRACE;
  for (sint32 i = 0; i < size; i++) {
    if (elements[i]) elements[i]->MARK_AND_TRACE;
  }
}

void JavaArray::TRACER {}

#define TRACE_VECTOR(type,alloc,name) {                             \
  for (std::vector<type, alloc<type> >::iterator i = name.begin(),  \
       e = name.end(); i!= e; ++i) {                                \
    (*i)->MARK_AND_TRACE; }}

void CommonClass::TRACER {
  classLoader->MARK_AND_TRACE;
#ifndef MULTIPLE_VM
  delegatee->MARK_AND_TRACE;
#endif
}

void Class::TRACER {
  CommonClass::PARENT_TRACER;
  bytes->MARK_AND_TRACE;
#ifndef MULTIPLE_VM
  _staticInstance->MARK_AND_TRACE;
#endif
}

void ClassArray::TRACER {
  CommonClass::PARENT_TRACER;
}

void JavaObject::TRACER {
  classOf->MARK_AND_TRACE;
}

#ifdef MULTIPLE_GC
extern "C" void JavaObjectTracer(JavaObject* obj, Collector* GC) {
#else
extern "C" void JavaObjectTracer(JavaObject* obj) {
#endif
  obj->classOf->MARK_AND_TRACE;
}


void JavaThread::TRACER {
  javaThread->MARK_AND_TRACE;
  // FIXME: do I need this?
  isolate->MARK_AND_TRACE;
  if (pendingException) pendingException->MARK_AND_TRACE;
}

void Jnjvm::TRACER {
  appClassLoader->MARK_AND_TRACE;
  TRACE_VECTOR(JavaObject*, gc_allocator, globalRefs);
#ifdef MULTIPLE_VM
  statics->MARK_AND_TRACE;
  delegatees->MARK_AND_TRACE;
#endif
  
}

void JavaIsolate::TRACER {
  Jnjvm::PARENT_TRACER;
  bootstrapThread->MARK_AND_TRACE;
  JnjvmClassLoader::bootstrapLoader->MARK_AND_TRACE;
}

void ClassMap::TRACER {
  for (iterator i = map.begin(), e = map.end(); i!= e; ++i) {
    i->second->MARK_AND_TRACE;
  }
}

void StaticInstanceMap::TRACER {
  for (iterator i = map.begin(), e = map.end(); i!= e; ++i) {
    i->second->second->MARK_AND_TRACE;
  }
}

void DelegateeMap::TRACER {
  for (iterator i = map.begin(), e = map.end(); i!= e; ++i) {
    i->second->MARK_AND_TRACE;
  }
}

void JnjvmClassLoader::TRACER {
  javaLoader->MARK_AND_TRACE;
  classes->MARK_AND_TRACE;
  isolate->MARK_AND_TRACE;
}

void JnjvmBootstrapLoader::TRACER {
  classes->MARK_AND_TRACE;
  
  for (std::vector<ZipArchive*>::iterator i = bootArchives.begin(),
       e = bootArchives.end(); i != e; ++i) {
    (*i)->bytes->MARK_AND_TRACE;
  }
}

#ifdef SERVICE_VM
void ServiceDomain::TRACER {
  JavaIsolate::PARENT_TRACER;
  classes->MARK_AND_TRACE;
}
#endif
