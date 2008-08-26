//===------------- Jni.cpp - Jni interface for JnJVM ----------------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <jni.h>

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

extern "C" const struct JNIInvokeInterface_ JNI_JavaVMTable;
extern "C" struct JNINativeInterface_ JNI_JNIEnvTable;

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

  JnjvmClassLoader* loader = 0;
  UserClass* currentClass = JavaJIT::getCallingClass();
  if (currentClass) loader = currentClass->classLoader;
  else loader = JnjvmClassLoader::bootstrapLoader;

  const UTF8* utf8 = loader->asciizConstructUTF8(asciiz);
  sint32 len = utf8->size;
  

  UserCommonClass* cl = loader->lookupClassFromUTF8(utf8, 0, len, true, true);
  Jnjvm* vm = JavaThread::get()->isolate;
  cl->initialiseClass(vm);
  return (jclass)(cl->getClassDelegatee(vm));
  
  END_EXCEPTION
  return 0;
}
  

jmethodID FromReflectedMethod(JNIEnv *env, jobject method) {
  
  BEGIN_EXCEPTION
  
  Jnjvm* vm = NativeUtil::myVM(env);
  Classpath* upcalls = vm->upcalls;
  JavaObject* meth = (JavaObject*)method;
  UserCommonClass* cl = meth->classOf;
  if (cl == upcalls->newConstructor)  {
    return (jmethodID)upcalls->constructorSlot->getInt32Field(meth);
  } else if (cl == upcalls->newMethod) {
    return (jmethodID)upcalls->methodSlot->getInt32Field(meth);
  } else {
    vm->unknownError("%s is not a constructor or a method", 
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
  
  UserCommonClass* cl2 = NativeUtil::resolvedImplClass(sup, false);
  UserCommonClass* cl1 = NativeUtil::resolvedImplClass(sub, false);

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
  UserCommonClass* cl = NativeUtil::resolvedImplClass(clazz, true);
  if (cl->isArray()) assert(0 && "implement me");
  JavaObject* res = ((UserClass*)cl)->doNew(vm);
  JavaMethod* init =
    cl->lookupMethod(Jnjvm::initName, 
                     cl->classLoader->asciizConstructUTF8("(Ljava/lang/String;)V"), 0, 1);
  init->invokeIntSpecial(vm, res, vm->asciizToStr(msg));
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
  
  UserCommonClass* cl = NativeUtil::resolvedImplClass(clazz, true);
  if (cl->isArray()) JavaThread::get()->isolate->unknownError("implement me");
  return (jobject)((UserClass*)cl)->doNew(JavaThread::get()->isolate);

  END_EXCEPTION
  return 0;
}


jobject NewObject(JNIEnv *env, jclass clazz, jmethodID methodID, ...) {
  BEGIN_EXCEPTION
    
  va_list ap;
  va_start(ap, methodID);
  JavaMethod* meth = (JavaMethod*)methodID;
  UserClass* cl = (UserClass*)NativeUtil::resolvedImplClass(clazz, true);
  Jnjvm* vm = JavaThread::get()->isolate;
  JavaObject* res = cl->doNew(vm);
  meth->invokeIntSpecialAP(vm, res, ap);
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
                   const jvalue *args) {
  assert(0 && "implement me");
  return 0;
}


jclass GetObjectClass(JNIEnv *env, jobject obj) {
  
  BEGIN_EXCEPTION

  verifyNull((JavaObject*)obj);
  Jnjvm* vm = JavaThread::get()->isolate;
  return (jclass)((JavaObject*)obj)->classOf->getClassDelegatee(vm);

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
  
  // TODO: find a better place for creating UTF8
  UserCommonClass* cl = NativeUtil::resolvedImplClass(clazz, true);
  const UTF8* name = cl->classLoader->asciizConstructUTF8(aname);
  const UTF8* type = cl->classLoader->asciizConstructUTF8(atype);
  JavaMethod* meth = cl->lookupMethod(
      name->javaToInternal(cl->classLoader->hashUTF8, 0, name->size),
      type->javaToInternal(cl->classLoader->hashUTF8, 0, type->size), false, true);

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
  Jnjvm* vm = JavaThread::get()->isolate;
  JavaObject* res = meth->invokeJavaObjectVirtualAP(vm, self, ap);
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
                          const jvalue * args) {
  assert(0 && "implement me");
  return 0;
}


jboolean CallBooleanMethod(JNIEnv *env, jobject _obj, jmethodID methodID, ...) {

  BEGIN_EXCEPTION

  va_list ap;
  va_start(ap, methodID);
  JavaObject* self = (JavaObject*)_obj;
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->isolate;
  uint32 res = meth->invokeIntVirtualAP(vm, self, ap);
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
                            const jvalue * args) {
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
                      const jvalue *args) {
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
                      const jvalue *args) {
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
                        const jvalue *args) {
  assert(0 && "implement me");
  return 0;
}



jint CallIntMethod(JNIEnv *env, jobject _obj, jmethodID methodID, ...) {

  BEGIN_EXCEPTION

  va_list ap;
  va_start(ap, methodID);
  JavaObject* obj = (JavaObject*)_obj;
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->isolate;
  uint32 res = meth->invokeIntVirtualAP(vm, obj, ap);
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
                    const jvalue *args) {
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
                      const jvalue *args) {
  assert(0 && "implement me");
  return 0;
}



jfloat CallFloatMethod(JNIEnv *env, jobject _obj, jmethodID methodID, ...) {
  
  BEGIN_EXCEPTION

  va_list ap;
  va_start(ap, methodID);
  JavaObject* obj = (JavaObject*)_obj;
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->isolate;
  jfloat res = meth->invokeFloatVirtualAP(vm, obj, ap);
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
                        const jvalue *args) {
  assert(0 && "implement me");
  return 0;
}



jdouble CallDoubleMethod(JNIEnv *env, jobject _obj, jmethodID methodID, ...) {
  
  BEGIN_EXCEPTION

  va_list ap;
  va_start(ap, methodID);
  JavaObject* obj = (JavaObject*)_obj;
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->isolate;
  jdouble res = meth->invokeDoubleVirtualAP(vm, obj, ap);
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
  Jnjvm* vm = JavaThread::get()->isolate;
  return meth->invokeDoubleVirtualAP(vm, obj, args);

  END_EXCEPTION
  return 0.0;

}


jdouble CallDoubleMethodA(JNIEnv *env, jobject _obj, jmethodID methodID,
                          const jvalue *args) {
  assert(0 && "implement me");
  return 0;
}



void CallVoidMethod(JNIEnv *env, jobject _obj, jmethodID methodID, ...) {
  
  BEGIN_EXCEPTION

  va_list ap;
  va_start(ap, methodID);
  JavaObject* obj = (JavaObject*)_obj;
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->isolate;
  meth->invokeIntVirtualAP(vm, obj, ap);
  va_end(ap);

  END_EXCEPTION
}


void CallVoidMethodV(JNIEnv *env, jobject _obj, jmethodID methodID,
                     va_list args) {
  
  BEGIN_EXCEPTION
  
  JavaObject* obj = (JavaObject*)_obj;
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->isolate;
  meth->invokeIntVirtualAP(vm, obj, args);

  END_EXCEPTION
}


void CallVoidMethodA(JNIEnv *env, jobject obj, jmethodID methodID,
                     const jvalue *args) {
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
                                    jmethodID methodID, const jvalue *args) {
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
                                      jmethodID methodID, const jvalue *args) {
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
                                jmethodID methodID, const jvalue *args) {
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
                                jmethodID methodID, const jvalue *args) {
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
                                  jmethodID methodID, const jvalue *args) {
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
                              jmethodID methodID, const jvalue *args) {
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
                                jmethodID methodID, const jvalue *args) {
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
                                  jmethodID methodID, const jvalue *args) {
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
                                    jmethodID methodID, const jvalue *args) {
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
  Jnjvm* vm = JavaThread::get()->isolate;
  meth->invokeIntSpecialAP(vm, obj, ap);
  va_end(ap);

  END_EXCEPTION
}


void CallNonvirtualVoidMethodV(JNIEnv *env, jobject obj, jclass clazz,
                               jmethodID methodID, va_list args) {
  assert(0 && "implement me");
}


void CallNonvirtualVoidMethodA(JNIEnv *env, jobject obj, jclass clazz,
                               jmethodID methodID, const jvalue * args) {
  assert(0 && "implement me");
}


jfieldID GetFieldID(JNIEnv *env, jclass clazz, const char *name,
		    const char *sig)  {

  BEGIN_EXCEPTION

  // TODO: find a better place to store the UTF8
  UserCommonClass* cl = NativeUtil::resolvedImplClass(clazz, true);
  return (jfieldID) 
    cl->lookupField(cl->classLoader->asciizConstructUTF8(name),
                    cl->classLoader->asciizConstructUTF8(sig), 0, 1);
  
  END_EXCEPTION
  return 0;

}


jobject GetObjectField(JNIEnv *env, jobject obj, jfieldID fieldID) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  JavaObject* o = (JavaObject*)obj;
  return (jobject)field->getObjectField(o);

  END_EXCEPTION
  return 0;
}


jboolean GetBooleanField(JNIEnv *env, jobject obj, jfieldID fieldID) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  JavaObject* o = (JavaObject*)obj;
  return (uint8)field->getInt8Field(o);

  END_EXCEPTION
  return 0;
}


