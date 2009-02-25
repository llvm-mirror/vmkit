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

using namespace llvm;

namespace llvm {
class FunctionPassManager;
}

namespace jnjvm {

class JnjvmModule;

class JnjvmModuleProvider : public ModuleProvider {
private:
  JavaMethod* staticLookup(Class* caller, uint32 index);
  
  llvm::FunctionPassManager* JavaFunctionPasses;
  llvm::FunctionPassManager* JavaNativeFunctionPasses;
  
  uint32 nbCallbacks;

public:
  
  JnjvmModuleProvider(JnjvmModule *m);
  ~JnjvmModuleProvider();
  
  llvm::Function* addCallback(Class* cl, uint32 index, Signdef* sign,
                              bool stat);

  bool materializeFunction(Function *F, std::string *ErrInfo = 0);
  void* materializeFunction(JavaMethod* meth);
  llvm::Function* parseFunction(JavaMethod* meth);

  Module* materializeModule(std::string *ErrInfo = 0) { return TheModule; }

  void printStats();
};

} // End jnjvm namespace

#endif
