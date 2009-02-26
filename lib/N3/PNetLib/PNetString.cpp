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


CLIString* CLIString::stringDup(const UTF8*& utf8, N3* vm) {
  PNetString* obj = (PNetString*)(*MSCorlib::pString)();
  obj->capacity = utf8->size;
  obj->length = utf8->size;
  if (utf8->size == 0) {
    obj->firstChar = 0;
  } else {
    obj->firstChar = utf8->at(0);
  }
  obj->value = utf8; 
  return obj;
}

char* CLIString::strToAsciiz() {
  return ((PNetString*)this)->value->UTF8ToAsciiz();
}

const UTF8* CLIString::strToUTF8(N3* vm) {
  return ((PNetString*)this)->value;
}

GlobalVariable* CLIString::llvmVar() {
  PNetString* str = (PNetString*)this;
  if (!str->_llvmVar) {
    VirtualMachine* vm = VMThread::get()->vm;
    if (!str->_llvmVar) {
      const Type* pty = mvm::MvmModule::ptrType;
      Constant* cons = 
        ConstantExpr::getIntToPtr(ConstantInt::get(Type::Int64Ty, uint64_t (this)),
                                  pty);
      str->_llvmVar = new GlobalVariable(pty, true,
                                    GlobalValue::ExternalLinkage,
                                    cons, "",
                                    vm->module->getLLVMModule());
    }
  }
  return str->_llvmVar;
}
