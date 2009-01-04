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
#include "JavaClass.h"
#include "JavaObject.h"
#include "JavaString.h"
#include "JavaThread.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"
#include "JnjvmClassLoader.h"
#include "LockedMap.h"

#ifdef ISOLATE_SHARING
#include "SharedMaps.h"
#include "IsolateSharedLoader.h"
#endif

using namespace jnjvm;

#define INIT(X) VirtualTable* X::VT = 0

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

#undef INIT

void ArrayObject::TRACER {
  if (classOf) classOf->classLoader->MARK_AND_TRACE;
  for (sint32 i = 0; i < size; i++) {
    if (elements[i]) elements[i]->MARK_AND_TRACE;
  }
  LockObj* l = lockObj();
  if (l) l->MARK_AND_TRACE;
}

#ifdef MULTIPLE_GC
extern "C" void ArrayObjectTracer(ArrayObject* obj, Collector* GC) {
#else
extern "C" void ArrayObjectTracer(ArrayObject* obj) {
#endif
  obj->CALL_TRACER;
}

#ifdef MULTIPLE_GC
extern "C" void JavaArrayTracer(JavaArray* obj, Collector* GC) {
#else
extern "C" void JavaArrayTracer(JavaArray* obj) {
#endif
}

void JavaArray::TRACER {}

#define TRACE_VECTOR(type,alloc,name) {                             \
  for (std::vector<type, alloc<type> >::iterator i = name.begin(),  \
       e = name.end(); i!= e; ++i) {                                \
    (*i)->MARK_AND_TRACE; }}

void CommonClass::TRACER {
  if (super) super->classLoader->MARK_AND_TRACE;
  for (uint32 i = 0; i < nbInterfaces; ++i) {
    interfaces[i]->classLoader->MARK_AND_TRACE;
  }
  classLoader->MARK_AND_TRACE;
  for (uint32 i = 0; i < NR_ISOLATES; ++i) {
    delegatee[i]->MARK_AND_TRACE;
  }

}

void Class::TRACER {
  CommonClass::CALL_TRACER;
  bytes->MARK_AND_TRACE;
  
  for (uint32 i =0; i < NR_ISOLATES; ++i) {
    TaskClassMirror &M = IsolateInfo[i];
    if (M.staticInstance) {
      ((Class*)this)->staticTracer(M.staticInstance);
    }
  }
}

void JavaObject::TRACER {
  if (classOf) classOf->classLoader->MARK_AND_TRACE;
  LockObj* l = lockObj();
  if (l) l->MARK_AND_TRACE;
}

#ifdef MULTIPLE_GC
extern "C" void JavaObjectTracer(JavaObject* obj, Collector* GC) {
#else
extern "C" void JavaObjectTracer(JavaObject* obj) {
#endif
  if (obj->classOf) obj->classOf->classLoader->MARK_AND_TRACE;
  LockObj* l = obj->lockObj();
  if (l) l->MARK_AND_TRACE;
}

static void traceClassMap(ClassMap* classes) {
  for (ClassMap::iterator i = classes->map.begin(), e = classes->map.end();
       i!= e; ++i) {
    CommonClass* cl = i->second;
    if (cl->isClass()) cl->asClass()->CALL_TRACER;
    else cl->CALL_TRACER;
  }
}

void JavaThread::TRACER {
  javaThread->MARK_AND_TRACE;
  if (pendingException) pendingException->MARK_AND_TRACE;
#ifdef SERVICE
  ServiceException->MARK_AND_TRACE;
#endif
}

void Jnjvm::TRACER {
  appClassLoader->MARK_AND_TRACE;
  TRACE_VECTOR(JavaObject*, gc_allocator, globalRefs);
  bootstrapLoader->MARK_AND_TRACE;
#if defined(ISOLATE_SHARING)
  JnjvmSharedLoader::sharedLoader->MARK_AND_TRACE;
#endif
  traceClassMap(bootstrapLoader->getClasses());
  
#define TRACE_DELEGATEE(prim) \
  prim->CALL_TRACER;

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

  TRACE_VECTOR(JavaString*, gc_allocator, bootstrapLoader->strings);

  mvm::Thread* th = th->get();
  th->CALL_TRACER;
  for (mvm::Thread* cur = (mvm::Thread*)th->next(); cur != th; 
       cur = (mvm::Thread*)cur->next()) {
    cur->CALL_TRACER;
  }

#ifdef SERVICE
  parent->MARK_AND_TRACE;
#endif
}

void JnjvmClassLoader::TRACER {
  javaLoader->MARK_AND_TRACE;
  traceClassMap(classes);
  isolate->MARK_AND_TRACE;
  TRACE_VECTOR(JavaString*, gc_allocator, strings);
}

void JnjvmBootstrapLoader::TRACER {}

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
