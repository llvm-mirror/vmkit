//===------ N3ModuleProvider.cpp - LLVM Module Provider for N3 ------------===//
//
//                              N3
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <llvm/Module.h>
#include <llvm/GVMaterializer.h>

#include "mvm/JIT.h"

#include "Assembly.h"
#include "CLIJit.h"
#include "N3ModuleProvider.h"
#include "VMClass.h"
#include "VMThread.h"
#include "N3.h"

using namespace llvm;
using namespace n3;


bool N3ModuleProvider::Materialize(GlobalValue *GV, std::string *ErrInfo) {
  Function* F = dyn_cast<Function>(GV);
  assert(F && "Not a function.");
  if (F->getLinkage() == GlobalValue::ExternalLinkage) return false;
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

bool N3ModuleProvider::isMaterializable(const llvm::GlobalValue* GV) const {
  return GV->getLinkage() == GlobalValue::ExternalWeakLinkage;
}
