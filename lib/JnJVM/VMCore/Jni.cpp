//===------------- Jni.cpp - Jni interface for JnJVM ----------------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <jni.h>

#include "ClasspathReflect.h"
#include "JavaArray.h"
#include "JavaClass.h"
#include "JavaObject.h"
#include "JavaString.h"
#include "JavaThread.h"
#include "JavaTypes.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"


using namespace jnjvm;

static Jnjvm* myVM(JNIEnv* env) {
  return JavaThread::get()->getJVM();
}

static UserClass* getClassFromStaticMethod(Jnjvm* vm, JavaMethod* meth,
                                           jclass clazz) {
#ifdef ISOLATE_SHARING
  return (UserClass*)NativeUtil::resolvedImplClass(vm, vm, clazz, true);
#else
  return meth->classDef;
#endif
}

static UserClass* getClassFromVirtualMethod(Jnjvm* vm, 
                                            JavaMethod* meth,
                                            UserCommonClass* cl) {
#ifdef ISOLATE_SHARING
  return cl->lookupClassFromMethod(this);
#else
  return meth->classDef;
#endif
}

extern "C" const struct JNIInvokeInterface_ JNI_JavaVMTable;
extern "C" struct JNINativeInterface_ JNI_JNIEnvTable;

jint GetVersion(JNIEnv *env) {
  return JNI_VERSION_1_4;
}


jclass DefineClass(JNIEnv *env, const char *name, jobject loader,
				   const jbyte *buf, jsize bufLen) {
  assert(0 && "implement me");
  return 0;
}


jclass FindClass(JNIEnv *env, const char *asciiz) {
  
  BEGIN_JNI_EXCEPTION

  JnjvmClassLoader* loader = 0;
  JavaThread* th = JavaThread::get();
  Jnjvm* vm = th->getJVM();
  UserClass* currentClass = th->getCallingClass(0);
  if (currentClass) loader = currentClass->classLoader;
  else loader = vm->bootstrapLoader;

  const UTF8* utf8 = vm->asciizToInternalUTF8(asciiz);
  
  UserCommonClass* cl = loader->lookupClassFromUTF8(utf8, vm, true, true);
  if (cl->asClass()) cl->asClass()->initialiseClass(vm);
  return (jclass)(cl->getClassDelegatee(vm));
  
  END_JNI_EXCEPTION
  return 0;
}
  

jmethodID FromReflectedMethod(JNIEnv *env, jobject method) {
  
  BEGIN_JNI_EXCEPTION
  
  Jnjvm* vm = myVM(env);
  Classpath* upcalls = vm->upcalls;
  JavaObject* meth = (JavaObject*)method;
  UserCommonClass* cl = meth->getClass();
  if (cl == upcalls->newConstructor)  {
    return (jmethodID)((JavaObjectMethod*)meth)->getInternalMethod();
  } else if (cl == upcalls->newMethod) {
    return (jmethodID)((JavaObjectConstructor*)meth)->getInternalMethod();
  } else {
    vm->unknownError("%s is not a constructor or a method", 
                     meth->printString());
  }
  
  END_JNI_EXCEPTION
  
  return 0;
}


jclass GetSuperclass(JNIEnv *env, jclass sub) {
  assert(0 && "implement me");
  return 0;
}
  
 
jboolean IsAssignableFrom(JNIEnv *env, jclass sub, jclass sup) {
  
  BEGIN_JNI_EXCEPTION
  
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserCommonClass* cl2 = 
    UserCommonClass::resolvedImplClass(vm, (JavaObject*)sup, false);
  UserCommonClass* cl1 = 
    UserCommonClass::resolvedImplClass(vm, (JavaObject*)sub, false);

  return cl1->isAssignableFrom(cl2);
  
  END_JNI_EXCEPTION

  return false;
}


jint Throw(JNIEnv *env, jthrowable obj) {
  assert(0 && "implement me");
  return 0;
}


jint ThrowNew(JNIEnv* env, jclass Cl, const char *msg) {
  
  BEGIN_JNI_EXCEPTION
  
  Jnjvm* vm = myVM(env);
  verifyNull(Cl);
  UserCommonClass* cl = 
    UserCommonClass::resolvedImplClass(vm, (JavaObject*)Cl, true);
  if (!cl->asClass()) assert(0 && "implement me");
  UserClass* realCl = cl->asClass();
  JavaObject* res = realCl->doNew(vm);
  JavaMethod* init = realCl->lookupMethod(vm->bootstrapLoader->initName,
              cl->classLoader->asciizConstructUTF8("(Ljava/lang/String;)V"),
              false, true, 0);
  init->invokeIntSpecial(vm, realCl, res, vm->asciizToStr(msg));
  th->pendingException = res;
  th->throwFromJNI();
  return 1;
  
  END_JNI_EXCEPTION

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
  return ref1 == ref2;
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
  
  BEGIN_JNI_EXCEPTION
  
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserCommonClass* cl = 
    UserCommonClass::resolvedImplClass(vm, (JavaObject*)clazz, true);
  if (cl->isArray()) JavaThread::get()->getJVM()->unknownError("implement me");
  return (jobject)((UserClass*)cl)->doNew(JavaThread::get()->getJVM());

  END_JNI_EXCEPTION
  return 0;
}


jobject NewObject(JNIEnv *env, jclass clazz, jmethodID methodID, ...) {
  BEGIN_JNI_EXCEPTION
    
  va_list ap;
  va_start(ap, methodID);
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = (UserClass*)
    UserCommonClass::resolvedImplClass(vm, (JavaObject*)clazz, true);
  JavaObject* res = cl->doNew(vm);
  meth->invokeIntSpecialAP(vm, cl, res, ap);
  va_end(ap);
  return (jobject)res;
  
  END_JNI_EXCEPTION
  return 0;
}


jobject NewObjectV(JNIEnv* env, jclass clazz, jmethodID methodID,
                   va_list args) {
  assert(0 && "implement me");
  return 0;
}


jobject NewObjectA(JNIEnv* env, jclass clazz, jmethodID methodID,
                   const jvalue *args) {
  BEGIN_JNI_EXCEPTION
  
  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaMethod* meth = (JavaMethod*)methodID;
  UserClass* cl = (UserClass*)
    UserCommonClass::resolvedImplClass(vm, (JavaObject*)clazz, true);
  JavaObject* res = cl->doNew(vm);
  meth->invokeIntSpecialBuf(vm, cl, res, (void*)args);
  return (jobject)res; 
  
  END_JNI_EXCEPTION
  return 0;
}


jclass GetObjectClass(JNIEnv *env, jobject obj) {
  
  BEGIN_JNI_EXCEPTION

  verifyNull((JavaObject*)obj);
  Jnjvm* vm = JavaThread::get()->getJVM();
  return (jclass)((JavaObject*)obj)->getClass()->getClassDelegatee(vm);

  END_JNI_EXCEPTION
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
  
  BEGIN_JNI_EXCEPTION
  
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserCommonClass* cl = UserCommonClass::resolvedImplClass(vm, (JavaObject*)clazz, true);
  const UTF8* name = vm->asciizToInternalUTF8(aname);
  const UTF8* type = vm->asciizToInternalUTF8(atype);
  JavaMethod* meth = cl->asClass() ? 
    cl->asClass()->lookupMethod(name, type, false, true, 0) : 0;

  return (jmethodID)meth;

  END_JNI_EXCEPTION
  return 0;
}


