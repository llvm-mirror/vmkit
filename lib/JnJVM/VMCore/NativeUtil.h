//===------- NativeUtil.h - Methods to call native functions --------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef JNJVM_NATIVE_UTIL_H
#define JNJVM_NATIVE_UTIL_H

#include <jni.h>

namespace jnjvm {

class ArrayObject;
class CommonClass;
class JavaMethod;
class JavaObject;
class Jnjvm;
class JnjvmClassLoader;
class Signdef;
class Typedef;


#define BEGIN_NATIVE_EXCEPTION(level) \
  JavaObject* excp = 0; \
  JavaThread* __th = JavaThread::get(); \
  __th->startNative(level); \
  try {

#define END_NATIVE_EXCEPTION \
  } catch(...) { \
    excp = JavaThread::getJavaException(); \
    JavaThread::clearException(); \
  } \
  if (excp) { \
    __th->pendingException = excp; \
    __th->returnFromNative(); \
  } \
  __th->endNative();

#define BEGIN_JNI_EXCEPTION \
  JavaObject* excp = 0; \
  try {

#define END_JNI_EXCEPTION \
  } catch(...) { \
    excp = JavaThread::getJavaException(); \
    JavaThread::clearException(); \
  } \
  if (excp) { \
    JavaThread* th = JavaThread::get(); \
    th->pendingException = excp; \
    th->returnFromJNI(); \
  }

class NativeUtil {
public:

  static Jnjvm* myVM(JNIEnv* env);
  static intptr_t nativeLookup(CommonClass* cl, JavaMethod* meth, bool& jnjvm);
  static UserCommonClass* resolvedImplClass(Jnjvm* vm, jclass clazz,
                                            bool doClinit);
  static void decapsulePrimitive(Jnjvm *vm, uintptr_t &buf, JavaObject* obj,
                                 const Typedef* signature);

  static JavaObject* getClassType(JnjvmClassLoader* loader, Typedef* type);
  static ArrayObject* getParameterTypes(JnjvmClassLoader* loader, JavaMethod* meth);
  static ArrayObject* getExceptionTypes(UserClass* cl, JavaMethod* meth);

};

}

#endif
