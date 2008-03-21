//===------------- Jni.cpp - Jni interface for JnJVM ----------------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <jni.h>

#include "llvm/ExecutionEngine/GenericValue.h"

#include "JavaArray.h"
#include "JavaClass.h"
#include "JavaJIT.h"
#include "JavaObject.h"
#include "JavaString.h"
#include "JavaThread.h"
#include "JavaTypes.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"
#include "NativeUtil.h"


using namespace jnjvm;

extern "C" const struct JNIInvokeInterface JNI_JavaVMTable;
extern "C" struct JNINativeInterface JNI_JNIEnvTable;

#define BEGIN_EXCEPTION \
  JavaObject* excp = 0; \
  try {

#define END_EXCEPTION \
  } catch(...) { \
    excp = JavaThread::getJavaException(); \
    JavaThread::clearException(); \
  } \
  if (excp) { \
    JavaThread* th = JavaThread::get(); \
    th->pendingException = excp; \
    th->returnFromNative(); \
  }

/*
static void* ptr_jvm = (JavaVM) &JNI_JavaVMTable;
static void *ptr_env = (void*) &JNI_JNIEnvTable;
*/

jint GetVersion(JNIEnv *env) {
  return JNI_VERSION_1_4;
}


jclass DefineClass(JNIEnv *env, const char *name, jobject loader,
				   const jbyte *buf, jsize bufLen) {
  assert(0 && "implement me");
  return 0;
}


jclass FindClass(JNIEnv *env, const char *asciiz) {
  
  BEGIN_EXCEPTION

  Jnjvm *vm = NativeUtil::myVM(env);
  const UTF8* utf8 = vm->asciizConstructUTF8(asciiz);
  sint32 len = utf8->size;
  
  JavaObject* loader = 0;
  Class* currentClass = JavaJIT::getCallingClass();
  if (currentClass) loader = currentClass->classLoader;

  CommonClass* cl = vm->lookupClassFromUTF8(utf8, 0, len, loader, true, true,
                                            true);
  return (jclass)(cl->getClassDelegatee());
  
  END_EXCEPTION
  return 0;
}
  

jmethodID FromReflectedMethod(JNIEnv *env, jobject method) {
  
  BEGIN_EXCEPTION
  
  JavaObject* meth = (JavaObject*)method;
  CommonClass* cl = meth->classOf;
  if (cl == Classpath::newConstructor)  {
    return (jmethodID)(*Classpath::constructorSlot)(meth).IntVal.getZExtValue();
  } else if (cl == Classpath::newMethod) {
    return (jmethodID)(*Classpath::methodSlot)(meth).IntVal.getZExtValue();
  } else {
    JavaThread::get()->isolate->unknownError(
                  "%s is not a constructor or a method", 
                  meth->printString());
  }
  
  END_EXCEPTION
  
  return 0;
}


jclass GetSuperclass(JNIEnv *env, jclass sub) {
  assert(0 && "implement me");
  return 0;
}
  
 
jboolean IsAssignableFrom(JNIEnv *env, jclass sub, jclass sup) {
  
  BEGIN_EXCEPTION
  
  CommonClass* cl2 = NativeUtil::resolvedImplClass(sup, false);
  CommonClass* cl1 = NativeUtil::resolvedImplClass(sub, false);

  AssessorDesc* prim = AssessorDesc::bogusClassToPrimitive(cl2);
  if (prim)
    return prim == AssessorDesc::bogusClassToPrimitive(cl1);
  else
    return cl1->isAssignableFrom(cl2);
  
  END_EXCEPTION

  return false;
}


jint Throw(JNIEnv *env, jthrowable obj) {
  assert(0 && "implement me");
  return 0;
}


jint ThrowNew(JNIEnv* env, jclass clazz, const char *msg) {
  
  BEGIN_EXCEPTION
  
  JavaThread* th = JavaThread::get();
  Jnjvm* vm = th->isolate;
  CommonClass* cl = NativeUtil::resolvedImplClass(clazz, true);
  if (cl->isArray) assert(0 && "implement me");
  JavaObject* res = ((Class*)cl)->doNew();
  JavaMethod* init =
    cl->lookupMethod(Jnjvm::initName, 
                     vm->asciizConstructUTF8("(Ljava/lang/String;)V"), 0, 1);
  init->invokeIntSpecial(res, vm->asciizToStr(msg));
  th->pendingException = res;
  th->returnFromNative();
  return 1;
  
  END_EXCEPTION

  return 0;
}


jthrowable ExceptionOccurred(JNIEnv *env) {
  return (jthrowable)JavaThread::get()->pendingException;
}


void ExceptionDescribe(JNIEnv *env) {
  assert(0 && "implement me");
}


void ExceptionClear(JNIEnv *env) {
  assert(0 && "implement me");
}


void FatalError(JNIEnv *env, const char *msg) {
  assert(0 && "implement me");
}


jint PushLocalFrame(JNIEnv* env, jint capacity) {
  assert(0 && "implement me");
  return 0;
}

jobject PopLocalFrame(JNIEnv* env, jobject result) {
  assert(0 && "implement me");
  return 0;
}


void DeleteLocalRef(JNIEnv *env, jobject localRef) {
}


jboolean IsSameObject(JNIEnv *env, jobject ref1, jobject ref2) {
  assert(0 && "implement me");
  return 0;
}


jobject NewLocalRef(JNIEnv *env, jobject ref) {
  assert(0 && "implement me");
  return 0;
}


jint EnsureLocalCapacity(JNIEnv* env, jint capacity) {
  assert(0 && "implement me");
  return 0;
}


jobject AllocObject(JNIEnv *env, jclass clazz) {
  
  BEGIN_EXCEPTION
  
  CommonClass* cl = NativeUtil::resolvedImplClass(clazz, true);
  if (cl->isArray) JavaThread::get()->isolate->unknownError("implement me");
  return (jobject)((Class*)cl)->doNew();

  END_EXCEPTION
  return 0;
}


jobject NewObject(JNIEnv *env, jclass clazz, jmethodID methodID, ...) {
  BEGIN_EXCEPTION
    
  va_list ap;
  va_start(ap, methodID);
  JavaMethod* meth = (JavaMethod*)methodID;
  Class* cl = (Class*)NativeUtil::resolvedImplClass(clazz, true);
  JavaObject* res = cl->doNew();
  meth->invokeIntSpecialAP(res, ap);
  va_end(ap);
  return (jobject)res;
  
  END_EXCEPTION
  return 0;
}


jobject NewObjectV(JNIEnv* env, jclass clazz, jmethodID methodID,
                   va_list args) {
  assert(0 && "implement me");
  return 0;
}


jobject NewObjectA(JNIEnv* env, jclass clazz, jmethodID methodID,
                   jvalue *args) {
  assert(0 && "implement me");
  return 0;
}


jclass GetObjectClass(JNIEnv *env, jobject obj) {
  
  BEGIN_EXCEPTION

  verifyNull((JavaObject*)obj);
  return (jclass)((JavaObject*)obj)->classOf->getClassDelegatee();

  END_EXCEPTION
  return 0;
}


jboolean IsInstanceOf(JNIEnv *env, jobject obj, jclass clazz) {
  assert(0 && "implement me");
  return 0;
}


jfieldID FromReflectedField(JNIEnv* env, jobject field) {
  assert(0 && "implement me");
  return 0;
}


jobject ToReflectedMethod(JNIEnv* env, jclass cls, jmethodID methodID,
                          jboolean isStatic) {
  assert(0 && "implement me");
  return 0;
}


jobject ToReflectedField(JNIEnv* env, jclass cls, jfieldID fieldID,
			 jboolean isStatic) {
  assert(0 && "implement me");
  return 0;
}


