//===----- JnjvmModuleProvider.h - LLVM Module Provider for Jnjvm ---------===//
//
//                              Jnjvm
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef JNJVM_MODULE_PROVIDER_H
#define JNJVM_MODULE_PROVIDER_H

#include <llvm/ModuleProvider.h>

namespace jnjvm {

class JavaJITCompiler;

class JnjvmModuleProvider : public llvm::ModuleProvider {
public:
 
  JavaJITCompiler* Comp;

  JnjvmModuleProvider(llvm::Module* M, JavaJITCompiler* C);
  ~JnjvmModuleProvider();

  bool materializeFunction(llvm::Function *F, std::string *ErrInfo = 0);

  llvm::Module* materializeModule(std::string *ErrInfo = 0) { 
    return TheModule;
  }
};

} // End jnjvm namespace

#endif
