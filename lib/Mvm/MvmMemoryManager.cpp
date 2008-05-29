//===----- MvmMemoryManager.cpp - LLVM Memory manager for Mvm -------------===//
//
//                              Mvm
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <assert.h>

#include "mvm/JIT.h"
#include "mvm/Method.h"
#include "mvm/Object.h"

#include "mvm/MvmMemoryManager.h"

using namespace mvm;
using namespace llvm;

unsigned char* MvmMemoryManager::startFunctionBody(const Function* F, 
                                                   uintptr_t &ActualSize) {
  Code* meth = new Code(); 
  currentMethod = meth;
  return realMemoryManager->startFunctionBody(F, ActualSize); 
}

unsigned char *MvmMemoryManager::allocateStub(const GlobalValue* GV,
                                              unsigned StubSize, 
                                              unsigned Alignment) {
  unsigned char* res = realMemoryManager->allocateStub(GV, StubSize, Alignment); 
  Code* meth = new Code();
  mvm::jit::addMethodInfo((void*)(res + StubSize), meth);
  currentMethod = meth;
  meth->FunctionStart = res;
  meth->FunctionEnd = res + StubSize;
  currentMethod = meth;

  return res;
}

void MvmMemoryManager::endFunctionBody(const Function *F, 
                                       unsigned char *FunctionStart,
                                       unsigned char *FunctionEnd) {
  mvm::jit::addMethodInfo((void*)FunctionEnd, currentMethod);
  currentMethod->FunctionStart = FunctionStart;
  currentMethod->FunctionEnd = FunctionEnd;
  realMemoryManager->endFunctionBody(F, FunctionStart, FunctionEnd);
}


void MvmMemoryManager::deallocateMemForFunction(const Function *F) {
  realMemoryManager->deallocateMemForFunction(F);
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
  unsigned char* res = realMemoryManager->startExceptionTable(F, ActualSize);
  
  currentMethod->exceptionTable = res;
  return (unsigned char*)res;
}                                                     

void MvmMemoryManager::endExceptionTable(const Function *F, 
                                         unsigned char *TableStart,
                                         unsigned char *TableEnd,
                                         unsigned char* FrameRegister) {
  realMemoryManager->endExceptionTable(F, TableStart, TableEnd, FrameRegister);
  currentMethod->frameRegister = FrameRegister;
}