jmethodID GetMethodID(JNIEnv* env, jclass clazz, const char *aname,
		      const char *atype) {
  
  BEGIN_EXCEPTION

  Jnjvm* vm = NativeUtil::myVM(env);
  CommonClass* cl = NativeUtil::resolvedImplClass(clazz, true);
  const UTF8* name = vm->asciizConstructUTF8(aname);
  const UTF8* type = vm->asciizConstructUTF8(atype);
  JavaMethod* meth = cl->lookupMethod(
      name->javaToInternal(vm, 0, name->size),
      type->javaToInternal(vm, 0, type->size), false, true);

  return (jmethodID)meth;

  END_EXCEPTION
  return 0;
}


jobject CallObjectMethod(JNIEnv *env, jobject obj, jmethodID methodID, ...) {

  BEGIN_EXCEPTION

  va_list ap;
  va_start(ap, methodID);
  JavaObject* self = (JavaObject*)obj;
  JavaMethod* meth = (JavaMethod*)methodID;
  JavaObject* res = meth->invokeJavaObjectVirtualAP(self, ap);
  va_end(ap);
  return (jobject)res;

  END_EXCEPTION
  return 0;
}


jobject CallObjectMethodV(JNIEnv *env, jobject obj, jmethodID methodID,
                          va_list args) {
  assert(0 && "implement me");
  return 0;
}


jobject CallObjectMethodA(JNIEnv *env, jobject obj, jmethodID methodID,
                          jvalue * args) {
  assert(0 && "implement me");
  return 0;
}


jboolean CallBooleanMethod(JNIEnv *env, jobject _obj, jmethodID methodID, ...) {

  BEGIN_EXCEPTION

  va_list ap;
  va_start(ap, methodID);
  JavaObject* self = (JavaObject*)_obj;
  JavaMethod* meth = (JavaMethod*)methodID;
  uint32 res = meth->invokeIntVirtualAP(self, ap);
  va_end(ap);
  return res;

  END_EXCEPTION
  return 0;
}


jboolean CallBooleanMethodV(JNIEnv *env, jobject obj, jmethodID methodID,
                            va_list args) {
  assert(0 && "implement me");
  return 0;
}


jboolean CallBooleanMethodA(JNIEnv *env, jobject obj, jmethodID methodID,
                            jvalue * args) {
  assert(0 && "implement me");
  return 0;
}


jbyte CallByteMethod(JNIEnv *env, jobject obj, jmethodID methodID, ...) {
  assert(0 && "implement me");
  return 0;
}

jbyte CallByteMethodV(JNIEnv *env, jobject obj, jmethodID methodID,
                      va_list args) {
  assert(0 && "implement me");
  return 0;
}


jbyte CallByteMethodA(JNIEnv *env, jobject obj, jmethodID methodID,
                      jvalue *args) {
  assert(0 && "implement me");
  return 0;
}


jchar CallCharMethod(JNIEnv *env, jobject obj, jmethodID methodID, ...) {
  assert(0 && "implement me");
  return 0;
}


jchar CallCharMethodV(JNIEnv *env, jobject obj, jmethodID methodID,
                      va_list args) {
  assert(0 && "implement me");
  return 0;
}


jchar CallCharMethodA(JNIEnv *env, jobject obj, jmethodID methodID,
                      jvalue *args) {
  assert(0 && "implement me");
  return 0;
}


jshort CallShortMethod(JNIEnv *env, jobject obj, jmethodID methodID, ...) {
  assert(0 && "implement me");
  return 0;
}


jshort CallShortMethodV(JNIEnv *env, jobject obj, jmethodID methodID,
                        va_list args) {
  assert(0 && "implement me");
  return 0;
}


jshort CallShortMethodA(JNIEnv *env, jobject obj, jmethodID methodID,
                        jvalue *args) {
  assert(0 && "implement me");
  return 0;
}



jint CallIntMethod(JNIEnv *env, jobject _obj, jmethodID methodID, ...) {

  BEGIN_EXCEPTION

  va_list ap;
  va_start(ap, methodID);
  JavaObject* obj = (JavaObject*)_obj;
  JavaMethod* meth = (JavaMethod*)methodID;
  uint32 res = meth->invokeIntVirtualAP(obj, ap);
  va_end(ap);
  return res;

  END_EXCEPTION
  return 0;
}


jint CallIntMethodV(JNIEnv *env, jobject obj, jmethodID methodID,
                    va_list args) {
  assert(0 && "implement me");
  return 0;
}


jint CallIntMethodA(JNIEnv *env, jobject obj, jmethodID methodID,
                    jvalue *args) {
  assert(0 && "implement me");
  return 0;
}



jlong CallLongMethod(JNIEnv *env, jobject obj, jmethodID methodID, ...) {
  assert(0 && "implement me");
  return 0;
}


jlong CallLongMethodV(JNIEnv *env, jobject obj, jmethodID methodID,
                      va_list args) {
  assert(0 && "implement me");
  return 0;
}


jlong CallLongMethodA(JNIEnv *env, jobject obj, jmethodID methodID,
                      jvalue *args) {
  assert(0 && "implement me");
  return 0;
}



jfloat CallFloatMethod(JNIEnv *env, jobject _obj, jmethodID methodID, ...) {
  
  BEGIN_EXCEPTION

  va_list ap;
  va_start(ap, methodID);
  JavaObject* obj = (JavaObject*)_obj;
  JavaMethod* meth = (JavaMethod*)methodID;
  jfloat res = meth->invokeFloatVirtualAP(obj, ap);
  va_end(ap);
  return res;

  END_EXCEPTION;
  return 0.0;
}


jfloat CallFloatMethodV(JNIEnv *env, jobject _obj, jmethodID methodID,
                        va_list args) {
  assert(0 && "implement me");
  return 0;
}


jfloat CallFloatMethodA(JNIEnv *env, jobject _obj, jmethodID methodID,
                        jvalue *args) {
  assert(0 && "implement me");
  return 0;
}



jdouble CallDoubleMethod(JNIEnv *env, jobject _obj, jmethodID methodID, ...) {
  
  BEGIN_EXCEPTION

  va_list ap;
  va_start(ap, methodID);
  JavaObject* obj = (JavaObject*)_obj;
  JavaMethod* meth = (JavaMethod*)methodID;
  jdouble res = meth->invokeDoubleVirtualAP(obj, ap);
  va_end(ap);
  return res;

  END_EXCEPTION
  return 0.0;
}


jdouble CallDoubleMethodV(JNIEnv *env, jobject _obj, jmethodID methodID,
                          va_list args) {
  
  BEGIN_EXCEPTION
  
  JavaObject* obj = (JavaObject*)_obj;
  JavaMethod* meth = (JavaMethod*)methodID;
  return meth->invokeDoubleVirtualAP(obj, args);

  END_EXCEPTION
  return 0.0;

}


jdouble CallDoubleMethodA(JNIEnv *env, jobject _obj, jmethodID methodID,
                          jvalue *args) {
  assert(0 && "implement me");
  return 0;
}



void CallVoidMethod(JNIEnv *env, jobject _obj, jmethodID methodID, ...) {
  
  BEGIN_EXCEPTION

  va_list ap;
  va_start(ap, methodID);
  JavaObject* obj = (JavaObject*)_obj;
  JavaMethod* meth = (JavaMethod*)methodID;
  meth->invokeIntVirtualAP(obj, ap);
  va_end(ap);

  END_EXCEPTION
}


void CallVoidMethodV(JNIEnv *env, jobject _obj, jmethodID methodID,
                     va_list args) {
  
  BEGIN_EXCEPTION
  
  JavaObject* obj = (JavaObject*)_obj;
  JavaMethod* meth = (JavaMethod*)methodID;
  meth->invokeIntVirtualAP(obj, args);

  END_EXCEPTION
}


