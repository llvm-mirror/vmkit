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
#include "JavaConstantPool.h"
#include "JavaIsolate.h"
#include "JavaObject.h"
#include "JavaString.h"
#include "JavaThread.h"
#include "JavaTypes.h"
#include "Jnjvm.h"
#include "LockedMap.h"
#include "Reader.h"
#ifdef SERVICE_VM
#include "ServiceDomain.h"
#endif
#include "Zip.h"

using namespace jnjvm;

#define INIT(X) VirtualTable* X::VT = 0

  INIT(JavaArray);
  INIT(ArrayUInt8);
  INIT(ArraySInt8);
  INIT(ArrayUInt16);
  INIT(ArraySInt16);
  INIT(ArrayUInt32);
  INIT(ArraySInt32);
  INIT(ArrayLong);
  INIT(ArrayFloat);
  INIT(ArrayDouble);
  INIT(ArrayObject);
  INIT(CommonClass);
  INIT(Class);
  INIT(ClassArray);
  INIT(JavaCond);
  INIT(LockObj);
  INIT(JavaObject);
  INIT(JavaThread);
  INIT(Typedef);
  INIT(Signdef);
  INIT(Jnjvm);
  INIT(Reader);
  INIT(ZipFile);
  INIT(ZipArchive);
  INIT(ClassMap);
  INIT(ZipFileMap);
  INIT(StringMap);
  INIT(TypeMap);
  INIT(StaticInstanceMap);
  INIT(JavaIsolate);
  INIT(JavaString);
  INIT(DelegateeMap);
#ifdef SERVICE_VM
  INIT(ServiceDomain);
#endif

#undef INIT

void JavaArray::TRACER {
  classOf->MARK_AND_TRACE;
  if (lockObj) lockObj->MARK_AND_TRACE;
}

void ArrayObject::TRACER {
  classOf->MARK_AND_TRACE;
  if (lockObj) lockObj->MARK_AND_TRACE;
  for (sint32 i = 0; i < size; i++) {
    if (elements[i]) elements[i]->MARK_AND_TRACE;
  }
}

#define ARRAYTRACER(name)         \
  void name::TRACER {             \
    if (lockObj)                  \
      lockObj->MARK_AND_TRACE;    \
  }
  

ARRAYTRACER(ArrayUInt8);
ARRAYTRACER(ArraySInt8);
ARRAYTRACER(ArrayUInt16);
ARRAYTRACER(ArraySInt16);
ARRAYTRACER(ArrayUInt32);
ARRAYTRACER(ArraySInt32);
ARRAYTRACER(ArrayLong);
ARRAYTRACER(ArrayFloat);
ARRAYTRACER(ArrayDouble);

#undef ARRAYTRACER


#define TRACE_VECTOR(type,alloc,name) {                             \
  for (std::vector<type, alloc<type> >::iterator i = name.begin(),  \
       e = name.end(); i!= e; ++i) {                                \
    (*i)->MARK_AND_TRACE; }}

void CommonClass::TRACER {
  classLoader->MARK_AND_TRACE;
#ifndef MULTIPLE_VM
  delegatee->MARK_AND_TRACE;
#endif
  
  for (method_iterator i = staticMethods.begin(), e = staticMethods.end();
       i!= e; ++i) {
    mvm::Code* c = i->second->code;
    if (c) c->MARK_AND_TRACE;
  }
  
  for (method_iterator i = virtualMethods.begin(), e = virtualMethods.end();
       i!= e; ++i) {
    mvm::Code* c = i->second->code;
    if (c) c->MARK_AND_TRACE;
  }
 
}

void Class::TRACER {
  CommonClass::PARENT_TRACER;
  bytes->MARK_AND_TRACE;
#ifndef MULTIPLE_VM
  _staticInstance->MARK_AND_TRACE;
#endif
  codeStaticTracer->MARK_AND_TRACE;
  codeVirtualTracer->MARK_AND_TRACE;
}

void ClassArray::TRACER {
  CommonClass::PARENT_TRACER;
}

void JavaCond::TRACER {
  // FIXME: do I need this?
  TRACE_VECTOR(JavaThread*, std::allocator, threads);
}

void LockObj::TRACER {
  varcond->MARK_AND_TRACE;
}

void JavaObject::TRACER {
  classOf->MARK_AND_TRACE;
  if (lockObj) lockObj->MARK_AND_TRACE;
}

#ifdef MULTIPLE_GC
extern "C" void JavaObjectTracer(JavaObject* obj, Collector* GC) {
#else
extern "C" void JavaObjectTracer(JavaObject* obj) {
#endif
  obj->classOf->MARK_AND_TRACE;
  if (obj->lockObj) obj->lockObj->MARK_AND_TRACE;
}


void JavaThread::TRACER {
  javaThread->MARK_AND_TRACE;
  // FIXME: do I need this?
  isolate->MARK_AND_TRACE;
  if (pendingException) pendingException->MARK_AND_TRACE;
}

void Typedef::TRACER {
}

void Signdef::TRACER {
  _staticCallBuf->MARK_AND_TRACE;
  _virtualCallBuf->MARK_AND_TRACE;
  _staticCallAP->MARK_AND_TRACE;
  _virtualCallAP->MARK_AND_TRACE;
}

void Jnjvm::TRACER {
  appClassLoader->MARK_AND_TRACE;
  hashStr->MARK_AND_TRACE;
  bootstrapClasses->MARK_AND_TRACE;
  javaTypes->MARK_AND_TRACE;
  TRACE_VECTOR(JavaObject*, gc_allocator, globalRefs);
#ifdef MULTIPLE_VM
  statics->MARK_AND_TRACE;
  delegatees->MARK_AND_TRACE;
#endif
}

void Reader::TRACER {
  bytes->MARK_AND_TRACE;
}

void ZipFile::TRACER {
}

void ZipArchive::TRACER {
  reader->MARK_AND_TRACE;
  filetable->MARK_AND_TRACE;
}

void JavaIsolate::TRACER {
  Jnjvm::PARENT_TRACER;
  bootstrapThread->MARK_AND_TRACE;
}

void JavaString::TRACER {
}

void ClassMap::TRACER {
  for (iterator i = map.begin(), e = map.end(); i!= e; ++i) {
    i->second->MARK_AND_TRACE;
  }
}

void ZipFileMap::TRACER {
  for (iterator i = map.begin(), e = map.end(); i!= e; ++i) {
    i->second->MARK_AND_TRACE;
  }
}

void StringMap::TRACER {
  for (iterator i = map.begin(), e = map.end(); i!= e; ++i) {
    i->second->MARK_AND_TRACE;
  }
}

void TypeMap::TRACER {
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

#ifdef SERVICE_VM
void ServiceDomain::TRACER {
  JavaIsolate::PARENT_TRACER;
  classes->MARK_AND_TRACE;
}
#endif
