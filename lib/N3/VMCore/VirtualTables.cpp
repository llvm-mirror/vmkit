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
  INIT(VMCond);
  INIT(LockObj);
  INIT(VMObject);
  INIT(VMThread);
  INIT(ThreadSystem);
  INIT(CLIString);
  INIT(Property);
  INIT(CacheNode);
  INIT(Enveloppe);
  INIT(Opinfo);
  INIT(CLIJit);
  INIT(Exception);
  
#undef INIT

void Opinfo::TRACER {
}

void CLIJit::TRACER {
  compilingMethod->CALL_TRACER;
  compilingClass->CALL_TRACER;
}

void ThreadSystem::TRACER {
  //nonDaemonLock->MARK_AND_TRACE;
  //nonDaemonVar->MARK_AND_TRACE;
}

void Reader::TRACER {
  bytes->MARK_AND_TRACE;
}

void CacheNode::TRACER {
  ((mvm::Object*)methPtr)->MARK_AND_TRACE;
  lastCible->CALL_TRACER;
  next->MARK_AND_TRACE;
  enveloppe->MARK_AND_TRACE;
}

void Enveloppe::TRACER {
  firstCache->MARK_AND_TRACE;
  //cacheLock->MARK_AND_TRACE;
  originalMethod->CALL_TRACER;
}

void VMArray::TRACER {
  VMObject::CALL_TRACER;
}

void ArrayObject::TRACER {
  VMObject::CALL_TRACER;
  for (sint32 i = 0; i < size; i++) {
    elements[i]->MARK_AND_TRACE;
  }
}

#define ARRAYTRACER(name)         \
  void name::TRACER {             \
    VMObject::CALL_TRACER;      \
  }
  

ARRAYTRACER(ArrayUInt8)
ARRAYTRACER(ArraySInt8)
ARRAYTRACER(ArrayUInt16)
ARRAYTRACER(ArraySInt16)
ARRAYTRACER(ArrayUInt32)
ARRAYTRACER(ArraySInt32)
ARRAYTRACER(ArrayLong)
ARRAYTRACER(ArrayFloat)
ARRAYTRACER(ArrayDouble)

#undef ARRAYTRACER


#define TRACE_VECTOR(type, name, alloc) { \
  for (std::vector<type, alloc<type> >::iterator i = name.begin(), e = name.end(); \
       i!= e; ++i) {                                                    \
    (*i)->MARK_AND_TRACE; }}

#define CALL_TRACER_VECTOR(type, name, alloc) { \
  for (std::vector<type, alloc<type> >::iterator i = name.begin(), e = name.end(); \
       i!= e; ++i) {                                                    \
    (*i)->CALL_TRACER; }}


void VMCommonClass::TRACER {
  name->MARK_AND_TRACE;
  nameSpace->MARK_AND_TRACE;
  super->CALL_TRACER;
  CALL_TRACER_VECTOR(VMClass*, interfaces, std::allocator);
  //lockVar->MARK_AND_TRACE;
  //condVar->MARK_AND_TRACE;
  CALL_TRACER_VECTOR(VMMethod*, virtualMethods, std::allocator);
  CALL_TRACER_VECTOR(VMMethod*, staticMethods, std::allocator);
  CALL_TRACER_VECTOR(VMField*, virtualFields, std::allocator);
  CALL_TRACER_VECTOR(VMField*, staticFields, std::allocator);
  delegatee->MARK_AND_TRACE;
  CALL_TRACER_VECTOR(VMCommonClass*, display, std::allocator);
  vm->CALL_TRACER;

  assembly->CALL_TRACER;
  //funcs->MARK_AND_TRACE;
  TRACE_VECTOR(Property*, properties, gc_allocator);
}

void VMClass::TRACER {
  VMCommonClass::CALL_TRACER;
  staticInstance->MARK_AND_TRACE;
  virtualInstance->MARK_AND_TRACE;
  CALL_TRACER_VECTOR(VMClass*, innerClasses, std::allocator);
  outerClass->CALL_TRACER;
  CALL_TRACER_VECTOR(VMMethod*, genericMethods, std::allocator);
}

void VMGenericClass::TRACER {
  VMClass::CALL_TRACER;
  CALL_TRACER_VECTOR(VMCommonClass*, genericParams, std::allocator);
}

