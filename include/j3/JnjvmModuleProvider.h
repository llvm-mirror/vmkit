//===-------- JnjvmModuleProvider.h - LLVM Module Provider for J3 ---------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef JNJVM_MODULE_PROVIDER_H
#define JNJVM_MODULE_PROVIDER_H

#include <llvm/GVMaterializer.h>

namespace j3 {

class JavaJITCompiler;

class JnjvmModuleProvider : public llvm::GVMaterializer {
public:
 
  JavaJITCompiler* Comp;
  llvm::Module* TheModule;

  JnjvmModuleProvider(llvm::Module* M, JavaJITCompiler* C);
  ~JnjvmModuleProvider();

  virtual bool Materialize(llvm::GlobalValue *GV, std::string *ErrInfo = 0);
  virtual bool isMaterializable(const llvm::GlobalValue*) const;
  virtual bool isDematerializable(const llvm::GlobalValue*) const {
    return false;
  }
  virtual bool MaterializeModule(llvm::Module*, std::string*) { return true; }
};

} // End j3 namespace

#endif
