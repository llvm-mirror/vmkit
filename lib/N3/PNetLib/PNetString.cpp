//===--- PNetString.cpp - Implementation of PNet string interface ---------===//
//
//                                N3
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/DerivedTypes.h"
#include "llvm/GlobalVariable.h"

#include "mvm/JIT.h"

#include "CLIString.h"
#include "MSCorlib.h"
#include "N3.h"
#include "PNetString.h"
#include "VMArray.h"
#include "VMClass.h"
#include "VMThread.h"

using namespace n3;
using namespace llvm;


CLIString* CLIString::stringDup(const ArrayChar* array, N3* vm) {
	llvm_gcroot(array, 0);
  declare_gcroot(PNetString*, obj) = (PNetString*)MSCorlib::pString->doNew();
  obj->capacity = array->size;
  obj->length = array->size;
  if (array->size == 0) {
    obj->firstChar = 0;
  } else {
    obj->firstChar = array->elements[0];
  }
  obj->value = array; 
  return obj;
}

GlobalVariable* CLIString::llvmVar(CLIString *self) {
	llvm_gcroot(self, 0);
  declare_gcroot(PNetString*, str) = (PNetString*)self;
  if (!str->_llvmVar) {
    N3* vm = VMThread::get()->getVM();
    if (!str->_llvmVar) {
      const Type* pty = mvm::MvmModule::ptrType;
      Constant* cons = 
        ConstantExpr::getIntToPtr(ConstantInt::get(Type::getInt64Ty(getGlobalContext()), uint64_t (self)),
                                  pty);
      str->_llvmVar = new GlobalVariable(*(vm->getLLVMModule()), pty, true,
                                    GlobalValue::ExternalLinkage,
                                    cons, "");
    }
  }
  return str->_llvmVar;
}
