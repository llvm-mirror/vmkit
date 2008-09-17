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
#include "JavaObject.h"
#include "JavaThread.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"
#include "JnjvmClassLoader.h"
#include "LockedMap.h"
#ifdef SERVICE_VM
#include "ServiceDomain.h"
#endif
#ifdef MULTIPLE_VM
#include "SharedMaps.h"
#include "IsolateSharedLoader.h"
#endif

using namespace jnjvm;

#define INIT(X) VirtualTable* X::VT = 0

  INIT(JavaArray);
  INIT(ArrayObject);
  INIT(Class);
  INIT(ClassArray);
  INIT(JavaObject);
  INIT(JavaThread);
  INIT(Jnjvm);
  INIT(ClassMap);
  INIT(JnjvmBootstrapLoader);
  INIT(JnjvmClassLoader);
#ifdef MULTIPLE_VM
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
  bootstrapThread->MARK_AND_TRACE;
  bootstrapLoader->MARK_AND_TRACE;
#ifdef MULTIPLE_VM
  JnjvmSharedLoader::sharedLoader->MARK_AND_TRACE;
#endif
}

void ClassMap::TRACER {
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
#define TRACE_DELEGATEE(prim) \
  prim->delegatee->MARK_AND_TRACE

  TRACE_DELEGATEE(upcalls->OfVoid);
  TRACE_DELEGATEE(upcalls->OfBool);
  TRACE_DELEGATEE(upcalls->OfByte);
  TRACE_DELEGATEE(upcalls->OfChar);
  TRACE_DELEGATEE(upcalls->OfShort);
  TRACE_DELEGATEE(upcalls->OfInt);
  TRACE_DELEGATEE(upcalls->OfFloat);
  TRACE_DELEGATEE(upcalls->OfLong);
  TRACE_DELEGATEE(upcalls->OfDouble);
#undef TRACE_DELEGATEE
}

#ifdef MULTIPLE_VM
void UserClass::TRACER {
  classLoader->MARK_AND_TRACE;
  delegatee->MARK_AND_TRACE;
  staticInstance->MARK_AND_TRACE;
  ctpInfo->MARK_AND_TRACE;
}

void UserClassPrimitive::TRACER {
  classLoader->MARK_AND_TRACE;
  delegatee->MARK_AND_TRACE;
}

void UserClassArray::TRACER {
  classLoader->MARK_AND_TRACE;
  delegatee->MARK_AND_TRACE;
}

void SharedClassByteMap::TRACER {
  for (iterator i = map.begin(), e = map.end(); i!= e; ++i) {
    i->first->MARK_AND_TRACE;
  }
}

void JnjvmSharedLoader::TRACER {
  byteClasses->MARK_AND_TRACE;
}
#endif

#ifdef SERVICE_VM
void ServiceDomain::TRACER {
  JavaIsolate::PARENT_TRACER;
  classes->MARK_AND_TRACE;
}
#endif
