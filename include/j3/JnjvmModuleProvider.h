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

#include <llvm/ModuleProvider.h>

namespace j3 {

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

} // End j3 namespace

#endif
