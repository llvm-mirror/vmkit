//===- ClasspathMethod.cpp ------------------------------------------------===//
//===------------- GNU classpath java/lang/reflect/Method -----------------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "types.h"

#include "Classpath.h"
#include "ClasspathReflect.h"
#include "JavaArray.h"
#include "JavaClass.h"
#include "JavaObject.h"
#include "JavaTypes.h"
#include "JavaThread.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"
#include "JnjvmClassLoader.h"

using namespace j3;

extern "C" {

JNIEXPORT jint JNICALL Java_java_lang_reflect_Method_getModifiersInternal(
#ifdef NATIVE_JNI
JNIEnv *env, 
#endif
JavaObjectMethod* Meth) { 
  
  jint res = 0;
  llvm_gcroot(Meth, 0);

  BEGIN_NATIVE_EXCEPTION(0)
  
  JavaMethod* meth = Meth->getInternalMethod(); 
  res = meth->access;

  END_NATIVE_EXCEPTION

  return res;
}

JNIEXPORT JavaObject* JNICALL Java_java_lang_reflect_Method_getReturnType(
#ifdef NATIVE_JNI
JNIEnv *env, 
#endif
JavaObjectMethod* Meth) {

  JavaObject* res = 0;
  llvm_gcroot(Meth, 0);
  llvm_gcroot(res, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  UserClass* cl = Meth->getClass();
  JavaMethod* meth = Meth->getInternalMethod(); 
  JnjvmClassLoader* loader = cl->classLoader;
  res = meth->getReturnType(loader);

  END_NATIVE_EXCEPTION

  return res;
}


JNIEXPORT JavaObject* JNICALL Java_java_lang_reflect_Method_getParameterTypes(
#ifdef NATIVE_JNI
JNIEnv *env, 
#endif
JavaObjectMethod* Meth) {

  JavaObject* res = 0;
  llvm_gcroot(Meth, 0);
  llvm_gcroot(res, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  UserClass* cl = Meth->getClass();
  JavaMethod* meth = Meth->getInternalMethod();
  JnjvmClassLoader* loader = cl->classLoader;
  
  res = (meth->getParameterTypes(loader));

  END_NATIVE_EXCEPTION

  return res;
}

static JavaObject* proceedMethod(JavaObjectMethod* Meth, JavaObject* obj,
                                 JavaArray* args, JavaObject* Cl, jint index) 
  __attribute__((noinline));

static JavaObject* proceedMethod(JavaObjectMethod* Meth, JavaObject* obj,
                                 JavaArray* args, JavaObject* Cl, jint index) {

  JavaObject* res = 0;
  JavaObject* exc = 0;
  
  llvm_gcroot(res, 0);
  llvm_gcroot(Meth, 0);
  llvm_gcroot(obj, 0);
  llvm_gcroot(args, 0);
  llvm_gcroot(Cl, 0);
  llvm_gcroot(exc, 0);

  Jnjvm* vm = JavaThread::get()->getJVM();

  JavaMethod* meth = Meth->getInternalMethod();
  
  sint32 nbArgs = args ? args->size : 0;
  Signdef* sign = meth->getSignature();
  sint32 size = sign->nbArguments;

  jvalue* buf = size ? (jvalue*)alloca(size * sizeof(jvalue)) : 0;
  if (size) memset(buf, 0, size * sizeof(jvalue));
  
  if (nbArgs == size) {
    UserCommonClass* _cl = UserCommonClass::resolvedImplClass(vm, Cl, false);
    UserClass* cl = (UserClass*)_cl;
    
    if (isVirtual(meth->access)) {
      verifyNull(obj);
      UserCommonClass* objCl = obj->getClass();
      if (!(objCl->isAssignableFrom(cl))) {
        vm->illegalArgumentException("<this> is not a valid type");
      }

      if (isInterface(cl->access)) {
        cl->initialiseClass(vm);
        UserClass* methodCl = 0;
        UserClass* lookup = objCl->isArray() ? objCl->super : objCl->asClass();
        meth = lookup->lookupMethod(meth->name, meth->type, false, true,
                                    &methodCl);
      }
    } else {
      cl->initialiseClass(vm);
    }
    
    JavaObject** ptr = (JavaObject**)(void*)(args->elements);
    Typedef* const* arguments = sign->getArgumentsType();
    for (sint32 i = 0; i < size; ++i) {
      ptr[i]->decapsulePrimitive(vm, &buf[i], arguments[i]);
      if (buf[i].l == reinterpret_cast<jobject>(ptr[i])) {
        buf[i].l = reinterpret_cast<jobject>(&ptr[i]);
      }
    }
    
    JavaThread* th = JavaThread::get();

#define RUN_METH(TYPE, VAR) \
    try{ \
      if (isVirtual(meth->access)) { \
        if (isPublic(meth->access) && !isFinal(meth->access) && \
            !isFinal(meth->classDef->access)) { \
          VAR = meth->invoke##TYPE##VirtualBuf(vm, cl, obj, buf); \
        } else { \
          VAR = meth->invoke##TYPE##SpecialBuf(vm, cl, obj, buf); \
        } \
      } else { \
        VAR = meth->invoke##TYPE##StaticBuf(vm, cl, buf); \
      } \
    } catch(...) { \
      exc = th->getJavaException(); \
      if (exc->getClass()->isAssignableFrom(vm->upcalls->newException)) { \
        th->clearException(); \
        th->getJVM()->invocationTargetException(exc); \
      } else { \
        th->throwPendingException(); \
      } \
    } \
    
    Typedef* retType = sign->getReturnType();
    if (retType->isPrimitive()) {
      PrimitiveTypedef* prim = (PrimitiveTypedef*)retType;
      if (prim->isVoid()) {
        res = 0;
        uint32 val = 0;
        RUN_METH(Int, val);
      } else if (prim->isBool()) {
        uint32 val = 0;
        RUN_METH(Int, val);
        res = vm->upcalls->boolClass->doNew(vm);
        vm->upcalls->boolValue->setInt8Field(res, val);
      } else if (prim->isByte()) {
        uint32 val = 0;
        RUN_METH(Int, val);
        res = vm->upcalls->byteClass->doNew(vm);
        vm->upcalls->byteValue->setInt8Field(res, val);
      } else if (prim->isChar()) {
        uint32 val = 0;
        RUN_METH(Int, val);
        res = vm->upcalls->charClass->doNew(vm);
        vm->upcalls->charValue->setInt16Field(res, val);
      } else if (prim->isShort()) {
        uint32 val = 0;
        RUN_METH(Int, val);
        res = vm->upcalls->shortClass->doNew(vm);
        vm->upcalls->shortValue->setInt16Field(res, val);
      } else if (prim->isInt()) {
        uint32 val = 0;
        RUN_METH(Int, val);
        res = vm->upcalls->intClass->doNew(vm);
        vm->upcalls->intValue->setInt32Field(res, val);
      } else if (prim->isLong()) {
        sint64 val = 0;
        RUN_METH(Long, val);
        res = vm->upcalls->longClass->doNew(vm);
        vm->upcalls->longValue->setLongField(res, val);
      } else if (prim->isFloat()) {
        float val = 0;
        RUN_METH(Float, val);
        res = vm->upcalls->floatClass->doNew(vm);
        vm->upcalls->floatValue->setFloatField(res, val);
      } else if (prim->isDouble()) {
        double val = 0;
        RUN_METH(Double, val);
        res = vm->upcalls->doubleClass->doNew(vm);
        vm->upcalls->doubleValue->setDoubleField(res, val);
      }
    } else {
      RUN_METH(JavaObject, res);
    } 
  } else {
    vm->illegalArgumentException("wrong number of arguments"); 
  }

  return res;
}

#undef RUN_METH

JNIEXPORT JavaObject* JNICALL Java_java_lang_reflect_Method_invokeNative(
#ifdef NATIVE_JNI
JNIEnv *env, 
#endif
JavaObjectMethod* Meth, JavaObject* obj, JavaArray* args, JavaObject* Cl, jint index) {
  
  JavaObject* res = 0;
  llvm_gcroot(res, 0);
  llvm_gcroot(Meth, 0);
  llvm_gcroot(obj, 0);
  llvm_gcroot(args, 0);
  llvm_gcroot(Cl, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  // Create a new function because we use alloca.
  res = proceedMethod(Meth, obj, args, Cl, index);
  
  END_NATIVE_EXCEPTION

  return res;
}

JNIEXPORT ArrayObject* JNICALL Java_java_lang_reflect_Method_getExceptionTypes(
#ifdef NATIVE_JNI
JNIEnv *env, 
#endif
JavaObjectMethod* Meth) {

  ArrayObject* res = 0;
  llvm_gcroot(Meth, 0);
  llvm_gcroot(res, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  verifyNull(Meth);
  UserClass* cl = Meth->getClass();
  JavaMethod* meth = Meth->getInternalMethod();
  JnjvmClassLoader* loader = cl->classLoader;
  res = meth->getExceptionTypes(loader);

  END_NATIVE_EXCEPTION

  return res;
}

JNIEXPORT JavaObject* JNICALL Java_java_lang_reflect_Method_getSignature(
#ifdef NATIVE_JNI
JNIEnv *env, 
#endif
JavaObjectMethod* Meth) {

  JavaObject* result = 0;
  llvm_gcroot(Meth, 0);
  llvm_gcroot(result, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  verifyNull(Meth);
  JavaMethod* meth = Meth->getInternalMethod();
  Jnjvm* vm = JavaThread::get()->getJVM();
  result = vm->internalUTF8ToStr(meth->type);
  
  END_NATIVE_EXCEPTION

  return result;
}

}
