//===------ NativeUtil.cpp - Methods to call native functions -------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/ExecutionEngine/GenericValue.h"

#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>

#include "JavaArray.h"
#include "JavaClass.h"
#include "JavaConstantPool.h"
#include "JavaThread.h"
#include "JavaTypes.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"
#include "NativeUtil.h"
#include "Reader.h"

using namespace jnjvm;

Jnjvm* NativeUtil::myVM(JNIEnv* env) {
  return JavaThread::get()->isolate;
}

#define PRE "Java_"
#define PRE_LEN 5

static char* jniConsFromMeth(CommonClass* cl, JavaMethod* meth) {
  const UTF8* jniConsClName = cl->name;
  const UTF8* jniConsName = meth->name;
  const UTF8* jniConsType = meth->type;
  sint32 clen = jniConsClName->size;
  sint32 mnlen = jniConsName->size;
  sint32 mtlen = jniConsType->size;

  char* buf = (char*)malloc(3 + PRE_LEN + mnlen + clen + (mtlen << 1));
  uint32 cur = 0;
  char* ptr = &(buf[PRE_LEN]);
  
  memcpy(buf, PRE, PRE_LEN);
  
  for (sint32 i =0; i < clen; ++i) {
    cur = jniConsClName->at(i);
    if (cur == '/') ptr[0] = '_';
    else ptr[0] = (uint8)cur;
    ++ptr;
  }
  
  ptr[0] = '_';
  ++ptr;
  
  for (sint32 i =0; i < mnlen; ++i) {
    cur = jniConsName->at(i);
    if (cur == '/') ptr[0] = '_';
    else ptr[0] = (uint8)cur;
    ++ptr;
  }
  
  ptr[0] = 0;

  return buf;

}

static char* jniConsFromMeth2(CommonClass* cl, JavaMethod* meth) {
  const UTF8* jniConsClName = cl->name;
  const UTF8* jniConsName = meth->name;
  const UTF8* jniConsType = meth->type;
  sint32 clen = jniConsClName->size;
  sint32 mnlen = jniConsName->size;
  sint32 mtlen = jniConsType->size;

  char* buf = (char*)malloc(3 + PRE_LEN + mnlen + clen + (mtlen << 1));
  uint32 cur = 0;
  char* ptr = &(buf[PRE_LEN]);
  
  memcpy(buf, PRE, PRE_LEN);
  
  for (sint32 i =0; i < clen; ++i) {
    cur = jniConsClName->at(i);
    if (cur == '/') ptr[0] = '_';
    else ptr[0] = (uint8)cur;
    ++ptr;
  }
  
  ptr[0] = '_';
  ++ptr;
  
  for (sint32 i =0; i < mnlen; ++i) {
    cur = jniConsName->at(i);
    if (cur == '/') ptr[0] = '_';
    else if (cur == '_') {
      ptr[0] = '_';
      ptr[1] = '1';
      ++ptr;
    }
    else ptr[0] = (uint8)cur;
    ++ptr;
  }
  
  ptr[0] = 0;

  return buf;

}

static char* jniConsFromMeth3(CommonClass* cl, JavaMethod* meth) {
  const UTF8* jniConsClName = cl->name;
  const UTF8* jniConsName = meth->name;
  const UTF8* jniConsType = meth->type;
  sint32 clen = jniConsClName->size;
  sint32 mnlen = jniConsName->size;
  sint32 mtlen = jniConsType->size;

  char* buf = (char*)malloc(3 + PRE_LEN + mnlen + clen + (mtlen << 1));
  uint32 cur = 0;
  char* ptr = &(buf[PRE_LEN]);
  
  memcpy(buf, PRE, PRE_LEN);
  
  for (sint32 i =0; i < clen; ++i) {
    cur = jniConsClName->at(i);
    if (cur == '/') ptr[0] = '_';
    else ptr[0] = (uint8)cur;
    ++ptr;
  }
  
  ptr[0] = '_';
  ++ptr;

  for (sint32 i =0; i < mnlen; ++i) {
    cur = jniConsName->at(i);
    if (cur == '/') ptr[0] = '_';
    else ptr[0] = (uint8)cur;
    ++ptr;
  }
  
  sint32 i = 0;
  while (i < jniConsType->size) {
    char c = jniConsType->at(i++);
    if (c == AssessorDesc::I_PARG) {
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
    } else if (c == AssessorDesc::I_END_REF) {
      ptr[0] = '_';
      ptr[1] = '2';
      ptr += 2;
    } else if (c == AssessorDesc::I_TAB) {
      ptr[0] = '_';
      ptr[1] = '3';
      ptr += 2;
    } else if (c == AssessorDesc::I_PARD) {
      break;
    } else {
      ptr[0] = c;
      ++ptr;
    }
  }

  ptr[0] = 0;

  return buf;

}
#undef PRE_LEN