void CallVoidMethodA(JNIEnv *env, jobject obj, jmethodID methodID,
                     jvalue *args) {
  assert(0 && "implement me");
}



jobject CallNonvirtualObjectMethod(JNIEnv *env, jobject obj, jclass clazz,
                                   jmethodID methodID, ...) {
  assert(0 && "implement me");
  return 0;
}


jobject CallNonvirtualObjectMethodV(JNIEnv *env, jobject obj, jclass clazz,
                                    jmethodID methodID, va_list args) {
  assert(0 && "implement me");
  return 0;
}


jobject CallNonvirtualObjectMethodA(JNIEnv *env, jobject obj, jclass clazz,
                                    jmethodID methodID, jvalue *args) {
  assert(0 && "implement me");
  return 0;
}



jboolean CallNonvirtualBooleanMethod(JNIEnv *env, jobject obj, jclass clazz,
                                     jmethodID methodID, ...) {
  assert(0 && "implement me");
  return 0;
}


jboolean CallNonvirtualBooleanMethodV(JNIEnv *env, jobject obj, jclass clazz,
                                      jmethodID methodID, va_list args) {
  assert(0 && "implement me");
  return 0;
}


jboolean CallNonvirtualBooleanMethodA(JNIEnv *env, jobject obj, jclass clazz,
                                      jmethodID methodID, jvalue *args) {
  assert(0 && "implement me");
  return 0;
}


jbyte CallNonvirtualByteMethod(JNIEnv *env, jobject obj, jclass clazz,
                               jmethodID methodID, ...) {
  assert(0 && "implement me");
  return 0;
}


jbyte CallNonvirtualByteMethodV (JNIEnv *env, jobject obj, jclass clazz,
                                 jmethodID methodID, va_list args) {
  assert(0 && "implement me");
  return 0;
}


jbyte CallNonvirtualByteMethodA(JNIEnv *env, jobject obj, jclass clazz,
                                jmethodID methodID, jvalue *args) {
  assert(0 && "implement me");
  return 0;
}



jchar CallNonvirtualCharMethod(JNIEnv *env, jobject obj, jclass clazz,
                               jmethodID methodID, ...) {
  assert(0 && "implement me");
  return 0;
}


jchar CallNonvirtualCharMethodV(JNIEnv *env, jobject obj, jclass clazz,
                                jmethodID methodID, va_list args) {
  assert(0 && "implement me");
  return 0;
}


jchar CallNonvirtualCharMethodA(JNIEnv *env, jobject obj, jclass clazz,
                                jmethodID methodID, jvalue *args) {
  assert(0 && "implement me");
  return 0;
}



jshort CallNonvirtualShortMethod(JNIEnv *env, jobject obj, jclass clazz,
                                 jmethodID methodID, ...) {
  assert(0 && "implement me");
  return 0;
}


jshort CallNonvirtualShortMethodV(JNIEnv *env, jobject obj, jclass clazz,
                                  jmethodID methodID, va_list args) {
  assert(0 && "implement me");
  return 0;
}


jshort CallNonvirtualShortMethodA(JNIEnv *env, jobject obj, jclass clazz,
                                  jmethodID methodID, jvalue *args) {
  assert(0 && "implement me");
  return 0;
}



jint CallNonvirtualIntMethod(JNIEnv *env, jobject obj, jclass clazz,
                             jmethodID methodID, ...) {
  assert(0 && "implement me");
  return 0;
}


jint CallNonvirtualIntMethodV(JNIEnv *env, jobject obj, jclass clazz,
                              jmethodID methodID, va_list args) {
  assert(0 && "implement me");
  return 0;
}


jint CallNonvirtualIntMethodA(JNIEnv *env, jobject obj, jclass clazz,
                              jmethodID methodID, jvalue *args) {
  assert(0 && "implement me");
  return 0;
}



jlong CallNonvirtualLongMethod(JNIEnv *env, jobject obj, jclass clazz,
                               jmethodID methodID, ...) {
  assert(0 && "implement me");
  return 0;
}


jlong CallNonvirtualLongMethodV(JNIEnv *env, jobject obj, jclass clazz,
                                jmethodID methodID, va_list args) {
  assert(0 && "implement me");
  return 0;
}


jlong CallNonvirtualLongMethodA(JNIEnv *env, jobject obj, jclass clazz,
                                jmethodID methodID, jvalue *args) {
  assert(0 && "implement me");
  return 0;
}



jfloat CallNonvirtualFloatMethod(JNIEnv *env, jobject obj, jclass clazz,
                                 jmethodID methodID, ...) {
  assert(0 && "implement me");
  return 0;
}


jfloat CallNonvirtualFloatMethodV(JNIEnv *env, jobject obj, jclass clazz,
                                  jmethodID methodID, va_list args) {
  assert(0 && "implement me");
  return 0;
}


jfloat CallNonvirtualFloatMethodA(JNIEnv *env, jobject obj, jclass clazz,
                                  jmethodID methodID, jvalue *args) {
  assert(0 && "implement me");
  return 0;
}



jdouble CallNonvirtualDoubleMethod(JNIEnv *env, jobject obj, jclass clazz,
                                   jmethodID methodID, ...) {
  assert(0 && "implement me");
  return 0;
}


jdouble CallNonvirtualDoubleMethodV(JNIEnv *env, jobject obj, jclass clazz,
                                    jmethodID methodID, va_list args) {
  assert(0 && "implement me");
  return 0;
}


jdouble CallNonvirtualDoubleMethodA(JNIEnv *env, jobject obj, jclass clazz,
                                    jmethodID methodID, jvalue *args) {
  assert(0 && "implement me");
  return 0;
}



void CallNonvirtualVoidMethod(JNIEnv *env, jobject _obj, jclass clazz,
                              jmethodID methodID, ...) {
  
  BEGIN_EXCEPTION

  va_list ap;
  va_start(ap, methodID);
  JavaObject* obj = (JavaObject*)_obj;
  JavaMethod* meth = (JavaMethod*)methodID;
  meth->invokeIntSpecialAP(obj, ap);
  va_end(ap);

  END_EXCEPTION
}


void CallNonvirtualVoidMethodV(JNIEnv *env, jobject obj, jclass clazz,
                               jmethodID methodID, va_list args) {
  assert(0 && "implement me");
}


void CallNonvirtualVoidMethodA(JNIEnv *env, jobject obj, jclass clazz,
                               jmethodID methodID, jvalue * args) {
  assert(0 && "implement me");
}


jfieldID GetFieldID(JNIEnv *env, jclass clazz, const char *name,
		    const char *sig)  {

  BEGIN_EXCEPTION

  Jnjvm* vm = NativeUtil::myVM(env);
  return (jfieldID)
    NativeUtil::resolvedImplClass(clazz, true)->lookupField(
                      vm->asciizConstructUTF8(name),
                      vm->asciizConstructUTF8(sig), 0, 1);
  
  END_EXCEPTION
  return 0;

}


jobject GetObjectField(JNIEnv *env, jobject obj, jfieldID fieldID) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  JavaObject* o = (JavaObject*)obj;
  return (jobject)((*field)(o).PointerVal);

  END_EXCEPTION
  return 0;
}


jboolean GetBooleanField(JNIEnv *env, jobject obj, jfieldID fieldID) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  JavaObject* o = (JavaObject*)obj;
  return (uint8)((*field)(o).IntVal.getZExtValue());

  END_EXCEPTION
  return 0;
}


jbyte GetByteField(JNIEnv *env, jobject obj, jfieldID fieldID) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  JavaObject* o = (JavaObject*)obj;
  return (sint8)((*field)(o).IntVal.getSExtValue());

  END_EXCEPTION
  return 0;
}


