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

AssessorDesc* AssessorDesc::dParg = 0;
AssessorDesc* AssessorDesc::dPard = 0;
AssessorDesc* AssessorDesc::dVoid = 0;
AssessorDesc* AssessorDesc::dBool = 0;
AssessorDesc* AssessorDesc::dByte = 0;
AssessorDesc* AssessorDesc::dChar = 0;
AssessorDesc* AssessorDesc::dShort = 0;
AssessorDesc* AssessorDesc::dInt = 0;
AssessorDesc* AssessorDesc::dFloat = 0;
AssessorDesc* AssessorDesc::dLong = 0;
AssessorDesc* AssessorDesc::dDouble = 0;
AssessorDesc* AssessorDesc::dTab = 0;
AssessorDesc* AssessorDesc::dRef = 0;

AssessorDesc::AssessorDesc(bool dt, char bid, uint32 nb, uint32 nw,
                           const char* name,
                           Jnjvm* vm, uint8 nid,
                           const char* assocName, ClassArray* cl,
                           arrayCtor_t ctor) {
  AssessorDesc* res = this;
  res->numId = nid;
  res->doTrace = dt;
  res->byteId = bid;
  res->nbb = nb;
  res->nbw = nw;
  res->asciizName = name;
  res->UTF8Name = vm->asciizConstructUTF8(name);
  res->arrayCtor = ctor;
  
  res->arrayClass = cl;
  if (assocName)
    res->assocClassName = vm->asciizConstructUTF8(assocName);
  else
    res->assocClassName = 0;
  
  if (bid != I_PARG && bid != I_PARD && bid != I_REF && bid != I_TAB) {
    res->classType = new CommonClass(vm, res->UTF8Name, false);
    res->classType->status = ready;
    res->classType->isPrimitive = true;
    res->classType->access = ACC_ABSTRACT | ACC_FINAL | ACC_PUBLIC;
  } else {
    res->classType = 0;
  }
}

void AssessorDesc::initialise(Jnjvm* vm) {

  dParg = new AssessorDesc(false, I_PARG, 0, 0, "(", vm, -1, 0, 0,
                                 0);
  dPard = new AssessorDesc(false, I_PARD, 0, 0, ")", vm, -1, 0, 0,
                                 0);
  dVoid = new AssessorDesc(false, I_VOID, 0, 0, "void",
                                 vm, VOID_ID, "java/lang/Void", 0, 0);
  dBool = new AssessorDesc(false, I_BOOL, 1, 1, "boolean", 
                                 vm,
                                 BOOL_ID, "java/lang/Boolean", 
                                 JavaArray::ofBool,
                                 (arrayCtor_t)ArrayUInt8::acons);
  dByte = new AssessorDesc(false, I_BYTE, 1, 1, "byte",
                                 vm, BYTE_ID, "java/lang/Byte",
                                 JavaArray::ofByte,
                                 (arrayCtor_t)ArraySInt8::acons);
  dChar = new AssessorDesc(false, I_CHAR, 2, 1, "char",
                                 vm, CHAR_ID, "java/lang/Character",
                                 JavaArray::ofChar,
                                 (arrayCtor_t)ArrayUInt16::acons);
  dShort = new AssessorDesc(false, I_SHORT, 2, 1, "short", 
                                  vm, SHORT_ID,
                                  "java/lang/Short",
                                  JavaArray::ofShort,
                                  (arrayCtor_t)ArraySInt16::acons);
  dInt = new AssessorDesc(false, I_INT, 4, 1, "int", vm,
                                INT_ID, "java/lang/Integer",
                                JavaArray::ofInt,
                                (arrayCtor_t)ArraySInt32::acons);
  dFloat = new AssessorDesc(false, I_FLOAT, 4, 1, "float", 
                                  vm,
                                  FLOAT_ID, "java/lang/Float",
                                  JavaArray::ofFloat,
                                  (arrayCtor_t)ArrayFloat::acons);
  dLong = new AssessorDesc(false, I_LONG, 8, 2, "long", 
                                 vm, LONG_ID, "java/lang/Long",
                                 JavaArray::ofLong,
                                  (arrayCtor_t)ArrayLong::acons);
  dDouble = new AssessorDesc(false, I_DOUBLE, 8, 2, "double", 
                                   vm,
                                   DOUBLE_ID, "java/lang/Double",
                                   JavaArray::ofDouble,
                                   (arrayCtor_t)ArrayDouble::acons);
  dTab = new AssessorDesc(true, I_TAB, sizeof(void*), 1, "array",
                                vm, ARRAY_ID, 0, 0,
                                (arrayCtor_t)ArrayObject::acons);
  dRef = new AssessorDesc(true, I_REF, sizeof(void*), 1, "reference",
                                vm, OBJECT_ID,
                                0, 0, 
                                (arrayCtor_t)ArrayObject::acons);
  
}