jbyte GetByteField(JNIEnv *env, jobject obj, jfieldID fieldID) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  JavaObject* o = (JavaObject*)obj;
  return (sint8)field->getInt8Field(o);

  END_EXCEPTION
  return 0;
}


jchar GetCharField(JNIEnv *env, jobject obj, jfieldID fieldID) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  JavaObject* o = (JavaObject*)obj;
  return (uint16)field->getInt16Field(o);

  END_EXCEPTION
  return 0;
}


jshort GetShortField(JNIEnv *env, jobject obj, jfieldID fieldID) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  JavaObject* o = (JavaObject*)obj;
  return (sint16)field->getInt16Field(o);

  END_EXCEPTION
  return 0;
}


jint GetIntField(JNIEnv *env, jobject obj, jfieldID fieldID) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  JavaObject* o = (JavaObject*)obj;
  return (sint32)field->getInt32Field(o);

  END_EXCEPTION
  return 0;
}


jlong GetLongField(JNIEnv *env, jobject obj, jfieldID fieldID) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  JavaObject* o = (JavaObject*)obj;
  return (sint64)field->getLongField(o);

  END_EXCEPTION
  return 0;
}


jfloat GetFloatField(JNIEnv *env, jobject obj, jfieldID fieldID) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  JavaObject* o = (JavaObject*)obj;
  return field->getFloatField(o);

  END_EXCEPTION
  return 0;
}


