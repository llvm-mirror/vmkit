//===------ VirtualMachine.cpp - Virtual machine description --------------===//
//
//                                N3
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <vector>
#include <stdarg.h>

#include "llvm/Function.h"
#include "llvm/Module.h"

#include "mvm/Object.h"
#include "mvm/PrintBuffer.h"
#include "mvm/Threads/Cond.h"
#include "mvm/Threads/Locks.h"

#include "types.h"

#include "Assembly.h"
#include "LockedMap.h"
#include "N3ModuleProvider.h"
#include "VirtualMachine.h"
#include "VMArray.h"
#include "VMClass.h"

using namespace n3;

#define DECLARE_EXCEPTION(EXCP) \
  const char* VirtualMachine::EXCP = #EXCP

DECLARE_EXCEPTION(SystemException);
DECLARE_EXCEPTION(OverFlowException);
DECLARE_EXCEPTION(OutOfMemoryException);
DECLARE_EXCEPTION(IndexOutOfRangeException);
DECLARE_EXCEPTION(NullReferenceException);
DECLARE_EXCEPTION(SynchronizationLocException);
DECLARE_EXCEPTION(ThreadInterruptedException);
DECLARE_EXCEPTION(MissingMethodException);
DECLARE_EXCEPTION(MissingFieldException);
DECLARE_EXCEPTION(ArrayTypeMismatchException);
DECLARE_EXCEPTION(ArgumentException);

/*
DECLARE_EXCEPTION(ArithmeticException);
DECLARE_EXCEPTION(InvocationTargetException);
DECLARE_EXCEPTION(ArrayStoreException);
DECLARE_EXCEPTION(ClassCastException);
DECLARE_EXCEPTION(ArrayIndexOutOfBoundsException);
DECLARE_EXCEPTION(SecurityException);
DECLARE_EXCEPTION(ClassFormatError);
DECLARE_EXCEPTION(ClassCircularityError);
DECLARE_EXCEPTION(NoClassDefFoundError);
DECLARE_EXCEPTION(UnsupportedClassVersionError);
DECLARE_EXCEPTION(NoSuchFieldError);
DECLARE_EXCEPTION(NoSuchMethodError);
DECLARE_EXCEPTION(InstantiationError);
DECLARE_EXCEPTION(IllegalAccessError);
DECLARE_EXCEPTION(IllegalAccessException);
DECLARE_EXCEPTION(VerifyError);
DECLARE_EXCEPTION(ExceptionInInitializerError);
DECLARE_EXCEPTION(LinkageError);
DECLARE_EXCEPTION(AbstractMethodError);
DECLARE_EXCEPTION(UnsatisfiedLinkError);
DECLARE_EXCEPTION(InternalError);
DECLARE_EXCEPTION(StackOverflowError);
DECLARE_EXCEPTION(ClassNotFoundException);
*/

#undef DECLARE_EXCEPTION
void ThreadSystem::print(mvm::PrintBuffer* buf) const {
  buf->write("ThreadSystem<>");
}

ThreadSystem* ThreadSystem::allocateThreadSystem() {
  ThreadSystem* res = gc_new(ThreadSystem)();
  res->nonDaemonThreads = 1;
  res->nonDaemonLock = new mvm::LockNormal();
  res->nonDaemonVar  = new mvm::Cond();
  return res;
}

const UTF8* VirtualMachine::asciizConstructUTF8(const char* asciiz) {
  return hashUTF8->lookupOrCreateAsciiz(asciiz);
}

const UTF8* VirtualMachine::readerConstructUTF8(const uint16* buf, uint32 len) {
  return hashUTF8->lookupOrCreateReader(buf, len);
}

void VirtualMachine::error(const char* className, const char* fmt, va_list ap) {
  fprintf(stderr, "Internal exception of type %s during bootstrap: ", className);
  vfprintf(stderr, fmt, ap);
  throw 1;
}

void VirtualMachine::indexOutOfBounds(const VMObject* obj, sint32 entry) {
  error(IndexOutOfRangeException, "%d", entry);
}

void VirtualMachine::negativeArraySizeException(sint32 size) {
  error(OverFlowException, "%d", size);
}

void VirtualMachine::nullPointerException(const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  error(NullReferenceException, fmt, va_arg(ap, char*));
}


void VirtualMachine::illegalMonitorStateException(const VMObject* obj) {
  error(SynchronizationLocException, "");
}

void VirtualMachine::interruptedException(const VMObject* obj) {
  error(ThreadInterruptedException, "");
}

void VirtualMachine::outOfMemoryError(sint32 n) {
  error(OutOfMemoryException, "");
}

void VirtualMachine::arrayStoreException() {
  error(ArrayTypeMismatchException, "");
}

void VirtualMachine::illegalArgumentException(const char* name) {
  error(ArgumentException, name);
}

void VirtualMachine::unknownError(const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  error(SystemException, fmt, ap);
}

void VirtualMachine::error(const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  error(SystemException, fmt, ap);
}

void VirtualMachine::error(const char* name, const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  error(name, fmt, ap);
}

VirtualMachine::~VirtualMachine() {
  delete module;
  delete TheModuleProvider;
}

VirtualMachine::VirtualMachine(mvm::BumpPtrAllocator &allocator)
	: mvm::VirtualMachine(allocator) {
  module = 0;
  TheModuleProvider = 0;
}

VMMethod* VirtualMachine::lookupFunction(Function* F) {
  return functions->lookup(F);
}
