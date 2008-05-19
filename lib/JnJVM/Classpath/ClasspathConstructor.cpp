//===- ClasspathConstructor.cpp -------------------------------------------===//
//===----------- GNU classpath java/lang/reflect/Constructor --------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <string.h>

#include "types.h"

#include "JavaArray.h"
#include "JavaClass.h"
#include "JavaObject.h"
#include "JavaTypes.h"
#include "JavaThread.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"
#include "NativeUtil.h"

using namespace jnjvm;

extern "C" {


JNIEXPORT jobject JNICALL Java_java_lang_reflect_Constructor_getParameterTypes(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject cons) {
  JavaMethod* meth = (JavaMethod*)(Classpath::constructorSlot->getVirtualInt32Field((JavaObject*)cons));
  JavaObject* loader = meth->classDef->classLoader;
  return (jobject)(NativeUtil::getParameterTypes(loader, meth));
}

JNIEXPORT jint JNICALL Java_java_lang_reflect_Constructor_getModifiersInternal(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
jobject cons) {
  JavaMethod* meth = (JavaMethod*)(Classpath::constructorSlot->getVirtualInt32Field((JavaObject*)cons));
  return meth->access;
}

JNIEXPORT jobject JNICALL Java_java_lang_reflect_Constructor_constructNative(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
                                                                             jobject _cons,
                                                                             jobject _args, 
                                                                             jclass Clazz, 
                                                                             jint _meth) {
  JavaMethod* meth = (JavaMethod*)_meth;
  ArrayObject* args = (ArrayObject*)_args;
  sint32 nbArgs = args ? args->size : 0;
  sint32 size = meth->signature->args.size();
  Jnjvm* vm = JavaThread::get()->isolate;

  void** buf = (void**)alloca(size * sizeof(uint64));
  void* _buf = (void*)buf;
  sint32 index = 0;
  if (nbArgs == size) {
    CommonClass* _cl = NativeUtil::resolvedImplClass(Clazz, false);
    if (!_cl->isArray) {
      Class* cl = (Class*)_cl;
      cl->initialiseClass();

      JavaObject* res = cl->doNew(vm);
    
      for (std::vector<Typedef*>::iterator i = meth->signature->args.begin(),
           e = meth->signature->args.end(); i != e; ++i, ++index) {
        NativeUtil::decapsulePrimitive(vm, buf, args->at(index), *i);
      }
      
      JavaObject* excp = 0;
      try {
        meth->invokeIntSpecialBuf(vm, res, _buf);
      }catch(...) {
        excp = JavaThread::getJavaException();
        JavaThread::clearException();
      }
      if (excp) {
        if (excp->classOf->isAssignableFrom(Classpath::newException)) {
          JavaThread::get()->isolate->invocationTargetException(excp);
        } else {
          JavaThread::throwException(excp);
        }
      }
    
      return (jobject)res;
    }
  }
  vm->illegalArgumentExceptionForMethod(meth, 0, 0); 
  return 0;
}

JNIEXPORT jobjectArray JNICALL Java_java_lang_reflect_Constructor_getExceptionTypes(
#ifdef NATIVE_JNI
JNIEnv *env, 
#endif
jobject cons) {
  verifyNull(cons);
  JavaMethod* meth = (JavaMethod*)Classpath::constructorSlot->getVirtualInt32Field((JavaObject*)cons);
  return (jobjectArray)NativeUtil::getExceptionTypes(meth);
}

}
