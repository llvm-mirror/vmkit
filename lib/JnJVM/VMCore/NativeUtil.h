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

#include "llvm/ExecutionEngine/ExecutionEngine.h"


namespace jnjvm {

class ArrayObject;
class CommonClass;
class JavaMethod;
class JavaObject;
class Jnjvm;
class Signdef;
class Typedef;


class NativeUtil {
public:

  static Jnjvm* myVM(JNIEnv* env);
  static void* nativeLookup(CommonClass* cl, JavaMethod* meth, bool& jnjvm);
  static CommonClass* resolvedImplClass(jclass clazz, bool doClinit);
  static void decapsulePrimitive(Jnjvm *vm, void**&buf, JavaObject* obj,
                                 Typedef* signature);

  static JavaObject* getClassType(JavaObject* loader, Typedef* type);
  static ArrayObject* getParameterTypes(JavaObject* loader, JavaMethod* meth);
  static ArrayObject* getExceptionTypes(JavaMethod* meth);

};

}

#endif
