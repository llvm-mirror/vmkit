//===-------- InlineMethods.cpp - Initialize the inline methods -----------===//
//
//                      The VMKit project
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <llvm/IR/CallingConv.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>

using namespace llvm;

namespace mmtk {

namespace mmtk_malloc {
  #include "MMTkMallocInline.inc"
}

namespace mmtk_array_write {
  #include "MMTkArrayWriteInline.inc"
}

namespace mmtk_field_write {
  #include "MMTkFieldWriteInline.inc"
}

namespace mmtk_non_heap_write {
  #include "MMTkNonHeapWriteInline.inc"
}

}

extern "C" void MMTk_InlineMethods(llvm::Module* module) {
  mmtk::mmtk_malloc::makeLLVMFunction(module);
  mmtk::mmtk_field_write::makeLLVMFunction(module);
  mmtk::mmtk_array_write::makeLLVMFunction(module);
  mmtk::mmtk_non_heap_write::makeLLVMFunction(module);
}
