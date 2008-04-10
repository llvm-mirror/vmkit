//===- ClasspathMethod.cpp ------------------------------------------------===//
//===------------- GNU classpath java/lang/reflect/Method -----------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <string.h>

#include "llvm/Type.h"

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

JNIEXPORT jint JNICALL Java_java_lang_reflect_Method_getModifiersInternal(
#ifdef NATIVE_JNI
JNIEnv *env, 
#endif
 jobject Meth) {
  JavaMethod* meth = (JavaMethod*)((*Classpath::methodSlot)((JavaObject*)Meth).IntVal.getZExtValue());
  return meth->access;
}

JNIEXPORT jclass JNICALL Java_java_lang_reflect_Method_getReturnType(
#ifdef NATIVE_JNI
JNIEnv *env, 
#endif
 jobject Meth) {
  JavaMethod* meth = (JavaMethod*)((*Classpath::methodSlot)((JavaObject*)Meth).IntVal.getZExtValue());
  JavaObject* loader = meth->classDef->classLoader;
  return (jclass)NativeUtil::getClassType(loader, meth->signature->ret);
}


JNIEXPORT jobject JNICALL Java_java_lang_reflect_Method_getParameterTypes(
#ifdef NATIVE_JNI
JNIEnv *env, 
#endif

                                                                          jobject Meth) {
  JavaMethod* meth = (JavaMethod*)((*Classpath::methodSlot)((JavaObject*)Meth).IntVal.getZExtValue());
  JavaObject* loader = meth->classDef->classLoader;
  return (jobject)(NativeUtil::getParameterTypes(loader, meth));
}

JNIEXPORT jobject JNICALL Java_java_lang_reflect_Method_invokeNative(
#ifdef NATIVE_JNI
JNIEnv *env, 
#endif
 jobject Meth, jobject _obj, jobject _args, jclass Cl, jint _meth) {

  JavaMethod* meth = (JavaMethod*)_meth;
  ArrayObject* args = (ArrayObject*)_args;
  sint32 nbArgs = args ? args->size : 0;
  sint32 size = meth->signature->args.size();
  Jnjvm* vm = JavaThread::get()->isolate;
  JavaObject* obj = (JavaObject*)_obj;

  void** buf = (void**)alloca(size * sizeof(uint64)); 
  void* _buf = (void*)buf;
  sint32 index = 0;
  if (nbArgs == size) {
    CommonClass* _cl = NativeUtil::resolvedImplClass(Cl, false);
    Class* cl = (Class*)_cl;
    
    if (isVirtual(meth->access)) {
      verifyNull(obj);
      if (!(obj->classOf->isAssignableFrom(meth->classDef))) {
        vm->illegalArgumentExceptionForMethod(meth, meth->classDef, obj->classOf);
      }

    } else {
      cl->initialiseClass();
    }

    
    for (std::vector<Typedef*>::iterator i = meth->signature->args.begin(),
         e = meth->signature->args.end(); i != e; ++i, ++index) {
      NativeUtil::decapsulePrimitive(vm, buf, args->at(index), *i);
    }
    
    JavaObject* exc = 0;

#define RUN_METH(TYPE) \
    try{ \
      if (isVirtual(meth->access)) { \
        if (isPublic(meth->access)) { \
          val = meth->invoke##TYPE##VirtualBuf(obj, _buf); \
        } else { \
          val = meth->invoke##TYPE##SpecialBuf(obj, _buf); \
        } \
      } else { \
        val = meth->invoke##TYPE##StaticBuf(_buf); \
      } \
    }catch(...) { \
      exc = JavaThread::getJavaException(); \
      assert(exc && "no exception?"); \
      JavaThread::clearException(); \
    } \
    \
    if (exc) { \
      if (exc->classOf->isAssignableFrom(Classpath::newException)) { \
        JavaThread::get()->isolate->invocationTargetException(exc); \
      } else { \
        JavaThread::throwException(exc); \
      } \
    } \
    
    JavaObject* res = 0;
    const AssessorDesc* retType = meth->signature->ret->funcs;
    if (retType == AssessorDesc::dVoid) {
      res = 0;
      uint32 val = 0;
      RUN_METH(Int);
    } else if (retType == AssessorDesc::dBool) {
      uint32 val = 0;
      RUN_METH(Int);
      res = (*Classpath::boolClass)(vm);
      (*Classpath::boolValue)(res, val);
    } else if (retType == AssessorDesc::dByte) {
      uint32 val = 0;
      RUN_METH(Int);
      res = (*Classpath::byteClass)(vm);
      (*Classpath::byteValue)(res, val);
    } else if (retType == AssessorDesc::dChar) {
      uint32 val = 0;
      RUN_METH(Int);
      res = (*Classpath::charClass)(vm);
      (*Classpath::charValue)(res, val);
    } else if (retType == AssessorDesc::dShort) {
      uint32 val = 0;
      RUN_METH(Int);
      res = (*Classpath::shortClass)(vm);
      (*Classpath::shortValue)(res, val);
    } else if (retType == AssessorDesc::dInt) {
      uint32 val = 0;
      RUN_METH(Int);
      res = (*Classpath::intClass)(vm);
      (*Classpath::intValue)(res, val);
    } else if (retType == AssessorDesc::dLong) {
      sint64 val = 0;
      RUN_METH(Long);
      res = (*Classpath::longClass)(vm);
      (*Classpath::longValue)(res, val);
    } else if (retType == AssessorDesc::dFloat) {
      float val = 0;
      RUN_METH(Float);
      res = (*Classpath::floatClass)(vm);
      (*Classpath::floatValue)(res, val);
    } else if (retType == AssessorDesc::dDouble) {
      double val = 0;
      RUN_METH(Double);
      res = (*Classpath::doubleClass)(vm);
      (*Classpath::doubleValue)(res, val);
    } else if (retType == AssessorDesc::dTab || retType == AssessorDesc::dRef) {
      JavaObject* val = 0;
      RUN_METH(JavaObject);
      res = val;
    } else {
      vm->unknownError("should not be here");
    }
    return (jobject)res;
  }
  vm->illegalArgumentExceptionForMethod(meth, 0, 0); 
  return 0;
}

#undef RUN_METH

JNIEXPORT jobjectArray JNICALL Java_java_lang_reflect_Method_getExceptionTypes(
#ifdef NATIVE_JNI
JNIEnv *env, 
#endif
 jobject _meth) {
  verifyNull(_meth);
  JavaMethod* meth = (JavaMethod*)(*Classpath::methodSlot)((JavaObject*)_meth).IntVal.getZExtValue();
  return (jobjectArray)NativeUtil::getExceptionTypes(meth);
}

}
