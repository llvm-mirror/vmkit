//===----- MvmMemoryManager.cpp - LLVM Memory manager for Mvm -------------===//
//
//                              Mvm
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <assert.h>

#include "mvm/Method.h"
#include "mvm/Object.h"

#include "mvm/MvmMemoryManager.h"

using namespace mvm;
using namespace llvm;

unsigned char* MvmMemoryManager::startFunctionBody(const Function* F, 
                                                   uintptr_t &ActualSize) {
  size_t nbb = ((ActualSize - 1) & -4) + 4 + sizeof(Method *);
#ifdef MULTIPLE_GC
  Collector* GC = GCMap[F->getParent()];
  assert(GC && "GC not available in a multi-GC environment");
  Code *res = (Code *)gc::operator new(nbb, Code::VT, GC);
  Method* meth = collector_new(Method, GC)(res, ActualSize);
#else
  Code *res = (Code *)gc::operator new(nbb, Code::VT);
  Method* meth = gc_new(Method)(res, ActualSize);
#endif
  meth->llvmFunction = F;
  res->method(meth);
  currentMethod = meth;
  return (unsigned char*)((unsigned int*)res + 2);
}

unsigned char *MvmMemoryManager::allocateStub(unsigned StubSize, 
                                              unsigned Alignment) {
  size_t nbb = ((StubSize - 1) & -4) + 4 + sizeof(Method *);
  Code *res = (Code *)gc::operator new(nbb, Code::VT); 
  Method* meth = gc_new(Method)(res, StubSize);
  res->method(meth);
  Object::pushRoot(meth);
  return (unsigned char*)((unsigned int*)res + 2);
}

void MvmMemoryManager::endFunctionBody(const Function *F, 
                                       unsigned char *FunctionStart,
                                       unsigned char *FunctionEnd) {
}


void MvmMemoryManager::deallocateMemForFunction(const Function *F) {
}

void MvmMemoryManager::AllocateGOT() {
  assert(GOTBase == 0 && "Cannot allocate the got multiple times");
  GOTBase = new unsigned char[sizeof(void*) * 8192];
  HasGOT = true;
}

unsigned char *MvmMemoryManager::getGOTBase() const {
  return GOTBase;
}

unsigned char *MvmMemoryManager::startExceptionTable(const Function* F, 
                                                     uintptr_t &ActualSize) {
#ifdef MULTIPLE_GC
  Collector* GC = GCMap[F->getParent()];
  assert(GC && "GC not available in a multi-GC environment");
  ExceptionTable *res = (ExceptionTable*)gc::operator new(ActualSize + 4, 
                                                          ExceptionTable::VT,
                                                          GC);
#else
  ExceptionTable *res = (ExceptionTable*)gc::operator new(ActualSize + 4, 
                                                          ExceptionTable::VT);
#endif
  currentMethod->exceptionTable(res);
  return (unsigned char*)((unsigned int*)res + 2);
}                                                     

void MvmMemoryManager::endExceptionTable(const Function *F, 
                                         unsigned char *TableStart,
                                         unsigned char *TableEnd,
                                         unsigned char* FrameRegister) {
  ExceptionTable* table = currentMethod->exceptionTable();
  table->frameRegister(FrameRegister);
}