jdouble GetDoubleField(JNIEnv *env, jobject obj, jfieldID fieldID) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  JavaObject* o = (JavaObject*)obj;
  return (jdouble)field->getDoubleField(o);

  END_EXCEPTION
  return 0;
}


void SetObjectField(JNIEnv *env, jobject obj, jfieldID fieldID, jobject value) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  JavaObject* o = (JavaObject*)obj;
  field->setObjectField(o, (JavaObject*)value);

  END_EXCEPTION
}


void SetBooleanField(JNIEnv *env, jobject obj, jfieldID fieldID,
                     jboolean value) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  JavaObject* o = (JavaObject*)obj;
  field->setInt8Field(o, (uint8)value);

  END_EXCEPTION
}


void SetByteField(JNIEnv *env, jobject obj, jfieldID fieldID, jbyte value) {

  BEGIN_EXCEPTION
  
  JavaField* field = (JavaField*)fieldID;
  JavaObject* o = (JavaObject*)obj;
  field->setInt8Field(o, (uint8)value);

  END_EXCEPTION
}


void SetCharField(JNIEnv *env, jobject obj, jfieldID fieldID, jchar value) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  JavaObject* o = (JavaObject*)obj;
  field->setInt16Field(o, (uint16)value);
  
  END_EXCEPTION
}


void SetShortField(JNIEnv *env, jobject obj, jfieldID fieldID, jshort value) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  JavaObject* o = (JavaObject*)obj;
  field->setInt16Field(o, (sint16)value);

  END_EXCEPTION
}


void SetIntField(JNIEnv *env, jobject obj, jfieldID fieldID, jint value) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  JavaObject* o = (JavaObject*)obj;
  field->setInt32Field(o, (sint32)value);

  END_EXCEPTION
}


void SetLongField(JNIEnv *env, jobject obj, jfieldID fieldID, jlong value) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  JavaObject* o = (JavaObject*)obj;
  field->setLongField(o, (sint64)value);

  END_EXCEPTION
}


