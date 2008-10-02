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


class NativeUtil {
public:

  static Jnjvm* myVM(JNIEnv* env);
  static void* nativeLookup(CommonClass* cl, JavaMethod* meth, bool& jnjvm);
  static UserCommonClass* resolvedImplClass(jclass clazz, bool doClinit);
  static void decapsulePrimitive(Jnjvm *vm, void**&buf, JavaObject* obj,
                                 Typedef* signature);

  static JavaObject* getClassType(JnjvmClassLoader* loader, Typedef* type);
  static ArrayObject* getParameterTypes(JnjvmClassLoader* loader, JavaMethod* meth);
  static ArrayObject* getExceptionTypes(UserClass* cl, JavaMethod* meth);

};

}

#endif
