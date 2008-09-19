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

const char AssessorDesc::I_TAB = '[';
const char AssessorDesc::I_END_REF = ';';
const char AssessorDesc::I_PARG = '(';
const char AssessorDesc::I_PARD = ')';
const char AssessorDesc::I_BYTE = 'B';
const char AssessorDesc::I_CHAR = 'C';
const char AssessorDesc::I_DOUBLE = 'D';
const char AssessorDesc::I_FLOAT = 'F';
const char AssessorDesc::I_INT = 'I';
const char AssessorDesc::I_LONG = 'J';
const char AssessorDesc::I_REF = 'L';
const char AssessorDesc::I_SHORT = 'S';
const char AssessorDesc::I_VOID = 'V';
const char AssessorDesc::I_BOOL = 'Z';
const char AssessorDesc::I_SEP = '/';

static void typeError(const UTF8* name, short int l) {
  if (l != 0) {
    JavaThread::get()->isolate->
      unknownError("wrong type %d in %s", l, name->printString());
  } else {
    JavaThread::get()->isolate->
      unknownError("wrong type %s", name->printString());
  }
}


bool AssessorDesc::analyseIntern(const UTF8* name, uint32 pos,
                                 uint32 meth,
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

const UTF8* AssessorDesc::constructArrayName(JnjvmClassLoader *loader,
                                             uint32 steps,
                                             const UTF8* className) {
  uint32 len = className->size;
  uint32 pos = steps;
  bool isTab = (className->elements[0] == I_TAB ? true : false);
  uint32 n = steps + len + (isTab ? 0 : 2);
  uint16* buf = (uint16*)alloca(n * sizeof(uint16));
    
  for (uint32 i = 0; i < steps; i++) {
    buf[i] = I_TAB;
  }

  if (!isTab) {
    ++pos;
    buf[steps] = I_REF;
  }

  for (uint32 i = 0; i < len; i++) {
    buf[pos + i] = className->elements[i];
  }

  if (!isTab) {
    buf[n - 1] = I_END_REF;
  }

  return loader->readerConstructUTF8(buf, n);
}

uint8 AssessorDesc::arrayType(unsigned int t) {
  if (t == JavaArray::T_CHAR) {
    return I_CHAR;
  } else if (t == JavaArray::T_BOOLEAN) {
    return I_BOOL;
  } else if (t == JavaArray::T_INT) {
    return I_INT;
  } else if (t == JavaArray::T_SHORT) {
    return I_SHORT;
  } else if (t == JavaArray::T_BYTE) {
    return I_BYTE;
  } else if (t == JavaArray::T_FLOAT) {
    return I_FLOAT;
  } else if (t == JavaArray::T_LONG) {
    return I_LONG;
  } else if (t == JavaArray::T_DOUBLE) {
    return I_DOUBLE;
  } else {
    JavaThread::get()->isolate->unknownError("unknown array type %d\n", t);
    return 0;
  }
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

UserClassPrimitive* 
AssessorDesc::byteIdToPrimitive(char id, Classpath* upcalls) {
  switch (id) {
    case I_FLOAT :
      return upcalls->OfFloat;
    case I_INT :
      return upcalls->OfInt;
    case I_SHORT :
      return upcalls->OfShort;
    case I_CHAR :
      return upcalls->OfChar;
    case I_DOUBLE :
      return upcalls->OfDouble;
    case I_BYTE :
      return upcalls->OfByte;
    case I_BOOL :
      return upcalls->OfBool;
    case I_LONG :
      return upcalls->OfLong;
    case I_VOID :
      return upcalls->OfVoid;
    default :
      return 0;
  }
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
    bool end = AssessorDesc::analyseIntern(name, pos, 0, pos);
    if (end) break;
    else {
      buf.push_back(loader->constructType(name->extract(loader->hashUTF8, pred, pos)));
    } 
  }
  
  if (pos == len) {
    typeError(name, 0);
  }
  
  AssessorDesc::analyseIntern(name, pos, 0, pred);

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

Typedef* Typedef::constructType(const UTF8* name, UTF8Map* map, Jnjvm* vm) {
  short int cur = name->elements[0];
  Typedef* res = 0;
  switch (cur) {
    case AssessorDesc::I_TAB :
      res = new ArrayTypedef(name);
      break;
    case AssessorDesc::I_REF :
      res = new ObjectTypedef(name, map);
      break;
    default :
      UserClassPrimitive* cl = vm->getPrimitiveClass((char)name->elements[0]);
      assert(cl && "No primitive");
      bool unsign = (cl == vm->upcalls->OfChar || cl == vm->upcalls->OfBool);
      res = new PrimitiveTypedef(name, cl, unsign, cur);
  }
  return res;
}

ObjectTypedef::ObjectTypedef(const UTF8* name, UTF8Map* map) {
  keyName = name;
  pseudoAssocClassName = name->extract(map, 1, name->size - 1);
}

intptr_t Signdef::staticCallBuf() {
  if (!_staticCallBuf) {
    LLVMSignatureInfo* LSI = initialLoader->TheModule->getSignatureInfo(this);
    LSI->getStaticBuf();
  }
  return _staticCallBuf;
}

intptr_t Signdef::virtualCallBuf() {
  if (!_virtualCallBuf) {
    LLVMSignatureInfo* LSI = initialLoader->TheModule->getSignatureInfo(this);
    LSI->getVirtualBuf();
  }
  return _virtualCallBuf;
}

intptr_t Signdef::staticCallAP() {
  if (!_staticCallAP) {
    LLVMSignatureInfo* LSI = initialLoader->TheModule->getSignatureInfo(this);
    LSI->getStaticAP();
  }
  return _staticCallAP;
}

intptr_t Signdef::virtualCallAP() {
  if (!_virtualCallAP) {
    LLVMSignatureInfo* LSI = initialLoader->TheModule->getSignatureInfo(this);
    LSI->getVirtualAP();
  }
  return _virtualCallAP;
}