const char* AssessorDesc::printString() const {
  mvm::PrintBuffer *buf= mvm::PrintBuffer::alloc();
  buf->write("AssessorDescriptor<");
  buf->write(asciizName);
  buf->write(">");
  return buf->contents()->cString();
}

static void typeError(const UTF8* name, short int l) {
  if (l != 0) {
    JavaThread::get()->isolate->
      unknownError("wrong type %d in %s", l, name->printString());
  } else {
    JavaThread::get()->isolate->
      unknownError("wrong type %s", name->printString());
  }
}


void AssessorDesc::analyseIntern(const UTF8* name, uint32 pos,
                                 uint32 meth, AssessorDesc*& ass,
                                 uint32& ret) {
  short int cur = name->elements[pos];
  switch (cur) {
    case I_PARG :
      ass = dParg;
      ret = pos + 1;
      break;
    case I_PARD :
      ass = dPard;
      ret = pos + 1;
      break;
    case I_BOOL :
      ass = dBool;
      ret = pos + 1;
      break;
    case I_BYTE :
      ass = dByte;
      ret = pos + 1;
      break;
    case I_CHAR :
      ass = dChar;
      ret = pos + 1;
      break;
    case I_SHORT :
      ass = dShort;
      ret = pos + 1;
      break;
    case I_INT :
      ass = dInt;
      ret = pos + 1;
      break;
    case I_FLOAT :
      ass = dFloat;
      ret = pos + 1;
      break;
    case I_DOUBLE :
      ass = dDouble;
      ret = pos + 1;
      break;
    case I_LONG :
      ass = dLong;
      ret = pos + 1;
      break;
    case I_VOID :
      ass = dVoid;
      ret = pos + 1;
      break;
    case I_TAB :
      if (meth == 1) {
        pos++;
      } else {
        AssessorDesc * typeIntern = dTab;
        while (name->elements[++pos] == I_TAB) {}
        analyseIntern(name, pos, 1, typeIntern, pos);
      }
      ass = dTab;
      ret = pos;
      break;
    case I_REF :
      if (meth != 2) {
        while (name->elements[++pos] != I_END_REF) {}
      }
      ass = dRef;
      ret = pos + 1;
      break;
    default :
      typeError(name, cur);
  }
}

const UTF8* AssessorDesc::constructArrayName(Jnjvm *vm, AssessorDesc* ass,
                                             uint32 steps,
                                             const UTF8* className) {
  if (ass) {
    uint16* buf = (uint16*)alloca((steps + 1) * sizeof(uint16));
    for (uint32 i = 0; i < steps; i++) {
      buf[i] = I_TAB;
    }
    buf[steps] = ass->byteId;
    return UTF8::readerConstruct(vm, buf, steps + 1);
  } else {
    uint32 len = className->size;
    uint32 pos = steps;
    AssessorDesc * funcs = (className->elements[0] == I_TAB ? dTab : dRef);
    uint32 n = steps + len + (funcs == dRef ? 2 : 0);
    uint16* buf = (uint16*)alloca(n * sizeof(uint16));
    
    for (uint32 i = 0; i < steps; i++) {
      buf[i] = I_TAB;
    }

    if (funcs == dRef) {
      ++pos;
      buf[steps] = funcs->byteId;
    }

    for (uint32 i = 0; i < len; i++) {
      buf[pos + i] = className->elements[i];
    }

    if (funcs == dRef) {
      buf[n - 1] = I_END_REF;
    }

    return UTF8::readerConstruct(vm, buf, n);
  }
}

