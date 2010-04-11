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

extern "C" void CLIObjectTracer(VMObject* obj) {
	VMObject::_trace(obj);
}

// N3 Objects
void VMObject::_trace(VMObject *self) {
  mvm::Collector::markAndTrace(self, &self->lockObj);
}

#define TRACE_VECTOR(type, name, alloc) { \
  for (std::vector<type, alloc<type> >::iterator i = name.begin(), e = name.end(); \
       i!= e; ++i) {                                                    \
    mvm::Collector::markAndTraceRoot(&(*i)); }}

#define CALL_TRACER_VECTOR(type, name, alloc) { \
  for (std::vector<type, alloc<type> >::iterator i = name.begin(), e = name.end(); \
       i!= e; ++i) {                                                    \
    (*i)->tracer(); }}

// internal objects
void VMThread::tracer() {
  mvm::Collector::markAndTraceRoot(&ooo_appThread);
  mvm::Collector::markAndTraceRoot(&ooo_pendingException);
	// I suppose that the vm is already traced by the gc
	//  vm->tracer();
}

void N3::tracer() {
	// If I understand, the gc already call trace for all VMThread
//   if (bootstrapThread) {
//     bootstrapThread->tracer();
//     for (VMThread* th = (VMThread*)bootstrapThread->next(); 
//          th != bootstrapThread; th = (VMThread*)th->next())
//       th->tracer();
//   }
  loadedAssemblies->tracer();
}

void Assembly::tracer() {
  loadedNameClasses->tracer();
  mvm::Collector::markAndTraceRoot(&ooo_delegatee);
}

void VMCommonClass::tracer() {
  mvm::Collector::markAndTraceRoot(&ooo_delegatee);
	CALL_TRACER_VECTOR(VMMethod*, virtualMethods, std::allocator);
	CALL_TRACER_VECTOR(VMMethod*, staticMethods, std::allocator);
  CALL_TRACER_VECTOR(Property*, properties, gc_allocator);
}

void VMClass::tracer() {
  VMCommonClass::tracer();
  mvm::Collector::markAndTraceRoot(&staticInstance);
  mvm::Collector::markAndTraceRoot(&virtualInstance);
}

void VMGenericClass::tracer() {
  VMClass::tracer();
}

void VMClassArray::tracer() {
  VMCommonClass::tracer();
}

void VMClassPointer::tracer() {
  VMCommonClass::tracer();
}

void VMMethod::tracer() {
  mvm::Collector::markAndTraceRoot(&ooo_delegatee);
}

void VMGenericMethod::tracer() {
  VMMethod::tracer();
}

void Property::tracer() {
  mvm::Collector::markAndTraceRoot(&ooo_delegatee);
}

// useless (never called or used) but it simplifies the definition of LockedMap
void VMField::tracer() {
}
