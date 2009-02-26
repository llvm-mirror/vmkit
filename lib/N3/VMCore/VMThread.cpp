//===------------ VMThread.cpp - VM thread description --------------------===//
//
//                                N3
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/Target/TargetData.h"

#include "mvm/JIT.h"
#include "mvm/PrintBuffer.h"
#include "mvm/Threads/Locks.h"
#include "mvm/Threads/Thread.h"

#include "Assembly.h"
#include "CLIJit.h"
#include "N3ModuleProvider.h"
#include "VMClass.h"
#include "VMObject.h"
#include "VMThread.h"
#include "VirtualMachine.h"

using namespace n3;

const unsigned int VMThread::StateRunning = 0;
const unsigned int VMThread::StateWaiting = 1;
const unsigned int VMThread::StateInterrupted = 2;

void VMThread::print(mvm::PrintBuffer* buf) const {
  buf->write("Thread:");
  vmThread->print(buf);
}

VMThread::~VMThread() {
  delete perFunctionPasses;
}

VMThread::VMThread() {
  perFunctionPasses = 0;
}

extern void AddStandardCompilePasses(llvm::FunctionPassManager*);

VMThread* VMThread::allocate(VMObject* thread, VirtualMachine* vm) {
  VMThread* key = new VMThread();
  key->vmThread = thread;
  key->vm = vm;
  key->lock = new mvm::LockNormal();
  key->varcond = new mvm::Cond();
  key->interruptFlag = 0;
  key->state = StateRunning;
  key->pendingException = 0;
  key->perFunctionPasses = new llvm::FunctionPassManager(vm->TheModuleProvider);
  key->perFunctionPasses->add(new llvm::TargetData(vm->module->getLLVMModule()));
  AddStandardCompilePasses(key->perFunctionPasses);
  return key;
}

VMObject* VMThread::currentThread() {
  VMThread* result = get();
  if (result != 0)
    return result->vmThread;
  else
    return 0;
}

void* VMThread::getCppException() {
  return (void*)((char*)VMThread::get()->internalPendingException - 8 * sizeof(void*));
}

VMObject* VMThread::getCLIException() {
  VMThread* th = VMThread::get();
  return th->pendingException;
}

extern "C" void* __cxa_allocate_exception(unsigned);
extern "C" void __cxa_throw(void*, void*, void*);


void VMThread::throwException(VMObject* obj) {
  VMThread* th = VMThread::get();
  assert(th->pendingException == 0 && "pending exception already there?");
  th->pendingException = obj;
  void* exc = __cxa_allocate_exception(0);
  th->internalPendingException = exc;
  __cxa_throw(exc, 0, 0); 
}

void VMThread::internalClearException() {
  pendingException = 0;
  internalPendingException = 0;
}

bool VMThread::compareException(VMClass* cl) {
  VMObject* pe = VMThread::get()->pendingException;
  assert(pe && "no pending exception?");
  return pe->classOf->subclassOf(cl);
}
