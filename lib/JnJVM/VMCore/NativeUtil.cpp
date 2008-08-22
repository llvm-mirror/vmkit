//===------ NativeUtil.cpp - Methods to call native functions -------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>

#include "JavaArray.h"
#include "JavaClass.h"
#include "JavaConstantPool.h"
#include "JavaJIT.h"
#include "JavaThread.h"
#include "JavaTypes.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"
#include "JnjvmClassLoader.h"
#include "NativeUtil.h"
#include "Reader.h"

using namespace jnjvm;

Jnjvm* NativeUtil::myVM(JNIEnv* env) {
  return JavaThread::get()->isolate;
}

#if defined(__MACH__)
#define SELF_HANDLE RTLD_DEFAULT
#else
#define SELF_HANDLE 0
#endif

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
    cur = jniConsClName->elements[i];
    if (cur == '/') ptr[0] = '_';
    else ptr[0] = (uint8)cur;
    ++ptr;
  }
  
  ptr[0] = '_';
  ++ptr;
  
  for (sint32 i =0; i < mnlen; ++i) {
    cur = jniConsName->elements[i];
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
    cur = jniConsClName->elements[i];
    if (cur == '/') ptr[0] = '_';
    else ptr[0] = (uint8)cur;
    ++ptr;
  }
  
  ptr[0] = '_';
  ++ptr;
  
  for (sint32 i =0; i < mnlen; ++i) {
    cur = jniConsName->elements[i];
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
    cur = jniConsClName->elements[i];
    if (cur == '/') ptr[0] = '_';
    else ptr[0] = (uint8)cur;
    ++ptr;
  }
  
  ptr[0] = '_';
  ++ptr;

  for (sint32 i =0; i < mnlen; ++i) {
    cur = jniConsName->elements[i];
    if (cur == '/') ptr[0] = '_';
    else ptr[0] = (uint8)cur;
    ++ptr;
  }
  
  sint32 i = 0;
  while (i < jniConsType->size) {
    char c = jniConsType->elements[i++];
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
  void* res = dlsym(SELF_HANDLE, buf);
  if (!res) {
#ifndef SERVICE_VM
    Jnjvm* vm = JavaThread::get()->isolate;
#else
    Jnjvm* vm = Jnjvm::bootstrapVM;
#endif
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
        printf("Native function %s not found. Probably "
               "not implemented by JnJVM?\n", meth->printString());
        JavaJIT::printBacktrace();
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
  CommonClass* cl = (CommonClass*)Classpath::vmdataClass->getVirtualObjectField(Cl);
  cl->resolveClass();
  JavaThread::get()->isolate->initialiseClass(cl);
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
        ((uint16*)buf)[0] = Classpath::shortValue->getVirtualInt16Field(obj);
        buf++;
        return;
      } else if (value == AssessorDesc::dByte) {
        ((sint16*)buf)[0] = 
          (sint16)Classpath::byteValue->getVirtualInt8Field(obj);
        buf++;
        return;
      } else {
        vm->illegalArgumentException("");
      }
    } else if (funcs == AssessorDesc::dByte) {
      if (value == AssessorDesc::dByte) {
        ((uint8*)buf)[0] = Classpath::byteValue->getVirtualInt8Field(obj);
        buf++;
        return;
      } else {
        vm->illegalArgumentException("");
      }
    } else if (funcs == AssessorDesc::dBool) {
      if (value == AssessorDesc::dBool) {
        ((uint8*)buf)[0] = Classpath::boolValue->getVirtualInt8Field(obj);
        buf++;
        return;
      } else {
        vm->illegalArgumentException("");
      }
    } else if (funcs == AssessorDesc::dInt) {
      sint32 val = 0;
      if (value == AssessorDesc::dInt) {
        val = Classpath::intValue->getVirtualInt32Field(obj);
      } else if (value == AssessorDesc::dByte) {
        val = (sint32)Classpath::byteValue->getVirtualInt8Field(obj);
      } else if (value == AssessorDesc::dChar) {
        val = (uint32)Classpath::charValue->getVirtualInt16Field(obj);
      } else if (value == AssessorDesc::dShort) {
        val = (sint32)Classpath::shortValue->getVirtualInt16Field(obj);
      } else {
        vm->illegalArgumentException("");
      }
      ((sint32*)buf)[0] = val;
      buf++;
      return;
    } else if (funcs == AssessorDesc::dChar) {
      uint16 val = 0;
      if (value == AssessorDesc::dChar) {
        val = (uint16)Classpath::charValue->getVirtualInt16Field(obj);
      } else {
        vm->illegalArgumentException("");
      }
      ((uint16*)buf)[0] = val;
      buf++;
      return;
    } else if (funcs == AssessorDesc::dFloat) {
      float val = 0;
      if (value == AssessorDesc::dFloat) {
        val = (float)Classpath::floatValue->getVirtualFloatField(obj);
      } else if (value == AssessorDesc::dByte) {
        val = (float)(sint32)Classpath::byteValue->getVirtualInt8Field(obj);
      } else if (value == AssessorDesc::dChar) {
        val = (float)(uint32)Classpath::charValue->getVirtualInt16Field(obj);
      } else if (value == AssessorDesc::dShort) {
        val = (float)(sint32)Classpath::shortValue->getVirtualInt16Field(obj);
      } else if (value == AssessorDesc::dInt) {
        val = (float)(sint32)Classpath::intValue->getVirtualInt32Field(obj);
      } else if (value == AssessorDesc::dLong) {
        val = (float)Classpath::longValue->getVirtualLongField(obj);
      } else {
        vm->illegalArgumentException("");
      }
      ((float*)buf)[0] = val;
      buf++;
      return;
    } else if (funcs == AssessorDesc::dDouble) {
      double val = 0;
      if (value == AssessorDesc::dDouble) {
        val = (double)Classpath::doubleValue->getVirtualDoubleField(obj);
      } else if (value == AssessorDesc::dFloat) {
        val = (double)Classpath::floatValue->getVirtualFloatField(obj);
      } else if (value == AssessorDesc::dByte) {
        val = (double)(sint64)Classpath::byteValue->getVirtualInt8Field(obj);
      } else if (value == AssessorDesc::dChar) {
        val = (double)(uint64)Classpath::charValue->getVirtualInt16Field(obj);
      } else if (value == AssessorDesc::dShort) {
        val = (double)(sint16)Classpath::shortValue->getVirtualInt16Field(obj);
      } else if (value == AssessorDesc::dInt) {
        val = (double)(sint32)Classpath::intValue->getVirtualInt32Field(obj);
      } else if (value == AssessorDesc::dLong) {
        val = (double)(sint64)Classpath::longValue->getVirtualLongField(obj);
      } else {
        vm->illegalArgumentException("");
      }
      ((double*)buf)[0] = val;
      buf += 2;
      return;
    } else if (funcs == AssessorDesc::dLong) {
      sint64 val = 0;
      if (value == AssessorDesc::dByte) {
        val = (sint64)Classpath::byteValue->getVirtualInt8Field(obj);
      } else if (value == AssessorDesc::dChar) {
        val = (sint64)(uint64)Classpath::charValue->getVirtualInt16Field(obj);
      } else if (value == AssessorDesc::dShort) {
        val = (sint64)Classpath::shortValue->getVirtualInt16Field(obj);
      } else if (value == AssessorDesc::dInt) {
        val = (sint64)Classpath::intValue->getVirtualInt32Field(obj);
      } else if (value == AssessorDesc::dLong) {
        val = (sint64)Classpath::intValue->getVirtualLongField(obj);
      } else {
        vm->illegalArgumentException("");
      }
      ((sint64*)buf)[0] = val;
      buf += 2;
      return;
    }
  }
  // can not be here
  return;
}