void AssessorDesc::introspectArrayName(Jnjvm *vm, const UTF8* utf8,
                                       uint32 start, AssessorDesc*& ass,
                                       const UTF8*& res) {
  uint32 pos = 0;
  uint32 intern = 0;
  AssessorDesc* funcs = 0;

  analyseIntern(utf8, start, 1, funcs, intern);

  if (funcs != dTab) {
    vm->unknownError("%s isn't an array", utf8->printString());
  }

  analyseIntern(utf8, intern, 0, funcs, pos);

  if (funcs == dRef) {
    ass = dRef;
    res = utf8->extract(vm, intern + 1, pos - 1);
  } else if (funcs == dTab) {
    ass = dTab;
    res = utf8->extract(vm, intern, pos);
  } else {
    ass = funcs;
    res = 0;
  }
}

void AssessorDesc::introspectArray(Jnjvm *vm, JavaObject* loader,
                                   const UTF8* utf8, uint32 start,
                                   AssessorDesc*& ass, CommonClass*& res) {
  uint32 pos = 0;
  uint32 intern = 0;
  AssessorDesc* funcs = 0;

  analyseIntern(utf8, start, 1, funcs, intern);

  if (funcs != dTab) {
    vm->unknownError("%s isn't an array", utf8->printString());
  }

  analyseIntern(utf8, intern, 0, funcs, pos);

  if (funcs == dRef) {
    ass = dRef;
    res = vm->loadName(utf8->extract(vm, intern + 1, pos - 1), loader, false,
                       false, true);
  } else if (funcs == dTab) {
    ass = dTab;
    res = vm->constructArray(utf8->extract(vm, intern, pos), loader);
  } else {
    ass = funcs;
    res = funcs->classType;
  }
}

AssessorDesc* AssessorDesc::arrayType(unsigned int t) {
  if (t == JavaArray::T_CHAR) {
    return AssessorDesc::dChar;
  } else if (t == JavaArray::T_BOOLEAN) {
    return AssessorDesc::dBool;
  } else if (t == JavaArray::T_INT) {
    return AssessorDesc::dInt;
  } else if (t == JavaArray::T_SHORT) {
    return AssessorDesc::dShort;
  } else if (t == JavaArray::T_BYTE) {
    return AssessorDesc::dByte;
  } else if (t == JavaArray::T_FLOAT) {
    return AssessorDesc::dFloat;
  } else if (t == JavaArray::T_LONG) {
    return AssessorDesc::dLong;
  } else if (t == JavaArray::T_DOUBLE) {
    return AssessorDesc::dDouble;
  } else {
    JavaThread::get()->isolate->unknownError("unknown array type %d\n", t);
    return 0;
  }
}

void Typedef::tPrintBuf(mvm::PrintBuffer* buf) const {
  if (pseudoAssocClassName == 0)
    buf->write(funcs->asciizName);
  else
    CommonClass::printClassName(pseudoAssocClassName, buf);
}

AssessorDesc* AssessorDesc::byteIdToPrimitive(char id) {
  switch (id) {
    case I_FLOAT :
      return dFloat;
    case I_INT :
      return dInt;
    case I_SHORT :
      return dShort;
    case I_CHAR :
      return dChar;
    case I_DOUBLE :
      return dDouble;
    case I_BYTE :
      return dByte;
    case I_BOOL :
      return dBool;
    case I_LONG :
      return dLong;
    case I_VOID :
      return dVoid;
    default :
      return 0;
  }
}

AssessorDesc* AssessorDesc::classToPrimitive(CommonClass* cl) {
  const UTF8* name = cl->name;
  if (name == dFloat->assocClassName) {
    return dFloat;
  } else if (name == dInt->assocClassName) {
    return dInt;
  } else if (name == dShort->assocClassName) {
    return dShort;
  } else if (name == dChar->assocClassName) {
    return dChar;
  } else if (name == dDouble->assocClassName) {
    return dDouble;
  } else if (name == dByte->assocClassName) {
    return dByte;
  } else if (name == dBool->assocClassName) {
    return dBool;
  } else if (name == dLong->assocClassName) {
    return dLong;
  } else if (name == dVoid->assocClassName) {
    return dVoid;
  } else {
    return 0;
  }
}

AssessorDesc* AssessorDesc::bogusClassToPrimitive(CommonClass* cl) {
  if (cl == dFloat->classType) {
    return dFloat;
  } else if (cl == dInt->classType) {
    return dInt;
  } else if (cl == dShort->classType) {
    return dShort;
  } else if (cl == dChar->classType) {
    return dChar;
  } else if (cl == dDouble->classType) {
    return dDouble;
  } else if (cl == dByte->classType) {
    return dByte;
  } else if (cl == dBool->classType) {
    return dBool;
  } else if (cl == dLong->classType) {
    return dLong;
  } else if (cl == dVoid->classType) {
    return dVoid;
  } else {
    return 0;
  }
}