void SetFloatField(JNIEnv *env, jobject obj, jfieldID fieldID, jfloat value) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  JavaObject* o = (JavaObject*)obj;
  field->setFloatField(o, (float)value);

  END_EXCEPTION
}


void SetDoubleField(JNIEnv *env, jobject obj, jfieldID fieldID, jdouble value) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  JavaObject* o = (JavaObject*)obj;
  field->setDoubleField(o, (float)value);

  END_EXCEPTION
}


jmethodID GetStaticMethodID(JNIEnv *env, jclass clazz, const char *aname,
			    const char *atype) {

  BEGIN_EXCEPTION
  
  // TODO: find a better place to store the UTF8
  UserCommonClass* cl = NativeUtil::resolvedImplClass(clazz, true);
  const UTF8* name = cl->classLoader->asciizConstructUTF8(aname);
  const UTF8* type = cl->classLoader->asciizConstructUTF8(atype);
  JavaMethod* meth = cl->lookupMethod(
      name->javaToInternal(cl->classLoader->hashUTF8, 0, name->size),
      type->javaToInternal(cl->classLoader->hashUTF8, 0, type->size), true, true);

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
                                const jvalue *args) {
  assert(0 && "implement me");
  return 0;
}


jboolean CallStaticBooleanMethod(JNIEnv *env, jclass clazz, jmethodID methodID,
                                 ...) {
  
  BEGIN_EXCEPTION
  
  va_list ap;
  va_start(ap, methodID);
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->isolate;
  uint32 res = meth->invokeIntStaticAP(vm, ap);
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
                                  const jvalue *args) {
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
                            const jvalue *args) {
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
                            const jvalue *args) {
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
                              const jvalue *args) {
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
                          const jvalue *args) {
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
                            const jvalue *args) {
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
                              const jvalue *args) {
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
                                const jvalue *args) {
  assert(0 && "implement me");
  return 0;
}


void CallStaticVoidMethod(JNIEnv *env, jclass clazz, jmethodID methodID, ...) {
  
  BEGIN_EXCEPTION

  va_list ap;
  va_start(ap, methodID);
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->isolate;
  meth->invokeIntStaticAP(vm, ap);
  va_end(ap);

  END_EXCEPTION
}


void CallStaticVoidMethodV(JNIEnv *env, jclass clazz, jmethodID methodID,
                           va_list args) {
  
  BEGIN_EXCEPTION
  
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->isolate;
  meth->invokeIntStaticAP(vm, args);

  END_EXCEPTION
}


void CallStaticVoidMethodA(JNIEnv *env, jclass clazz, jmethodID methodID,
                           const jvalue * args) {
  assert(0 && "implement me");
}


jfieldID GetStaticFieldID(JNIEnv *env, jclass clazz, const char *name,
                          const char *sig) {
  
  BEGIN_EXCEPTION
  
  // TODO: find a better place to store the UTF8
  UserCommonClass* cl = NativeUtil::resolvedImplClass(clazz, true);
  return (jfieldID)
    cl->lookupField(cl->classLoader->asciizConstructUTF8(name),
                    cl->classLoader->asciizConstructUTF8(sig), true, true);

  END_EXCEPTION
  return 0;
}


jobject GetStaticObjectField(JNIEnv *env, jclass clazz, jfieldID fieldID) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  UserClass* cl = (UserClass*)NativeUtil::resolvedImplClass(clazz, true);
  JavaObject* Stat = cl->getStaticInstance();
  return (jobject)field->getObjectField(Stat);

  END_EXCEPTION
  return 0;
}


jboolean GetStaticBooleanField(JNIEnv *env, jclass clazz, jfieldID fieldID) {
  
  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  UserClass* cl = (UserClass*)NativeUtil::resolvedImplClass(clazz, true);
  JavaObject* Stat = cl->getStaticInstance();
  return (jboolean)field->getInt8Field(Stat);

  END_EXCEPTION
  return 0;
}


jbyte GetStaticByteField(JNIEnv *env, jclass clazz, jfieldID fieldID) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  UserClass* cl = (UserClass*)NativeUtil::resolvedImplClass(clazz, true);
  JavaObject* Stat = cl->getStaticInstance();
  return (jbyte)field->getInt8Field(Stat);

  END_EXCEPTION
  return 0;
}