static void* loadName(char* buf, bool& jnjvm) {
  void* res = dlsym(0, buf);
  if (!res) {
    Jnjvm *vm = JavaThread::get()->isolate;
    for (std::vector<void*>::iterator i = vm->nativeLibs.begin(), 
              e = vm->nativeLibs.end(); i!= e; ++i) {
      res = dlsym((*i), buf);
      if (res) break;
    }
  } else {
    jnjvm = true;
  }
  return res;
}

void* NativeUtil::nativeLookup(CommonClass* cl, JavaMethod* meth, bool& jnjvm) {
  char* buf = jniConsFromMeth(cl, meth);
  void* res = loadName(buf, jnjvm);
  if (!res) {
    buf = jniConsFromMeth2(cl, meth);
    res = loadName(buf, jnjvm);
    if (!res) {
      buf = jniConsFromMeth3(cl, meth);
      res = loadName(buf, jnjvm);
      if (!res) {
        printf("error for %s\n", meth->printString());
        JavaThread::get()->isolate->unknownError("can not find native method %s",
                                                 meth->printString());
      }
    }
  }
  free(buf);
  return res;
}

CommonClass* NativeUtil::resolvedImplClass(jclass clazz, bool doClinit) {
  JavaObject *Cl = (JavaObject*)clazz;
  CommonClass* cl = (CommonClass*)((*Cl)(Classpath::vmdataClass).PointerVal);
  cl->resolveClass(doClinit);
  return cl;
}