jchar GetCharField(JNIEnv *env, jobject obj, jfieldID fieldID) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  JavaObject* o = (JavaObject*)obj;
  return (uint16)((*field)(o).IntVal.getZExtValue());

  END_EXCEPTION
  return 0;
}


jshort GetShortField(JNIEnv *env, jobject obj, jfieldID fieldID) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  JavaObject* o = (JavaObject*)obj;
  return (sint16)((*field)(o).IntVal.getSExtValue());

  END_EXCEPTION
  return 0;
}


jint GetIntField(JNIEnv *env, jobject obj, jfieldID fieldID) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  JavaObject* o = (JavaObject*)obj;
  return (sint32)((*field)(o).IntVal.getSExtValue());

  END_EXCEPTION
  return 0;
}


jlong GetLongField(JNIEnv *env, jobject obj, jfieldID fieldID) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  JavaObject* o = (JavaObject*)obj;
  return (sint64)((*field)(o).IntVal.getSExtValue());

  END_EXCEPTION
  return 0;
}


jfloat GetFloatField(JNIEnv *env, jobject obj, jfieldID fieldID) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  JavaObject* o = (JavaObject*)obj;
  return (*field)(o).FloatVal;

  END_EXCEPTION
  return 0;
}


jdouble GetDoubleField(JNIEnv *env, jobject obj, jfieldID fieldID) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  JavaObject* o = (JavaObject*)obj;
  return (*field)(o).DoubleVal;

  END_EXCEPTION
  return 0;
}


void SetObjectField(JNIEnv *env, jobject obj, jfieldID fieldID, jobject value) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  JavaObject* o = (JavaObject*)obj;
  (*field)(o, (JavaObject*)value);

  END_EXCEPTION
}


void SetBooleanField(JNIEnv *env, jobject obj, jfieldID fieldID,
                     jboolean value) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  JavaObject* o = (JavaObject*)obj;
  (*field)(o, (uint32)value);

  END_EXCEPTION
}


void SetByteField(JNIEnv *env, jobject obj, jfieldID fieldID, jbyte value) {

  BEGIN_EXCEPTION
  
  JavaField* field = (JavaField*)fieldID;
  JavaObject* o = (JavaObject*)obj;
  (*field)(o, (uint32)value);

  END_EXCEPTION
}


void SetCharField(JNIEnv *env, jobject obj, jfieldID fieldID, jchar value) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  JavaObject* o = (JavaObject*)obj;
  (*field)(o, (uint32)value);
  
  END_EXCEPTION
}


void SetShortField(JNIEnv *env, jobject obj, jfieldID fieldID, jshort value) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  JavaObject* o = (JavaObject*)obj;
  (*field)(o, (uint32)value);

  END_EXCEPTION
}


void SetIntField(JNIEnv *env, jobject obj, jfieldID fieldID, jint value) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  JavaObject* o = (JavaObject*)obj;
  (*field)(o, (uint32)value);

  END_EXCEPTION
}


void SetLongField(JNIEnv *env, jobject obj, jfieldID fieldID, jlong value) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  JavaObject* o = (JavaObject*)obj;
  (*field)(o, (sint64)value);

  END_EXCEPTION
}


void SetFloatField(JNIEnv *env, jobject obj, jfieldID fieldID, jfloat value) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  JavaObject* o = (JavaObject*)obj;
  (*field)(o, (float)value);

  END_EXCEPTION
}


void SetDoubleField(JNIEnv *env, jobject obj, jfieldID fieldID, jdouble value) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  JavaObject* o = (JavaObject*)obj;
  (*field)(o, (double)value);

  END_EXCEPTION
}


jmethodID GetStaticMethodID(JNIEnv *env, jclass clazz, const char *aname,
			    const char *atype) {

  BEGIN_EXCEPTION

  Jnjvm* vm = NativeUtil::myVM(env);
  CommonClass* cl = NativeUtil::resolvedImplClass(clazz, true);
  const UTF8* name = vm->asciizConstructUTF8(aname);
  const UTF8* type = vm->asciizConstructUTF8(atype);
  JavaMethod* meth = cl->lookupMethod(
      name->javaToInternal(vm, 0, name->size),
      type->javaToInternal(vm, 0, type->size), true, true);

  return (jmethodID)meth;

  END_EXCEPTION
  return 0;
}


jobject CallStaticObjectMethod(JNIEnv *env, jclass clazz, jmethodID methodID, ...) {
  assert(0 && "implement me");
  return 0;
}


jobject CallStaticObjectMethodV(JNIEnv *env, jclass clazz, jmethodID methodID,
                                va_list args) {
  assert(0 && "implement me");
  return 0;
}


jobject CallStaticObjectMethodA(JNIEnv *env, jclass clazz, jmethodID methodID,
                                jvalue *args) {
  assert(0 && "implement me");
  return 0;
}


jboolean CallStaticBooleanMethod(JNIEnv *env, jclass clazz, jmethodID methodID,
                                 ...) {
  
  BEGIN_EXCEPTION
  
  va_list ap;
  va_start(ap, methodID);
  JavaMethod* meth = (JavaMethod*)methodID;
  uint32 res = meth->invokeIntStaticAP(ap);
  va_end(ap);
  return res;

  END_EXCEPTION
  return 0;
}


jboolean CallStaticBooleanMethodV(JNIEnv *env, jclass clazz, jmethodID methodID,
                                  va_list args) {
  assert(0 && "implement me");
  return 0;
}


jboolean CallStaticBooleanMethodA(JNIEnv *env, jclass clazz, jmethodID methodID,
                                  jvalue *args) {
  assert(0 && "implement me");
  return 0;
}


jbyte CallStaticByteMethod(JNIEnv *env, jclass clazz, jmethodID methodID, ...) {
  assert(0 && "implement me");
  return 0;
}


jbyte CallStaticByteMethodV(JNIEnv *env, jclass clazz, jmethodID methodID,
                            va_list args) {
  assert(0 && "implement me");
  return 0;
}


jbyte CallStaticByteMethodA(JNIEnv *env, jclass clazz, jmethodID methodID,
                            jvalue *args) {
  assert(0 && "implement me");
  return 0;
}


jchar CallStaticCharMethod(JNIEnv *env, jclass clazz, jmethodID methodID, ...) {
  assert(0 && "implement me");
  return 0;
}


jchar CallStaticCharMethodV(JNIEnv *env, jclass clazz, jmethodID methodID,
                            va_list args) {
  assert(0 && "implement me");
  return 0;
}


jchar CallStaticCharMethodA(JNIEnv *env, jclass clazz, jmethodID methodID,
                            jvalue *args) {
  assert(0 && "implement me");
  return 0;
}


jshort CallStaticShortMethod(JNIEnv *env, jclass clazz, jmethodID methodID,
                             ...) {
  assert(0 && "implement me");
  return 0;
}


jshort CallStaticShortMethodV(JNIEnv *env, jclass clazz, jmethodID methodID,
                              va_list args) {
  assert(0 && "implement me");
  return 0;
}


jshort CallStaticShortMethodA(JNIEnv *env, jclass clazz, jmethodID methodID,
                              jvalue *args) {
  assert(0 && "implement me");
  return 0;
}


jint CallStaticIntMethod(JNIEnv *env, jclass clazz, jmethodID methodID, ...) {
  assert(0 && "implement me");
  return 0;
}


jint CallStaticIntMethodV(JNIEnv *env, jclass clazz, jmethodID methodID,
                          va_list args) {
  assert(0 && "implement me");
  return 0;
}


jint CallStaticIntMethodA(JNIEnv *env, jclass clazz, jmethodID methodID,
                          jvalue *args) {
  assert(0 && "implement me");
  return 0;
}


jlong CallStaticLongMethod(JNIEnv *env, jclass clazz, jmethodID methodID, ...) {
  assert(0 && "implement me");
  return 0;
}