jchar GetStaticCharField(JNIEnv *env, jclass clazz, jfieldID fieldID) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  UserClass* cl = (UserClass*)NativeUtil::resolvedImplClass(clazz, true);
  JavaObject* Stat = cl->getStaticInstance();
  return (jchar)field->getInt16Field(Stat);

  END_EXCEPTION
  return 0;
}


jshort GetStaticShortField(JNIEnv *env, jclass clazz, jfieldID fieldID) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  UserClass* cl = (UserClass*)NativeUtil::resolvedImplClass(clazz, true);
  JavaObject* Stat = cl->getStaticInstance();
  return (jshort)field->getInt16Field(Stat);

  END_EXCEPTION
  return 0;
}


jint GetStaticIntField(JNIEnv *env, jclass clazz, jfieldID fieldID) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  UserClass* cl = (UserClass*)NativeUtil::resolvedImplClass(clazz, true);
  JavaObject* Stat = cl->getStaticInstance();
  return (jint)field->getInt32Field(Stat);

  END_EXCEPTION
  return 0;
}


jlong GetStaticLongField(JNIEnv *env, jclass clazz, jfieldID fieldID) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  UserClass* cl = (UserClass*)NativeUtil::resolvedImplClass(clazz, true);
  JavaObject* Stat = cl->getStaticInstance();
  return (jlong)field->getLongField(Stat);

  END_EXCEPTION
  return 0;
}


jfloat GetStaticFloatField(JNIEnv *env, jclass clazz, jfieldID fieldID) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  UserClass* cl = (UserClass*)NativeUtil::resolvedImplClass(clazz, true);
  JavaObject* Stat = cl->getStaticInstance();
  return (jfloat)field->getFloatField(Stat);

  END_EXCEPTION
  return 0;
}


jdouble GetStaticDoubleField(JNIEnv *env, jclass clazz, jfieldID fieldID) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  UserClass* cl = (UserClass*)NativeUtil::resolvedImplClass(clazz, true);
  JavaObject* Stat = cl->getStaticInstance();
  return (jdouble)field->getDoubleField(Stat);

  END_EXCEPTION
  return 0;
}


void SetStaticObjectField(JNIEnv *env, jclass clazz, jfieldID fieldID,
                          jobject value) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  UserClass* cl = (UserClass*)NativeUtil::resolvedImplClass(clazz, true);
  JavaObject* Stat = cl->getStaticInstance();
  field->setObjectField(Stat, (JavaObject*)value);
  
  END_EXCEPTION
}


void SetStaticBooleanField(JNIEnv *env, jclass clazz, jfieldID fieldID,
                           jboolean value) {
  
  BEGIN_EXCEPTION
  
  JavaField* field = (JavaField*)fieldID;
  UserClass* cl = (UserClass*)NativeUtil::resolvedImplClass(clazz, true);
  JavaObject* Stat = cl->getStaticInstance();
  field->setInt8Field(Stat, (uint8)value);

  END_EXCEPTION
}


void SetStaticByteField(JNIEnv *env, jclass clazz, jfieldID fieldID,
                        jbyte value) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  UserClass* cl = (UserClass*)NativeUtil::resolvedImplClass(clazz, true);
  JavaObject* Stat = cl->getStaticInstance();
  field->setInt8Field(Stat, (sint8)value);

  END_EXCEPTION
}


void SetStaticCharField(JNIEnv *env, jclass clazz, jfieldID fieldID,
                        jchar value) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  UserClass* cl = (UserClass*)NativeUtil::resolvedImplClass(clazz, true);
  JavaObject* Stat = cl->getStaticInstance();
  field->setInt16Field(Stat, (uint16)value);

  END_EXCEPTION
}


void SetStaticShortField(JNIEnv *env, jclass clazz, jfieldID fieldID,
                         jshort value) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  UserClass* cl = (UserClass*)NativeUtil::resolvedImplClass(clazz, true);
  JavaObject* Stat = cl->getStaticInstance();
  field->setInt16Field(Stat, (sint16)value);

  END_EXCEPTION
}


void SetStaticIntField(JNIEnv *env, jclass clazz, jfieldID fieldID,
                       jint value) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  UserClass* cl = (UserClass*)NativeUtil::resolvedImplClass(clazz, true);
  JavaObject* Stat = cl->getStaticInstance();
  field->setInt32Field(Stat, (sint32)value);

  END_EXCEPTION
}


