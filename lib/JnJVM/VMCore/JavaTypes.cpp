//===------------- JavaTypes.cpp - Java primitives ------------------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <vector>

#include "llvm/DerivedTypes.h"
#include "llvm/Function.h"

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

AssessorDesc* AssessorDesc::allocate(bool dt, char bid, uint32 nb, uint32 nw,
                                     const char* name, Jnjvm* vm,
                                     const llvm::Type* t,
                                     const char* assocName, arrayCtor_t ctor) {
  AssessorDesc* res = vm_new(vm, AssessorDesc)();
  res->doTrace = dt;
  res->byteId = bid;
  res->nbb = nb;
  res->nbw = nw;
  res->asciizName = name;
  res->llvmType = t;
  if (t && t != llvm::Type::VoidTy) {
    res->llvmTypePtr = llvm::PointerType::getUnqual(t);
    res->llvmNullConstant = llvm::Constant::getNullValue(t);
  }
  res->arrayCtor = ctor;
  if (assocName)
    res->assocClassName = vm->asciizConstructUTF8(assocName);
  else
    res->assocClassName = 0;
  
  if (bid != I_PARG && bid != I_PARD && bid != I_REF && bid != I_TAB) {
    res->classType = vm->constructClass(vm->asciizConstructUTF8(name),
                                        CommonClass::jnjvmClassLoader);
    res->classType->status = ready;
    res->classType->access = ACC_ABSTRACT | ACC_FINAL | ACC_PUBLIC;
  } else {
    res->classType = 0;
  }
  return res;
}

void AssessorDesc::initialise(Jnjvm* vm) {

  dParg = AssessorDesc::allocate(false, I_PARG, 0, 0, "(", vm, 0, 0, 0);
  dPard = AssessorDesc::allocate(false, I_PARD, 0, 0, ")", vm, 0, 0, 0);
  dVoid = AssessorDesc::allocate(false, I_VOID, 0, 0, "void", vm, 
                                 llvm::Type::VoidTy, "java/lang/Void", 0);
  dBool = AssessorDesc::allocate(false, I_BOOL, 1, 1, "boolean", vm,
                                 llvm::Type::Int8Ty, "java/lang/Boolean", 
                                 (arrayCtor_t)ArrayUInt8::acons);
  dByte = AssessorDesc::allocate(false, I_BYTE, 1, 1, "byte", vm,
                                 llvm::Type::Int8Ty, "java/lang/Byte",
                                 (arrayCtor_t)ArraySInt8::acons);
  dChar = AssessorDesc::allocate(false, I_CHAR, 2, 1, "char", vm,
                                 llvm::Type::Int16Ty, "java/lang/Character",
                                 (arrayCtor_t)ArrayUInt16::acons);
  dShort = AssessorDesc::allocate(false, I_SHORT, 2, 1, "short", vm,
                                  llvm::Type::Int16Ty, "java/lang/Short",
                                  (arrayCtor_t)ArraySInt16::acons);
  dInt = AssessorDesc::allocate(false, I_INT, 4, 1, "int", vm,
                                llvm::Type::Int32Ty, "java/lang/Integer",
                                (arrayCtor_t)ArraySInt32::acons);
  dFloat = AssessorDesc::allocate(false, I_FLOAT, 4, 1, "float", vm,
                                  llvm::Type::FloatTy, "java/lang/Float",
                                  (arrayCtor_t)ArrayFloat::acons);
  dLong = AssessorDesc::allocate(false, I_LONG, 8, 2, "long", vm,
                                 llvm::Type::Int64Ty, "java/lang/Long",
                                 (arrayCtor_t)ArrayLong::acons);
  dDouble = AssessorDesc::allocate(false, I_DOUBLE, 8, 2, "double", vm,
                                   llvm::Type::DoubleTy, "java/lang/Double",
                                   (arrayCtor_t)ArrayDouble::acons);
  dTab = AssessorDesc::allocate(true, I_TAB, 4, 1, "array", vm,
                                JavaObject::llvmType, 0,
                                (arrayCtor_t)ArrayObject::acons);
  dRef = AssessorDesc::allocate(true, I_REF, 4, 1, "reference", vm,
                                JavaObject::llvmType, 0,
                                (arrayCtor_t)ArrayObject::acons);
  
  mvm::Object::pushRoot((mvm::Object*)dParg);
  mvm::Object::pushRoot((mvm::Object*)dPard);
  mvm::Object::pushRoot((mvm::Object*)dVoid);
  mvm::Object::pushRoot((mvm::Object*)dBool);
  mvm::Object::pushRoot((mvm::Object*)dByte);
  mvm::Object::pushRoot((mvm::Object*)dChar);
  mvm::Object::pushRoot((mvm::Object*)dShort);
  mvm::Object::pushRoot((mvm::Object*)dInt);
  mvm::Object::pushRoot((mvm::Object*)dFloat);
  mvm::Object::pushRoot((mvm::Object*)dLong);
  mvm::Object::pushRoot((mvm::Object*)dDouble);
  mvm::Object::pushRoot((mvm::Object*)dTab);
  mvm::Object::pushRoot((mvm::Object*)dRef);
}