jlong CallStaticLongMethodV(JNIEnv *env, jclass clazz, jmethodID methodID,
			    va_list args) {
  assert(0 && "implement me");
  return 0;
}


jlong CallStaticLongMethodA(JNIEnv *env, jclass clazz, jmethodID methodID,
                            jvalue *args) {
  assert(0 && "implement me");
  return 0;
}



jfloat CallStaticFloatMethod(JNIEnv *env, jclass clazz, jmethodID methodID,
                             ...) {
  assert(0 && "implement me");
  return 0;
}


jfloat CallStaticFloatMethodV(JNIEnv *env, jclass clazz, jmethodID methodID,
                              va_list args) {
  assert(0 && "implement me");
  return 0;
}


jfloat CallStaticFloatMethodA(JNIEnv *env, jclass clazz, jmethodID methodID,
                              jvalue *args) {
  assert(0 && "implement me");
  return 0;
}


jdouble CallStaticDoubleMethod(JNIEnv *env, jclass clazz, jmethodID methodID,
                               ...) {
  assert(0 && "implement me");
  return 0;
}


jdouble CallStaticDoubleMethodV(JNIEnv *env, jclass clazz, jmethodID methodID,
                                va_list args) {
  assert(0 && "implement me");
  return 0;
}


jdouble CallStaticDoubleMethodA(JNIEnv *env, jclass clazz, jmethodID methodID,
                                jvalue *args) {
  assert(0 && "implement me");
  return 0;
}


void CallStaticVoidMethod(JNIEnv *env, jclass clazz, jmethodID methodID, ...) {
  
  BEGIN_EXCEPTION

  va_list ap;
  va_start(ap, methodID);
  JavaMethod* meth = (JavaMethod*)methodID;
  meth->invokeIntStaticAP(ap);
  va_end(ap);

  END_EXCEPTION
}


void CallStaticVoidMethodV(JNIEnv *env, jclass clazz, jmethodID methodID,
                           va_list args) {
  
  BEGIN_EXCEPTION
  
  JavaMethod* meth = (JavaMethod*)methodID;
  meth->invokeIntStaticAP(args);

  END_EXCEPTION
}


void CallStaticVoidMethodA(JNIEnv *env, jclass clazz, jmethodID methodID,
                           jvalue * args) {
  assert(0 && "implement me");
}


jfieldID GetStaticFieldID(JNIEnv *env, jclass clazz, const char *name,
                          const char *sig) {
  
  BEGIN_EXCEPTION
  
  Jnjvm* vm = NativeUtil::myVM(env);
  return (jfieldID)
    NativeUtil::resolvedImplClass(clazz, true)->lookupField(
                      vm->asciizConstructUTF8(name),
                      vm->asciizConstructUTF8(sig), true, true);

  END_EXCEPTION
  return 0;
}


jobject GetStaticObjectField(JNIEnv *env, jclass clazz, jfieldID fieldID) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  llvm::GenericValue res = (*field)();
  return (jobject)res.PointerVal;

  END_EXCEPTION
  return 0;
}


jboolean GetStaticBooleanField(JNIEnv *env, jclass clazz, jfieldID fieldID) {
  
  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  llvm::GenericValue res = (*field)();
  return (jboolean)res.IntVal.getZExtValue();

  END_EXCEPTION
  return 0;
}


jbyte GetStaticByteField(JNIEnv *env, jclass clazz, jfieldID fieldID) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  llvm::GenericValue res = (*field)();
  return (jbyte)res.IntVal.getSExtValue();

  END_EXCEPTION
  return 0;
}


jchar GetStaticCharField(JNIEnv *env, jclass clazz, jfieldID fieldID) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  llvm::GenericValue res = (*field)();
  return (jchar)res.IntVal.getZExtValue();

  END_EXCEPTION
  return 0;
}


jshort GetStaticShortField(JNIEnv *env, jclass clazz, jfieldID fieldID) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  llvm::GenericValue res = (*field)();
  return (jshort)res.IntVal.getSExtValue();

  END_EXCEPTION
  return 0;
}


jint GetStaticIntField(JNIEnv *env, jclass clazz, jfieldID fieldID) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  llvm::GenericValue res = (*field)();
  return (jint)res.IntVal.getSExtValue();

  END_EXCEPTION
  return 0;
}


jlong GetStaticLongField(JNIEnv *env, jclass clazz, jfieldID fieldID) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  llvm::GenericValue res = (*field)();
  return (jlong)res.IntVal.getZExtValue();

  END_EXCEPTION
  return 0;
}


jfloat GetStaticFloatField(JNIEnv *env, jclass clazz, jfieldID fieldID) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  llvm::GenericValue res = (*field)();
  return (jfloat)res.FloatVal;

  END_EXCEPTION
  return 0;
}


jdouble GetStaticDoubleField(JNIEnv *env, jclass clazz, jfieldID fieldID) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  llvm::GenericValue res = (*field)();
  return (jdouble)res.DoubleVal;

  END_EXCEPTION
  return 0;
}


void SetStaticObjectField(JNIEnv *env, jclass clazz, jfieldID fieldID,
                          jobject value) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  (*field)((JavaObject*)value);
  
  END_EXCEPTION
}


void SetStaticBooleanField(JNIEnv *env, jclass clazz, jfieldID fieldID,
                           jboolean value) {
  
  BEGIN_EXCEPTION
  
  JavaField* field = (JavaField*)fieldID;
  (*field)((uint32)value);

  END_EXCEPTION
}


void SetStaticByteField(JNIEnv *env, jclass clazz, jfieldID fieldID,
                        jbyte value) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  (*field)((uint32)value);

  END_EXCEPTION
}


void SetStaticCharField(JNIEnv *env, jclass clazz, jfieldID fieldID,
                        jchar value) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  (*field)((uint32)value);

  END_EXCEPTION
}


void SetStaticShortField(JNIEnv *env, jclass clazz, jfieldID fieldID,
                         jshort value) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  (*field)((uint32)value);

  END_EXCEPTION
}


void SetStaticIntField(JNIEnv *env, jclass clazz, jfieldID fieldID,
                       jint value) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  (*field)((uint32)value);

  END_EXCEPTION
}


void SetStaticLongField(JNIEnv *env, jclass clazz, jfieldID fieldID,
                        jlong value) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  (*field)((sint64)value);

  END_EXCEPTION
}


void SetStaticFloatField(JNIEnv *env, jclass clazz, jfieldID fieldID,
                         jfloat value) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  (*field)((float)value);

  END_EXCEPTION
}


void SetStaticDoubleField(JNIEnv *env, jclass clazz, jfieldID fieldID,
                          jdouble value) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  (*field)((double)value);

  END_EXCEPTION
}


jstring NewString(JNIEnv *env, const jchar *buf, jsize len) {
  assert(0 && "implement me");
  return 0;
}


jsize GetStringLength(JNIEnv *env, jstring str) {
  assert(0 && "implement me");
  return 0;
}


const jchar *GetStringChars(JNIEnv *env, jstring str, jboolean *isCopy) {
  assert(0 && "implement me");
  return 0;
}


void ReleaseStringChars(JNIEnv *env, jstring str, const jchar *chars) {
  assert(0 && "implement me");
}


jstring NewStringUTF(JNIEnv *env, const char *bytes) {

  BEGIN_EXCEPTION

  Jnjvm* vm = NativeUtil::myVM(env);
  return (jstring)(vm->asciizToStr(bytes));

  END_EXCEPTION
  return 0;
}


jsize GetStringUTFLength (JNIEnv *env, jstring string) {   
  assert(0 && "implement me");
  return 0;
}


