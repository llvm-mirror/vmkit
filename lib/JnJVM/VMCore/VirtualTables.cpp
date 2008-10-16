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
#ifdef ISOLATE_SHARING
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
  INIT(JnjvmBootstrapLoader);
  INIT(JnjvmClassLoader);
  INIT(LockObj);
#if defined(ISOLATE_SHARING)
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
  LockObj* l = lockObj();
  if (l) l->MARK_AND_TRACE;
}

void JavaArray::TRACER {}

#define TRACE_VECTOR(type,alloc,name) {                             \
  for (std::vector<type, alloc<type> >::iterator i = name.begin(),  \
       e = name.end(); i!= e; ++i) {                                \
    (*i)->MARK_AND_TRACE; }}

void CommonClass::TRACER {
  super->MARK_AND_TRACE;
  TRACE_VECTOR(Class*, gc_allocator, interfaces);
  classLoader->MARK_AND_TRACE;
#if !defined(ISOLATE)
  delegatee->MARK_AND_TRACE;
#endif
}

void Class::TRACER {
  CommonClass::CALL_TRACER;
  TRACE_VECTOR(Class*, gc_allocator, innerClasses);
  outerClass->MARK_AND_TRACE;
  bytes->MARK_AND_TRACE;
#if !defined(ISOLATE)
  _staticInstance->MARK_AND_TRACE;
#endif
}

void ClassArray::TRACER {
  CommonClass::CALL_TRACER;
}

void JavaObject::TRACER {
  classOf->MARK_AND_TRACE;
  LockObj* l = lockObj();
  if (l) l->MARK_AND_TRACE;
}

#ifdef MULTIPLE_GC
extern "C" void JavaObjectTracer(JavaObject* obj, Collector* GC) {
#else
extern "C" void JavaObjectTracer(JavaObject* obj) {
#endif
  obj->classOf->MARK_AND_TRACE;
  LockObj* l = obj->lockObj();
  if (l) l->MARK_AND_TRACE;
}


void JavaThread::TRACER {
  javaThread->MARK_AND_TRACE;
  if (pendingException) pendingException->MARK_AND_TRACE;
}

void Jnjvm::TRACER {
  appClassLoader->MARK_AND_TRACE;
  TRACE_VECTOR(JavaObject*, gc_allocator, globalRefs);
  bootstrapThread->MARK_AND_TRACE;
  bootstrapLoader->MARK_AND_TRACE;
#if defined(ISOLATE_SHARING)
  JnjvmSharedLoader::sharedLoader->MARK_AND_TRACE;
#endif
}

static void traceClassMap(ClassMap* classes) {
  for (ClassMap::iterator i = classes->map.begin(), e = classes->map.end();
       i!= e; ++i) {
    i->second->MARK_AND_TRACE;
  }
}

void JnjvmClassLoader::TRACER {
  javaLoader->MARK_AND_TRACE;
  traceClassMap(classes);
  isolate->MARK_AND_TRACE;
  TRACE_VECTOR(JavaString*, gc_allocator, strings);
}

void JnjvmBootstrapLoader::TRACER {
  traceClassMap(classes);
  
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
  TRACE_VECTOR(JavaString*, gc_allocator, strings);
}

#if defined(ISOLATE_SHARING)
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
  JavaIsolate::CALL_TRACER;
  classes->MARK_AND_TRACE;
}
#endif
