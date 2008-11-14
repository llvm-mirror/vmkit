//===----- MvmMemoryManager.cpp - LLVM Memory manager for Mvm -------------===//
//
//                              Mvm
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "mvm/JIT.h"
#include "mvm/Object.h"

#include "mvm/MvmMemoryManager.h"

using namespace mvm;
using namespace llvm;

void MvmMemoryManager::endFunctionBody(const Function *F, 
                                       unsigned char *FunctionStart,
                                       unsigned char *FunctionEnd) {
  MvmModule::addMethodInfo((void*)FunctionEnd, F);
  realMemoryManager->endFunctionBody(F, FunctionStart, FunctionEnd);
}