void NativeUtil::decapsulePrimitive(Jnjvm *vm, void** &buf,
                                    JavaObject* obj,
                                    Typedef* signature) {
  const AssessorDesc* funcs = signature->funcs;

  if (funcs == AssessorDesc::dRef || funcs == AssessorDesc::dTab) {
    if (obj && !(obj->classOf->isOfTypeName(signature->pseudoAssocClassName))) {
      vm->illegalArgumentException("wrong type argument");
    }
    ((JavaObject**)buf)[0] = obj;
    buf++;
    return;
  } else if (obj == 0) {
    vm->illegalArgumentException("");
  } else {
    CommonClass* cl = obj->classOf;
    AssessorDesc* value = AssessorDesc::classToPrimitive(cl);
    
    if (value == 0) {
      vm->illegalArgumentException("");
    }
    
    if (funcs == AssessorDesc::dShort) {
      if (value == AssessorDesc::dShort) {
        llvm::GenericValue val = (*Classpath::shortValue)(obj);
        ((uint32*)buf)[0] = (uint32)val.IntVal.getSExtValue();
        buf++;
        return;
      } else if (value == AssessorDesc::dByte) {
        llvm::GenericValue val = (*Classpath::shortValue)(obj);
        ((uint32*)buf)[0] = (uint32)val.IntVal.getSExtValue();
        buf++;
        return;
      } else {
        vm->illegalArgumentException("");
      }
    } else if (funcs == AssessorDesc::dByte) {
      if (value == AssessorDesc::dByte) {
        llvm::GenericValue val = (*Classpath::byteValue)(obj);
        ((uint32*)buf)[0] = (uint32)val.IntVal.getSExtValue();
        buf++;
        return;
      } else {
        vm->illegalArgumentException("");
      }
    } else if (funcs == AssessorDesc::dBool) {
      if (value == AssessorDesc::dBool) {
        llvm::GenericValue val = (*Classpath::boolValue)(obj);
        ((uint32*)buf)[0] = (uint32)val.IntVal.getZExtValue();
        buf++;
        return;
      } else {
        vm->illegalArgumentException("");
      }
    } else if (funcs == AssessorDesc::dInt) {
      if (value == AssessorDesc::dInt) {
        llvm::GenericValue val = (*Classpath::intValue)(obj);
        ((uint32*)buf)[0] = (uint32)val.IntVal.getSExtValue();
        buf++;
      } else if (value == AssessorDesc::dByte) {
        llvm::GenericValue val = (*Classpath::byteValue)(obj);
        ((uint32*)buf)[0] = (uint32)val.IntVal.getSExtValue();
        buf++;
        return;
      } else if (value == AssessorDesc::dChar) {
        llvm::GenericValue val = (*Classpath::charValue)(obj);
        ((uint32*)buf)[0] = (uint32)val.IntVal.getZExtValue();
        buf++;
        return;
      } else if (value == AssessorDesc::dShort) {
        llvm::GenericValue val = (*Classpath::shortValue)(obj);
        ((uint32*)buf)[0] = val.IntVal.getSExtValue();
        buf++;
        return;
      } else {
        vm->illegalArgumentException("");
      }
    } else if (funcs == AssessorDesc::dChar) {
      if (value == AssessorDesc::dChar) {
        llvm::GenericValue val = (*Classpath::charValue)(obj);
        ((uint32*)buf)[0] = (uint32)val.IntVal.getZExtValue();
        buf++;
        return;
      } else {
        vm->illegalArgumentException("");
      }
    } else if (funcs == AssessorDesc::dFloat) {
      if (value == AssessorDesc::dFloat) {
        llvm::GenericValue val = (*Classpath::floatValue)(obj);
        ((float*)buf)[0] = val.FloatVal;
        buf++;
        return;
      } else if (value == AssessorDesc::dByte) {
        llvm::GenericValue val = (*Classpath::byteValue)(obj);
        float res = (float)(val.IntVal.getSExtValue());
        ((float*)buf)[0] = res;
        buf++;
        return;
      } else if (value == AssessorDesc::dChar) {
        llvm::GenericValue val = (*Classpath::charValue)(obj);
        float res = (float)(val.IntVal.getZExtValue());
        ((float*)buf)[0] = res;
        buf++;
        return;
      } else if (value == AssessorDesc::dShort) {
        llvm::GenericValue val = (*Classpath::shortValue)(obj);
        float res = (float)(val.IntVal.getSExtValue());
        ((float*)buf)[0] = res;
        buf++;
        return;
      } else if (value == AssessorDesc::dInt) {
        llvm::GenericValue val = (*Classpath::intValue)(obj);
        float res = (float)(val.IntVal.getSExtValue());
        ((float*)buf)[0] = res;
        buf++;
        return;
      } else if (value == AssessorDesc::dLong) {
        llvm::GenericValue val = (*Classpath::longValue)(obj);
        float res = (float)(val.IntVal.getSExtValue());
        ((float*)buf)[0] = res;
        buf++;
        return;
      } else {
        vm->illegalArgumentException("");
      }
    } else if (funcs == AssessorDesc::dDouble) {
      if (value == AssessorDesc::dDouble) {
        llvm::GenericValue gv = (*Classpath::doubleValue)(obj);
        ((double*)buf)[0] = gv.DoubleVal;
        buf++;
        buf++;
        return;
      } else if (value == AssessorDesc::dFloat) {
        llvm::GenericValue val = (*Classpath::floatValue)(obj);
        double res = (double)(val.FloatVal);
        ((double*)buf)[0] = res;
        buf++;
        buf++;
        return;
      } else if (value == AssessorDesc::dByte) {
        llvm::GenericValue val = (*Classpath::byteValue)(obj);
        double res = (double)(val.IntVal.getSExtValue());
        ((double*)buf)[0] = res;
        buf++;
        buf++;
        return;
      } else if (value == AssessorDesc::dChar) {
        llvm::GenericValue val = (*Classpath::charValue)(obj);
        double res  = (double)(val.IntVal.getZExtValue());
        ((double*)buf)[0] = res;
        buf++;
        buf++;
        return;
      } else if (value == AssessorDesc::dShort) {
        llvm::GenericValue val = (*Classpath::shortValue)(obj);
        double res = (double)(val.IntVal.getSExtValue());
        ((double*)buf)[0] = res;
        buf++;
        buf++;
        return;
      } else if (value == AssessorDesc::dInt) {
        llvm::GenericValue val = (*Classpath::intValue)(obj);
        double res = (double)(val.IntVal.getSExtValue());
        ((double*)buf)[0] = res;
        buf++;
        buf++;
        return;
      } else if (value == AssessorDesc::dLong) {
        llvm::GenericValue val = (*Classpath::longValue)(obj);
        double res = (double)(val.IntVal.getSExtValue());
        ((double*)buf)[0] = res;
        buf++;
        buf++;
        return;
      } else {
        vm->illegalArgumentException("");
      }
    } else if (funcs == AssessorDesc::dLong) {
      if (value == AssessorDesc::dByte) {
        llvm::GenericValue val = (*Classpath::byteValue)(obj);
        ((uint64*)buf)[0] = val.IntVal.getSExtValue();
        buf++;
        buf++;
        return;
      } else if (value == AssessorDesc::dChar) {
        llvm::GenericValue val = (*Classpath::charValue)(obj);
        ((uint64*)buf)[0] = val.IntVal.getZExtValue();
        buf++;
        buf++;
        return;
      } else if (value == AssessorDesc::dShort) {
        llvm::GenericValue val = (*Classpath::shortValue)(obj);
        ((uint64*)buf)[0] = val.IntVal.getSExtValue();
        buf++;
        buf++;
        return;
      } else if (value == AssessorDesc::dInt) {
        llvm::GenericValue val = (*Classpath::intValue)(obj);
        ((uint64*)buf)[0] = val.IntVal.getSExtValue();
        buf++;
        buf++;
        return;
      } else if (value == AssessorDesc::dLong) {
        llvm::GenericValue val = (*Classpath::longValue)(obj);
        ((uint64*)buf)[0] = val.IntVal.getSExtValue();
        buf++;
        buf++;
        return;
      } else {
        vm->illegalArgumentException("");
      }
    }
  }
  // can not be here
  return;
}

