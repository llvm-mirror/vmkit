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
  INIT(Attribut);
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
  INIT(FunctionMap);
  INIT(FunctionDefMap);
  INIT(JavaIsolate);
  INIT(JavaString);
  INIT(CacheNode);
  INIT(Enveloppe);
  INIT(DelegateeMap);
#ifdef SERVICE_VM
  INIT(ServiceDomain);
#endif

#undef INIT

void JavaArray::tracer(size_t sz) {
  JavaObject::tracer(sz);
}

void ArrayObject::tracer(size_t sz) {
  JavaObject::tracer(sz);
  for (sint32 i = 0; i < size; i++) {
    elements[i]->markAndTrace();
  }
}

#define ARRAYTRACER(name)         \
  void name::tracer(size_t sz) {  \
    JavaObject::tracer(sz);       \
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


void Attribut::tracer(size_t sz) {
  name->markAndTrace();
}

#define TRACE_VECTOR(type,alloc,name) {                             \
  for (std::vector<type, alloc<type> >::iterator i = name.begin(),  \
       e = name.end(); i!= e; ++i) {                                \
    (*i)->markAndTrace(); }}

void CommonClass::tracer(size_t sz) {
  name->markAndTrace();
  super->markAndTrace();
  superUTF8->markAndTrace();
  TRACE_VECTOR(const UTF8*, std::allocator, interfacesUTF8);
  TRACE_VECTOR(Class*, std::allocator, interfaces);
  //lockVar->markAndTrace();
  //condVar->markAndTrace();
  TRACE_VECTOR(JavaMethod*, std::allocator, virtualMethods);
  TRACE_VECTOR(JavaMethod*, std::allocator, staticMethods);
  TRACE_VECTOR(JavaField*, std::allocator, virtualFields);
  TRACE_VECTOR(JavaField*, std::allocator, staticFields);
  classLoader->markAndTrace();
#ifndef MULTIPLE_VM
  delegatee->markAndTrace();
#endif
  TRACE_VECTOR(CommonClass*, std::allocator, display);
  isolate->markAndTrace();
}

void Class::tracer(size_t sz) {
  CommonClass::tracer(sz);
  bytes->markAndTrace();
  _staticInstance->markAndTrace();
  virtualInstance->markAndTrace();
  ctpInfo->markAndTrace();
  TRACE_VECTOR(Attribut*, gc_allocator, attributs);
  TRACE_VECTOR(Class*, std::allocator, innerClasses);
  outerClass->markAndTrace();
  codeStaticTracer->markAndTrace();
  codeVirtualTracer->markAndTrace();
}

void ClassArray::tracer(size_t sz) {
  CommonClass::tracer(sz);
  _baseClass->markAndTrace();
  _funcs->markAndTrace();

}

void JavaMethod::tracer(size_t sz) {
  signature->markAndTrace();
  TRACE_VECTOR(Attribut*, gc_allocator, attributs);
  TRACE_VECTOR(Enveloppe*, gc_allocator, caches);
  classDef->markAndTrace();
  name->markAndTrace();
  type->markAndTrace();
  code->markAndTrace();
}

void JavaField::tracer(size_t sz) {
  name->markAndTrace();
  signature->markAndTrace();
  type->markAndTrace();
  TRACE_VECTOR(Attribut*, gc_allocator, attributs);
  classDef->markAndTrace();
}

void JavaCtpInfo::tracer(size_t sz) {
  classDef->markAndTrace();
  // Everything is hashed in the constant pool,
  // do not trace them here
}

void JavaCond::tracer(size_t sz) {
  TRACE_VECTOR(JavaThread*, std::allocator, threads);
}

void LockObj::tracer(size_t sz) {
  //lock->markAndTrace();
  varcond->markAndTrace();
}

void JavaObject::tracer(size_t sz) {
  classOf->markAndTrace();
  lockObj->markAndTrace();
}

void JavaThread::tracer(size_t sz) {
  javaThread->markAndTrace();
  isolate->markAndTrace();
  //lock->markAndTrace();
  //varcond->markAndTrace();
  pendingException->markAndTrace();
}

void AssessorDesc::tracer(size_t sz) {
  classType->markAndTrace();
}

void Typedef::tracer(size_t sz) {
  keyName->markAndTrace();
  pseudoAssocClassName->markAndTrace();
  funcs->markAndTrace();
  isolate->markAndTrace();
}

void Signdef::tracer(size_t sz) {
  Typedef::tracer(sz);
  TRACE_VECTOR(Typedef*, std::allocator, args);
  ret->markAndTrace();
  _staticCallBuf->markAndTrace();
  _virtualCallBuf->markAndTrace();
  _staticCallAP->markAndTrace();
  _virtualCallAP->markAndTrace();
}

void ThreadSystem::tracer(size_t sz) {
  //nonDaemonLock->markAndTrace();
  //nonDaemonVar->markAndTrace();
}

void Jnjvm::tracer(size_t sz) {
  appClassLoader->markAndTrace();
  hashUTF8->markAndTrace();
  hashStr->markAndTrace();
  bootstrapClasses->markAndTrace();
  loadedMethods->markAndTrace();
  loadedFields->markAndTrace();
  javaTypes->markAndTrace();
  TRACE_VECTOR(JavaObject*, gc_allocator, globalRefs);
  //globalRefsLock->markAndTrace();
  functions->markAndTrace();
#ifdef MULTIPLE_VM
  statics->markAndTrace();
  delegatees->markAndTrace();
#endif
  //protectModule->markAndTrace();
}

void Reader::tracer(size_t sz) {
  bytes->markAndTrace();
}

void ZipFile::tracer(size_t sz) {
}

void ZipArchive::tracer(size_t sz) {
  reader->markAndTrace();
  filetable->markAndTrace();
}

void JavaIsolate::tracer(size_t sz) {
  Jnjvm::tracer(sz);
  threadSystem->markAndTrace();
  bootstrapThread->markAndTrace();
}

void JavaString::tracer(size_t sz) {
}

void CacheNode::tracer(size_t sz) {
  ((mvm::Object*)methPtr)->markAndTrace();
  lastCible->markAndTrace();
  next->markAndTrace();
  enveloppe->markAndTrace();
}

void Enveloppe::tracer(size_t sz) {
  firstCache->markAndTrace();
  ctpInfo->markAndTrace();
  //cacheLock->markAndTrace();
}

#ifdef SERVICE_VM
void ServiceDomain::tracer(size_t sz) {
  Jnjvm::tracer(sz);
}
#endif
