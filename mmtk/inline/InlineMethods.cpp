//===-------- InlineMethods.cpp - Initialize the inline methods -----------===//
//
//                      The VMKit project
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <llvm/CallingConv.h>
#include <llvm/Constants.h>
#include <llvm/DerivedTypes.h>
#include <llvm/Instructions.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Type.h>

using namespace llvm;

namespace mmtk {

namespace mmtk_runtime {
  #include "MMTkInline.inc"
}

extern "C" void MMTk_InlineMethods(llvm::Module* module) {
  mmtk_runtime::makeLLVMFunction(module);
}

}
