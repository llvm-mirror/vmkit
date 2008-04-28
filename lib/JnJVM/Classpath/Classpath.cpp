//===-------- Classpath.cpp - Configuration for classpath -------------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//



#include "Classpath.h"

#include "ClasspathConstructor.h"
#include "ClasspathMethod.h"
#include "ClasspathVMClass.h"
#include "ClasspathVMClassLoader.h"
#include "ClasspathVMField.h"
#include "ClasspathVMObject.h"
#include "ClasspathVMRuntime.h"
#include "ClasspathVMStackWalker.h"
#include "ClasspathVMSystem.h"
#include "ClasspathVMSystemProperties.h"
#include "ClasspathVMThread.h"
#include "ClasspathVMThrowable.h"

#include "JavaClass.h"
#include "Jnjvm.h"
#include "NativeUtil.h"


typedef void (*function_t) (void);

function_t faketable[] = {
  (function_t)Java_java_lang_VMSystem_arraycopy,
  (function_t)Java_java_lang_VMSystem_identityHashCode,
  (function_t)Java_java_lang_VMThread_currentThread,
  (function_t)Java_java_lang_VMClassLoader_getPrimitiveClass,
  (function_t)Java_java_lang_VMClass_isArray,
  (function_t)Java_java_lang_VMClass_getDeclaredConstructors,
  (function_t)Java_java_lang_VMClass_getDeclaredMethods,
  (function_t)Java_java_lang_VMClass_forName,
  (function_t)Java_java_lang_VMClass_getModifiers,
  (function_t)Java_gnu_classpath_VMSystemProperties_preInit,
  (function_t)Java_java_lang_VMObject_clone,
  (function_t)Java_java_lang_VMObject_getClass,
  (function_t)Java_java_lang_VMRuntime_mapLibraryName,
  (function_t)Java_java_lang_VMRuntime_nativeLoad,
  (function_t)Java_java_lang_reflect_Constructor_getParameterTypes,
  (function_t)Java_java_lang_reflect_Constructor_getModifiersInternal,
  (function_t)Java_java_lang_reflect_Method_getModifiersInternal,
  (function_t)Java_java_lang_reflect_Constructor_constructNative,
  (function_t)Java_java_lang_VMClassLoader_findLoadedClass,
  (function_t)Java_java_lang_VMClassLoader_loadClass,
  (function_t)Java_java_lang_VMClass_getName,
  (function_t)Java_java_lang_VMThrowable_fillInStackTrace,
  (function_t)Java_java_lang_VMClassLoader_defineClass,
  (function_t)Java_java_lang_VMClassLoader_resolveClass,
  (function_t)Java_java_lang_VMClass_isPrimitive,
  (function_t)Java_java_lang_VMClass_isInterface,
  (function_t)Java_java_lang_VMClass_getComponentType,
  (function_t)Java_java_lang_VMRuntime_gc,
  (function_t)Java_java_lang_VMClass_getClassLoader,
  (function_t)Java_java_lang_VMClass_isAssignableFrom,
  (function_t)Java_java_lang_reflect_Field_getModifiersInternal,
  (function_t)Java_gnu_classpath_VMStackWalker_getClassContext
};



extern "C" int ClasspathBoot(int argc, char** argv, char** env) {
  void* p;
  p = &faketable;
  p = &GNUClasspathLibs;
  p = &GNUClasspathGlibj;
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
}

extern "C" {
JNIEXPORT bool JNICALL Java_java_util_concurrent_atomic_AtomicLong_VMSupportsCS8(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
) {
  return false;
}
}