void SetStaticLongField(JNIEnv *env, jclass clazz, jfieldID fieldID,
                        jlong value) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  UserClass* cl = (UserClass*)NativeUtil::resolvedImplClass(clazz, true);
  JavaObject* Stat = cl->getStaticInstance();
  field->setLongField(Stat, (sint64)value);

  END_EXCEPTION
}


void SetStaticFloatField(JNIEnv *env, jclass clazz, jfieldID fieldID,
                         jfloat value) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  UserClass* cl = (UserClass*)NativeUtil::resolvedImplClass(clazz, true);
  JavaObject* Stat = cl->getStaticInstance();
  field->setFloatField(Stat, (float)value);

  END_EXCEPTION
}


void SetStaticDoubleField(JNIEnv *env, jclass clazz, jfieldID fieldID,
                          jdouble value) {

  BEGIN_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  UserClass* cl = (UserClass*)NativeUtil::resolvedImplClass(clazz, true);
  JavaObject* Stat = cl->getStaticInstance();
  field->setDoubleField(Stat, (double)value);

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
  
  UserCommonClass* base = NativeUtil::resolvedImplClass(elementClass, true);
  JnjvmClassLoader* loader = base->classLoader;
  const UTF8* name = base->name;
  const UTF8* arrayName = AssessorDesc::constructArrayName(loader, 0, 1, name);
  UserClassArray* array = loader->constructArray(arrayName);
  ArrayObject* res = ArrayObject::acons(length, array, &(vm->allocator));
  if (initialElement) {
    memset(res->elements, (int)initialElement, 
               length * sizeof(JavaObject*));
  }
  return (jobjectArray)res;
  END_EXCEPTION
  return 0;
}


jobject GetObjectArrayElement(JNIEnv *env, jobjectArray array, jsize index) {
  BEGIN_EXCEPTION
  
  ArrayObject* JA = (ArrayObject*)array;
  if (index >= JA->size)
    JavaThread::get()->isolate->indexOutOfBounds(JA, index);
  return (jobject)JA->elements[index];
  
  END_EXCEPTION

  return 0;
}


void SetObjectArrayElement(JNIEnv *env, jobjectArray array, jsize index,
                           jobject val) {
  
  BEGIN_EXCEPTION
  
  ArrayObject* JA = (ArrayObject*)array;
  if (index >= JA->size)
    JavaThread::get()->isolate->indexOutOfBounds(JA, index);
  JA->elements[index] = (JavaObject*)val;

  END_EXCEPTION
}


jbooleanArray NewBooleanArray(JNIEnv *env, jsize len) {
  
  BEGIN_EXCEPTION
  
  ArrayUInt8* res = 0;
  Jnjvm* vm = NativeUtil::myVM(env);
  res = ArrayUInt8::acons(len, vm->bootstrapLoader->upcalls->ArrayOfBool,
                          &vm->allocator);
  return (jbooleanArray)res;

  END_EXCEPTION
  return 0;
}


jbyteArray NewByteArray(JNIEnv *env, jsize len) {
  
  BEGIN_EXCEPTION

  ArraySInt8* res = 0;
  Jnjvm* vm = NativeUtil::myVM(env);
  res = ArraySInt8::acons(len, vm->bootstrapLoader->upcalls->ArrayOfByte,
                          &vm->allocator);
  return (jbyteArray) res;

  END_EXCEPTION
  return 0;
}


jcharArray NewCharArray(JNIEnv *env, jsize len) {
  
  BEGIN_EXCEPTION
  
  ArrayUInt16* res = 0;
  Jnjvm* vm = NativeUtil::myVM(env);
  res = ArrayUInt16::acons(len, vm->bootstrapLoader->upcalls->ArrayOfChar,
                           &vm->allocator);
  return (jcharArray) res;

  END_EXCEPTION
  return 0;
}


jshortArray NewShortArray(JNIEnv *env, jsize len) {
  
  BEGIN_EXCEPTION
  
  ArraySInt16* res = 0;
  Jnjvm* vm = NativeUtil::myVM(env);
  res = ArraySInt16::acons(len, vm->bootstrapLoader->upcalls->ArrayOfShort,
                           &vm->allocator);
  return (jshortArray) res;

  END_EXCEPTION
  return 0;
}


