//===---------- LLVMMaterializer.h - LLVM Materializer for J3 -------------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef J3_LLVM_MATERIALIZER_H
#define J3_LLVM_MATERIALIZER_H

#include <llvm/GVMaterializer.h>

namespace j3 {

class JavaLLVMLazyJITCompiler;

class LLVMMaterializer : public llvm::GVMaterializer {
public:
 
  JavaLLVMLazyJITCompiler* Comp;
  llvm::Module* TheModule;

  LLVMMaterializer(llvm::Module* M, JavaLLVMLazyJITCompiler* C);

  virtual bool Materialize(llvm::GlobalValue *GV, std::string *ErrInfo = 0);
  virtual bool isMaterializable(const llvm::GlobalValue*) const;
  virtual bool isDematerializable(const llvm::GlobalValue*) const {
    return false;
  }
  virtual bool MaterializeModule(llvm::Module*, std::string*) { return true; }
};

} // End j3 namespace

#endif