JavaObject* NativeUtil::getClassType(JavaObject* loader, Typedef* type) {
  CommonClass* res = type->assocClass(loader);
  return res->getClassDelegatee();
}

ArrayObject* NativeUtil::getParameterTypes(JavaObject* loader, JavaMethod* meth) {
  std::vector<Typedef*>& args = meth->signature->args;
  ArrayObject* res = ArrayObject::acons(args.size(), Classpath::classArrayClass,
                                        JavaThread::get()->isolate);

  sint32 index = 0;
  for (std::vector<Typedef*>::iterator i = args.begin(), e = args.end();
          i != e; ++i, ++index) {
    res->setAt(index, getClassType(loader, (*i)));
  }

  return res;

}

ArrayObject* NativeUtil::getExceptionTypes(JavaMethod* meth) {
  Attribut* exceptionAtt = Attribut::lookup(&meth->attributs,
                                            Attribut::exceptionsAttribut);
  if (exceptionAtt == 0) {
    return ArrayObject::acons(0, Classpath::classArrayClass,
                              JavaThread::get()->isolate);
  } else {
    Class* cl = meth->classDef;
    JavaCtpInfo* ctp = cl->ctpInfo;
    Reader* reader = exceptionAtt->toReader(JavaThread::get()->isolate,
                                            cl->bytes, exceptionAtt);
    uint16 nbe = reader->readU2();
    ArrayObject* res = ArrayObject::acons(nbe, Classpath::classArrayClass,
                                          JavaThread::get()->isolate);

    for (uint16 i = 0; i < nbe; ++i) {
      uint16 idx = reader->readU2();
      CommonClass* cl = ctp->loadClass(idx);
      cl->resolveClass(false);
      JavaObject* obj = cl->getClassDelegatee();
      res->elements[i] = obj;
    }
    return res;
  }
}