jintArray NewIntArray(JNIEnv *env, jsize len) {
  
  BEGIN_EXCEPTION
  
  ArraySInt32* res = 0;
  Jnjvm* vm = NativeUtil::myVM(env);
  res = ArraySInt32::acons(len, vm->bootstrapLoader->upcalls->ArrayOfInt,
                           &vm->allocator);
  return (jintArray) res;

  END_EXCEPTION
  return 0;
}


jlongArray NewLongArray(JNIEnv *env, jsize len) {
  
  BEGIN_EXCEPTION
  
  ArrayLong* res = 0;
  Jnjvm* vm = NativeUtil::myVM(env);
  res = ArrayLong::acons(len, vm->bootstrapLoader->upcalls->ArrayOfLong,
                         &vm->allocator);
  return (jlongArray) res;

  END_EXCEPTION
  return 0;
}


jfloatArray NewFloatArray(JNIEnv *env, jsize len) {
  
  BEGIN_EXCEPTION
  
  ArrayFloat* res = 0;
  Jnjvm* vm = NativeUtil::myVM(env);
  res = ArrayFloat::acons(len, vm->bootstrapLoader->upcalls->ArrayOfFloat,
                          &vm->allocator);
  return (jfloatArray) res;

  END_EXCEPTION
  return 0;
}


jdoubleArray NewDoubleArray(JNIEnv *env, jsize len) {
  
  BEGIN_EXCEPTION
  
  ArrayDouble* res = 0;
  Jnjvm* vm = NativeUtil::myVM(env);
  res = ArrayDouble::acons(len, vm->bootstrapLoader->upcalls->ArrayOfDouble,
                           &vm->allocator);
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
			   jsize len, const jboolean *buf) {
  assert(0 && "implement me");
}


void SetByteArrayRegion(JNIEnv *env, jbyteArray array, jsize start, jsize len,
			                  const jbyte *buf) {
  assert(0 && "implement me");
}


void SetCharArrayRegion(JNIEnv *env, jcharArray array, jsize start, jsize len,
			                  const jchar *buf) {
  assert(0 && "implement me");
}


void SetShortArrayRegion(JNIEnv *env, jshortArray array, jsize start,
			                   jsize len, const jshort *buf) {
  assert(0 && "implement me");
}


void SetIntArrayRegion(JNIEnv *env, jintArray array, jsize start, jsize len,
		                   const jint *buf) {
  assert(0 && "implement me");
}


void SetLongArrayRegion(JNIEnv* env, jlongArray array, jsize start, jsize len,
			                  const jlong *buf) {
  assert(0 && "implement me");
}


void SetFloatArrayRegion(JNIEnv *env, jfloatArray array, jsize start,
			                   jsize len, const jfloat *buf) {
  assert(0 && "implement me");
}


void SetDoubleArrayRegion(JNIEnv *env, jdoubleArray array, jsize start,
			                    jsize len, const jdouble *buf) {
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

extern "C" void JavaObjectAquire(JavaObject* obj);
extern "C" void JavaObjectRelease(JavaObject* obj);

jint MonitorEnter(JNIEnv *env, jobject obj) {
  
  BEGIN_EXCEPTION
  
  JavaObjectAquire(((JavaObject*)obj));
  return 1;

  END_EXCEPTION
  return 0;
}


jint MonitorExit(JNIEnv *env, jobject obj) {

  BEGIN_EXCEPTION

  JavaObjectRelease((JavaObject*)obj);
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
  for (std::vector<JavaObject*, gc_allocator<JavaObject*> >::iterator i =
                                                      vm->globalRefs.begin(),
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

  Jnjvm* vm = NativeUtil::myVM(env);
  JavaObject* buf = (JavaObject*)_buf;
  JavaObject* address = vm->upcalls->bufferAddress->getObjectField(buf);
  if (address != 0) {
    int res = vm->upcalls->dataPointer32->getInt32Field(address);
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

jobjectRefType GetObjectRefType(JNIEnv* env, jobject obj) {
  assert(0 && "implement me");
  return (jobjectRefType)0;
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


const struct JNIInvokeInterface_ JNI_JavaVMTable = {
	NULL,
	NULL,
	NULL,

	DestroyJavaVM,
	AttachCurrentThread,
	DetachCurrentThread,
	GetEnv,
	AttachCurrentThreadAsDaemon
};


struct JNINativeInterface_ JNI_JNIEnvTable = {
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
	&GetDirectBufferCapacity,

  /* ---- JNI 1.6 functions ---- */
  &GetObjectRefType
};