void AssessorDesc::print(mvm::PrintBuffer* buf) const {
  buf->write("AssessorDescriptor<");
  buf->write(asciizName);
  buf->write(">");
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
  short int cur = name->at(pos);
  if (cur == I_PARG) {
    ass = dParg;
    ret = pos + 1;
  } else if (cur == I_PARD) {
    ass = dPard;
    ret = pos + 1;
  } else if (cur == I_BOOL) {
    ass = dBool;
    ret = pos + 1;
  } else if (cur == I_BYTE) {
    ass = dByte;
    ret = pos + 1;
  } else if (cur == I_CHAR) {
    ass = dChar;
    ret = pos + 1;
  } else if (cur == I_SHORT) {
    ass = dShort;
    ret = pos + 1;
  } else if (cur == I_INT) {
    ass = dInt;
    ret = pos + 1;
  } else if (cur == I_FLOAT) {
    ass = dFloat;
    ret = pos + 1;
  } else if (cur == I_DOUBLE) {
    ass = dDouble;
    ret = pos + 1;
  } else if (cur == I_LONG) {
    ass = dLong;
    ret = pos + 1;
  } else if (cur == I_VOID) {
    ass = dVoid;
    ret = pos + 1;
  } else if (cur == I_TAB) {
    if (meth == 1) {
      pos++;
    } else {
      AssessorDesc * typeIntern = dTab;
      while (name->at(++pos) == I_TAB) {}
      analyseIntern(name, pos, 1, typeIntern, pos);
    }
    ass = dTab;
    ret = pos;
  } else if (cur == I_REF) {
    if (meth != 2) {
      while (name->at(++pos) != I_END_REF) {}
    }
    ass = dRef;
    ret = pos + 1;
  } else {
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
    AssessorDesc * funcs = (className->at(0) == I_TAB ? dTab : dRef);
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
      buf[pos + i] = className->at(i);
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

static void _arrayType(Jnjvm *vm, unsigned int t, AssessorDesc*& funcs,
                       llvm::Function*& ctor) {
  if (t == JavaArray::T_CHAR) {
    funcs = AssessorDesc::dChar;
    ctor = JavaJIT::UTF8AconsLLVM;
  } else if (t == JavaArray::T_BOOLEAN) {
    funcs = AssessorDesc::dBool;
    ctor = JavaJIT::Int8AconsLLVM;
  } else if (t == JavaArray::T_INT) {
    funcs = AssessorDesc::dInt;
    ctor = JavaJIT::Int32AconsLLVM;
  } else if (t == JavaArray::T_SHORT) {
    funcs = AssessorDesc::dShort;
    ctor = JavaJIT::Int16AconsLLVM;
  } else if (t == JavaArray::T_BYTE) {
    funcs = AssessorDesc::dByte;
    ctor = JavaJIT::Int8AconsLLVM;
  } else if (t == JavaArray::T_FLOAT) {
    funcs = AssessorDesc::dFloat;
    ctor = JavaJIT::FloatAconsLLVM;
  } else if (t == JavaArray::T_LONG) {
    funcs = AssessorDesc::dLong;
    ctor = JavaJIT::LongAconsLLVM;
  } else if (t == JavaArray::T_DOUBLE) {
    funcs = AssessorDesc::dDouble;
    ctor = JavaJIT::DoubleAconsLLVM;
  } else {
    vm->unknownError("unknown array type %d\n", t);
  }
}

void AssessorDesc::arrayType(Jnjvm *vm, JavaObject* loader, unsigned int t,
                             ClassArray*& cl, AssessorDesc*& ass, 
                             llvm::Function*& ctr) {
  _arrayType(vm, t, ass, ctr);
  cl = vm->constructArray(constructArrayName(vm, ass, 1, 0), loader);
  assert(cl);
}

void Typedef::tPrintBuf(mvm::PrintBuffer* buf) const {
  if (pseudoAssocClassName == 0)
    buf->write(funcs->asciizName);
  else
    CommonClass::printClassName(pseudoAssocClassName, buf);
}

AssessorDesc* AssessorDesc::byteIdToPrimitive(char id) {
  if (id == I_FLOAT) {
    return dFloat;
  } else if (id == I_INT) {
    return dInt;
  } else if (id == I_SHORT) {
    return dShort;
  } else if (id == I_CHAR) {
    return dChar;
  } else if (id == I_DOUBLE) {
    return dDouble;
  } else if (id == I_BYTE) {
    return dByte;
  } else if (id == I_BOOL) {
    return dBool;
  } else if (id == I_LONG) {
    return dLong;
  } else if (id == I_VOID) {
    return dVoid;
  } else {
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

const llvm::FunctionType* Signdef::createVirtualType(
            const std::vector<Typedef*>* args, Typedef* ret) {
  std::vector<const llvm::Type*> llvmArgs;
  unsigned int size = args->size();

  llvmArgs.push_back(JavaObject::llvmType);

  for (uint32 i = 0; i < size; ++i) {
    llvmArgs.push_back(args->at(i)->funcs->llvmType);
  }

#ifdef MULTIPLE_VM
  llvmArgs.push_back(mvm::jit::ptrType); // domain
#endif

  mvm::jit::protectTypes();//->lock();
  llvm::FunctionType* res =  llvm::FunctionType::get(ret->funcs->llvmType,
                                                     llvmArgs, false);
  mvm::jit::unprotectTypes();//->unlock();

  return res;

}

const llvm::FunctionType* Signdef::createStaticType(
            const std::vector<Typedef*>* args, Typedef* ret) {
  std::vector<const llvm::Type*> llvmArgs;
  unsigned int size = args->size();


  for (uint32 i = 0; i < size; ++i) {
    llvmArgs.push_back(args->at(i)->funcs->llvmType);
  }

#ifdef MULTIPLE_VM
  llvmArgs.push_back(mvm::jit::ptrType); // domain
#endif
  
  mvm::jit::protectTypes();//->lock();
  llvm::FunctionType* res =  llvm::FunctionType::get(ret->funcs->llvmType,
                                                     llvmArgs, false);
  mvm::jit::unprotectTypes();//->unlock();

  return res;
}

const llvm::FunctionType* Signdef::createNativeType(
            const std::vector<Typedef*>* args, Typedef* ret) {
  std::vector<const llvm::Type*> llvmArgs;
  unsigned int size = args->size();

  llvmArgs.push_back(mvm::jit::ptrType); // JNIEnv
  llvmArgs.push_back(JavaObject::llvmType); // Class

  for (uint32 i = 0; i < size; ++i) {
    llvmArgs.push_back(args->at(i)->funcs->llvmType);
  }

#ifdef MULTIPLE_VM
  llvmArgs.push_back(mvm::jit::ptrType); // domain
#endif
  

  mvm::jit::protectTypes();//->lock();
  llvm::FunctionType* res =  llvm::FunctionType::get(ret->funcs->llvmType,
                                                     llvmArgs, false);
  mvm::jit::unprotectTypes();//->unlock();

  return res;
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
  res->staticType = Signdef::createStaticType(&buf, res->ret);
  res->virtualType = Signdef::createVirtualType(&buf, res->ret);
  res->nativeType = Signdef::createNativeType(&buf, res->ret);
  mvm::jit::protectTypes();//->lock();
  res->staticTypePtr  = llvm::PointerType::getUnqual(res->staticType);
  res->virtualTypePtr = llvm::PointerType::getUnqual(res->virtualType);
  res->nativeTypePtr  = llvm::PointerType::getUnqual(res->nativeType);
  
  std::vector<const llvm::Type*> Args;
  Args.push_back(mvm::jit::ptrType); // vm
  Args.push_back(res->staticTypePtr);
  Args.push_back(llvm::PointerType::getUnqual(llvm::Type::Int32Ty));
  res->staticBufType = llvm::FunctionType::get(res->ret->funcs->llvmType, Args, false);
  
  std::vector<const llvm::Type*> Args2;
  Args2.push_back(mvm::jit::ptrType); // vm
  Args2.push_back(res->virtualTypePtr);
  Args2.push_back(JavaObject::llvmType);
  Args2.push_back(llvm::PointerType::getUnqual(llvm::Type::Int32Ty));
  res->virtualBufType = llvm::FunctionType::get(res->ret->funcs->llvmType, Args2, false);

  mvm::jit::unprotectTypes();//->unlock();
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