JavaObject* NativeUtil::getClassType(JnjvmClassLoader* loader, Typedef* type) {
  CommonClass* res = type->assocClass(loader);
  return res->getClassDelegatee();
}

ArrayObject* NativeUtil::getParameterTypes(JnjvmClassLoader* loader, JavaMethod* meth) {
  std::vector<Typedef*>& args = meth->getSignature()->args;
  ArrayObject* res = ArrayObject::acons(args.size(), Classpath::classArrayClass,
                                        &(JavaThread::get()->isolate->allocator));

  sint32 index = 0;
  for (std::vector<Typedef*>::iterator i = args.begin(), e = args.end();
          i != e; ++i, ++index) {
    res->elements[index] = getClassType(loader, (*i));
  }

  return res;

}

ArrayObject* NativeUtil::getExceptionTypes(JavaMethod* meth) {
  Attribut* exceptionAtt = meth->lookupAttribut(Attribut::exceptionsAttribut);
  if (exceptionAtt == 0) {
    return ArrayObject::acons(0, Classpath::classArrayClass,
                              &(JavaThread::get()->isolate->allocator));
  } else {
    Class* cl = meth->classDef;
    JavaConstantPool* ctp = cl->ctpInfo;
    Reader reader(exceptionAtt, cl->bytes);
    uint16 nbe = reader.readU2();
    ArrayObject* res = ArrayObject::acons(nbe, Classpath::classArrayClass,
                                          &(JavaThread::get()->isolate->allocator));

    for (uint16 i = 0; i < nbe; ++i) {
      uint16 idx = reader.readU2();
      CommonClass* cl = ctp->loadClass(idx);
      cl->resolveClass();
      JavaObject* obj = cl->getClassDelegatee();
      res->elements[i] = obj;
    }
    return res;
  }
}
