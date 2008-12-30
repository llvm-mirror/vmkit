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
class JavaMethod;
class JavaObject;
class Jnjvm;
class JnjvmClassLoader;
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

/// NativeUtil - This class groups a set of static function useful when dealing
/// with Java objects in native code.
///
class NativeUtil {
public:

  /// myVM - Get the current virtual machine.
  ///
  static Jnjvm* myVM(JNIEnv* env);

  /// resolvedImplClass - Return the internal representation of the
  /// java.lang.Class object. The class must be resolved.
  //
  static UserCommonClass* resolvedImplClass(Jnjvm* vm, jclass clazz,
                                            bool doClinit);

  /// decapsulePrimitive - Based on the signature argument, decapsule
  /// obj as a primitive and put it in the buffer.
  ///
  static void decapsulePrimitive(Jnjvm *vm, uintptr_t &buf, JavaObject* obj,
                                 const Typedef* signature);

};

}

#endif
