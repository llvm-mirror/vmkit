//===------------- JavaTypes.cpp - Java primitives ------------------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <vector>

#include "mvm/JIT.h"

#include "JavaAccess.h"
#include "JavaArray.h"
#include "JavaClass.h"
#include "JavaConstantPool.h"
#include "JavaJIT.h"
#include "JavaThread.h"
#include "JavaTypes.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"

using namespace jnjvm;



static void typeError(const UTF8* name, short int l) {
  if (l != 0) {
    JavaThread::get()->isolate->
      unknownError("wrong type %d in %s", l, name->printString());
  } else {
    JavaThread::get()->isolate->
      unknownError("wrong type %s", name->printString());
  }
}


static bool analyseIntern(const UTF8* name, uint32 pos, uint32 meth,
                          uint32& ret) {
  short int cur = name->elements[pos];
  switch (cur) {
    case I_PARD :
      ret = pos + 1;
      return true;
    case I_BOOL :
      ret = pos + 1;
      return false;
    case I_BYTE :
      ret = pos + 1;
      return false;
    case I_CHAR :
      ret = pos + 1;
      return false;
    case I_SHORT :
      ret = pos + 1;
      return false;
    case I_INT :
      ret = pos + 1;
      return false;
    case I_FLOAT :
      ret = pos + 1;
      return false;
    case I_DOUBLE :
      ret = pos + 1;
      return false;
    case I_LONG :
      ret = pos + 1;
      return false;
    case I_VOID :
      ret = pos + 1;
      return false;
    case I_TAB :
      if (meth == 1) {
        pos++;
      } else {
        while (name->elements[++pos] == I_TAB) {}
        analyseIntern(name, pos, 1, pos);
      }
      ret = pos;
      return false;
    case I_REF :
      if (meth != 2) {
        while (name->elements[++pos] != I_END_REF) {}
      }
      ret = pos + 1;
      return false;
    default :
      typeError(name, cur);
  }
  return false;
}

void PrimitiveTypedef::tPrintBuf(mvm::PrintBuffer* buf) const {
  prim->name->print(buf);
}

void ArrayTypedef::tPrintBuf(mvm::PrintBuffer* buf) const {
  CommonClass::printClassName(keyName, buf);
}

void ObjectTypedef::tPrintBuf(mvm::PrintBuffer* buf) const {
  CommonClass::printClassName(pseudoAssocClassName, buf);
}

const char* Typedef::printString() const {
  mvm::PrintBuffer *buf= mvm::PrintBuffer::alloc();
  buf->write("Type<");
  tPrintBuf(buf);
  buf->write(">");
  return buf->contents()->cString();
}

UserCommonClass* ArrayTypedef::assocClass(JnjvmClassLoader* loader) const {
  return loader->constructArray(keyName);
}

UserCommonClass* ObjectTypedef::assocClass(JnjvmClassLoader* loader) const {
  return loader->loadName(pseudoAssocClassName, false, true);
}

void Typedef::humanPrintArgs(const std::vector<Typedef*>* args,
                             mvm::PrintBuffer* buf) {
  buf->write("(");
  for (uint32 i = 0; i < args->size(); i++) {
    args->at(i)->tPrintBuf(buf);
    if (i != args->size() - 1) {
      buf->write(", ");
    }
  }
  buf->write(")");
}

const char* Signdef::printString() const {
  mvm::PrintBuffer *buf= mvm::PrintBuffer::alloc();
  buf->write("Signature<");
  ret->tPrintBuf(buf);
  buf->write("...");
  Typedef::humanPrintArgs(&args, buf);
  buf->write(">");
  return buf->contents()->cString();
}

void Signdef::printWithSign(CommonClass* cl, const UTF8* name,
                            mvm::PrintBuffer* buf) {
  ret->tPrintBuf(buf);
  buf->write(" ");
  CommonClass::printClassName(cl->name, buf);
  buf->write("::");
  name->print(buf);
  Typedef::humanPrintArgs(&args, buf);
}

Signdef::Signdef(const UTF8* name, JnjvmClassLoader* loader) {
  Signdef* res = this;
  std::vector<Typedef*> buf;
  uint32 len = (uint32)name->size;
  uint32 pos = 1;
  uint32 pred = 0;

  while (pos < len) {
    pred = pos;
    bool end = analyseIntern(name, pos, 0, pos);
    if (end) break;
    else {
      buf.push_back(loader->constructType(name->extract(loader->hashUTF8, pred, pos)));
    } 
  }
  
  if (pos == len) {
    typeError(name, 0);
  }
  
  analyseIntern(name, pos, 0, pred);

  if (pred != len) {
    typeError(name, 0);
  }

  res->args = buf;
  res->ret = loader->constructType(name->extract(loader->hashUTF8, pos, pred));
  res->initialLoader = loader;
  res->keyName = name;
  res->_virtualCallBuf = 0;
  res->_staticCallBuf = 0;
  res->_virtualCallAP = 0;
  res->_staticCallAP = 0;
  res->JInfo = 0;
  
}

ObjectTypedef::ObjectTypedef(const UTF8* name, UTF8Map* map) {
  keyName = name;
  pseudoAssocClassName = name->extract(map, 1, name->size - 1);
}

intptr_t Signdef::staticCallBuf() {
  if (!_staticCallBuf) {
    LLVMSignatureInfo* LSI = initialLoader->getModule()->getSignatureInfo(this);
    LSI->getStaticBuf();
  }
  return _staticCallBuf;
}

intptr_t Signdef::virtualCallBuf() {
  if (!_virtualCallBuf) {
    LLVMSignatureInfo* LSI = initialLoader->getModule()->getSignatureInfo(this);
    LSI->getVirtualBuf();
  }
  return _virtualCallBuf;
}

intptr_t Signdef::staticCallAP() {
  if (!_staticCallAP) {
    LLVMSignatureInfo* LSI = initialLoader->getModule()->getSignatureInfo(this);
    LSI->getStaticAP();
  }
  return _staticCallAP;
}

intptr_t Signdef::virtualCallAP() {
  if (!_virtualCallAP) {
    LLVMSignatureInfo* LSI = initialLoader->getModule()->getSignatureInfo(this);
    LSI->getVirtualAP();
  }
  return _virtualCallAP;
}