void VMClassArray::TRACER {
  VMCommonClass::CALL_TRACER;
  baseClass->CALL_TRACER;
}

void VMClassPointer::TRACER {
  VMCommonClass::CALL_TRACER;
  baseClass->CALL_TRACER;
}


void VMMethod::TRACER {
  delegatee->MARK_AND_TRACE;
  //signature->MARK_AND_TRACE;
  classDef->CALL_TRACER;
  CALL_TRACER_VECTOR(Param*, params, gc_allocator);
  TRACE_VECTOR(Enveloppe*, caches, gc_allocator);
  name->MARK_AND_TRACE;
}

void VMGenericMethod::TRACER {
  VMMethod::CALL_TRACER;
  CALL_TRACER_VECTOR(VMCommonClass*, genericParams, std::allocator);
}

void VMField::TRACER {
  signature->CALL_TRACER;
  classDef->CALL_TRACER;
  name->MARK_AND_TRACE;
}

void VMCond::TRACER {
  for (std::vector<VMThread*, std::allocator<VMThread*> >::iterator i = threads.begin(), e = threads.end();
       i!= e; ++i) {
    (*i)->CALL_TRACER; 
  }
}

void LockObj::TRACER {
  //lock->MARK_AND_TRACE;
  varcond->MARK_AND_TRACE;
}

void VMObject::TRACER {
  classOf->CALL_TRACER;
  lockObj->MARK_AND_TRACE;
}

void VMThread::TRACER {
  vmThread->MARK_AND_TRACE;
  vm->CALL_TRACER;
  //lock->MARK_AND_TRACE;
  //varcond->MARK_AND_TRACE;
  pendingException->MARK_AND_TRACE;
}

void VirtualMachine::TRACER {
  threadSystem->MARK_AND_TRACE;
  hashUTF8->CALL_TRACER;
  functions->CALL_TRACER;
  if (bootstrapThread) {
    bootstrapThread->CALL_TRACER;
    for (VMThread* th = (VMThread*)bootstrapThread->next(); 
         th != bootstrapThread; th = (VMThread*)th->next())
      th->CALL_TRACER;
  }
}

void Param::TRACER {
  method->CALL_TRACER;
  name->MARK_AND_TRACE;
}

void Property::TRACER {
  type->CALL_TRACER;
  //signature->MARK_AND_TRACE;
  name->MARK_AND_TRACE;
  delegatee->MARK_AND_TRACE;
}

void Assembly::TRACER {
  loadedNameClasses->CALL_TRACER;
  loadedTokenClasses->CALL_TRACER;
  loadedTokenMethods->CALL_TRACER;
  loadedTokenFields->CALL_TRACER;
  //lockVar->MARK_AND_TRACE;
  //condVar->MARK_AND_TRACE;
  name->MARK_AND_TRACE;
  bytes->MARK_AND_TRACE;
  textSection->CALL_TRACER;
  rsrcSection->CALL_TRACER;
  relocSection->CALL_TRACER;
  CLIHeader->CALL_TRACER;
  vm->CALL_TRACER;
  delegatee->MARK_AND_TRACE;
  // TODO trace assembly refs...
}

void N3::TRACER {
  VirtualMachine::CALL_TRACER;
  hashUTF8->CALL_TRACER;
  hashStr->CALL_TRACER;
  loadedAssemblies->CALL_TRACER;
}

void Section::TRACER {
}

void Stream::TRACER {
}

void Table::TRACER {
}

void Header::TRACER {
  versionName->MARK_AND_TRACE;
  tildStream->CALL_TRACER;
  stringStream->CALL_TRACER;
  usStream->CALL_TRACER;
  blobStream->CALL_TRACER;
  guidStream->CALL_TRACER;
  CALL_TRACER_VECTOR(Table*, tables, gc_allocator);
}

void CLIString::TRACER {
}

void Exception::TRACER {
  catchClass->CALL_TRACER;
}

#ifdef MULTIPLE_GC
extern "C" void CLIObjectTracer(VMObject* obj, Collector* GC) {
#else
extern "C" void CLIObjectTracer(VMObject* obj) {
#endif
  obj->classOf->CALL_TRACER;
  obj->lockObj->MARK_AND_TRACE;
}