const char *GetStringUTFChars(JNIEnv *env, jstring string, jboolean *isCopy) {

  BEGIN_EXCEPTION

  if (isCopy != 0) (*isCopy) = true;
  return strdup(((JavaString*)string)->strToAsciiz());

  END_EXCEPTION
  return 0;
}


void ReleaseStringUTFChars(JNIEnv *env, jstring string, const char *utf) {
  free((void*)utf);
}


jsize GetArrayLength(JNIEnv *env, jarray array) {

  BEGIN_EXCEPTION

  return ((JavaArray*)array)->size;

  END_EXCEPTION
  return 0;
}


jobjectArray NewObjectArray(JNIEnv *env, jsize length, jclass elementClass,
                            jobject initialElement) {
  BEGIN_EXCEPTION
  Jnjvm* vm = NativeUtil::myVM(env);
  if (length < 0) vm->negativeArraySizeException(length);
  
  CommonClass* base = NativeUtil::resolvedImplClass(elementClass, true);
  JavaObject* loader = base->classLoader;
  const UTF8* name = base->name;
  const UTF8* arrayName = AssessorDesc::constructArrayName(vm, 0, 1, name);
  ClassArray* array = vm->constructArray(arrayName, loader);
  ArrayObject* res = ArrayObject::acons(length, array);
  if (initialElement) {
    memset(res->elements, (int)initialElement, 
               length * sizeof(JavaObject*));
  }
  return (jobjectArray)res;
  END_EXCEPTION
  return 0;
}


jobject GetObjectArrayElement(JNIEnv *env, jobjectArray array, jsize index) {
  assert(0 && "implement me");
  return 0;
}


void SetObjectArrayElement(JNIEnv *env, jobjectArray array, jsize index,
                           jobject val) {
  
  BEGIN_EXCEPTION
  
  ((ArrayObject*)array)->setAt(index, (JavaObject*)val);

  END_EXCEPTION
}


jbooleanArray NewBooleanArray(JNIEnv *env, jsize len) {
  
  BEGIN_EXCEPTION
  
  ArrayUInt8* res = 0;
  res = ArrayUInt8::acons(len, JavaArray::ofBool);
  return (jbooleanArray)res;

  END_EXCEPTION
  return 0;
}


jbyteArray NewByteArray(JNIEnv *env, jsize len) {
  
  BEGIN_EXCEPTION

  ArraySInt8* res = 0;
  res = ArraySInt8::acons(len, JavaArray::ofByte);
  return (jbyteArray) res;

  END_EXCEPTION
  return 0;
}


jcharArray NewCharArray(JNIEnv *env, jsize len) {
  
  BEGIN_EXCEPTION
  
  ArrayUInt16* res = 0;
  res = ArrayUInt16::acons(len, JavaArray::ofChar);
  return (jcharArray) res;

  END_EXCEPTION
  return 0;
}


jshortArray NewShortArray(JNIEnv *env, jsize len) {
  
  BEGIN_EXCEPTION
  
  ArraySInt16* res = 0;
  res = ArraySInt16::acons(len, JavaArray::ofShort);
  return (jshortArray) res;

  END_EXCEPTION
  return 0;
}


jintArray NewIntArray(JNIEnv *env, jsize len) {
  
  BEGIN_EXCEPTION
  
  ArraySInt32* res = 0;
  res = ArraySInt32::acons(len, JavaArray::ofInt);
  return (jintArray) res;

  END_EXCEPTION
  return 0;
}


jlongArray NewLongArray(JNIEnv *env, jsize len) {
  
  BEGIN_EXCEPTION
  
  ArrayLong* res = 0;
  res = ArrayLong::acons(len, JavaArray::ofLong);
  return (jlongArray) res;

  END_EXCEPTION
  return 0;
}


jfloatArray NewFloatArray(JNIEnv *env, jsize len) {
  
  BEGIN_EXCEPTION
  
  ArrayFloat* res = 0;
  res = ArrayFloat::acons(len, JavaArray::ofFloat);
  return (jfloatArray) res;

  END_EXCEPTION
  return 0;
}


jdoubleArray NewDoubleArray(JNIEnv *env, jsize len) {
  
  BEGIN_EXCEPTION
  
  ArrayDouble* res = 0;
  res = ArrayDouble::acons(len, JavaArray::ofDouble);
  return (jdoubleArray) res;

  END_EXCEPTION
  return 0;
}


jboolean *GetBooleanArrayElements(JNIEnv *env, jbooleanArray array,
				  jboolean *isCopy) {
  
  BEGIN_EXCEPTION
  
  if (isCopy) (*isCopy) = false;
  return (jboolean*)((ArrayUInt8*)array)->elements;

  END_EXCEPTION
  return 0;
}


jbyte *GetByteArrayElements(JNIEnv *env, jbyteArray array, jboolean *isCopy) {

  BEGIN_EXCEPTION

  if (isCopy) (*isCopy) = false;
  return ((ArraySInt8*)array)->elements;

  END_EXCEPTION
  return 0;
}


jchar *GetCharArrayElements(JNIEnv *env, jcharArray array, jboolean *isCopy) {

  BEGIN_EXCEPTION

  if (isCopy) (*isCopy) = false;
  return ((ArrayUInt16*)array)->elements;

  END_EXCEPTION
  return 0;
}


jshort *GetShortArrayElements(JNIEnv *env, jshortArray array,
                              jboolean *isCopy) {
  
  BEGIN_EXCEPTION
  
  if (isCopy) (*isCopy) = false;
  return ((ArraySInt16*)array)->elements;

  END_EXCEPTION
  return 0;
}


jint *GetIntArrayElements(JNIEnv *env, jintArray array, jboolean *isCopy) {

  BEGIN_EXCEPTION

  if (isCopy) (*isCopy) = false;
  return ((ArraySInt32*)array)->elements;

  END_EXCEPTION
  return 0;
}


jlong *GetLongArrayElements(JNIEnv *env, jlongArray array, jboolean *isCopy) {

  BEGIN_EXCEPTION

  if (isCopy) (*isCopy) = false;
  return ((ArrayLong*)array)->elements;

  END_EXCEPTION
  return 0;
}


jfloat *GetFloatArrayElements(JNIEnv *env, jfloatArray array,
                              jboolean *isCopy) {

  BEGIN_EXCEPTION

  if (isCopy) (*isCopy) = false;
  return ((ArrayFloat*)array)->elements;

  END_EXCEPTION
  return 0;
}


jdouble *GetDoubleArrayElements(JNIEnv *env, jdoubleArray array,
				jboolean *isCopy) {
  
  BEGIN_EXCEPTION
  
  if (isCopy) (*isCopy) = false;
  return ((ArrayDouble*)array)->elements;

  END_EXCEPTION
  return 0;
}


void ReleaseBooleanArrayElements(JNIEnv *env, jbooleanArray array,
				 jboolean *elems, jint mode) {
}


void ReleaseByteArrayElements(JNIEnv *env, jbyteArray array, jbyte *elems,
			      jint mode) {
}


void ReleaseCharArrayElements(JNIEnv *env, jcharArray array, jchar *elems,
			      jint mode) {
}


void ReleaseShortArrayElements(JNIEnv *env, jshortArray array, jshort *elems,
			       jint mode) {
}


void ReleaseIntArrayElements(JNIEnv *env, jintArray array, jint *elems,
			     jint mode) {
}


void ReleaseLongArrayElements(JNIEnv *env, jlongArray array, jlong *elems,
			      jint mode) {
}


void ReleaseFloatArrayElements(JNIEnv *env, jfloatArray array, jfloat *elems,
			       jint mode) {
}


void ReleaseDoubleArrayElements(JNIEnv *env, jdoubleArray array,
				jdouble *elems, jint mode) {
}


void GetBooleanArrayRegion(JNIEnv *env, jbooleanArray array, jsize start,
			   jsize len, jboolean *buf) {
  assert(0 && "implement me");
}


