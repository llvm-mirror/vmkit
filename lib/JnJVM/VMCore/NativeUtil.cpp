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

UserCommonClass* NativeUtil::resolvedImplClass(jclass clazz, bool doClinit) {
  Jnjvm* vm = JavaThread::get()->isolate;
  JavaObject *Cl = (JavaObject*)clazz;
  UserCommonClass* cl = 
    (UserCommonClass*)vm->upcalls->vmdataClass->getObjectField(Cl);
  cl->resolveClass();
  if (doClinit) cl->initialiseClass(vm);
  return cl;
}

void NativeUtil::decapsulePrimitive(Jnjvm *vm, void** &buf,
                                    JavaObject* obj,
                                    Typedef* signature) {

  if (!signature->isPrimitive()) {
    if (obj && !(obj->classOf->isOfTypeName(signature->getName()))) {
      vm->illegalArgumentException("wrong type argument");
    }
    ((JavaObject**)buf)[0] = obj;
    buf++;
    return;
  } else if (obj == 0) {
    vm->illegalArgumentException("");
  } else {
    UserCommonClass* cl = obj->classOf;
    UserClassPrimitive* value = cl->toPrimitive(vm);
    PrimitiveTypedef* prim = (PrimitiveTypedef*)signature;

    if (value == 0) {
      vm->illegalArgumentException("");
    }
    
    if (prim->isShort()) {
      if (value == vm->upcalls->OfShort) {
        ((uint16*)buf)[0] = vm->upcalls->shortValue->getInt16Field(obj);
        buf++;
        return;
      } else if (value == vm->upcalls->OfByte) {
        ((sint16*)buf)[0] = 
          (sint16)vm->upcalls->byteValue->getInt8Field(obj);
        buf++;
        return;
      } else {
        vm->illegalArgumentException("");
      }
    } else if (prim->isByte()) {
      if (value == vm->upcalls->OfByte) {
        ((uint8*)buf)[0] = vm->upcalls->byteValue->getInt8Field(obj);
        buf++;
        return;
      } else {
        vm->illegalArgumentException("");
      }
    } else if (prim->isBool()) {
      if (value == vm->upcalls->OfBool) {
        ((uint8*)buf)[0] = vm->upcalls->boolValue->getInt8Field(obj);
        buf++;
        return;
      } else {
        vm->illegalArgumentException("");
      }
    } else if (prim->isInt()) {
      sint32 val = 0;
      if (value == vm->upcalls->OfInt) {
        val = vm->upcalls->intValue->getInt32Field(obj);
      } else if (value == vm->upcalls->OfByte) {
        val = (sint32)vm->upcalls->byteValue->getInt8Field(obj);
      } else if (value == vm->upcalls->OfChar) {
        val = (uint32)vm->upcalls->charValue->getInt16Field(obj);
      } else if (value == vm->upcalls->OfShort) {
        val = (sint32)vm->upcalls->shortValue->getInt16Field(obj);
      } else {
        vm->illegalArgumentException("");
      }
      ((sint32*)buf)[0] = val;
      buf++;
      return;
    } else if (prim->isChar()) {
      uint16 val = 0;
      if (value == vm->upcalls->OfChar) {
        val = (uint16)vm->upcalls->charValue->getInt16Field(obj);
      } else {
        vm->illegalArgumentException("");
      }
      ((uint16*)buf)[0] = val;
      buf++;
      return;
    } else if (prim->isFloat()) {
      float val = 0;
      if (value == vm->upcalls->OfFloat) {
        val = (float)vm->upcalls->floatValue->getFloatField(obj);
      } else if (value == vm->upcalls->OfByte) {
        val = (float)(sint32)vm->upcalls->byteValue->getInt8Field(obj);
      } else if (value == vm->upcalls->OfChar) {
        val = (float)(uint32)vm->upcalls->charValue->getInt16Field(obj);
      } else if (value == vm->upcalls->OfShort) {
        val = (float)(sint32)vm->upcalls->shortValue->getInt16Field(obj);
      } else if (value == vm->upcalls->OfInt) {
        val = (float)(sint32)vm->upcalls->intValue->getInt32Field(obj);
      } else if (value == vm->upcalls->OfLong) {
        val = (float)vm->upcalls->longValue->getLongField(obj);
      } else {
        vm->illegalArgumentException("");
      }
      ((float*)buf)[0] = val;
      buf++;
      return;
    } else if (prim->isDouble()) {
      double val = 0;
      if (value == vm->upcalls->OfDouble) {
        val = (double)vm->upcalls->doubleValue->getDoubleField(obj);
      } else if (value == vm->upcalls->OfFloat) {
        val = (double)vm->upcalls->floatValue->getFloatField(obj);
      } else if (value == vm->upcalls->OfByte) {
        val = (double)(sint64)vm->upcalls->byteValue->getInt8Field(obj);
      } else if (value == vm->upcalls->OfChar) {
        val = (double)(uint64)vm->upcalls->charValue->getInt16Field(obj);
      } else if (value == vm->upcalls->OfShort) {
        val = (double)(sint16)vm->upcalls->shortValue->getInt16Field(obj);
      } else if (value == vm->upcalls->OfInt) {
        val = (double)(sint32)vm->upcalls->intValue->getInt32Field(obj);
      } else if (value == vm->upcalls->OfLong) {
        val = (double)(sint64)vm->upcalls->longValue->getLongField(obj);
      } else {
        vm->illegalArgumentException("");
      }
      ((double*)buf)[0] = val;
      buf += 2;
      return;
    } else if (prim->isLong()) {
      sint64 val = 0;
      if (value == vm->upcalls->OfByte) {
        val = (sint64)vm->upcalls->byteValue->getInt8Field(obj);
      } else if (value == vm->upcalls->OfChar) {
        val = (sint64)(uint64)vm->upcalls->charValue->getInt16Field(obj);
      } else if (value == vm->upcalls->OfShort) {
        val = (sint64)vm->upcalls->shortValue->getInt16Field(obj);
      } else if (value == vm->upcalls->OfInt) {
        val = (sint64)vm->upcalls->intValue->getInt32Field(obj);
      } else if (value == vm->upcalls->OfLong) {
        val = (sint64)vm->upcalls->intValue->getLongField(obj);
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
  Jnjvm* vm = JavaThread::get()->isolate;
  UserCommonClass* res = type->assocClass(loader);
  assert(res && "No associated class");
  return res->getClassDelegatee(vm);
}

ArrayObject* NativeUtil::getParameterTypes(JnjvmClassLoader* loader, JavaMethod* meth) {
  Jnjvm* vm = JavaThread::get()->isolate;
  std::vector<Typedef*>& args = meth->getSignature()->args;
  ArrayObject* res = ArrayObject::acons(args.size(), vm->upcalls->classArrayClass,
                                        &(JavaThread::get()->isolate->allocator));

  sint32 index = 0;
  for (std::vector<Typedef*>::iterator i = args.begin(), e = args.end();
          i != e; ++i, ++index) {
    res->elements[index] = getClassType(loader, (*i));
  }

  return res;

}

ArrayObject* NativeUtil::getExceptionTypes(UserClass* cl, JavaMethod* meth) {
  Attribut* exceptionAtt = meth->lookupAttribut(Attribut::exceptionsAttribut);
  Jnjvm* vm = JavaThread::get()->isolate;
  if (exceptionAtt == 0) {
    return ArrayObject::acons(0, vm->upcalls->classArrayClass,
                              &(JavaThread::get()->isolate->allocator));
  } else {
    UserConstantPool* ctp = cl->getConstantPool();
    Reader reader(exceptionAtt, cl->getBytes());
    uint16 nbe = reader.readU2();
    ArrayObject* res = ArrayObject::acons(nbe, vm->upcalls->classArrayClass,
                                          &(JavaThread::get()->isolate->allocator));

    for (uint16 i = 0; i < nbe; ++i) {
      uint16 idx = reader.readU2();
      UserCommonClass* cl = ctp->loadClass(idx);
      cl->resolveClass();
      JavaObject* obj = cl->getClassDelegatee(vm);
      res->elements[i] = obj;
    }
    return res;
  }
}
