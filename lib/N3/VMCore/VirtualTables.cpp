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
  INIT(VMGenericClass);
  INIT(VMClassArray);
  INIT(VMClassPointer);
  INIT(VMMethod);
  INIT(VMGenericMethod);
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

void Opinfo::TRACER {
}

void CLIJit::TRACER {
  compilingMethod->MARK_AND_TRACE;
  compilingClass->MARK_AND_TRACE;
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
  lastCible->MARK_AND_TRACE;
  next->MARK_AND_TRACE;
  enveloppe->MARK_AND_TRACE;
}

void Enveloppe::TRACER {
  firstCache->MARK_AND_TRACE;
  //cacheLock->MARK_AND_TRACE;
  originalMethod->MARK_AND_TRACE;
}

void VMArray::TRACER {
  VMObject::PARENT_TRACER;
}

void ArrayObject::TRACER {
  VMObject::PARENT_TRACER;
  for (sint32 i = 0; i < size; i++) {
    elements[i]->MARK_AND_TRACE;
  }
}

#define ARRAYTRACER(name)         \
  void name::TRACER {             \
    VMObject::PARENT_TRACER;      \
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

void VMCommonClass::TRACER {
  name->MARK_AND_TRACE;
  nameSpace->MARK_AND_TRACE;
  super->MARK_AND_TRACE;
  TRACE_VECTOR(VMClass*, interfaces, std::allocator);
  //lockVar->MARK_AND_TRACE;
  //condVar->MARK_AND_TRACE;
  TRACE_VECTOR(VMMethod*, virtualMethods, std::allocator);
  TRACE_VECTOR(VMMethod*, staticMethods, std::allocator);
  TRACE_VECTOR(VMField*, virtualFields, std::allocator);
  TRACE_VECTOR(VMField*, staticFields, std::allocator);
  delegatee->MARK_AND_TRACE;
  TRACE_VECTOR(VMCommonClass*, display, std::allocator);
  vm->MARK_AND_TRACE;

  assembly->MARK_AND_TRACE;
  //funcs->MARK_AND_TRACE;
  TRACE_VECTOR(Property*, properties, gc_allocator);
}

void VMClass::TRACER {
  VMCommonClass::PARENT_TRACER;
  staticInstance->MARK_AND_TRACE;
  virtualInstance->MARK_AND_TRACE;
  TRACE_VECTOR(VMClass*, innerClasses, std::allocator);
  outerClass->MARK_AND_TRACE;
  TRACE_VECTOR(VMMethod*, genericMethods, std::allocator);
}

void VMGenericClass::TRACER {
  VMClass::PARENT_TRACER;
  TRACE_VECTOR(VMCommonClass*, genericParams, std::allocator);
}

void VMClassArray::TRACER {
  VMCommonClass::PARENT_TRACER;
  baseClass->MARK_AND_TRACE;
}

void VMClassPointer::TRACER {
  VMCommonClass::PARENT_TRACER;
  baseClass->MARK_AND_TRACE;
}


void VMMethod::TRACER {
  delegatee->MARK_AND_TRACE;
  //signature->MARK_AND_TRACE;
  classDef->MARK_AND_TRACE;
  TRACE_VECTOR(Param*, params, gc_allocator);
  TRACE_VECTOR(Enveloppe*, caches, gc_allocator);
  name->MARK_AND_TRACE;
}

void VMGenericMethod::TRACER {
  VMMethod::PARENT_TRACER;
  TRACE_VECTOR(VMCommonClass*, genericParams, std::allocator);
}

void VMField::TRACER {
  signature->MARK_AND_TRACE;
  classDef->MARK_AND_TRACE;
  name->MARK_AND_TRACE;
}

void VMCond::TRACER {
  TRACE_VECTOR(VMThread*, threads, std::allocator);
}

void LockObj::TRACER {
  //lock->MARK_AND_TRACE;
  varcond->MARK_AND_TRACE;
}

void VMObject::TRACER {
  classOf->MARK_AND_TRACE;
  lockObj->MARK_AND_TRACE;
}

void VMThread::TRACER {
  vmThread->MARK_AND_TRACE;
  vm->MARK_AND_TRACE;
  //lock->MARK_AND_TRACE;
  //varcond->MARK_AND_TRACE;
  pendingException->MARK_AND_TRACE;
}

void VirtualMachine::TRACER {
  threadSystem->MARK_AND_TRACE;
  hashUTF8->MARK_AND_TRACE;
  functions->MARK_AND_TRACE;
  //protectModule->MARK_AND_TRACE;
  bootstrapThread->MARK_AND_TRACE;
}

void Param::TRACER {
  method->MARK_AND_TRACE;
  name->MARK_AND_TRACE;
}

void Property::TRACER {
  type->MARK_AND_TRACE;
  //signature->MARK_AND_TRACE;
  name->MARK_AND_TRACE;
  delegatee->MARK_AND_TRACE;
}

void Assembly::TRACER {
  loadedNameClasses->MARK_AND_TRACE;
  loadedTokenClasses->MARK_AND_TRACE;
  loadedTokenMethods->MARK_AND_TRACE;
  loadedTokenFields->MARK_AND_TRACE;
  //lockVar->MARK_AND_TRACE;
  //condVar->MARK_AND_TRACE;
  name->MARK_AND_TRACE;
  bytes->MARK_AND_TRACE;
  textSection->MARK_AND_TRACE;
  rsrcSection->MARK_AND_TRACE;
  relocSection->MARK_AND_TRACE;
  CLIHeader->MARK_AND_TRACE;
  vm->MARK_AND_TRACE;
  delegatee->MARK_AND_TRACE;
  // TODO trace assembly refs...
}

void N3::TRACER {
  VirtualMachine::PARENT_TRACER;
  hashUTF8->MARK_AND_TRACE;
  hashStr->MARK_AND_TRACE;
  loadedAssemblies->MARK_AND_TRACE;
}

void Section::TRACER {
}

void Stream::TRACER {
}

void Table::TRACER {
}

void Header::TRACER {
  versionName->MARK_AND_TRACE;
  tildStream->MARK_AND_TRACE;
  stringStream->MARK_AND_TRACE;
  usStream->MARK_AND_TRACE;
  blobStream->MARK_AND_TRACE;
  guidStream->MARK_AND_TRACE;
  TRACE_VECTOR(Table*, tables, gc_allocator);
}

void CLIString::TRACER {
}

void Exception::TRACER {
  catchClass->MARK_AND_TRACE;
}
