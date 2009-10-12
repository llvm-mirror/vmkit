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
  INIT(ArrayChar);
  INIT(ArrayUInt16);
  INIT(ArraySInt16);
  INIT(ArrayUInt32);
  INIT(ArraySInt32);
  INIT(ArrayLong);
  INIT(ArrayFloat);
  INIT(ArrayDouble);
  INIT(ArrayObject);
  INIT(LockObj);
  INIT(VMObject);
  INIT(CLIString);
  
#undef INIT

#ifdef MULTIPLE_GC
extern "C" void CLIObjectTracer(VMObject* obj, Collector* GC) {
#else
extern "C" void CLIObjectTracer(VMObject* obj) {
#endif
  obj->lockObj->MARK_AND_TRACE;
}

// N3 Objects
void LockObj::TRACER {
}

void VMObject::TRACER {
  lockObj->MARK_AND_TRACE;
}

void CLIString::TRACER {
	VMObject::CALL_TRACER;
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
ARRAYTRACER(ArrayChar)
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

// internal objects
void VMThread::TRACER {
	declare_gcroot(VMObject*, th) = vmThread;
  th->MARK_AND_TRACE;
  pendingException->MARK_AND_TRACE;
	// I suppose that the vm is already traced by the gc
	//  vm->CALL_TRACER;
}

void N3::TRACER {
	// If I understand, the gc already call trace for all VMThread
//   if (bootstrapThread) {
//     bootstrapThread->CALL_TRACER;
//     for (VMThread* th = (VMThread*)bootstrapThread->next(); 
//          th != bootstrapThread; th = (VMThread*)th->next())
//       th->CALL_TRACER;
//   }
  loadedAssemblies->CALL_TRACER;
}

void Assembly::TRACER {
  loadedNameClasses->CALL_TRACER;
  delegatee->MARK_AND_TRACE;
}

void VMCommonClass::TRACER {
  delegatee->MARK_AND_TRACE;
	CALL_TRACER_VECTOR(VMMethod*, virtualMethods, std::allocator);
	CALL_TRACER_VECTOR(VMMethod*, staticMethods, std::allocator);
  CALL_TRACER_VECTOR(Property*, properties, gc_allocator);
}

void VMClass::TRACER {
  VMCommonClass::CALL_TRACER;
  staticInstance->MARK_AND_TRACE;
  virtualInstance->MARK_AND_TRACE;
}

void VMGenericClass::TRACER {
  VMClass::CALL_TRACER;
}

void VMClassArray::TRACER {
  VMCommonClass::CALL_TRACER;
}

void VMClassPointer::TRACER {
  VMCommonClass::CALL_TRACER;
}

void VMMethod::TRACER {
  delegatee->MARK_AND_TRACE;
}

void VMGenericMethod::TRACER {
  VMMethod::CALL_TRACER;
}

void Property::TRACER {
  delegatee->MARK_AND_TRACE;
}

// never called but it simplifies the definition of LockedMap
void VMField::TRACER {
}