void GetByteArrayRegion(JNIEnv *env, jbyteArray array, jsize start, jsize len,
			jbyte *buf) {
  assert(0 && "implement me");
}


void GetCharArrayRegion(JNIEnv *env, jcharArray array, jsize start, jsize len,
			jchar *buf) {
  assert(0 && "implement me");
}


void GetShortArrayRegion(JNIEnv *env, jshortArray array, jsize start,
			 jsize len, jshort *buf) {
  assert(0 && "implement me");
}


void GetIntArrayRegion(JNIEnv *env, jintArray array, jsize start, jsize len,
		       jint *buf) {
  assert(0 && "implement me");
}


void GetLongArrayRegion(JNIEnv *env, jlongArray array, jsize start, jsize len,
		        jlong *buf) {
  assert(0 && "implement me");
}


void GetFloatArrayRegion(JNIEnv *env, jfloatArray array, jsize start,
			 jsize len, jfloat *buf) {
  assert(0 && "implement me");
}


void GetDoubleArrayRegion(JNIEnv *env, jdoubleArray array, jsize start,
			  jsize len, jdouble *buf) {
  assert(0 && "implement me");
}


void SetBooleanArrayRegion(JNIEnv *env, jbooleanArray array, jsize start,
			   jsize len, jboolean *buf) {
  assert(0 && "implement me");
}


void SetByteArrayRegion(JNIEnv *env, jbyteArray array, jsize start, jsize len,
			jbyte *buf) {
  assert(0 && "implement me");
}


void SetCharArrayRegion(JNIEnv *env, jcharArray array, jsize start, jsize len,
			jchar *buf) {
  assert(0 && "implement me");
}


void SetShortArrayRegion(JNIEnv *env, jshortArray array, jsize start,
			 jsize len, jshort *buf) {
  assert(0 && "implement me");
}


void SetIntArrayRegion(JNIEnv *env, jintArray array, jsize start, jsize len,
		       jint *buf) {
  assert(0 && "implement me");
}


void SetLongArrayRegion(JNIEnv* env, jlongArray array, jsize start, jsize len,
			jlong *buf) {
  assert(0 && "implement me");
}


void SetFloatArrayRegion(JNIEnv *env, jfloatArray array, jsize start,
			 jsize len, jfloat *buf) {
  assert(0 && "implement me");
}


void SetDoubleArrayRegion(JNIEnv *env, jdoubleArray array, jsize start,
			  jsize len, jdouble *buf) {
  assert(0 && "implement me");
}


jint RegisterNatives(JNIEnv *env, jclass clazz, const JNINativeMethod *methods,
		     jint nMethods) {
  assert(0 && "implement me");
  return 0;
}


jint UnregisterNatives(JNIEnv *env, jclass clazz) {
  assert(0 && "implement me");
  return 0;
}


jint MonitorEnter(JNIEnv *env, jobject obj) {
  
  BEGIN_EXCEPTION
  
  ((JavaObject*)obj)->aquire();
  return 1;

  END_EXCEPTION
  return 0;
}


jint MonitorExit(JNIEnv *env, jobject obj) {

  BEGIN_EXCEPTION

  ((JavaObject*)obj)->unlock();
  return 1;

  END_EXCEPTION
  return 0;
}


jint GetJavaVM(JNIEnv *env, JavaVM **vm) {
  BEGIN_EXCEPTION
  Jnjvm* myvm = JavaThread::get()->isolate;
  (*vm) = (JavaVM*)(void*)(&(myvm->javavmEnv));
  return 0;
  END_EXCEPTION
  return 0;
}


void GetStringRegion(JNIEnv* env, jstring str, jsize start, jsize len,
                     jchar *buf) {
  assert(0 && "implement me");
}


void GetStringUTFRegion(JNIEnv* env, jstring str, jsize start, jsize len,
                        char *buf) {
  assert(0 && "implement me");
}


void *GetPrimitiveArrayCritical(JNIEnv *env, jarray array, jboolean *isCopy) {
  assert(0 && "implement me");
  return 0;
}


void ReleasePrimitiveArrayCritical(JNIEnv *env, jarray array, void *carray,
				   jint mode) {
  assert(0 && "implement me");
}


const jchar *GetStringCritical(JNIEnv *env, jstring string, jboolean *isCopy) {
  assert(0 && "implement me");
  return 0;
}


void ReleaseStringCritical(JNIEnv *env, jstring string, const jchar *cstring) {
  assert(0 && "implement me");
}


jweak NewWeakGlobalRef(JNIEnv* env, jobject obj) {
  assert(0 && "implement me");
  return 0;
}


void DeleteWeakGlobalRef(JNIEnv* env, jweak ref) {
  assert(0 && "implement me");
}


jobject NewGlobalRef(JNIEnv* env, jobject obj) {
  Jnjvm* vm = NativeUtil::myVM(env);
  vm->globalRefsLock->lock();
  vm->globalRefs.push_back((JavaObject*)obj);
  vm->globalRefsLock->unlock();
  return obj;
}


void DeleteGlobalRef(JNIEnv* env, jobject globalRef) {
  Jnjvm* vm = NativeUtil::myVM(env);
  vm->globalRefsLock->lock();
  for (std::vector<JavaObject*>::iterator i = vm->globalRefs.begin(), 
            e = vm->globalRefs.end(); i!= e; ++i) {
    if ((*i) == (JavaObject*)globalRef) {
      vm->globalRefs.erase(i);
      break;
    }
  }
  vm->globalRefsLock->unlock();
}


jboolean ExceptionCheck(JNIEnv *env) {
  assert(0 && "implement me");
  return 0;
}


jobject NewDirectByteBuffer(JNIEnv *env, void *address, jlong capacity) {
  assert(0 && "implement me");
  return 0;
}


void *GetDirectBufferAddress(JNIEnv *env, jobject _buf) {

  BEGIN_EXCEPTION

  JavaObject* buf = (JavaObject*)_buf;
  JavaObject* address = (JavaObject*)((*Classpath::bufferAddress)(buf).PointerVal);
  if (address != 0) {
    int res = (*Classpath::dataPointer32)(address).IntVal.getZExtValue();
    return (void*)res;
  } else {
    return 0;
  }

  END_EXCEPTION
  return 0;
}


jlong GetDirectBufferCapacity(JNIEnv* env, jobject buf) {
  assert(0 && "implement me");
  return 0;
}


jint DestroyJavaVM(JavaVM *vm) {
  assert(0 && "implement me");
  return 0;
}


jint AttachCurrentThread(JavaVM *vm, void **env, void *thr_args) {
  assert(0 && "implement me");
  return 0;
}


jint DetachCurrentThread(JavaVM *vm) {
  assert(0 && "implement me");
  return 0;
}


jint GetEnv(JavaVM *vm, void **env, jint version) {

  BEGIN_EXCEPTION

  JavaObject* th = JavaThread::currentThread();
  Jnjvm* myvm = JavaThread::get()->isolate;
  if (th != 0) {
    (*env) = &(myvm->jniEnv);
    return JNI_OK;
  } else {
    (*env) = 0;
    return JNI_EDETACHED;
  }

  END_EXCEPTION
  return 0;
}



jint AttachCurrentThreadAsDaemon(JavaVM *vm, void **par1, void *par2) {
  assert(0 && "implement me");
  return 0;
}


const struct JNIInvokeInterface JNI_JavaVMTable = {
	NULL,
	NULL,
	NULL,

	DestroyJavaVM,
	AttachCurrentThread,
	DetachCurrentThread,
	GetEnv,
	AttachCurrentThreadAsDaemon
};


struct JNINativeInterface JNI_JNIEnvTable = {
	NULL,
	NULL,
	NULL,
	NULL,    
	&GetVersion,

