//===------------- JavaTypes.cpp - Java primitives ------------------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <vector>

#include "JavaArray.h"
#include "JavaClass.h"
#include "JavaCompiler.h"
#include "JavaTypes.h"

using namespace jnjvm;

void PrimitiveTypedef::tPrintBuf(mvm::PrintBuffer* buf) const {
  prim->name->printUTF8(buf);
}

void ArrayTypedef::tPrintBuf(mvm::PrintBuffer* buf) const {
  UserCommonClass::printClassName(keyName, buf);
}

void ObjectTypedef::tPrintBuf(mvm::PrintBuffer* buf) const {
  UserCommonClass::printClassName(pseudoAssocClassName, buf);
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

void Signdef::humanPrintArgs(mvm::PrintBuffer* buf) const {
  buf->write("(");
  Typedef* const* arguments = getArgumentsType();
  for (uint32 i = 0; i < nbArguments; i++) {
    arguments[i]->tPrintBuf(buf);
    if (i != nbArguments - 1) {
      buf->write(", ");
    }
  }
  buf->write(")");
}

const char* Signdef::printString(const char* ext) const {
  mvm::PrintBuffer *buf= mvm::PrintBuffer::alloc();
  buf->write("Signature<");
  getReturnType()->tPrintBuf(buf);
  buf->write("...");
  humanPrintArgs(buf);
  buf->write(ext);
  buf->write(">");
  return buf->contents()->cString();
}

void Signdef::printWithSign(UserCommonClass* cl, const UTF8* name,
                            mvm::PrintBuffer* buf) const {
  getReturnType()->tPrintBuf(buf);
  buf->write(" ");
  UserCommonClass::printClassName(cl->name, buf);
  buf->write("::");
  name->printUTF8(buf);
  humanPrintArgs(buf);
}

Signdef::Signdef(const UTF8* name, JnjvmClassLoader* loader,
                 std::vector<Typedef*>& args, Typedef* ret) {
  
  arguments[0] = ret;
  Typedef** myArgs = &(arguments[1]);
  nbArguments = args.size();
  uint32 index = 0;
  for (std::vector<Typedef*>::iterator i = args.begin(), e = args.end();
       i != e; ++i) {
    myArgs[index++] = *i;
  }
  initialLoader = loader;
  keyName = name;
  JInfo = 0;
  _virtualCallBuf = 0;
  _staticCallBuf = 0;
  _virtualCallAP = 0;
  _staticCallAP = 0;
  
}

ObjectTypedef::ObjectTypedef(const UTF8* name, UTF8Map* map) {
  keyName = name;
  pseudoAssocClassName = name->extract(map, 1, name->size - 1);
}

intptr_t Signdef::staticCallBuf() {
  if (!_staticCallBuf) {
    initialLoader->getCompiler()->staticCallBuf(this);
  }
  return _staticCallBuf;
}

intptr_t Signdef::virtualCallBuf() {
  if (!_virtualCallBuf) {
    initialLoader->getCompiler()->virtualCallBuf(this);
  }
  return _virtualCallBuf;
}

intptr_t Signdef::staticCallAP() {
  if (!_staticCallAP) {
    initialLoader->getCompiler()->staticCallAP(this);
  }
  return _staticCallAP;
}

intptr_t Signdef::virtualCallAP() {
  if (!_virtualCallAP) {
    initialLoader->getCompiler()->virtualCallAP(this);
  }
  return _virtualCallAP;
}


void Signdef::nativeName(char* ptr, const char* ext) const {
  sint32 i = 0;
  while (i < keyName->size) {
    char c = keyName->elements[i++];
    if (c == I_PARG) {
      ptr[0] = '_';
      ptr[1] = '_';
      ptr += 2;
    } else if (c == '/') {
      ptr[0] = '_';
      ++ptr;
    } else if (c == '_') {
      ptr[0] = '_';
      ptr[1] = '1';
      ptr += 2;
    } else if (c == I_END_REF) {
      ptr[0] = '_';
      ptr[1] = '2';
      ptr += 2;
    } else if (c == I_TAB) {
      ptr[0] = '_';
      ptr[1] = '3';
      ptr += 2;
    } else if (c == I_PARD) {
      break;
    } else {
      ptr[0] = c;
      ++ptr;
    }
  }

  assert(ext && "I need an extension");
  memcpy(ptr, ext, strlen(ext) + 1);
}
