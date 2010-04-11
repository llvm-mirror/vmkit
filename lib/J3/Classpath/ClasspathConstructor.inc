//===- ClasspathConstructor.cpp -------------------------------------------===//
//===----------- GNU classpath java/lang/reflect/Constructor --------------===//
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
#include "JavaThread.h"
#include "JavaTypes.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"
#include "JnjvmClassLoader.h"

using namespace j3;

extern "C" {

JNIEXPORT jobject JNICALL Java_java_lang_reflect_Constructor_getParameterTypes(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
JavaObjectConstructor* cons) {

  JavaObject* res = 0;
  
  llvm_gcroot(cons, 0);
  llvm_gcroot(res, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  verifyNull(cons);
  UserClass* cl = cons->getClass();
  JavaMethod* meth = cons->getInternalMethod();
  JnjvmClassLoader* loader = cl->classLoader;

  res = meth->getParameterTypes(loader);
  END_NATIVE_EXCEPTION

  return (jobject)res;
}

JNIEXPORT jint JNICALL Java_java_lang_reflect_Constructor_getModifiersInternal(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
JavaObjectConstructor* cons) {

  llvm_gcroot(cons, 0);
  jint res = 0;

  BEGIN_NATIVE_EXCEPTION(0)

  verifyNull(cons);
  JavaMethod* meth = cons->getInternalMethod();
  res = meth->access;

  END_NATIVE_EXCEPTION

  return res;
}

static JavaObject* proceedConstructor(JavaObjectConstructor* cons,
                                      JavaArray* args,
                                      JavaObject* Clazz, jint index) 
  __attribute__ ((noinline));
 
static JavaObject* proceedConstructor(JavaObjectConstructor* cons,
                                      JavaArray* args,
                                      JavaObject* Clazz, jint index) {
  JavaObject* res = 0;
  JavaObject* excp = 0;

  llvm_gcroot(cons, 0);
  llvm_gcroot(args, 0);
  llvm_gcroot(Clazz, 0);
  llvm_gcroot(res, 0);
  llvm_gcroot(excp, 0);

  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaMethod* meth = cons->getInternalMethod();
  UserClass* cl = cons->getClass();
  sint32 nbArgs = args ? args->size : 0;
  Signdef* sign = meth->getSignature();
  sint32 size = sign->nbArguments;

  if (isAbstract(cl->access)) vm->instantiationException(cl);

  // Allocate a buffer to store the arguments.
  jvalue* buf = size ? (jvalue*)alloca(size * sizeof(jvalue)) : 0;
  if (size) memset(buf, 0, size * sizeof(jvalue));

  if (nbArgs == size) {
    UserCommonClass* _cl = UserCommonClass::resolvedImplClass(vm, Clazz, false);
    UserClass* cl = _cl->asClass();
    if (cl) {
      cl->initialiseClass(vm);
      res = cl->doNew(vm);
      JavaObject** ptr = (JavaObject**)(void*)(args->elements);
      
      Typedef* const* arguments = sign->getArgumentsType();
      // Store the arguments, unboxing primitives if necessary.
      for (sint32 i = 0; i < size; ++i) {
        ptr[i]->decapsulePrimitive(vm, &buf[i], arguments[i]);
        if (buf[i].l == reinterpret_cast<jobject>(ptr[i])) {
          buf[i].l = reinterpret_cast<jobject>(&ptr[i]);
        }
      }
      
      JavaThread* th = JavaThread::get();
      TRY {
        meth->invokeIntSpecialBuf(vm, cl, res, buf);
      } CATCH {
        excp = th->getJavaException();
        if (excp->getClass()->isAssignableFrom(vm->upcalls->newException)) {
          th->clearException();
          // If it's an exception, we encapsule it in an
          // invocationTargetException
          vm->invocationTargetException(excp);
        } else {
          // If it's an error, throw it again.
          th->throwPendingException();
        }
        return NULL;
      }
    
    } else {
      vm->illegalArgumentException("class is not a regular class");
    }
  } else {
    vm->illegalArgumentException("wrong number of arguments");
  }
 
  return res;
}

JNIEXPORT JavaObject* JNICALL Java_java_lang_reflect_Constructor_constructNative(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
JavaObjectConstructor* cons, JavaArray* args, JavaObject* Clazz, jint index) {

  JavaObject* res = 0;

  llvm_gcroot(res, 0);
  llvm_gcroot(cons, 0);
  llvm_gcroot(args, 0);
  llvm_gcroot(Clazz, 0);
  
  BEGIN_NATIVE_EXCEPTION(0)

  // Proceed in another function because we are using alloca.
  res = proceedConstructor(cons, args, Clazz, index);
  
  END_NATIVE_EXCEPTION
  
  return res;
}

JNIEXPORT 
ArrayObject* JNICALL Java_java_lang_reflect_Constructor_getExceptionTypes(
#ifdef NATIVE_JNI
JNIEnv *env, 
#endif
JavaObjectConstructor* cons) {
  
  ArrayObject* res = 0;

  llvm_gcroot(res, 0);
  llvm_gcroot(cons, 0);

  BEGIN_NATIVE_EXCEPTION(0)
  
  verifyNull(cons);
  UserClass* cl = cons->getClass();
  JavaMethod* meth = cons->getInternalMethod();
  JnjvmClassLoader* loader = cl->classLoader;

  res = (ArrayObject*)meth->getExceptionTypes(loader);

  END_NATIVE_EXCEPTION

  return res;
}

JNIEXPORT JavaObject* JNICALL Java_java_lang_reflect_Constructor_getSignature(
#ifdef NATIVE_JNI
JNIEnv *env, 
#endif
JavaObjectConstructor* Meth) {

  JavaObject* result = 0;

  llvm_gcroot(result, 0);
  llvm_gcroot(Meth, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  verifyNull(Meth);
  JavaMethod* meth = Meth->getInternalMethod();
  Jnjvm* vm = JavaThread::get()->getJVM();
  result = vm->internalUTF8ToStr(meth->type);
  
  END_NATIVE_EXCEPTION

  return result;
}


}