	&DefineClass,
	&FindClass,
	&FromReflectedMethod,
	&FromReflectedField,
	&ToReflectedMethod,
	&GetSuperclass,
	&IsAssignableFrom,
	&ToReflectedField,

	&Throw,
	&ThrowNew,
	&ExceptionOccurred,
	&ExceptionDescribe,
	&ExceptionClear,
	&FatalError,
	&PushLocalFrame,
	&PopLocalFrame,

	&NewGlobalRef,
	&DeleteGlobalRef,
	&DeleteLocalRef,
	&IsSameObject,
	&NewLocalRef,
	&EnsureLocalCapacity,

	&AllocObject,
	&NewObject,
	&NewObjectV,
	&NewObjectA,

	&GetObjectClass,
	&IsInstanceOf,

	&GetMethodID,

	&CallObjectMethod,
	&CallObjectMethodV,
	&CallObjectMethodA,
	&CallBooleanMethod,
	&CallBooleanMethodV,
	&CallBooleanMethodA,
	&CallByteMethod,
	&CallByteMethodV,
	&CallByteMethodA,
	&CallCharMethod,
	&CallCharMethodV,
	&CallCharMethodA,
	&CallShortMethod,
	&CallShortMethodV,
	&CallShortMethodA,
	&CallIntMethod,
	&CallIntMethodV,
	&CallIntMethodA,
	&CallLongMethod,
	&CallLongMethodV,
	&CallLongMethodA,
	&CallFloatMethod,
	&CallFloatMethodV,
	&CallFloatMethodA,
	&CallDoubleMethod,
	&CallDoubleMethodV,
	&CallDoubleMethodA,
	&CallVoidMethod,
	&CallVoidMethodV,
	&CallVoidMethodA,

	&CallNonvirtualObjectMethod,
	&CallNonvirtualObjectMethodV,
	&CallNonvirtualObjectMethodA,
	&CallNonvirtualBooleanMethod,
	&CallNonvirtualBooleanMethodV,
	&CallNonvirtualBooleanMethodA,
	&CallNonvirtualByteMethod,
	&CallNonvirtualByteMethodV,
	&CallNonvirtualByteMethodA,
	&CallNonvirtualCharMethod,
	&CallNonvirtualCharMethodV,
	&CallNonvirtualCharMethodA,
	&CallNonvirtualShortMethod,
	&CallNonvirtualShortMethodV,
	&CallNonvirtualShortMethodA,
	&CallNonvirtualIntMethod,
	&CallNonvirtualIntMethodV,
	&CallNonvirtualIntMethodA,
	&CallNonvirtualLongMethod,
	&CallNonvirtualLongMethodV,
	&CallNonvirtualLongMethodA,
	&CallNonvirtualFloatMethod,
	&CallNonvirtualFloatMethodV,
	&CallNonvirtualFloatMethodA,
	&CallNonvirtualDoubleMethod,
	&CallNonvirtualDoubleMethodV,
	&CallNonvirtualDoubleMethodA,
	&CallNonvirtualVoidMethod,
	&CallNonvirtualVoidMethodV,
	&CallNonvirtualVoidMethodA,

	&GetFieldID,

	&GetObjectField,
	&GetBooleanField,
	&GetByteField,
	&GetCharField,
	&GetShortField,
	&GetIntField,
	&GetLongField,
	&GetFloatField,
	&GetDoubleField,
	&SetObjectField,
	&SetBooleanField,
	&SetByteField,
	&SetCharField,
	&SetShortField,
	&SetIntField,
	&SetLongField,
	&SetFloatField,
	&SetDoubleField,

	&GetStaticMethodID,

	&CallStaticObjectMethod,
	&CallStaticObjectMethodV,
	&CallStaticObjectMethodA,
	&CallStaticBooleanMethod,
	&CallStaticBooleanMethodV,
	&CallStaticBooleanMethodA,
	&CallStaticByteMethod,
	&CallStaticByteMethodV,
	&CallStaticByteMethodA,
	&CallStaticCharMethod,
	&CallStaticCharMethodV,
	&CallStaticCharMethodA,
	&CallStaticShortMethod,
	&CallStaticShortMethodV,
	&CallStaticShortMethodA,
	&CallStaticIntMethod,
	&CallStaticIntMethodV,
	&CallStaticIntMethodA,
	&CallStaticLongMethod,
	&CallStaticLongMethodV,
	&CallStaticLongMethodA,
	&CallStaticFloatMethod,
	&CallStaticFloatMethodV,
	&CallStaticFloatMethodA,
	&CallStaticDoubleMethod,
	&CallStaticDoubleMethodV,
	&CallStaticDoubleMethodA,
	&CallStaticVoidMethod,
	&CallStaticVoidMethodV,
	&CallStaticVoidMethodA,

	&GetStaticFieldID,

	&GetStaticObjectField,
	&GetStaticBooleanField,
	&GetStaticByteField,
	&GetStaticCharField,
	&GetStaticShortField,
	&GetStaticIntField,
	&GetStaticLongField,
	&GetStaticFloatField,
	&GetStaticDoubleField,
	&SetStaticObjectField,
	&SetStaticBooleanField,
	&SetStaticByteField,
	&SetStaticCharField,
	&SetStaticShortField,
	&SetStaticIntField,
	&SetStaticLongField,
	&SetStaticFloatField,
	&SetStaticDoubleField,

	&NewString,
	&GetStringLength,
	&GetStringChars,
	&ReleaseStringChars,

	&NewStringUTF,
	&GetStringUTFLength,
	&GetStringUTFChars,
	&ReleaseStringUTFChars,

	&GetArrayLength,

	&NewObjectArray,
	&GetObjectArrayElement,
	&SetObjectArrayElement,

	&NewBooleanArray,
	&NewByteArray,
	&NewCharArray,
	&NewShortArray,
	&NewIntArray,
	&NewLongArray,
	&NewFloatArray,
	&NewDoubleArray,

	&GetBooleanArrayElements,
	&GetByteArrayElements,
	&GetCharArrayElements,
	&GetShortArrayElements,
	&GetIntArrayElements,
	&GetLongArrayElements,
	&GetFloatArrayElements,
	&GetDoubleArrayElements,

	&ReleaseBooleanArrayElements,
	&ReleaseByteArrayElements,
	&ReleaseCharArrayElements,
	&ReleaseShortArrayElements,
	&ReleaseIntArrayElements,
	&ReleaseLongArrayElements,
	&ReleaseFloatArrayElements,
	&ReleaseDoubleArrayElements,

	&GetBooleanArrayRegion,
	&GetByteArrayRegion,
	&GetCharArrayRegion,
	&GetShortArrayRegion,
	&GetIntArrayRegion,
	&GetLongArrayRegion,
	&GetFloatArrayRegion,
	&GetDoubleArrayRegion,
	&SetBooleanArrayRegion,
	&SetByteArrayRegion,
	&SetCharArrayRegion,
	&SetShortArrayRegion,
	&SetIntArrayRegion,
	&SetLongArrayRegion,
	&SetFloatArrayRegion,
	&SetDoubleArrayRegion,

	&RegisterNatives,
	&UnregisterNatives,

	&MonitorEnter,
	&MonitorExit,

	&GetJavaVM,

	/* new JNI 1.2 functions */

	&GetStringRegion,
	&GetStringUTFRegion,

	&GetPrimitiveArrayCritical,
	&ReleasePrimitiveArrayCritical,

	&GetStringCritical,
	&ReleaseStringCritical,

	&NewWeakGlobalRef,
	&DeleteWeakGlobalRef,

	&ExceptionCheck,

	/* new JNI 1.4 functions */

	&NewDirectByteBuffer,
	&GetDirectBufferAddress,
	&GetDirectBufferCapacity
};