jobject CallObjectMethod(JNIEnv *env, jobject obj, jmethodID methodID, ...) {

  BEGIN_JNI_EXCEPTION

  va_list ap;
  va_start(ap, methodID);
  JavaObject* self = (JavaObject*)obj;
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromVirtualMethod(vm, meth, self->getClass());
  JavaObject* res = meth->invokeJavaObjectVirtualAP(vm, cl, self, ap);
  va_end(ap);
  return (jobject)res;

  END_JNI_EXCEPTION
  return 0;
}


jobject CallObjectMethodV(JNIEnv *env, jobject _obj, jmethodID methodID,
                          va_list args) {
  
  BEGIN_JNI_EXCEPTION
  
  JavaObject* obj = (JavaObject*)_obj;
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromVirtualMethod(vm, meth, obj->getClass());
  return (jobject)meth->invokeJavaObjectVirtualAP(vm, cl, obj, args);

  END_JNI_EXCEPTION
  
  return 0;
}


jobject CallObjectMethodA(JNIEnv *env, jobject obj, jmethodID methodID,
                          const jvalue * args) {
  assert(0 && "implement me");
  return 0;
}


jboolean CallBooleanMethod(JNIEnv *env, jobject _obj, jmethodID methodID, ...) {

  BEGIN_JNI_EXCEPTION

  va_list ap;
  va_start(ap, methodID);
  JavaObject* self = (JavaObject*)_obj;
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromVirtualMethod(vm, meth, self->getClass());
  uint32 res = meth->invokeIntVirtualAP(vm, cl, self, ap);
  va_end(ap);
  return res;

  END_JNI_EXCEPTION
  return 0;
}


