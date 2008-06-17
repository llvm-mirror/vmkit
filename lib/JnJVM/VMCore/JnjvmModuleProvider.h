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
  
  std::map<llvm::Function*, JavaMethod*> functions;
  std::map<llvm::Function*, std::pair<Class*, uint32> > callbacks;
  
  bool lookupCallback(llvm::Function*, std::pair<Class*, uint32>&);
  bool lookupFunction(llvm::Function*, JavaMethod*& meth);

  typedef std::map<llvm::Function*, JavaMethod*>::iterator
    function_iterator;  
  
  typedef std::map<llvm::Function*, std::pair<Class*, uint32> >::iterator
    callback_iterator;  

  llvm::FunctionPassManager* perFunctionPasses;

public:
  
  JnjvmModuleProvider(JnjvmModule *m);
  ~JnjvmModuleProvider();
  
  llvm::Function* addCallback(Class* cl, uint32 index, Signdef* sign,
                              bool stat);
  void addFunction(llvm::Function* F, JavaMethod* meth);

  bool materializeFunction(Function *F, std::string *ErrInfo = 0);
  void* materializeFunction(JavaMethod* meth);
  llvm::Function* parseFunction(JavaMethod* meth);

  Module* materializeModule(std::string *ErrInfo = 0) { return TheModule; }
};

} // End jnjvm namespace

#endif
