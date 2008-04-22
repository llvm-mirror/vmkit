//===--- JnjvmModuleProvider.cpp - LLVM Module Provider for Jnjvm ---------===//
//
//                              Jnjvm
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <llvm/Module.h>
#include <llvm/ModuleProvider.h>

#include "mvm/JIT.h"

#include "JnjvmModuleProvider.h"

#include "JavaClass.h"
#include "JavaConstantPool.h"
#include "JavaJIT.h"
#include "JavaThread.h"
#include "Jnjvm.h"

using namespace llvm;
using namespace jnjvm;
#include <iostream>

static JavaMethod* staticLookup(Class* caller, uint32 index) { 
  JavaCtpInfo* ctpInfo = caller->ctpInfo;
  

  bool isStatic = ctpInfo->isAStaticCall(index);

  CommonClass* cl = 0;
  const UTF8* utf8 = 0;
  Signdef* sign = 0;

  ctpInfo->resolveInterfaceOrMethod(index, cl, utf8, sign);
  
  JavaMethod* meth = cl->lookupMethod(utf8, sign->keyName, isStatic, true);
  
  meth->compiledPtr();

  ctpInfo->ctpRes[index] = meth->llvmFunction;

  return meth;
}


bool JnjvmModuleProvider::materializeFunction(Function *F, 
                                              std::string *ErrInfo) {
  
  if (mvm::jit::executionEngine->getPointerToGlobalIfAvailable(F))
    return false;

  if (!(F->hasNotBeenReadFromBitcode())) 
    return false;
  
  JavaMethod* meth = functionDefs->lookup(F);
  
  if (!meth) {
    // It's a callback
    std::pair<Class*, uint32> * p = functions->lookup(F);
    meth = staticLookup(p->first, p->second); 
  }
  
  void* val = meth->compiledPtr();
  if (F->isDeclaration())
    mvm::jit::executionEngine->updateGlobalMapping(F, val);
  return false;
}