jboolean CallBooleanMethodV(JNIEnv *env, jobject _obj, jmethodID methodID,
                            va_list args) {
  
  BEGIN_JNI_EXCEPTION
  
  JavaObject* obj = (JavaObject*)_obj;
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromVirtualMethod(vm, meth, obj->getClass());
  return (jboolean)meth->invokeIntVirtualAP(vm, cl, obj, args);

  END_JNI_EXCEPTION
  
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

jbyte CallByteMethodV(JNIEnv *env, jobject _obj, jmethodID methodID,
                      va_list args) {
  
  BEGIN_JNI_EXCEPTION
  
  JavaObject* obj = (JavaObject*)_obj;
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromVirtualMethod(vm, meth, obj->getClass());
  return (jbyte)meth->invokeIntVirtualAP(vm, cl, obj, args);

  END_JNI_EXCEPTION
  
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


jchar CallCharMethodV(JNIEnv *env, jobject _obj, jmethodID methodID,
                      va_list args) {
  
  BEGIN_JNI_EXCEPTION
  
  JavaObject* obj = (JavaObject*)_obj;
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromVirtualMethod(vm, meth, obj->getClass());
  return (jchar)meth->invokeIntVirtualAP(vm, cl, obj, args);

  END_JNI_EXCEPTION
  
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


jshort CallShortMethodV(JNIEnv *env, jobject _obj, jmethodID methodID,
                        va_list args) {
  
  BEGIN_JNI_EXCEPTION
  
  JavaObject* obj = (JavaObject*)_obj;
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromVirtualMethod(vm, meth, obj->getClass());
  return (jshort)meth->invokeIntVirtualAP(vm, cl, obj, args);

  END_JNI_EXCEPTION
  
  return 0;
}


jshort CallShortMethodA(JNIEnv *env, jobject obj, jmethodID methodID,
                        const jvalue *args) {
  assert(0 && "implement me");
  return 0;
}



jint CallIntMethod(JNIEnv *env, jobject _obj, jmethodID methodID, ...) {

  BEGIN_JNI_EXCEPTION

  va_list ap;
  va_start(ap, methodID);
  JavaObject* obj = (JavaObject*)_obj;
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromVirtualMethod(vm, meth, obj->getClass());
  uint32 res = meth->invokeIntVirtualAP(vm, cl, obj, ap);
  va_end(ap);
  return res;

  END_JNI_EXCEPTION
  return 0;
}


jint CallIntMethodV(JNIEnv *env, jobject _obj, jmethodID methodID,
                    va_list args) {
  
  BEGIN_JNI_EXCEPTION
  
  JavaObject* obj = (JavaObject*)_obj;
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromVirtualMethod(vm, meth, obj->getClass());
  return (jint)meth->invokeIntVirtualAP(vm, cl, obj, args);

  END_JNI_EXCEPTION
  
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


jlong CallLongMethodV(JNIEnv *env, jobject _obj, jmethodID methodID,
                      va_list args) {
  
  BEGIN_JNI_EXCEPTION
  
  JavaObject* obj = (JavaObject*)_obj;
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromVirtualMethod(vm, meth, obj->getClass());
  return (jlong)meth->invokeLongVirtualAP(vm, cl, obj, args);

  END_JNI_EXCEPTION
  
  return 0;
}


jlong CallLongMethodA(JNIEnv *env, jobject obj, jmethodID methodID,
                      const jvalue *args) {
  assert(0 && "implement me");
  return 0;
}



jfloat CallFloatMethod(JNIEnv *env, jobject _obj, jmethodID methodID, ...) {
  
  BEGIN_JNI_EXCEPTION

  va_list ap;
  va_start(ap, methodID);
  JavaObject* obj = (JavaObject*)_obj;
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromVirtualMethod(vm, meth, obj->getClass());
  jfloat res = meth->invokeFloatVirtualAP(vm, cl, obj, ap);
  va_end(ap);
  return res;

  END_JNI_EXCEPTION;
  return 0.0;
}


jfloat CallFloatMethodV(JNIEnv *env, jobject _obj, jmethodID methodID,
                        va_list args) {
  
  BEGIN_JNI_EXCEPTION
  
  JavaObject* obj = (JavaObject*)_obj;
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromVirtualMethod(vm, meth, obj->getClass());
  return (jfloat)meth->invokeFloatVirtualAP(vm, cl, obj, args);

  END_JNI_EXCEPTION
  
  return 0.0f;
}


jfloat CallFloatMethodA(JNIEnv *env, jobject _obj, jmethodID methodID,
                        const jvalue *args) {
  assert(0 && "implement me");
  return 0;
}



jdouble CallDoubleMethod(JNIEnv *env, jobject _obj, jmethodID methodID, ...) {
  
  BEGIN_JNI_EXCEPTION

  va_list ap;
  va_start(ap, methodID);
  JavaObject* obj = (JavaObject*)_obj;
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromVirtualMethod(vm, meth, obj->getClass());
  jdouble res = meth->invokeDoubleVirtualAP(vm, cl, obj, ap);
  va_end(ap);
  return res;

  END_JNI_EXCEPTION
  return 0.0;
}


jdouble CallDoubleMethodV(JNIEnv *env, jobject _obj, jmethodID methodID,
                          va_list args) {
  
  BEGIN_JNI_EXCEPTION
  
  JavaObject* obj = (JavaObject*)_obj;
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromVirtualMethod(vm, meth, obj->getClass());
  return (jdouble)meth->invokeDoubleVirtualAP(vm, cl, obj, args);

  END_JNI_EXCEPTION
  return 0.0;

}


jdouble CallDoubleMethodA(JNIEnv *env, jobject _obj, jmethodID methodID,
                          const jvalue *args) {
  assert(0 && "implement me");
  return 0;
}



void CallVoidMethod(JNIEnv *env, jobject _obj, jmethodID methodID, ...) {
  
  BEGIN_JNI_EXCEPTION

  va_list ap;
  va_start(ap, methodID);
  JavaObject* obj = (JavaObject*)_obj;
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromVirtualMethod(vm, meth, obj->getClass());
  meth->invokeIntVirtualAP(vm, cl, obj, ap);
  va_end(ap);

  END_JNI_EXCEPTION
}


void CallVoidMethodV(JNIEnv *env, jobject _obj, jmethodID methodID,
                     va_list args) {
  
  BEGIN_JNI_EXCEPTION
  
  JavaObject* obj = (JavaObject*)_obj;
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromVirtualMethod(vm, meth, obj->getClass());
  meth->invokeIntVirtualAP(vm, cl, obj, args);

  END_JNI_EXCEPTION
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
  
  BEGIN_JNI_EXCEPTION

  va_list ap;
  va_start(ap, methodID);
  JavaObject* obj = (JavaObject*)_obj;
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromVirtualMethod(vm, meth, obj->getClass());
  meth->invokeIntSpecialAP(vm, cl, obj, ap);
  va_end(ap);

  END_JNI_EXCEPTION
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

  BEGIN_JNI_EXCEPTION

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserCommonClass* cl = 
    UserCommonClass::resolvedImplClass(vm, (JavaObject*)clazz, true);
  return (jfieldID) (cl->asClass() ? 
    cl->asClass()->lookupField(vm->asciizToUTF8(name),
                               vm->asciizToUTF8(sig), 0, 1,
                               0) : 0);
  
  END_JNI_EXCEPTION
  return 0;

}


jobject GetObjectField(JNIEnv *env, jobject obj, jfieldID fieldID) {

  BEGIN_JNI_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  JavaObject* o = (JavaObject*)obj;
  return (jobject)field->getObjectField(o);

  END_JNI_EXCEPTION
  return 0;
}


jboolean GetBooleanField(JNIEnv *env, jobject obj, jfieldID fieldID) {

  BEGIN_JNI_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  JavaObject* o = (JavaObject*)obj;
  return (uint8)field->getInt8Field(o);

  END_JNI_EXCEPTION
  return 0;
}


jbyte GetByteField(JNIEnv *env, jobject obj, jfieldID fieldID) {

  BEGIN_JNI_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  JavaObject* o = (JavaObject*)obj;
  return (sint8)field->getInt8Field(o);

  END_JNI_EXCEPTION
  return 0;
}


jchar GetCharField(JNIEnv *env, jobject obj, jfieldID fieldID) {

  BEGIN_JNI_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  JavaObject* o = (JavaObject*)obj;
  return (uint16)field->getInt16Field(o);

  END_JNI_EXCEPTION
  return 0;
}


jshort GetShortField(JNIEnv *env, jobject obj, jfieldID fieldID) {

  BEGIN_JNI_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  JavaObject* o = (JavaObject*)obj;
  return (sint16)field->getInt16Field(o);

  END_JNI_EXCEPTION
  return 0;
}


jint GetIntField(JNIEnv *env, jobject obj, jfieldID fieldID) {

  BEGIN_JNI_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  JavaObject* o = (JavaObject*)obj;
  return (sint32)field->getInt32Field(o);

  END_JNI_EXCEPTION
  return 0;
}


jlong GetLongField(JNIEnv *env, jobject obj, jfieldID fieldID) {

  BEGIN_JNI_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  JavaObject* o = (JavaObject*)obj;
  return (sint64)field->getLongField(o);

  END_JNI_EXCEPTION
  return 0;
}


jfloat GetFloatField(JNIEnv *env, jobject obj, jfieldID fieldID) {

  BEGIN_JNI_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  JavaObject* o = (JavaObject*)obj;
  return field->getFloatField(o);

  END_JNI_EXCEPTION
  return 0;
}


jdouble GetDoubleField(JNIEnv *env, jobject obj, jfieldID fieldID) {

  BEGIN_JNI_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  JavaObject* o = (JavaObject*)obj;
  return (jdouble)field->getDoubleField(o);

  END_JNI_EXCEPTION
  return 0;
}


void SetObjectField(JNIEnv *env, jobject obj, jfieldID fieldID, jobject value) {

  BEGIN_JNI_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  JavaObject* o = (JavaObject*)obj;
  field->setObjectField(o, (JavaObject*)value);

  END_JNI_EXCEPTION
}


void SetBooleanField(JNIEnv *env, jobject obj, jfieldID fieldID,
                     jboolean value) {

  BEGIN_JNI_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  JavaObject* o = (JavaObject*)obj;
  field->setInt8Field(o, (uint8)value);

  END_JNI_EXCEPTION
}


void SetByteField(JNIEnv *env, jobject obj, jfieldID fieldID, jbyte value) {

  BEGIN_JNI_EXCEPTION
  
  JavaField* field = (JavaField*)fieldID;
  JavaObject* o = (JavaObject*)obj;
  field->setInt8Field(o, (uint8)value);

  END_JNI_EXCEPTION
}


void SetCharField(JNIEnv *env, jobject obj, jfieldID fieldID, jchar value) {

  BEGIN_JNI_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  JavaObject* o = (JavaObject*)obj;
  field->setInt16Field(o, (uint16)value);
  
  END_JNI_EXCEPTION
}


void SetShortField(JNIEnv *env, jobject obj, jfieldID fieldID, jshort value) {

  BEGIN_JNI_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  JavaObject* o = (JavaObject*)obj;
  field->setInt16Field(o, (sint16)value);

  END_JNI_EXCEPTION
}


void SetIntField(JNIEnv *env, jobject obj, jfieldID fieldID, jint value) {

  BEGIN_JNI_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  JavaObject* o = (JavaObject*)obj;
  field->setInt32Field(o, (sint32)value);

  END_JNI_EXCEPTION
}


void SetLongField(JNIEnv *env, jobject obj, jfieldID fieldID, jlong value) {

  BEGIN_JNI_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  JavaObject* o = (JavaObject*)obj;
  field->setLongField(o, (sint64)value);

  END_JNI_EXCEPTION
}


void SetFloatField(JNIEnv *env, jobject obj, jfieldID fieldID, jfloat value) {

  BEGIN_JNI_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  JavaObject* o = (JavaObject*)obj;
  field->setFloatField(o, (float)value);

  END_JNI_EXCEPTION
}


void SetDoubleField(JNIEnv *env, jobject obj, jfieldID fieldID, jdouble value) {

  BEGIN_JNI_EXCEPTION

  JavaField* field = (JavaField*)fieldID;
  JavaObject* o = (JavaObject*)obj;
  field->setDoubleField(o, (float)value);

  END_JNI_EXCEPTION
}


jmethodID GetStaticMethodID(JNIEnv *env, jclass clazz, const char *aname,
			    const char *atype) {

  BEGIN_JNI_EXCEPTION
  
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserCommonClass* cl = 
    UserCommonClass::resolvedImplClass(vm, (JavaObject*)clazz, true);
  const UTF8* name = vm->asciizToInternalUTF8(aname);
  const UTF8* type = vm->asciizToInternalUTF8(atype);
  JavaMethod* meth = cl->asClass() ? 
    cl->asClass()->lookupMethod(name, type, true, true, 0) : 0;

  return (jmethodID)meth;

  END_JNI_EXCEPTION
  return 0;
}


jobject CallStaticObjectMethod(JNIEnv *env, jclass clazz, jmethodID methodID,
                               ...) {
  
  BEGIN_JNI_EXCEPTION
  
  va_list ap;
  va_start(ap, methodID);
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromStaticMethod(vm, meth, clazz);
  jobject res = (jobject) meth->invokeJavaObjectStaticAP(vm, cl, ap);
  va_end(ap);
  return res;

  END_JNI_EXCEPTION
  return 0;
}


jobject CallStaticObjectMethodV(JNIEnv *env, jclass clazz, jmethodID methodID,
                                va_list args) {
  BEGIN_JNI_EXCEPTION
  
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromStaticMethod(vm, meth, clazz);
  return (jobject)meth->invokeJavaObjectStaticAP(vm, cl, args);

  END_JNI_EXCEPTION
  return 0;
}


jobject CallStaticObjectMethodA(JNIEnv *env, jclass clazz, jmethodID methodID,
                                const jvalue *args) {
  assert(0 && "implement me");
  return 0;
}


jboolean CallStaticBooleanMethod(JNIEnv *env, jclass clazz, jmethodID methodID,
                                 ...) {
  
  BEGIN_JNI_EXCEPTION
  
  va_list ap;
  va_start(ap, methodID);
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromStaticMethod(vm, meth, clazz);
  uint32 res = meth->invokeIntStaticAP(vm, cl, ap);
  va_end(ap);
  return res;

  END_JNI_EXCEPTION
  return 0;
}


jboolean CallStaticBooleanMethodV(JNIEnv *env, jclass clazz, jmethodID methodID,
                                  va_list args) {
  
  BEGIN_JNI_EXCEPTION
  
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromStaticMethod(vm, meth, clazz);
  return (jboolean)meth->invokeIntStaticAP(vm, cl, args);

  END_JNI_EXCEPTION
  return 0;
}


jboolean CallStaticBooleanMethodA(JNIEnv *env, jclass clazz, jmethodID methodID,
                                  const jvalue *args) {
  assert(0 && "implement me");
  return 0;
}


jbyte CallStaticByteMethod(JNIEnv *env, jclass clazz, jmethodID methodID, ...) {
  
  BEGIN_JNI_EXCEPTION
  
  va_list ap;
  va_start(ap, methodID);
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromStaticMethod(vm, meth, clazz);
  jbyte res = (jbyte) meth->invokeIntStaticAP(vm, cl, ap);
  va_end(ap);
  return res;

  END_JNI_EXCEPTION
  return 0;
}


jbyte CallStaticByteMethodV(JNIEnv *env, jclass clazz, jmethodID methodID,
                            va_list args) {
  
  BEGIN_JNI_EXCEPTION
  
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromStaticMethod(vm, meth, clazz);
  return (jbyte)meth->invokeIntStaticAP(vm, cl, args);

  END_JNI_EXCEPTION
  return 0;
}


jbyte CallStaticByteMethodA(JNIEnv *env, jclass clazz, jmethodID methodID,
                            const jvalue *args) {
  assert(0 && "implement me");
  return 0;
}


jchar CallStaticCharMethod(JNIEnv *env, jclass clazz, jmethodID methodID, ...) {
  
  BEGIN_JNI_EXCEPTION
  
  va_list ap;
  va_start(ap, methodID);
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromStaticMethod(vm, meth, clazz);
  jchar res = (jchar) meth->invokeIntStaticAP(vm, cl, ap);
  va_end(ap);
  return res;

  END_JNI_EXCEPTION
  return 0;
}


jchar CallStaticCharMethodV(JNIEnv *env, jclass clazz, jmethodID methodID,
                            va_list args) {
  BEGIN_JNI_EXCEPTION
  
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromStaticMethod(vm, meth, clazz);
  return (jchar)meth->invokeIntStaticAP(vm, cl, args);

  END_JNI_EXCEPTION
  return 0;
}


jchar CallStaticCharMethodA(JNIEnv *env, jclass clazz, jmethodID methodID,
                            const jvalue *args) {
  assert(0 && "implement me");
  return 0;
}


jshort CallStaticShortMethod(JNIEnv *env, jclass clazz, jmethodID methodID,
                             ...) {
  
  BEGIN_JNI_EXCEPTION
  
  va_list ap;
  va_start(ap, methodID);
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromStaticMethod(vm, meth, clazz);
  jshort res = (jshort) meth->invokeIntStaticAP(vm, cl, ap);
  va_end(ap);
  return res;

  END_JNI_EXCEPTION
  return 0;
}


jshort CallStaticShortMethodV(JNIEnv *env, jclass clazz, jmethodID methodID,
                              va_list args) {
  
  BEGIN_JNI_EXCEPTION
  
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromStaticMethod(vm, meth, clazz);
  return (jshort)meth->invokeIntStaticAP(vm, cl, args);

  END_JNI_EXCEPTION
  return 0;
}


jshort CallStaticShortMethodA(JNIEnv *env, jclass clazz, jmethodID methodID,
                              const jvalue *args) {
  assert(0 && "implement me");
  return 0;
}


jint CallStaticIntMethod(JNIEnv *env, jclass clazz, jmethodID methodID, ...) {
  
  BEGIN_JNI_EXCEPTION
  
  va_list ap;
  va_start(ap, methodID);
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromStaticMethod(vm, meth, clazz);
  jint res = (jint) meth->invokeIntStaticAP(vm, cl, ap);
  va_end(ap);
  return res;

  END_JNI_EXCEPTION
  return 0;
}


jint CallStaticIntMethodV(JNIEnv *env, jclass clazz, jmethodID methodID,
                          va_list args) {
  BEGIN_JNI_EXCEPTION
  
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromStaticMethod(vm, meth, clazz);
  return (jint)meth->invokeIntStaticAP(vm, cl, args);

  END_JNI_EXCEPTION
  
  return 0;
}


jint CallStaticIntMethodA(JNIEnv *env, jclass clazz, jmethodID methodID,
                          const jvalue *args) {
  assert(0 && "implement me");
  return 0;
}


jlong CallStaticLongMethod(JNIEnv *env, jclass clazz, jmethodID methodID, ...) {
  
  BEGIN_JNI_EXCEPTION
  
  va_list ap;
  va_start(ap, methodID);
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromStaticMethod(vm, meth, clazz);
  jlong res = (jlong) meth->invokeLongStaticAP(vm, cl, ap);
  va_end(ap);
  return res;

  END_JNI_EXCEPTION
  return 0;
}


jlong CallStaticLongMethodV(JNIEnv *env, jclass clazz, jmethodID methodID,
			    va_list args) {

  BEGIN_JNI_EXCEPTION
  
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromStaticMethod(vm, meth, clazz);
  return (jlong)meth->invokeLongStaticAP(vm, cl, args);

  END_JNI_EXCEPTION
  
  return 0;
}


jlong CallStaticLongMethodA(JNIEnv *env, jclass clazz, jmethodID methodID,
                            const jvalue *args) {
  assert(0 && "implement me");
  return 0;
}



jfloat CallStaticFloatMethod(JNIEnv *env, jclass clazz, jmethodID methodID,
                             ...) {
  
  BEGIN_JNI_EXCEPTION
  
  va_list ap;
  va_start(ap, methodID);
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromStaticMethod(vm, meth, clazz);
  jfloat res = (jfloat) meth->invokeFloatStaticAP(vm, cl, ap);
  va_end(ap);
  return res;

  END_JNI_EXCEPTION
  return 0.0f;
}


jfloat CallStaticFloatMethodV(JNIEnv *env, jclass clazz, jmethodID methodID,
                              va_list args) {
  
  BEGIN_JNI_EXCEPTION
  
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromStaticMethod(vm, meth, clazz);
  return (jfloat)meth->invokeFloatStaticAP(vm, cl, args);

  END_JNI_EXCEPTION
  
  return 0.0f;
}


jfloat CallStaticFloatMethodA(JNIEnv *env, jclass clazz, jmethodID methodID,
                              const jvalue *args) {
  assert(0 && "implement me");
  return 0;
}


jdouble CallStaticDoubleMethod(JNIEnv *env, jclass clazz, jmethodID methodID,
                               ...) {
  
  BEGIN_JNI_EXCEPTION
  
  va_list ap;
  va_start(ap, methodID);
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromStaticMethod(vm, meth, clazz);
  jdouble res = (jdouble) meth->invokeDoubleStaticAP(vm, cl, ap);
  va_end(ap);
  return res;

  END_JNI_EXCEPTION
  return 0.0;
}


jdouble CallStaticDoubleMethodV(JNIEnv *env, jclass clazz, jmethodID methodID,
                                va_list args) {
  BEGIN_JNI_EXCEPTION
  
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromStaticMethod(vm, meth, clazz);
  return (jdouble)meth->invokeDoubleStaticAP(vm, cl, args);

  END_JNI_EXCEPTION
  
  return 0.0;
}


jdouble CallStaticDoubleMethodA(JNIEnv *env, jclass clazz, jmethodID methodID,
                                const jvalue *args) {
  assert(0 && "implement me");
  return 0;
}


void CallStaticVoidMethod(JNIEnv *env, jclass clazz, jmethodID methodID, ...) {
  
  BEGIN_JNI_EXCEPTION

  va_list ap;
  va_start(ap, methodID);
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromStaticMethod(vm, meth, clazz);
  meth->invokeIntStaticAP(vm, cl, ap);
  va_end(ap);

  END_JNI_EXCEPTION
}


void CallStaticVoidMethodV(JNIEnv *env, jclass clazz, jmethodID methodID,
                           va_list args) {
  
  BEGIN_JNI_EXCEPTION
  
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromStaticMethod(vm, meth, clazz);
  meth->invokeIntStaticAP(vm, cl, args);

  END_JNI_EXCEPTION
}


void CallStaticVoidMethodA(JNIEnv *env, jclass clazz, jmethodID methodID,
                           const jvalue * args) {
  assert(0 && "implement me");
}


jfieldID GetStaticFieldID(JNIEnv *env, jclass clazz, const char *name,
                          const char *sig) {
  
  BEGIN_JNI_EXCEPTION
  
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserCommonClass* cl = 
    UserCommonClass::resolvedImplClass(vm, (JavaObject*)clazz, true);
  return (jfieldID) (cl->asClass() ? 
    cl->asClass()->lookupField(vm->asciizToUTF8(name),
                               vm->asciizToUTF8(sig),
                               true, true, 0) : 0);

  END_JNI_EXCEPTION
  return 0;
}


jobject GetStaticObjectField(JNIEnv *env, jclass clazz, jfieldID fieldID) {

  BEGIN_JNI_EXCEPTION

  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* field = (JavaField*)fieldID;
  UserClass* cl = (UserClass*)
    UserCommonClass::resolvedImplClass(vm, (JavaObject*)clazz, true);
  void* Stat = cl->getStaticInstance();
  return (jobject)field->getObjectField(Stat);

  END_JNI_EXCEPTION
  return 0;
}


jboolean GetStaticBooleanField(JNIEnv *env, jclass clazz, jfieldID fieldID) {
  
  BEGIN_JNI_EXCEPTION

  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* field = (JavaField*)fieldID;
  UserClass* cl = (UserClass*)
    UserCommonClass::resolvedImplClass(vm, (JavaObject*)clazz, true);
  void* Stat = cl->getStaticInstance();
  return (jboolean)field->getInt8Field(Stat);

  END_JNI_EXCEPTION
  return 0;
}


jbyte GetStaticByteField(JNIEnv *env, jclass clazz, jfieldID fieldID) {

  BEGIN_JNI_EXCEPTION

  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* field = (JavaField*)fieldID;
  UserClass* cl = (UserClass*)
    UserCommonClass::resolvedImplClass(vm, (JavaObject*)clazz, true);
  void* Stat = cl->getStaticInstance();
  return (jbyte)field->getInt8Field(Stat);

  END_JNI_EXCEPTION
  return 0;
}


jchar GetStaticCharField(JNIEnv *env, jclass clazz, jfieldID fieldID) {

  BEGIN_JNI_EXCEPTION

  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* field = (JavaField*)fieldID;
  UserClass* cl = (UserClass*)
    UserCommonClass::resolvedImplClass(vm, (JavaObject*)clazz, true);
  void* Stat = cl->getStaticInstance();
  return (jchar)field->getInt16Field(Stat);

  END_JNI_EXCEPTION
  return 0;
}


jshort GetStaticShortField(JNIEnv *env, jclass clazz, jfieldID fieldID) {

  BEGIN_JNI_EXCEPTION

  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* field = (JavaField*)fieldID;
  UserClass* cl = (UserClass*)
    UserCommonClass::resolvedImplClass(vm, (JavaObject*)clazz, true);
  void* Stat = cl->getStaticInstance();
  return (jshort)field->getInt16Field(Stat);

  END_JNI_EXCEPTION
  return 0;
}


jint GetStaticIntField(JNIEnv *env, jclass clazz, jfieldID fieldID) {

  BEGIN_JNI_EXCEPTION

  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* field = (JavaField*)fieldID;
  UserClass* cl = (UserClass*)
    UserCommonClass::resolvedImplClass(vm, (JavaObject*)clazz, true);
  void* Stat = cl->getStaticInstance();
  return (jint)field->getInt32Field(Stat);

  END_JNI_EXCEPTION
  return 0;
}


jlong GetStaticLongField(JNIEnv *env, jclass clazz, jfieldID fieldID) {

  BEGIN_JNI_EXCEPTION

  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* field = (JavaField*)fieldID;
  UserClass* cl = (UserClass*)
    UserCommonClass::resolvedImplClass(vm, (JavaObject*)clazz, true);
  void* Stat = cl->getStaticInstance();
  return (jlong)field->getLongField(Stat);

  END_JNI_EXCEPTION
  return 0;
}


jfloat GetStaticFloatField(JNIEnv *env, jclass clazz, jfieldID fieldID) {

  BEGIN_JNI_EXCEPTION

  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* field = (JavaField*)fieldID;
  UserClass* cl = (UserClass*)
    UserCommonClass::resolvedImplClass(vm, (JavaObject*)clazz, true);
  void* Stat = cl->getStaticInstance();
  return (jfloat)field->getFloatField(Stat);

  END_JNI_EXCEPTION
  return 0;
}


jdouble GetStaticDoubleField(JNIEnv *env, jclass clazz, jfieldID fieldID) {

  BEGIN_JNI_EXCEPTION

  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* field = (JavaField*)fieldID;
  UserClass* cl = (UserClass*)
    UserCommonClass::resolvedImplClass(vm, (JavaObject*)clazz, true);
  void* Stat = cl->getStaticInstance();
  return (jdouble)field->getDoubleField(Stat);

  END_JNI_EXCEPTION
  return 0;
}


void SetStaticObjectField(JNIEnv *env, jclass clazz, jfieldID fieldID,
                          jobject value) {

  BEGIN_JNI_EXCEPTION

  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* field = (JavaField*)fieldID;
  UserClass* cl = (UserClass*)
    UserCommonClass::resolvedImplClass(vm, (JavaObject*)clazz, true);
  void* Stat = cl->getStaticInstance();
  field->setObjectField(Stat, (JavaObject*)value);
  
  END_JNI_EXCEPTION
}


void SetStaticBooleanField(JNIEnv *env, jclass clazz, jfieldID fieldID,
                           jboolean value) {
  
  BEGIN_JNI_EXCEPTION
  
  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* field = (JavaField*)fieldID;
  UserClass* cl = (UserClass*)
    UserCommonClass::resolvedImplClass(vm, (JavaObject*)clazz, true);
  void* Stat = cl->getStaticInstance();
  field->setInt8Field(Stat, (uint8)value);

  END_JNI_EXCEPTION
}


void SetStaticByteField(JNIEnv *env, jclass clazz, jfieldID fieldID,
                        jbyte value) {

  BEGIN_JNI_EXCEPTION

  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* field = (JavaField*)fieldID;
  UserClass* cl = (UserClass*)
    UserCommonClass::resolvedImplClass(vm, (JavaObject*)clazz, true);
  void* Stat = cl->getStaticInstance();
  field->setInt8Field(Stat, (sint8)value);

  END_JNI_EXCEPTION
}


void SetStaticCharField(JNIEnv *env, jclass clazz, jfieldID fieldID,
                        jchar value) {

  BEGIN_JNI_EXCEPTION

  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* field = (JavaField*)fieldID;
  UserClass* cl = (UserClass*)
    UserCommonClass::resolvedImplClass(vm, (JavaObject*)clazz, true);
  void* Stat = cl->getStaticInstance();
  field->setInt16Field(Stat, (uint16)value);

  END_JNI_EXCEPTION
}


void SetStaticShortField(JNIEnv *env, jclass clazz, jfieldID fieldID,
                         jshort value) {

  BEGIN_JNI_EXCEPTION

  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* field = (JavaField*)fieldID;
  UserClass* cl = (UserClass*)
    UserCommonClass::resolvedImplClass(vm, (JavaObject*)clazz, true);
  void* Stat = cl->getStaticInstance();
  field->setInt16Field(Stat, (sint16)value);

  END_JNI_EXCEPTION
}


void SetStaticIntField(JNIEnv *env, jclass clazz, jfieldID fieldID,
                       jint value) {

  BEGIN_JNI_EXCEPTION

  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* field = (JavaField*)fieldID;
  UserClass* cl = (UserClass*)
    UserCommonClass::resolvedImplClass(vm, (JavaObject*)clazz, true);
  void* Stat = cl->getStaticInstance();
  field->setInt32Field(Stat, (sint32)value);

  END_JNI_EXCEPTION
}


void SetStaticLongField(JNIEnv *env, jclass clazz, jfieldID fieldID,
                        jlong value) {

  BEGIN_JNI_EXCEPTION

  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* field = (JavaField*)fieldID;
  UserClass* cl = (UserClass*)
    UserCommonClass::resolvedImplClass(vm, (JavaObject*)clazz, true);
  void* Stat = cl->getStaticInstance();
  field->setLongField(Stat, (sint64)value);

  END_JNI_EXCEPTION
}


void SetStaticFloatField(JNIEnv *env, jclass clazz, jfieldID fieldID,
                         jfloat value) {

  BEGIN_JNI_EXCEPTION

  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* field = (JavaField*)fieldID;
  UserClass* cl = (UserClass*)
    UserCommonClass::resolvedImplClass(vm, (JavaObject*)clazz, true);
  void* Stat = cl->getStaticInstance();
  field->setFloatField(Stat, (float)value);

  END_JNI_EXCEPTION
}


void SetStaticDoubleField(JNIEnv *env, jclass clazz, jfieldID fieldID,
                          jdouble value) {

  BEGIN_JNI_EXCEPTION

  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* field = (JavaField*)fieldID;
  UserClass* cl = (UserClass*)
    UserCommonClass::resolvedImplClass(vm, (JavaObject*)clazz, true);
  void* Stat = cl->getStaticInstance();
  field->setDoubleField(Stat, (double)value);

  END_JNI_EXCEPTION
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

  BEGIN_JNI_EXCEPTION

  Jnjvm* vm = myVM(env);
  return (jstring)(vm->asciizToStr(bytes));

  END_JNI_EXCEPTION
  return 0;
}


jsize GetStringUTFLength (JNIEnv *env, jstring string) {   
  assert(0 && "implement me");
  return 0;
}


const char *GetStringUTFChars(JNIEnv *env, jstring string, jboolean *isCopy) {

  BEGIN_JNI_EXCEPTION

  if (isCopy != 0) (*isCopy) = true;
  return strdup(((JavaString*)string)->strToAsciiz());

  END_JNI_EXCEPTION
  return 0;
}


void ReleaseStringUTFChars(JNIEnv *env, jstring string, const char *utf) {
  free((void*)utf);
}


jsize GetArrayLength(JNIEnv *env, jarray array) {

  BEGIN_JNI_EXCEPTION

  return ((JavaArray*)array)->size;

  END_JNI_EXCEPTION
  return 0;
}


jobjectArray NewObjectArray(JNIEnv *env, jsize length, jclass elementClass,
                            jobject initialElement) {
  BEGIN_JNI_EXCEPTION
  Jnjvm* vm = myVM(env);
  if (length < 0) vm->negativeArraySizeException(length);
  
  UserCommonClass* base = 
    UserCommonClass::resolvedImplClass(vm, (JavaObject*)elementClass, true);
  JnjvmClassLoader* loader = base->classLoader;
  const UTF8* name = base->getName();
  const UTF8* arrayName = loader->constructArrayName(1, name);
  UserClassArray* array = loader->constructArray(arrayName);
  ArrayObject* res = (ArrayObject*)array->doNew(length, vm);
  if (initialElement) {
    for (sint32 i = 0; i < length; ++i) {
      res->elements[i] = (JavaObject*)initialElement;
    }
  }
  return (jobjectArray)res;
  END_JNI_EXCEPTION
  return 0;
}


jobject GetObjectArrayElement(JNIEnv *env, jobjectArray array, jsize index) {
  BEGIN_JNI_EXCEPTION
  
  ArrayObject* JA = (ArrayObject*)array;
  if (index >= JA->size)
    JavaThread::get()->getJVM()->indexOutOfBounds(JA, index);
  return (jobject)JA->elements[index];
  
  END_JNI_EXCEPTION

  return 0;
}


void SetObjectArrayElement(JNIEnv *env, jobjectArray array, jsize index,
                           jobject val) {
  
  BEGIN_JNI_EXCEPTION
  
  ArrayObject* JA = (ArrayObject*)array;
  if (index >= JA->size)
    JavaThread::get()->getJVM()->indexOutOfBounds(JA, index);
  JA->elements[index] = (JavaObject*)val;

  END_JNI_EXCEPTION
}


jbooleanArray NewBooleanArray(JNIEnv *env, jsize len) {
  
  BEGIN_JNI_EXCEPTION
  
  Jnjvm* vm = myVM(env);
  return (jbooleanArray)vm->upcalls->ArrayOfByte->doNew(len, vm);

  END_JNI_EXCEPTION
  return 0;
}


jbyteArray NewByteArray(JNIEnv *env, jsize len) {
  
  BEGIN_JNI_EXCEPTION

  Jnjvm* vm = myVM(env);
  return (jbyteArray)vm->upcalls->ArrayOfByte->doNew(len, vm);

  END_JNI_EXCEPTION
  return 0;
}


jcharArray NewCharArray(JNIEnv *env, jsize len) {
  
  BEGIN_JNI_EXCEPTION
  
  Jnjvm* vm = myVM(env);
  return (jcharArray)vm->upcalls->ArrayOfChar->doNew(len, vm);

  END_JNI_EXCEPTION
  return 0;
}


jshortArray NewShortArray(JNIEnv *env, jsize len) {
  
  BEGIN_JNI_EXCEPTION
  
  Jnjvm* vm = myVM(env);
  return (jshortArray)vm->upcalls->ArrayOfShort->doNew(len, vm);

  END_JNI_EXCEPTION
  return 0;
}


jintArray NewIntArray(JNIEnv *env, jsize len) {
  
  BEGIN_JNI_EXCEPTION
  
  Jnjvm* vm = myVM(env);
  return (jintArray)vm->upcalls->ArrayOfInt->doNew(len, vm);

  END_JNI_EXCEPTION
  return 0;
}


jlongArray NewLongArray(JNIEnv *env, jsize len) {
  
  BEGIN_JNI_EXCEPTION
  
  Jnjvm* vm = myVM(env);
  return (jlongArray)vm->upcalls->ArrayOfLong->doNew(len, vm);

  END_JNI_EXCEPTION
  return 0;
}


jfloatArray NewFloatArray(JNIEnv *env, jsize len) {
  
  BEGIN_JNI_EXCEPTION
  
  Jnjvm* vm = myVM(env);
  return (jfloatArray)vm->upcalls->ArrayOfFloat->doNew(len, vm);

  END_JNI_EXCEPTION
  return 0;
}


jdoubleArray NewDoubleArray(JNIEnv *env, jsize len) {
  
  BEGIN_JNI_EXCEPTION
  
  Jnjvm* vm = myVM(env);
  return (jdoubleArray)vm->upcalls->ArrayOfDouble->doNew(len, vm);

  END_JNI_EXCEPTION
  return 0;
}


jboolean *GetBooleanArrayElements(JNIEnv *env, jbooleanArray array,
				  jboolean *isCopy) {
  
  BEGIN_JNI_EXCEPTION
  
  if (isCopy) (*isCopy) = false;
  return (jboolean*)((ArrayUInt8*)array)->elements;

  END_JNI_EXCEPTION
  return 0;
}


jbyte *GetByteArrayElements(JNIEnv *env, jbyteArray array, jboolean *isCopy) {

  BEGIN_JNI_EXCEPTION

  if (isCopy) (*isCopy) = false;
  return ((ArraySInt8*)array)->elements;

  END_JNI_EXCEPTION
  return 0;
}


jchar *GetCharArrayElements(JNIEnv *env, jcharArray array, jboolean *isCopy) {

  BEGIN_JNI_EXCEPTION

  if (isCopy) (*isCopy) = false;
  return ((ArrayUInt16*)array)->elements;

  END_JNI_EXCEPTION
  return 0;
}


jshort *GetShortArrayElements(JNIEnv *env, jshortArray array,
                              jboolean *isCopy) {
  
  BEGIN_JNI_EXCEPTION
  
  if (isCopy) (*isCopy) = false;
  return ((ArraySInt16*)array)->elements;

  END_JNI_EXCEPTION
  return 0;
}


jint *GetIntArrayElements(JNIEnv *env, jintArray array, jboolean *isCopy) {

  BEGIN_JNI_EXCEPTION

  if (isCopy) (*isCopy) = false;
  return ((ArraySInt32*)array)->elements;

  END_JNI_EXCEPTION
  return 0;
}


jlong *GetLongArrayElements(JNIEnv *env, jlongArray array, jboolean *isCopy) {

  BEGIN_JNI_EXCEPTION

  if (isCopy) (*isCopy) = false;
  return (jlong*)(void*)(((ArrayLong*)array)->elements);

  END_JNI_EXCEPTION
  return 0;
}


jfloat *GetFloatArrayElements(JNIEnv *env, jfloatArray array,
                              jboolean *isCopy) {

  BEGIN_JNI_EXCEPTION

  if (isCopy) (*isCopy) = false;
  return ((ArrayFloat*)array)->elements;

  END_JNI_EXCEPTION
  return 0;
}


jdouble *GetDoubleArrayElements(JNIEnv *env, jdoubleArray array,
				jboolean *isCopy) {
  
  BEGIN_JNI_EXCEPTION
  
  if (isCopy) (*isCopy) = false;
  return ((ArrayDouble*)array)->elements;

  END_JNI_EXCEPTION
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
  ArrayUInt8* Array = (ArrayUInt8*)array;
  memcpy(buf, &(Array->elements[start]), len * sizeof(uint8));
}


void GetByteArrayRegion(JNIEnv *env, jbyteArray array, jsize start, jsize len,
			jbyte *buf) {
  ArraySInt8* Array = (ArraySInt8*)array;
  memcpy(buf, &(Array->elements[start]), len * sizeof(sint8));
}


void GetCharArrayRegion(JNIEnv *env, jcharArray array, jsize start, jsize len,
			jchar *buf) {
  ArrayUInt16* Array = (ArrayUInt16*)array;
  memcpy(buf, &(Array->elements[start]), len * sizeof(uint16));
}


void GetShortArrayRegion(JNIEnv *env, jshortArray array, jsize start,
			 jsize len, jshort *buf) {
  ArraySInt16* Array = (ArraySInt16*)array;
  memcpy(buf, &(Array->elements[start]), len * sizeof(sint16));
}


void GetIntArrayRegion(JNIEnv *env, jintArray array, jsize start, jsize len,
		       jint *buf) {
  ArraySInt32* Array = (ArraySInt32*)array;
  memcpy(buf, &(Array->elements[start]), len * sizeof(sint32));
}


void GetLongArrayRegion(JNIEnv *env, jlongArray array, jsize start, jsize len,
		        jlong *buf) {
  ArrayLong* Array = (ArrayLong*)array;
  memcpy(buf, &(Array->elements[start]), len * sizeof(sint64));
}


void GetFloatArrayRegion(JNIEnv *env, jfloatArray array, jsize start,
			 jsize len, jfloat *buf) {
  ArrayFloat* Array = (ArrayFloat*)array;
  memcpy(buf, &(Array->elements[start]), len * sizeof(float));
}


void GetDoubleArrayRegion(JNIEnv *env, jdoubleArray array, jsize start,
			  jsize len, jdouble *buf) {
  ArrayDouble* Array = (ArrayDouble*)array;
  memcpy(buf, &(Array->elements[start]), len * sizeof(double));
}


void SetBooleanArrayRegion(JNIEnv *env, jbooleanArray array, jsize start,
			   jsize len, const jboolean *buf) {
  ArrayUInt8* Array = (ArrayUInt8*)array;
  memcpy(&(Array->elements[start]), buf, len * sizeof(uint8));
}


void SetByteArrayRegion(JNIEnv *env, jbyteArray array, jsize start, jsize len,
			                  const jbyte *buf) {
  ArraySInt8* Array = (ArraySInt8*)array;
  memcpy(&(Array->elements[start]), buf, len * sizeof(sint8));
}


void SetCharArrayRegion(JNIEnv *env, jcharArray array, jsize start, jsize len,
			                  const jchar *buf) {
  ArrayUInt16* Array = (ArrayUInt16*)array;
  memcpy(&(Array->elements[start]), buf, len * sizeof(uint16));
}


void SetShortArrayRegion(JNIEnv *env, jshortArray array, jsize start,
			                   jsize len, const jshort *buf) {
  ArraySInt16* Array = (ArraySInt16*)array;
  memcpy(&(Array->elements[start]), buf, len * sizeof(sint16));
}


void SetIntArrayRegion(JNIEnv *env, jintArray array, jsize start, jsize len,
		                   const jint *buf) {
  ArraySInt32* Array = (ArraySInt32*)array;
  memcpy(&(Array->elements[start]), buf, len * sizeof(sint32));
}


void SetLongArrayRegion(JNIEnv* env, jlongArray array, jsize start, jsize len,
			                  const jlong *buf) {
  ArrayLong* Array = (ArrayLong*)array;
  memcpy(&(Array->elements[start]), buf, len * sizeof(sint64));
}


void SetFloatArrayRegion(JNIEnv *env, jfloatArray array, jsize start,
			                   jsize len, const jfloat *buf) {
  ArrayFloat* Array = (ArrayFloat*)array;
  memcpy(&(Array->elements[start]), buf, len * sizeof(float));
}


void SetDoubleArrayRegion(JNIEnv *env, jdoubleArray array, jsize start,
			                    jsize len, const jdouble *buf) {
  ArrayDouble* Array = (ArrayDouble*)array;
  memcpy(&(Array->elements[start]), buf, len * sizeof(double));
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
  
  BEGIN_JNI_EXCEPTION
  
  ((JavaObject*)obj)->acquire();
  return 1;

  END_JNI_EXCEPTION
  return 0;
}


jint MonitorExit(JNIEnv *env, jobject obj) {

  BEGIN_JNI_EXCEPTION

  ((JavaObject*)obj)->release();
  return 1;

  END_JNI_EXCEPTION
  return 0;
}


jint GetJavaVM(JNIEnv *env, JavaVM **vm) {
  BEGIN_JNI_EXCEPTION
  Jnjvm* myvm = JavaThread::get()->getJVM();
  (*vm) = (JavaVM*)(void*)(&(myvm->javavmEnv));
  return 0;
  END_JNI_EXCEPTION
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
  BEGIN_JNI_EXCEPTION
  
  if (isCopy) (*isCopy) = false;
  return ((JavaArray*)array)->elements;

  END_JNI_EXCEPTION
  return 0;
}


void ReleasePrimitiveArrayCritical(JNIEnv *env, jarray array, void *carray,
				   jint mode) {
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
  Jnjvm* vm = myVM(env);
  vm->globalRefsLock.lock();
  vm->globalRefs.push_back((JavaObject*)obj);
  vm->globalRefsLock.unlock();
  return obj;
}


void DeleteGlobalRef(JNIEnv* env, jobject globalRef) {
  Jnjvm* vm = myVM(env);
  vm->globalRefsLock.lock();
  for (std::vector<JavaObject*, gc_allocator<JavaObject*> >::iterator i =
                                                      vm->globalRefs.begin(),
            e = vm->globalRefs.end(); i!= e; ++i) {
    if ((*i) == (JavaObject*)globalRef) {
      vm->globalRefs.erase(i);
      break;
    }
  }
  vm->globalRefsLock.unlock();
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

  BEGIN_JNI_EXCEPTION

  Jnjvm* vm = myVM(env);
  JavaObject* buf = (JavaObject*)_buf;
  JavaObject* address = vm->upcalls->bufferAddress->getObjectField(buf);
  if (address != 0) {
    int res = vm->upcalls->dataPointer32->getInt32Field(address);
    return (void*)res;
  } else {
    return 0;
  }

  END_JNI_EXCEPTION
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

  BEGIN_JNI_EXCEPTION

  JavaThread* _th = JavaThread::get();
  JavaObject* th = _th->currentThread();
  Jnjvm* myvm = _th->getJVM();
  if (th != 0) {
    (*env) = &(myvm->jniEnv);
    return JNI_OK;
  } else {
    (*env) = 0;
    return JNI_EDETACHED;
  }

  END_JNI_EXCEPTION
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
