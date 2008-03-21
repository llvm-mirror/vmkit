//===----- VirtualTables.cpp - Virtual methods for N3 objects -------------===//
//
//                                N3
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "mvm/Object.h"

#include "Assembly.h"
#include "CLIString.h"
#include "CLIJit.h"
#include "LockedMap.h"
#include "N3.h"
#include "Reader.h"
#include "VirtualMachine.h"
#include "VMArray.h"
#include "VMCache.h"
#include "VMClass.h"
#include "VMObject.h"
#include "VMThread.h"

using namespace n3;

#define INIT(X) VirtualTable* X::VT = 0

  INIT(VMArray);
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
  INIT(VMCommonClass);
  INIT(VMClass);
  INIT(VMClassArray);
  INIT(VMClassPointer);
  INIT(VMMethod);
  INIT(VMField);
  INIT(VMCond);
  INIT(LockObj);
  INIT(VMObject);
  INIT(VMThread);
  INIT(VirtualMachine);
  INIT(UTF8Map);
  INIT(ClassNameMap);
  INIT(ClassTokenMap);
  INIT(FieldTokenMap);
  INIT(MethodTokenMap);
  INIT(StringMap);
  INIT(FunctionMap);
  INIT(N3);
  INIT(Assembly);
  INIT(Section);
  INIT(Stream);
  INIT(Table);
  INIT(Header);
  INIT(AssemblyMap);
  INIT(ThreadSystem);
  INIT(CLIString);
  INIT(Property);
  INIT(Param);
  INIT(CacheNode);
  INIT(Enveloppe);
  INIT(Reader);
  INIT(Opinfo);
  INIT(CLIJit);
  INIT(Exception);
  
#undef INIT

void Opinfo::tracer(size_t sz) {
}

void CLIJit::tracer(size_t sz) {
  compilingMethod->markAndTrace();
  compilingClass->markAndTrace();
}

void ThreadSystem::tracer(size_t sz) {
  //nonDaemonLock->markAndTrace();
  //nonDaemonVar->markAndTrace();
}

void Reader::tracer(size_t sz) {
  bytes->markAndTrace();
}

void CacheNode::tracer(size_t sz) {
  ((mvm::Object*)methPtr)->markAndTrace();
  lastCible->markAndTrace();
  next->markAndTrace();
  enveloppe->markAndTrace();
}

void Enveloppe::tracer(size_t sz) {
  firstCache->markAndTrace();
  //cacheLock->markAndTrace();
  originalMethod->markAndTrace();
}

void VMArray::tracer(size_t sz) {
  VMObject::tracer(sz);
}

void ArrayObject::tracer(size_t sz) {
  VMObject::tracer(sz);
  for (sint32 i = 0; i < size; i++) {
    elements[i]->markAndTrace();
  }
}

#define ARRAYTRACER(name)         \
  void name::tracer(size_t sz) {  \
    VMObject::tracer(sz);         \
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


#define TRACE_VECTOR(type,name) { \
  for (std::vector<type>::iterator i = name.begin(), e = name.end(); \
       i!= e; ++i) {                                                    \
    (*i)->markAndTrace(); }}

void VMCommonClass::tracer(size_t sz) {
  name->markAndTrace();
  nameSpace->markAndTrace();
  super->markAndTrace();
  TRACE_VECTOR(VMClass*, interfaces);
  //lockVar->markAndTrace();
  //condVar->markAndTrace();
  TRACE_VECTOR(VMMethod*, virtualMethods);
  TRACE_VECTOR(VMMethod*, staticMethods);
  TRACE_VECTOR(VMField*, virtualFields);
  TRACE_VECTOR(VMField*, staticFields);
  delegatee->markAndTrace();
  TRACE_VECTOR(VMCommonClass*, display);
  vm->markAndTrace();

  assembly->markAndTrace();
  //funcs->markAndTrace();
  TRACE_VECTOR(Property*, properties);
  codeVirtualTracer->markAndTrace();
  codeStaticTracer->markAndTrace();
}

void VMClass::tracer(size_t sz) {
  VMCommonClass::tracer(sz);
  staticInstance->markAndTrace();
  virtualInstance->markAndTrace();
  TRACE_VECTOR(VMClass*, innerClasses);
  outerClass->markAndTrace();
}

void VMClassArray::tracer(size_t sz) {
  VMCommonClass::tracer(sz);
  baseClass->markAndTrace();
}

void VMClassPointer::tracer(size_t sz) {
  VMCommonClass::tracer(sz);
  baseClass->markAndTrace();
}


void VMMethod::tracer(size_t sz) {
  delegatee->markAndTrace();
  //signature->markAndTrace();
  classDef->markAndTrace();
  TRACE_VECTOR(Param*, params);
  TRACE_VECTOR(Enveloppe*, caches);
  name->markAndTrace();
  code->markAndTrace();
}

void VMField::tracer(size_t sz) {
  signature->markAndTrace();
  classDef->markAndTrace();
  name->markAndTrace();
}

void VMCond::tracer(size_t sz) {
  TRACE_VECTOR(VMThread*, threads);
}

void LockObj::tracer(size_t sz) {
  //lock->markAndTrace();
  varcond->markAndTrace();
}

void VMObject::tracer(size_t sz) {
  classOf->markAndTrace();
  lockObj->markAndTrace();
}

void VMThread::tracer(size_t sz) {
  vmThread->markAndTrace();
  vm->markAndTrace();
  //lock->markAndTrace();
  //varcond->markAndTrace();
  pendingException->markAndTrace();
}

void VirtualMachine::tracer(size_t sz) {
  threadSystem->markAndTrace();
  hashUTF8->markAndTrace();
  functions->markAndTrace();
  //protectModule->markAndTrace();
  bootstrapThread->markAndTrace();
}

void Param::tracer(size_t sz) {
  method->markAndTrace();
  name->markAndTrace();
}

void Property::tracer(size_t sz) {
  type->markAndTrace();
  //signature->markAndTrace();
  name->markAndTrace();
  delegatee->markAndTrace();
}

void Assembly::tracer(size_t sz) {
  loadedNameClasses->markAndTrace();
  loadedTokenClasses->markAndTrace();
  loadedTokenMethods->markAndTrace();
  loadedTokenFields->markAndTrace();
  //lockVar->markAndTrace();
  //condVar->markAndTrace();
  name->markAndTrace();
  bytes->markAndTrace();
  textSection->markAndTrace();
  rsrcSection->markAndTrace();
  relocSection->markAndTrace();
  CLIHeader->markAndTrace();
  vm->markAndTrace();
  delegatee->markAndTrace();
  // TODO trace assembly refs...
}

void N3::tracer(size_t sz) {
  VirtualMachine::tracer(sz);
  hashUTF8->markAndTrace();
  hashStr->markAndTrace();
  loadedAssemblies->markAndTrace();
}

void Section::tracer(size_t sz) {
}

void Stream::tracer(size_t sz) {
}

void Table::tracer(size_t sz) {
}

void Header::tracer(size_t sz) {
  versionName->markAndTrace();
  tildStream->markAndTrace();
  stringStream->markAndTrace();
  usStream->markAndTrace();
  blobStream->markAndTrace();
  guidStream->markAndTrace();
  TRACE_VECTOR(Table*, tables);
}

void CLIString::tracer(size_t sz) {
}

void Exception::tracer(size_t sz) {
  catchClass->markAndTrace();
}
