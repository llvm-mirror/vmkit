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
  INIT(UTF8);
  INIT(CommonClass);
  INIT(Class);
  INIT(ClassArray);
  INIT(JavaMethod);
  INIT(JavaField);
  INIT(JavaCtpInfo);
  INIT(JavaCond);
  INIT(LockObj);
  INIT(JavaObject);
  INIT(JavaThread);
  INIT(AssessorDesc);
  INIT(Typedef);
  INIT(Signdef);
  INIT(ThreadSystem);
  INIT(Jnjvm);
  INIT(Reader);
  INIT(ZipFile);
  INIT(ZipArchive);
  INIT(UTF8Map);
  INIT(ClassMap);
  INIT(FieldMap);
  INIT(MethodMap);
  INIT(ZipFileMap);
  INIT(StringMap);
  INIT(TypeMap);
  INIT(StaticInstanceMap);
  INIT(JavaIsolate);
  INIT(JavaString);
  INIT(CacheNode);
  INIT(Enveloppe);
  INIT(DelegateeMap);
#ifdef SERVICE_VM
  INIT(ServiceDomain);
#endif

#undef INIT

void JavaArray::TRACER {
  classOf->MARK_AND_TRACE;
  lockObj->MARK_AND_TRACE;
}

void ArrayObject::TRACER {
  classOf->MARK_AND_TRACE;
  lockObj->MARK_AND_TRACE;
  for (sint32 i = 0; i < size; i++) {
    elements[i]->MARK_AND_TRACE;
  }
}

#define ARRAYTRACER(name)         \
  void name::TRACER {             \
    classOf->MARK_AND_TRACE;      \
    lockObj->MARK_AND_TRACE;      \
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
}

void Class::TRACER {
  CommonClass::PARENT_TRACER;
  bytes->MARK_AND_TRACE;
#ifndef MULTIPLE_VM
  _staticInstance->MARK_AND_TRACE;
#endif
  ctpInfo->MARK_AND_TRACE;
  codeStaticTracer->MARK_AND_TRACE;
  codeVirtualTracer->MARK_AND_TRACE;
}

void ClassArray::TRACER {
  CommonClass::PARENT_TRACER;
}

void JavaMethod::TRACER {
  TRACE_VECTOR(Enveloppe*, gc_allocator, caches);
  code->MARK_AND_TRACE;
}

void JavaField::TRACER {
}

void JavaCtpInfo::TRACER {
  // Everything is hashed in the constant pool,
  // do not trace them here
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
  lockObj->MARK_AND_TRACE;
}

#ifdef MULTIPLE_GC
extern "C" void JavaObjectTracer(JavaObject* obj, Collector* GC) {
#else
extern "C" void JavaObjectTracer(JavaObject* obj) {
#endif
  obj->classOf->MARK_AND_TRACE;
  obj->lockObj->MARK_AND_TRACE;
}


void JavaThread::TRACER {
  javaThread->MARK_AND_TRACE;
  // FIXME: do I need this?
  isolate->MARK_AND_TRACE;
  pendingException->MARK_AND_TRACE;
}

void AssessorDesc::TRACER {
}

void Typedef::TRACER {
}

void Signdef::TRACER {
  _staticCallBuf->MARK_AND_TRACE;
  _virtualCallBuf->MARK_AND_TRACE;
  _staticCallAP->MARK_AND_TRACE;
  _virtualCallAP->MARK_AND_TRACE;
}

void ThreadSystem::TRACER {
}

void Jnjvm::TRACER {
  appClassLoader->MARK_AND_TRACE;
  hashUTF8->MARK_AND_TRACE;
  hashStr->MARK_AND_TRACE;
  bootstrapClasses->MARK_AND_TRACE;
  loadedMethods->MARK_AND_TRACE;
  loadedFields->MARK_AND_TRACE;
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
  threadSystem->MARK_AND_TRACE;
  bootstrapThread->MARK_AND_TRACE;
}

void JavaString::TRACER {
}

void CacheNode::TRACER {
  ((mvm::Object*)methPtr)->MARK_AND_TRACE;
  lastCible->MARK_AND_TRACE;
  next->MARK_AND_TRACE;
  enveloppe->MARK_AND_TRACE;
}

void Enveloppe::TRACER {
  firstCache->MARK_AND_TRACE;
  //cacheLock->MARK_AND_TRACE;
}

void UTF8Map::TRACER {
  for (iterator i = map.begin(), e = map.end(); i!= e; ++i) {
    i->second->MARK_AND_TRACE;
  }
}

void ClassMap::TRACER {
  for (iterator i = map.begin(), e = map.end(); i!= e; ++i) {
    i->second->MARK_AND_TRACE;
  }
}

void FieldMap::TRACER {
  for (iterator i = map.begin(), e = map.end(); i!= e; ++i) {
    i->second->MARK_AND_TRACE;
  }
}

  
void MethodMap::TRACER {
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
