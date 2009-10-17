//===------ N3ModuleProvider.cpp - LLVM Module Provider for N3 ------------===//
//
//                              N3
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <llvm/Module.h>
#include <llvm/ModuleProvider.h>

#include "mvm/JIT.h"

#include "Assembly.h"
#include "CLIJit.h"
#include "N3ModuleProvider.h"
#include "VMClass.h"
#include "VMThread.h"
#include "N3.h"

using namespace llvm;
using namespace n3;


bool N3ModuleProvider::materializeFunction(Function *F, std::string *ErrInfo) {
  if (!F->empty()) return false;
  VMMethod* meth = functions->lookup(F);

  if (!meth) {
    // VT methods
    return false;
  } else {
		meth->compileToNative();
    return false;
  }
}