void Typedef::print(mvm::PrintBuffer* buf) const {
  buf->write("Type<");
  tPrintBuf(buf);
  buf->write(">");
}

CommonClass* Typedef::assocClass(JavaObject* loader) {
  if (pseudoAssocClassName == 0) {
    return funcs->classType;
  } else if (funcs == AssessorDesc::dRef) {
    return isolate->loadName(pseudoAssocClassName, loader, false, true, true);
  } else {
    return isolate->constructArray(pseudoAssocClassName, loader);
  }
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

void Signdef::print(mvm::PrintBuffer* buf) const {
  buf->write("Signature<");
  ret->tPrintBuf(buf);
  buf->write("...");
  Typedef::humanPrintArgs(&args, buf);
  buf->write(">");
}

void Signdef::printWithSign(CommonClass* cl, const UTF8* name,
                            mvm::PrintBuffer* buf) {
  ret->tPrintBuf(buf);
  buf->write(" ");
  CommonClass::printClassName(cl->name, buf);
  buf->write("::");
  name->print(buf);
  humanPrintArgs(&args, buf);
}

unsigned int Signdef::nbInWithThis(unsigned int flag) {
  return args.size() + (isStatic(flag) ? 0 : 1);
}

Signdef* Signdef::signDup(const UTF8* name, Jnjvm *vm) {
  std::vector<Typedef*> buf;
  uint32 len = (uint32)name->size;
  uint32 pos = 1;
  uint32 pred = 0;
  AssessorDesc* funcs = 0;

  while (pos < len) {
    pred = pos;
    AssessorDesc::analyseIntern(name, pos, 0, funcs, pos);
    if (funcs == AssessorDesc::dPard) break;
    else {
      buf.push_back(vm->constructType(name->extract(vm, pred, pos)));
    } 
  }
  
  if (pos == len) {
    typeError(name, 0);
  }
  
  AssessorDesc::analyseIntern(name, pos, 0, funcs, pred);

  if (pred != len) {
    typeError(name, 0);
  }

  Signdef* res = vm_new(vm, Signdef)();
  res->args = buf;
  res->ret = vm->constructType(name->extract(vm, pos, pred));
  res->isolate = vm;
  res->keyName = name;
  res->pseudoAssocClassName = name;
  res->funcs = 0;
  res->nbIn = buf.size();
  res->_virtualCallBuf = 0;
  res->_staticCallBuf = 0;
  res->_virtualCallAP = 0;
  res->_staticCallAP = 0;
  return res;
  
}

Typedef* Typedef::typeDup(const UTF8* name, Jnjvm *vm) {
  AssessorDesc* funcs = 0;
  uint32 next;
  AssessorDesc::analyseIntern(name, 0, 0, funcs, next);

  if (funcs == AssessorDesc::dParg) {
    return Signdef::signDup(name, vm);
  } else {
    Typedef* res = vm_new(vm, Typedef)();
    res->isolate = vm;
    res->keyName = name;
    res->funcs = funcs;
    if (funcs == AssessorDesc::dRef) {
      res->pseudoAssocClassName = name->extract(vm, 1, next - 1);
    } else if (funcs == AssessorDesc::dTab) {
      res->pseudoAssocClassName = name;
    }
    return res;
  }

}

void* Signdef::staticCallBuf() {
  if (!_staticCallBuf) {
    LLVMSignatureInfo* LSI = isolate->TheModule->getSignatureInfo(this);
    LSI->getStaticBuf();
  }
  return _staticCallBuf;
}

void* Signdef::virtualCallBuf() {
  if (!_virtualCallBuf) {
    LLVMSignatureInfo* LSI = isolate->TheModule->getSignatureInfo(this);
    LSI->getVirtualBuf();
  }
  return _virtualCallBuf;
}

void* Signdef::staticCallAP() {
  if (!_staticCallAP) {
    LLVMSignatureInfo* LSI = isolate->TheModule->getSignatureInfo(this);
    LSI->getStaticAP();
  }
  return _staticCallAP;
}

void* Signdef::virtualCallAP() {
  if (!_virtualCallAP) {
    LLVMSignatureInfo* LSI = isolate->TheModule->getSignatureInfo(this);
    LSI->getVirtualAP();
  }
  return _virtualCallAP;
}
