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
  JavaThread* __th = JavaThread::get(); \
  __th->startNative(level); \
  bool __exc = false; \
  try {

#define END_NATIVE_EXCEPTION \
  } catch(...) { \
    __exc = true; \
  } \
  if (__exc) { \
    __th->throwFromNative(); \
  } \
  __th->endNative();

#define BEGIN_JNI_EXCEPTION \
  JavaThread* th = JavaThread::get(); \
  bool __exc = 0; \
  try {

#define END_JNI_EXCEPTION \
  } catch(...) { \
    __exc = true; \
  } \
  if (__exc) { \
    th->throwFromJNI(); \
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
