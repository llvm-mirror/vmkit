//===-------- Classpath.cpp - Configuration for classpath -------------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//



#include "Classpath.h"

#include "ClasspathConstructor.cpp"
#include "ClasspathMethod.cpp"
#include "ClasspathVMClass.cpp"
#include "ClasspathVMClassLoader.cpp"
#include "ClasspathVMField.cpp"
#include "ClasspathVMObject.cpp"
#include "ClasspathVMRuntime.cpp"
#include "ClasspathVMStackWalker.cpp"
#include "ClasspathVMSystem.cpp"
#include "ClasspathVMSystemProperties.cpp"
#include "ClasspathVMThread.cpp"
#include "ClasspathVMThrowable.cpp"

#include "JavaClass.h"
#include "Jnjvm.h"
#include "NativeUtil.h"


// Called by JnJVM to ensure the compiler will link the classpath methods
extern "C" int ClasspathBoot(int argc, char** argv, char** env) {
  return 1;
}

using namespace jnjvm;

extern "C" {
JNIEXPORT bool JNICALL Java_java_io_VMObjectStreamClass_hasClassInitializer(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jclass Cl) {

  CommonClass* cl = NativeUtil::resolvedImplClass(Cl, true);
  if (cl->lookupMethodDontThrow(Jnjvm::clinitName, Jnjvm::clinitType, true, false))
    return true;
  else
    return false;
}

JNIEXPORT bool JNICALL Java_java_util_concurrent_atomic_AtomicLong_VMSupportsCS8(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
) {
  return false;
}

JNIEXPORT bool JNICALL Java_sun_misc_Unsafe_compareAndSwapLong(
#ifdef NATIVE_JNI
    JNIEnv *env, 
#endif
    JavaObject* unsafe, JavaObject* obj, jlong offset, jlong expect, jlong update) {

  jlong *ptr; 
  jlong  value;

  ptr = (jlong *) (((uint8 *) obj) + offset);

  value = *ptr;

  if (value == expect) {
    *ptr = update;
    return true;
  } else {
    return false;
  }

}

}
