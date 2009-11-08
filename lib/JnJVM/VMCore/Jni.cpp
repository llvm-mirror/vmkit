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
                                           JavaObject* clazz) {
#ifdef ISOLATE_SHARING
  return (UserClass*)UserCommonClass::resolvedImplClass(vm, clazz, false);
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
  fprintf(stderr, "Implement me\n"); abort();
  return 0;
}


jclass FindClass(JNIEnv *env, const char *asciiz) {
  
  BEGIN_JNI_EXCEPTION

  JnjvmClassLoader* loader = 0;
  JavaThread* th = JavaThread::get();
  Jnjvm* vm = th->getJVM();
  UserClass* currentClass = th->getCallingClassLevel(0);
  if (currentClass) loader = currentClass->classLoader;
  else loader = vm->appClassLoader;

  UserCommonClass* cl = loader->loadClassFromAsciiz(asciiz, true, true);
  if (cl && cl->asClass()) cl->asClass()->initialiseClass(vm);
  jclass res = (jclass)cl->getClassDelegateePtr(vm);
  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}
  

jmethodID FromReflectedMethod(JNIEnv *env, jobject method) {
  
  BEGIN_JNI_EXCEPTION
 
  // Local object references. 
  JavaObject* meth = *(JavaObject**)method;
  llvm_gcroot(meth, 0);

  Jnjvm* vm = myVM(env);
  Classpath* upcalls = vm->upcalls;
  UserCommonClass* cl = meth->getClass();
  if (cl == upcalls->newConstructor)  {
    jmethodID res = (jmethodID)((JavaObjectMethod*)meth)->getInternalMethod();
    RETURN_FROM_JNI(res);
  } else if (cl == upcalls->newMethod) {
    jmethodID res = (jmethodID)((JavaObjectConstructor*)meth)->getInternalMethod();
    RETURN_FROM_JNI(res);
  }
  
  END_JNI_EXCEPTION
  
  RETURN_FROM_JNI(0);
}


jclass GetSuperclass(JNIEnv *env, jclass sub) {
  fprintf(stderr, "Implement me\n"); abort();
  return 0;
}
  
 
jboolean IsAssignableFrom(JNIEnv *env, jclass _sub, jclass _sup) {
  
  BEGIN_JNI_EXCEPTION
 
  // Local object references.
  JavaObject* sub = *(JavaObject**)_sub;
  JavaObject* sup = *(JavaObject**)_sup;
  llvm_gcroot(sub, 0);
  llvm_gcroot(sup, 0);
  
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserCommonClass* cl2 = 
    UserCommonClass::resolvedImplClass(vm, sup, false);
  UserCommonClass* cl1 = 
    UserCommonClass::resolvedImplClass(vm, sub, false);

  jboolean res = (jboolean)cl1->isAssignableFrom(cl2);
  RETURN_FROM_JNI(res);
  
  END_JNI_EXCEPTION

  RETURN_FROM_JNI(false);
}


jint Throw(JNIEnv *env, jthrowable obj) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}


jint ThrowNew(JNIEnv* env, jclass _Cl, const char *msg) {
  
  BEGIN_JNI_EXCEPTION
 
  // Local object references.
  JavaObject* Cl = *(JavaObject**)_Cl;
  JavaObject* res = 0;
  llvm_gcroot(Cl, 0);
  llvm_gcroot(res, 0);
  
  Jnjvm* vm = JavaThread::get()->getJVM();
  
  verifyNull(Cl);
  UserCommonClass* cl = UserCommonClass::resolvedImplClass(vm, Cl, true);
  if (!cl->isClass()) RETURN_FROM_JNI(0);

  UserClass* realCl = cl->asClass();
  res = realCl->doNew(vm);
  JavaMethod* init = realCl->lookupMethod(vm->bootstrapLoader->initName,
                                          vm->bootstrapLoader->initExceptionSig,
                                          false, true, 0);
  init->invokeIntSpecial(vm, realCl, res, vm->asciizToStr(msg));
  th->pendingException = res;
  
  th->endNative();
  th->addresses.pop_back();
  th->lastKnownFrame = th->lastKnownFrame->previousFrame;
  th->enterUncooperativeCode(SP);
  th->throwFromJNI();
  
  RETURN_FROM_JNI(1);
  
  END_JNI_EXCEPTION

  RETURN_FROM_JNI(0);
}


jthrowable ExceptionOccurred(JNIEnv *env) {
  
  BEGIN_JNI_EXCEPTION

  JavaObject* obj = JavaThread::get()->pendingException;
  llvm_gcroot(obj, 0);
  jthrowable res = (jthrowable)th->pushJNIRef(obj);
  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION

  RETURN_FROM_JNI(0);
}


void ExceptionDescribe(JNIEnv *env) {
  fprintf(stderr, "Implement me\n");
  abort();
}


void ExceptionClear(JNIEnv *env) {
  fprintf(stderr, "Implement me\n"); 
  abort();
}


void FatalError(JNIEnv *env, const char *msg) {
  fprintf(stderr, "Implement me\n");
  abort();
}


jint PushLocalFrame(JNIEnv* env, jint capacity) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}

jobject PopLocalFrame(JNIEnv* env, jobject result) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}


void DeleteLocalRef(JNIEnv *env, jobject localRef) {
}


jboolean IsSameObject(JNIEnv *env, jobject ref1, jobject ref2) {
  
  BEGIN_JNI_EXCEPTION

  JavaObject* Ref1 = *(JavaObject**)ref1;
  JavaObject* Ref2 = *(JavaObject**)ref2;
  llvm_gcroot(Ref1, 0);
  llvm_gcroot(Ref2, 0);

  RETURN_FROM_JNI(Ref1 == Ref2);
  
  END_JNI_EXCEPTION
  RETURN_FROM_JNI(false);
}


jobject NewLocalRef(JNIEnv *env, jobject ref) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}


jint EnsureLocalCapacity(JNIEnv* env, jint capacity) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}


jobject AllocObject(JNIEnv *env, jclass _clazz) {
  
  BEGIN_JNI_EXCEPTION
 
  // Local object references.  
  JavaObject* clazz = *(JavaObject**)_clazz;
  JavaObject* res = 0;
  llvm_gcroot(clazz, 0);
  llvm_gcroot(res, 0);

  JavaThread* th = JavaThread::get();
  Jnjvm* vm = th->getJVM();

  UserCommonClass* cl = UserCommonClass::resolvedImplClass(vm, clazz, true);
  if (!cl->isClass()) RETURN_FROM_JNI(0);

  // Store local reference
  res = cl->asClass()->doNew(vm);
 
  jobject ret = (jobject)th->pushJNIRef(res);
  RETURN_FROM_JNI(ret);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


jobject NewObject(JNIEnv *env, jclass _clazz, jmethodID methodID, ...) {
  BEGIN_JNI_EXCEPTION
  
 
  // Local object references
  JavaObject* clazz = *(JavaObject**)_clazz;
  JavaObject* res = 0;
  llvm_gcroot(clazz, 0);
  llvm_gcroot(res, 0);
  
  JavaMethod* meth = (JavaMethod*)methodID;
  JavaThread* th = JavaThread::get();
  Jnjvm* vm = th->getJVM();
  UserCommonClass* cl = UserCommonClass::resolvedImplClass(vm, clazz, true);
  if (!cl->isClass()) RETURN_FROM_JNI(0);
  
  // Store local reference
  res = cl->asClass()->doNew(vm);

  va_list ap;
  va_start(ap, methodID);
  meth->invokeIntSpecialAP(vm, cl->asClass(), res, ap, true);
  va_end(ap);
 
  jobject ret = (jobject)th->pushJNIRef(res);
  RETURN_FROM_JNI(ret);
  
  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


jobject NewObjectV(JNIEnv* env, jclass clazz, jmethodID methodID,
                   va_list args) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}

#define BufToBuf(_args, _buf, signature) \
  Typedef* const* arguments = signature->getArgumentsType(); \
  uintptr_t __buf = (uintptr_t)_buf; \
  uintptr_t __args = (uintptr_t)_args;\
  for (uint32 i = 0; i < signature->nbArguments; ++i) { \
    const Typedef* type = arguments[i];\
    if (type->isPrimitive()) {\
      const PrimitiveTypedef* prim = (PrimitiveTypedef*)type;\
      if (prim->isLong()) {\
        ((sint64*)__buf)[0] = ((sint64*)__args)[0];\
      } else if (prim->isInt()){ \
        ((sint32*)__buf)[0] = ((sint32*)__args)[0];\
      } else if (prim->isChar()) { \
        ((uint32*)__buf)[0] = ((uint32*)__args)[0];\
      } else if (prim->isShort()) { \
        ((uint32*)__buf)[0] = ((uint32*)__args)[0];\
      } else if (prim->isByte()) { \
        ((uint32*)__buf)[0] = ((uint32*)__args)[0];\
      } else if (prim->isBool()) { \
        ((uint32*)__buf)[0] = ((uint32*)__args)[0];\
      } else if (prim->isFloat()) {\
        ((float*)__buf)[0] = ((float*)__args)[0];\
      } else if (prim->isDouble()) {\
        ((double*)__buf)[0] = ((double*)__args)[0];\
      } else {\
        fprintf(stderr, "Can't happen");\
        abort();\
      }\
    } else{\
      JavaObject** obj = ((JavaObject***)__args)[0];\
      if (obj) {\
        ((JavaObject**)__buf)[0] = *obj;\
      } else {\
        ((JavaObject**)__buf)[0] = 0;\
      }\
    }\
    __buf += 8; \
    __args += 8; \
  }\


jobject NewObjectA(JNIEnv* env, jclass _clazz, jmethodID methodID,
                   const jvalue *args) {
  
  BEGIN_JNI_EXCEPTION
  
  // Local object references
  JavaObject* clazz = *(JavaObject**)_clazz;
  JavaObject* res = 0;
  llvm_gcroot(clazz, 0);
  llvm_gcroot(res, 0);

  JavaThread* th = JavaThread::get();
  Jnjvm* vm = th->getJVM();
  JavaMethod* meth = (JavaMethod*)methodID;

  Signdef* sign = meth->getSignature();
  uintptr_t buf = (uintptr_t)alloca(sign->nbArguments * sizeof(uint64));
  BufToBuf(args, buf, sign);

  UserCommonClass* cl = UserCommonClass::resolvedImplClass(vm, clazz, true);
  
  // Store local reference
  res = cl->asClass()->doNew(vm);

  meth->invokeIntSpecialBuf(vm, cl->asClass(), res, (void*)buf);

  jobject ret = (jobject)th->pushJNIRef(res);
  RETURN_FROM_JNI(ret);
  
  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


jclass GetObjectClass(JNIEnv *env, jobject _obj) {
  
  BEGIN_JNI_EXCEPTION

  // Local object references
  JavaObject* obj = *(JavaObject**)_obj;
  llvm_gcroot(obj, 0);

  JavaThread* th = JavaThread::get();
  Jnjvm* vm = th->getJVM();
  
  // Store local reference
  jclass res = (jclass)obj->getClass()->getClassDelegateePtr(vm);
  RETURN_FROM_JNI(res);
  
  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


jboolean IsInstanceOf(JNIEnv *env, jobject obj, jclass clazz) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}


jfieldID FromReflectedField(JNIEnv* env, jobject field) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}


jobject ToReflectedMethod(JNIEnv* env, jclass cls, jmethodID methodID,
                               jboolean isStatic) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}


jobject ToReflectedField(JNIEnv* env, jclass cls, jfieldID fieldID,
			 jboolean isStatic) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}


jmethodID GetMethodID(JNIEnv* env, jclass _clazz, const char *aname,
		                  const char *atype) {
  
  BEGIN_JNI_EXCEPTION
 
  // Local object references.
  JavaObject* clazz = *(JavaObject**)_clazz;
  llvm_gcroot(clazz, 0);

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserCommonClass* cl = UserCommonClass::resolvedImplClass(vm, clazz, true);

  UserClass* realCl = cl->isClass() ? cl->asClass() : cl->super;

  const UTF8* name = cl->classLoader->hashUTF8->lookupAsciiz(aname);
  if (name) {
    const UTF8* type = cl->classLoader->hashUTF8->lookupAsciiz(atype);
    if (type) {
      JavaMethod* meth = realCl->lookupMethod(name, type, false, true, 0);
      RETURN_FROM_JNI((jmethodID)meth);
    }
  }

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


jobject CallObjectMethod(JNIEnv *env, jobject _obj, jmethodID methodID, ...) {

  BEGIN_JNI_EXCEPTION

  verifyNull(_obj);

  va_list ap;
  va_start(ap, methodID);
  
  // Local object references.
  JavaObject* obj = *(JavaObject**)_obj;
  JavaObject* res = 0;
  llvm_gcroot(obj, 0);
  llvm_gcroot(res, 0);
  
  JavaMethod* meth = (JavaMethod*)methodID;
  JavaThread* th = JavaThread::get();
  Jnjvm* vm = th->getJVM();
  UserClass* cl = getClassFromVirtualMethod(vm, meth, obj->getClass());
  
  res = meth->invokeJavaObjectVirtualAP(vm, cl, obj, ap, true);
  va_end(ap);

  jobject ret = (jobject)th->pushJNIRef(res);
  RETURN_FROM_JNI(ret);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


jobject CallObjectMethodV(JNIEnv *env, jobject _obj, jmethodID methodID,
                          va_list args) {
  
  BEGIN_JNI_EXCEPTION
  
  verifyNull(_obj);
 
  // Local object references.
  JavaObject* obj = *(JavaObject**)_obj;
  JavaObject* res = 0;
  llvm_gcroot(obj, 0);
  llvm_gcroot(res, 0);
  
  JavaMethod* meth = (JavaMethod*)methodID;
  JavaThread* th = JavaThread::get();
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromVirtualMethod(vm, meth, obj->getClass());
  
  // Store local reference.
  res = meth->invokeJavaObjectVirtualAP(vm, cl, obj, args, true);
  
  jobject ret = (jobject)th->pushJNIRef(res);
  RETURN_FROM_JNI(ret);

  END_JNI_EXCEPTION
  
  RETURN_FROM_JNI(0);
}


jobject CallObjectMethodA(JNIEnv *env, jobject obj, jmethodID methodID,
                          const jvalue * args) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}


jboolean CallBooleanMethod(JNIEnv *env, jobject _obj, jmethodID methodID, ...) {

  BEGIN_JNI_EXCEPTION
  
  verifyNull(_obj);
  
  va_list ap;
  va_start(ap, methodID);
  
  // Local object references.  
  JavaObject* self = *(JavaObject**)_obj;
  llvm_gcroot(self, 0);

  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromVirtualMethod(vm, meth, self->getClass());
  
  uint32 res = meth->invokeIntVirtualAP(vm, cl, self, ap, true);
  va_end(ap);

  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


jboolean CallBooleanMethodV(JNIEnv *env, jobject _obj, jmethodID methodID,
                            va_list args) {
  
  BEGIN_JNI_EXCEPTION
  
  verifyNull(_obj);
  
  // Local object references.
  JavaObject* obj = *(JavaObject**)_obj;
  llvm_gcroot(obj, 0);
  
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromVirtualMethod(vm, meth, obj->getClass());
  jboolean res = (jboolean)meth->invokeIntVirtualAP(vm, cl, obj, args, true);
  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION
  
  RETURN_FROM_JNI(0);
}


jboolean CallBooleanMethodA(JNIEnv *env, jobject obj, jmethodID methodID,
                            const jvalue * args) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}


jbyte CallByteMethod(JNIEnv *env, jobject obj, jmethodID methodID, ...) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}

jbyte CallByteMethodV(JNIEnv *env, jobject _obj, jmethodID methodID,
                      va_list args) {
  
  BEGIN_JNI_EXCEPTION
 
  verifyNull(_obj);
  
  // Local object references.
  JavaObject* obj = *(JavaObject**)_obj;
  llvm_gcroot(obj, 0);

  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromVirtualMethod(vm, meth, obj->getClass());
  jbyte res = (jbyte)meth->invokeIntVirtualAP(vm, cl, obj, args, true);
  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION
  
  RETURN_FROM_JNI(0);
}


jbyte CallByteMethodA(JNIEnv *env, jobject obj, jmethodID methodID,
                      const jvalue *args) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}


jchar CallCharMethod(JNIEnv *env, jobject obj, jmethodID methodID, ...) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}


jchar CallCharMethodV(JNIEnv *env, jobject _obj, jmethodID methodID,
                      va_list args) {
  
  BEGIN_JNI_EXCEPTION
  
  verifyNull(_obj);
  
  // Local object references.
  JavaObject* obj = *(JavaObject**)_obj;
  llvm_gcroot(obj, 0);
 
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromVirtualMethod(vm, meth, obj->getClass());
  jchar res = (jchar)meth->invokeIntVirtualAP(vm, cl, obj, args, true);
  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION
  
  RETURN_FROM_JNI(0);
}


jchar CallCharMethodA(JNIEnv *env, jobject obj, jmethodID methodID,
                      const jvalue *args) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}


jshort CallShortMethod(JNIEnv *env, jobject obj, jmethodID methodID, ...) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}


jshort CallShortMethodV(JNIEnv *env, jobject _obj, jmethodID methodID,
                        va_list args) {
  
  BEGIN_JNI_EXCEPTION
  
  verifyNull(_obj);
  
  // Local object references.
  JavaObject* obj = *(JavaObject**)_obj;
  llvm_gcroot(obj, 0);
  
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromVirtualMethod(vm, meth, obj->getClass());
  jshort res = (jshort)meth->invokeIntVirtualAP(vm, cl, obj, args, true);
  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION
  
  RETURN_FROM_JNI(0);
}


jshort CallShortMethodA(JNIEnv *env, jobject obj, jmethodID methodID,
                        const jvalue *args) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}



jint CallIntMethod(JNIEnv *env, jobject _obj, jmethodID methodID, ...) {

  BEGIN_JNI_EXCEPTION
  
  verifyNull(_obj);
  
  va_list ap;
  va_start(ap, methodID);
  
  // Local object references.
  JavaObject* obj = *(JavaObject**)_obj;
  llvm_gcroot(obj, 0);

  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromVirtualMethod(vm, meth, obj->getClass());
  
  uint32 res = meth->invokeIntVirtualAP(vm, cl, obj, ap, true);
  va_end(ap);
  
  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


jint CallIntMethodV(JNIEnv *env, jobject _obj, jmethodID methodID,
                    va_list args) {
  
  BEGIN_JNI_EXCEPTION
  
  verifyNull(_obj);
  
  // Local object references.
  JavaObject* obj = *(JavaObject**)_obj;
  llvm_gcroot(obj, 0);
  
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromVirtualMethod(vm, meth, obj->getClass());
  
  jint res = (jint)meth->invokeIntVirtualAP(vm, cl, obj, args, true);
  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION
  
  RETURN_FROM_JNI(0);
}


jint CallIntMethodA(JNIEnv *env, jobject _obj, jmethodID methodID,
                    const jvalue *args) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}



jlong CallLongMethod(JNIEnv *env, jobject _obj, jmethodID methodID, ...) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}


jlong CallLongMethodV(JNIEnv *env, jobject _obj, jmethodID methodID,
                      va_list args) {
  
  BEGIN_JNI_EXCEPTION
  
  verifyNull(_obj);
  
  // Local object references.
  JavaObject* obj = *(JavaObject**)_obj;
  llvm_gcroot(obj, 0);
  
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromVirtualMethod(vm, meth, obj->getClass());
  jlong res = (jlong)meth->invokeLongVirtualAP(vm, cl, obj, args, true);
  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION
  
  RETURN_FROM_JNI(0);
}


jlong CallLongMethodA(JNIEnv *env, jobject obj, jmethodID methodID,
                      const jvalue *args) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}



jfloat CallFloatMethod(JNIEnv *env, jobject _obj, jmethodID methodID, ...) {
  
  BEGIN_JNI_EXCEPTION
  
  verifyNull(_obj);
  
  va_list ap;
  va_start(ap, methodID);
  
  // Local object references.
  JavaObject* obj = *(JavaObject**)_obj;
  llvm_gcroot(obj, 0);

  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromVirtualMethod(vm, meth, obj->getClass());
  jfloat res = meth->invokeFloatVirtualAP(vm, cl, obj, ap, true);
  va_end(ap);
  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION;
  RETURN_FROM_JNI(0.0);
}


jfloat CallFloatMethodV(JNIEnv *env, jobject _obj, jmethodID methodID,
                        va_list args) {
  
  BEGIN_JNI_EXCEPTION
 
  verifyNull(_obj);
  
  // Local object references.
  JavaObject* obj = *(JavaObject**)_obj;
  llvm_gcroot(obj, 0);
  
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromVirtualMethod(vm, meth, obj->getClass());
  jfloat res = (jfloat)meth->invokeFloatVirtualAP(vm, cl, obj, args, true);
  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION
  
  RETURN_FROM_JNI(0.0f);
}


jfloat CallFloatMethodA(JNIEnv *env, jobject _obj, jmethodID methodID,
                        const jvalue *args) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}



jdouble CallDoubleMethod(JNIEnv *env, jobject _obj, jmethodID methodID, ...) {
  
  BEGIN_JNI_EXCEPTION
  
  verifyNull(_obj);
  
  va_list ap;
  va_start(ap, methodID);
  
  // Local object references.
  JavaObject* obj = *(JavaObject**)_obj;
  llvm_gcroot(obj, 0);

  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromVirtualMethod(vm, meth, obj->getClass());
  jdouble res = meth->invokeDoubleVirtualAP(vm, cl, obj, ap, true);
  va_end(ap);
  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0.0);
}


jdouble CallDoubleMethodV(JNIEnv *env, jobject _obj, jmethodID methodID,
                          va_list args) {
  
  BEGIN_JNI_EXCEPTION
  
  verifyNull(_obj);
  
  // Local object references.
  JavaObject* obj = *(JavaObject**)_obj;
  llvm_gcroot(obj, 0);
  
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromVirtualMethod(vm, meth, obj->getClass());
  jdouble res = (jdouble)meth->invokeDoubleVirtualAP(vm, cl, obj, args, true);
  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0.0);

}


jdouble CallDoubleMethodA(JNIEnv *env, jobject _obj, jmethodID methodID,
                          const jvalue *args) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}



void CallVoidMethod(JNIEnv *env, jobject _obj, jmethodID methodID, ...) {
  
  BEGIN_JNI_EXCEPTION

  verifyNull(_obj);

  va_list ap;
  va_start(ap, methodID);

  // Local object references.
  JavaObject* obj = *(JavaObject**)_obj;
  llvm_gcroot(obj, 0);

  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromVirtualMethod(vm, meth, obj->getClass());
  meth->invokeIntVirtualAP(vm, cl, obj, ap, true);
  va_end(ap);

  RETURN_VOID_FROM_JNI;

  END_JNI_EXCEPTION
  
  RETURN_VOID_FROM_JNI;
}


void CallVoidMethodV(JNIEnv *env, jobject _obj, jmethodID methodID,
                     va_list args) {
  
  BEGIN_JNI_EXCEPTION
  
  verifyNull(_obj);
  
  // Local object references.
  JavaObject* obj = *(JavaObject**)_obj;
  llvm_gcroot(obj, 0);

  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromVirtualMethod(vm, meth, obj->getClass());
  meth->invokeIntVirtualAP(vm, cl, obj, args, true);
  
  RETURN_VOID_FROM_JNI;

  END_JNI_EXCEPTION
  
  RETURN_VOID_FROM_JNI;
}


void CallVoidMethodA(JNIEnv *env, jobject obj, jmethodID methodID,
                     const jvalue *args) {
  fprintf(stderr, "Implement me\n");
  abort();
}



jobject CallNonvirtualObjectMethod(JNIEnv *env, jobject obj, jclass clazz,
                                   jmethodID methodID, ...) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}


jobject CallNonvirtualObjectMethodV(JNIEnv *env, jobject obj, jclass clazz,
                                    jmethodID methodID, va_list args) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}


jobject CallNonvirtualObjectMethodA(JNIEnv *env, jobject obj, jclass clazz,
                                    jmethodID methodID, const jvalue *args) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}



jboolean CallNonvirtualBooleanMethod(JNIEnv *env, jobject obj, jclass clazz,
                                     jmethodID methodID, ...) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}


jboolean CallNonvirtualBooleanMethodV(JNIEnv *env, jobject obj, jclass clazz,
                                      jmethodID methodID, va_list args) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}


jboolean CallNonvirtualBooleanMethodA(JNIEnv *env, jobject obj, jclass clazz,
                                      jmethodID methodID, const jvalue *args) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}


jbyte CallNonvirtualByteMethod(JNIEnv *env, jobject obj, jclass clazz,
                               jmethodID methodID, ...) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}


jbyte CallNonvirtualByteMethodV (JNIEnv *env, jobject obj, jclass clazz,
                                 jmethodID methodID, va_list args) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}


jbyte CallNonvirtualByteMethodA(JNIEnv *env, jobject obj, jclass clazz,
                                jmethodID methodID, const jvalue *args) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}



jchar CallNonvirtualCharMethod(JNIEnv *env, jobject obj, jclass clazz,
                               jmethodID methodID, ...) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}


jchar CallNonvirtualCharMethodV(JNIEnv *env, jobject obj, jclass clazz,
                                jmethodID methodID, va_list args) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}


jchar CallNonvirtualCharMethodA(JNIEnv *env, jobject obj, jclass clazz,
                                jmethodID methodID, const jvalue *args) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}



jshort CallNonvirtualShortMethod(JNIEnv *env, jobject obj, jclass clazz,
                                 jmethodID methodID, ...) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}


jshort CallNonvirtualShortMethodV(JNIEnv *env, jobject obj, jclass clazz,
                                  jmethodID methodID, va_list args) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}


jshort CallNonvirtualShortMethodA(JNIEnv *env, jobject obj, jclass clazz,
                                  jmethodID methodID, const jvalue *args) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}



jint CallNonvirtualIntMethod(JNIEnv *env, jobject obj, jclass clazz,
                             jmethodID methodID, ...) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}


jint CallNonvirtualIntMethodV(JNIEnv *env, jobject obj, jclass clazz,
                              jmethodID methodID, va_list args) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}


jint CallNonvirtualIntMethodA(JNIEnv *env, jobject obj, jclass clazz,
                              jmethodID methodID, const jvalue *args) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}



jlong CallNonvirtualLongMethod(JNIEnv *env, jobject obj, jclass clazz,
                               jmethodID methodID, ...) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}


jlong CallNonvirtualLongMethodV(JNIEnv *env, jobject obj, jclass clazz,
                                jmethodID methodID, va_list args) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}


jlong CallNonvirtualLongMethodA(JNIEnv *env, jobject obj, jclass clazz,
                                jmethodID methodID, const jvalue *args) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}



jfloat CallNonvirtualFloatMethod(JNIEnv *env, jobject obj, jclass clazz,
                                 jmethodID methodID, ...) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}


jfloat CallNonvirtualFloatMethodV(JNIEnv *env, jobject obj, jclass clazz,
                                  jmethodID methodID, va_list args) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}


jfloat CallNonvirtualFloatMethodA(JNIEnv *env, jobject obj, jclass clazz,
                                  jmethodID methodID, const jvalue *args) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}



jdouble CallNonvirtualDoubleMethod(JNIEnv *env, jobject obj, jclass clazz,
                                   jmethodID methodID, ...) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}


jdouble CallNonvirtualDoubleMethodV(JNIEnv *env, jobject obj, jclass clazz,
                                    jmethodID methodID, va_list args) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}


jdouble CallNonvirtualDoubleMethodA(JNIEnv *env, jobject obj, jclass clazz,
                                    jmethodID methodID, const jvalue *args) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}



void CallNonvirtualVoidMethod(JNIEnv *env, jobject _obj, jclass clazz,
                              jmethodID methodID, ...) {
  
  BEGIN_JNI_EXCEPTION
  
  verifyNull(_obj);
  
  va_list ap;
  va_start(ap, methodID);
  
  // Local object references.
  JavaObject* obj = *(JavaObject**)_obj;
  llvm_gcroot(obj, 0);

  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromVirtualMethod(vm, meth, obj->getClass());
  meth->invokeIntSpecialAP(vm, cl, obj, ap, true);
  va_end(ap);
  
  RETURN_VOID_FROM_JNI;

  END_JNI_EXCEPTION
  
  RETURN_VOID_FROM_JNI;
}


void CallNonvirtualVoidMethodV(JNIEnv *env, jobject obj, jclass clazz,
                               jmethodID methodID, va_list args) {
  fprintf(stderr, "Implement me\n");
  abort();
}


void CallNonvirtualVoidMethodA(JNIEnv *env, jobject obj, jclass clazz,
                               jmethodID methodID, const jvalue * args) {
  fprintf(stderr, "Implement me\n");
  abort();
}


jfieldID GetFieldID(JNIEnv *env, jclass _clazz, const char *aname,
		    const char *sig)  {

  BEGIN_JNI_EXCEPTION

  // Local object references.
  JavaObject* clazz = *(JavaObject**)_clazz;
  llvm_gcroot(clazz, 0);

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserCommonClass* cl = UserCommonClass::resolvedImplClass(vm, clazz, true);

  if (cl->isClass()) {
    const UTF8* name = cl->classLoader->hashUTF8->lookupAsciiz(aname);
    if (name) {
      const UTF8* type = cl->classLoader->hashUTF8->lookupAsciiz(sig);
      if (type) {
        JavaField* field = cl->asClass()->lookupField(name, type, false, true,
                                                      0);
        RETURN_FROM_JNI((jfieldID)field);
      }
    }
  }
  
  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);

}


jobject GetObjectField(JNIEnv *env, jobject _obj, jfieldID fieldID) {

  BEGIN_JNI_EXCEPTION
  
  // Local object references.
  JavaObject* obj = *(JavaObject**)_obj;
  JavaObject* res = 0;
  llvm_gcroot(obj, 0);
  llvm_gcroot(res, 0);

  JavaField* field = (JavaField*)fieldID;

  // Store local reference.
  res = field->getObjectField(obj);

  JavaThread* th = JavaThread::get();
  jobject ret = (jobject)th->pushJNIRef(res);
  RETURN_FROM_JNI(ret);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


jboolean GetBooleanField(JNIEnv *env, jobject _obj, jfieldID fieldID) {

  BEGIN_JNI_EXCEPTION
  
  // Local object references.
  JavaObject* obj = *(JavaObject**)_obj;
  llvm_gcroot(obj, 0);

  JavaField* field = (JavaField*)fieldID;
  uint8 res = (uint8)field->getInt8Field(obj);
  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


jbyte GetByteField(JNIEnv *env, jobject _obj, jfieldID fieldID) {

  BEGIN_JNI_EXCEPTION
  
  // Local object references.
  JavaObject* obj = *(JavaObject**)_obj;
  llvm_gcroot(obj, 0);

  JavaField* field = (JavaField*)fieldID;
  sint8 res = (sint8)field->getInt8Field(obj);
  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


jchar GetCharField(JNIEnv *env, jobject _obj, jfieldID fieldID) {

  BEGIN_JNI_EXCEPTION

  // Local object references.
  JavaObject* obj = *(JavaObject**)_obj;
  llvm_gcroot(obj, 0);
  
  JavaField* field = (JavaField*)fieldID;
  uint16 res = (uint16)field->getInt16Field(obj);
  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


jshort GetShortField(JNIEnv *env, jobject _obj, jfieldID fieldID) {

  BEGIN_JNI_EXCEPTION
  
  // Local object references.
  JavaObject* obj = *(JavaObject**)_obj;
  llvm_gcroot(obj, 0);

  JavaField* field = (JavaField*)fieldID;
  sint16 res = (sint16)field->getInt16Field(obj);
  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


jint GetIntField(JNIEnv *env, jobject _obj, jfieldID fieldID) {

  BEGIN_JNI_EXCEPTION
  
  // Local object references.
  JavaObject* obj = *(JavaObject**)_obj;
  llvm_gcroot(obj, 0);

  JavaField* field = (JavaField*)fieldID;
  sint32 res = (sint32)field->getInt32Field(obj);
  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


jlong GetLongField(JNIEnv *env, jobject _obj, jfieldID fieldID) {

  BEGIN_JNI_EXCEPTION

  // Local object references.
  JavaObject* obj = *(JavaObject**)_obj;
  llvm_gcroot(obj, 0);
  
  JavaField* field = (JavaField*)fieldID;
  sint64 res = (sint64)field->getLongField(obj);
  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


jfloat GetFloatField(JNIEnv *env, jobject _obj, jfieldID fieldID) {

  BEGIN_JNI_EXCEPTION
  
  // Local object references.
  JavaObject* obj = *(JavaObject**)_obj;
  llvm_gcroot(obj, 0);

  JavaField* field = (JavaField*)fieldID;
  jfloat res = (jfloat)field->getFloatField(obj);
  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


jdouble GetDoubleField(JNIEnv *env, jobject _obj, jfieldID fieldID) {

  BEGIN_JNI_EXCEPTION

  // Local object references.
  JavaObject* obj = *(JavaObject**)_obj;
  llvm_gcroot(obj, 0);
  
  JavaField* field = (JavaField*)fieldID;
  jdouble res = (jdouble)field->getDoubleField(obj);
  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


void SetObjectField(JNIEnv *env, jobject _obj, jfieldID fieldID, jobject _value) {

  BEGIN_JNI_EXCEPTION
  
  // Local object references.
  JavaObject* obj = *(JavaObject**)_obj;
  JavaObject* value = *(JavaObject**)_value;
  llvm_gcroot(obj, 0);
  llvm_gcroot(value, 0);

  JavaField* field = (JavaField*)fieldID;
  field->setObjectField(obj, value);
  
  RETURN_VOID_FROM_JNI;

  END_JNI_EXCEPTION
  
  RETURN_VOID_FROM_JNI;
}


void SetBooleanField(JNIEnv *env, jobject _obj, jfieldID fieldID,
                     jboolean value) {

  BEGIN_JNI_EXCEPTION
  
  // Local object references.
  JavaObject* obj = *(JavaObject**)_obj;
  llvm_gcroot(obj, 0);

  JavaField* field = (JavaField*)fieldID;
  field->setInt8Field(obj, (uint8)value);
  
  RETURN_VOID_FROM_JNI;

  END_JNI_EXCEPTION
  
  RETURN_VOID_FROM_JNI;
}


void SetByteField(JNIEnv *env, jobject _obj, jfieldID fieldID, jbyte value) {

  BEGIN_JNI_EXCEPTION
  
  // Local object references.
  JavaObject* obj = *(JavaObject**)_obj;
  llvm_gcroot(obj, 0);
  
  JavaField* field = (JavaField*)fieldID;
  field->setInt8Field(obj, (uint8)value);
  
  RETURN_VOID_FROM_JNI;

  END_JNI_EXCEPTION

  RETURN_VOID_FROM_JNI;
}


void SetCharField(JNIEnv *env, jobject _obj, jfieldID fieldID, jchar value) {

  BEGIN_JNI_EXCEPTION

  // Local object references.
  JavaObject* obj = *(JavaObject**)_obj;
  llvm_gcroot(obj, 0);

  JavaField* field = (JavaField*)fieldID;
  field->setInt16Field(obj, (uint16)value);
  
  RETURN_VOID_FROM_JNI;
  
  END_JNI_EXCEPTION
  
  RETURN_VOID_FROM_JNI;
}


void SetShortField(JNIEnv *env, jobject _obj, jfieldID fieldID, jshort value) {

  BEGIN_JNI_EXCEPTION

  // Local object references.
  JavaObject* obj = *(JavaObject**)_obj;
  llvm_gcroot(obj, 0);
  
  JavaField* field = (JavaField*)fieldID;
  field->setInt16Field(obj, (sint16)value);
  
  RETURN_VOID_FROM_JNI;

  END_JNI_EXCEPTION
  
  RETURN_VOID_FROM_JNI;
}


void SetIntField(JNIEnv *env, jobject _obj, jfieldID fieldID, jint value) {

  BEGIN_JNI_EXCEPTION

  // Local object references.
  JavaObject* obj = *(JavaObject**)_obj;
  llvm_gcroot(obj, 0);
  
  JavaField* field = (JavaField*)fieldID;
  field->setInt32Field(obj, (sint32)value);
  
  RETURN_VOID_FROM_JNI;

  END_JNI_EXCEPTION
  
  RETURN_VOID_FROM_JNI;
}


void SetLongField(JNIEnv *env, jobject _obj, jfieldID fieldID, jlong value) {

  BEGIN_JNI_EXCEPTION

  // Local object references.
  JavaObject* obj = *(JavaObject**)_obj;
  llvm_gcroot(obj, 0);
  
  JavaField* field = (JavaField*)fieldID;
  field->setLongField(obj, (sint64)value);
  
  RETURN_VOID_FROM_JNI;

  END_JNI_EXCEPTION
  
  RETURN_VOID_FROM_JNI;
}


void SetFloatField(JNIEnv *env, jobject _obj, jfieldID fieldID, jfloat value) {

  BEGIN_JNI_EXCEPTION

  // Local object references.
  JavaObject* obj = *(JavaObject**)_obj;
  llvm_gcroot(obj, 0);
  
  JavaField* field = (JavaField*)fieldID;
  field->setFloatField(obj, (float)value);
  
  RETURN_VOID_FROM_JNI;

  END_JNI_EXCEPTION
  
  RETURN_VOID_FROM_JNI;
}


void SetDoubleField(JNIEnv *env, jobject _obj, jfieldID fieldID, jdouble value) {

  BEGIN_JNI_EXCEPTION

  // Local object references.
  JavaObject* obj = *(JavaObject**)_obj;
  llvm_gcroot(obj, 0);
  
  JavaField* field = (JavaField*)fieldID;
  field->setDoubleField(obj, (float)value);
  
  RETURN_VOID_FROM_JNI;

  END_JNI_EXCEPTION
  
  RETURN_VOID_FROM_JNI;
}


jmethodID GetStaticMethodID(JNIEnv *env, jclass _clazz, const char *aname,
			    const char *atype) {

  BEGIN_JNI_EXCEPTION
  
  // Local object references.
  JavaObject* clazz = *(JavaObject**)_clazz;
  llvm_gcroot(clazz, 0);
  
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserCommonClass* cl = UserCommonClass::resolvedImplClass(vm, clazz, true);

  if (cl->isClass()) {
    const UTF8* name = cl->classLoader->hashUTF8->lookupAsciiz(aname);
    if (name) {
      const UTF8* type = cl->classLoader->hashUTF8->lookupAsciiz(atype);
      if (type) {
        JavaMethod* meth = cl->asClass()->lookupMethod(name, type, true, true,
                                                       0);
        RETURN_FROM_JNI((jmethodID)meth);
      }
    }
  }

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


jobject CallStaticObjectMethod(JNIEnv *env, jclass _clazz, jmethodID methodID,
                               ...) {
  
  BEGIN_JNI_EXCEPTION
  
  va_list ap;
  va_start(ap, methodID);
 
  // Local object references.
  JavaObject* clazz = *(JavaObject**)_clazz;
  JavaObject* res = 0;
  llvm_gcroot(clazz, 0);
  llvm_gcroot(res, 0);


  JavaMethod* meth = (JavaMethod*)methodID;
  JavaThread* th = JavaThread::get();
  Jnjvm* vm = th->getJVM();
  UserClass* cl = getClassFromStaticMethod(vm, meth, clazz);
  
  // Store local reference.
  res = meth->invokeJavaObjectStaticAP(vm, cl, ap, true);
  va_end(ap);
  
  jobject ret = (jobject)th->pushJNIRef(res);
  RETURN_FROM_JNI(ret);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


jobject CallStaticObjectMethodV(JNIEnv *env, jclass _clazz, jmethodID methodID,
                                     va_list args) {
  BEGIN_JNI_EXCEPTION
  
  // Local object references.
  JavaObject* clazz = *(JavaObject**)_clazz;
  JavaObject* res = 0;
  llvm_gcroot(clazz, 0);
  llvm_gcroot(res, 0);
  
  JavaMethod* meth = (JavaMethod*)methodID;
  JavaThread* th = JavaThread::get();
  Jnjvm* vm = th->getJVM();
  UserClass* cl = getClassFromStaticMethod(vm, meth, clazz);
  
  // Store local reference.
  res = meth->invokeJavaObjectStaticAP(vm, cl, args, true);

  jobject ret = (jobject)th->pushJNIRef(res);
  RETURN_FROM_JNI(ret);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


jobject CallStaticObjectMethodA(JNIEnv *env, jclass clazz, jmethodID methodID,
                                const jvalue *args) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}


jboolean CallStaticBooleanMethod(JNIEnv *env, jclass _clazz, jmethodID methodID,
                                 ...) {
  
  BEGIN_JNI_EXCEPTION
  
  va_list ap;
  va_start(ap, methodID);
  
  // Local object references.
  JavaObject* clazz = *(JavaObject**)_clazz;
  llvm_gcroot(clazz, 0);
  
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromStaticMethod(vm, meth, clazz);
  uint32 res = meth->invokeIntStaticAP(vm, cl, ap, true);
  va_end(ap);
  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


jboolean CallStaticBooleanMethodV(JNIEnv *env, jclass _clazz, jmethodID methodID,
                                  va_list args) {
  
  BEGIN_JNI_EXCEPTION
  
  // Local object references.
  JavaObject* clazz = *(JavaObject**)_clazz;
  llvm_gcroot(clazz, 0);
  
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromStaticMethod(vm, meth, clazz);
  jboolean res = (jboolean)meth->invokeIntStaticAP(vm, cl, args, true);
  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


jboolean CallStaticBooleanMethodA(JNIEnv *env, jclass clazz, jmethodID methodID,
                                  const jvalue *args) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}


jbyte CallStaticByteMethod(JNIEnv *env, jclass _clazz, jmethodID methodID, ...) {
  
  BEGIN_JNI_EXCEPTION
  
  // Local object references.
  JavaObject* clazz = *(JavaObject**)_clazz;
  llvm_gcroot(clazz, 0);
  
  va_list ap;
  va_start(ap, methodID);
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromStaticMethod(vm, meth, clazz);
  jbyte res = (jbyte) meth->invokeIntStaticAP(vm, cl, ap, true);
  va_end(ap);
  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


jbyte CallStaticByteMethodV(JNIEnv *env, jclass _clazz, jmethodID methodID,
                            va_list args) {
  
  BEGIN_JNI_EXCEPTION
  
  // Local object references.
  JavaObject* clazz = *(JavaObject**)_clazz;
  llvm_gcroot(clazz, 0);
  
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromStaticMethod(vm, meth, clazz);
  jbyte res = (jbyte)meth->invokeIntStaticAP(vm, cl, args, true);
  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


jbyte CallStaticByteMethodA(JNIEnv *env, jclass clazz, jmethodID methodID,
                            const jvalue *args) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}


jchar CallStaticCharMethod(JNIEnv *env, jclass _clazz, jmethodID methodID, ...) {
  
  BEGIN_JNI_EXCEPTION
  
  va_list ap;
  va_start(ap, methodID);
  
  // Local object references.
  JavaObject* clazz = *(JavaObject**)_clazz;
  llvm_gcroot(clazz, 0);
  
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromStaticMethod(vm, meth, clazz);
  jchar res = (jchar) meth->invokeIntStaticAP(vm, cl, ap, true);
  va_end(ap);
  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


jchar CallStaticCharMethodV(JNIEnv *env, jclass _clazz, jmethodID methodID,
                            va_list args) {
  BEGIN_JNI_EXCEPTION
  
  // Local object references.
  JavaObject* clazz = *(JavaObject**)_clazz;
  llvm_gcroot(clazz, 0);
  
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromStaticMethod(vm, meth, clazz);
  jchar res = (jchar)meth->invokeIntStaticAP(vm, cl, args, true);
  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


jchar CallStaticCharMethodA(JNIEnv *env, jclass _clazz, jmethodID methodID,
                            const jvalue *args) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}


jshort CallStaticShortMethod(JNIEnv *env, jclass _clazz, jmethodID methodID,
                             ...) {
  
  BEGIN_JNI_EXCEPTION
  
  va_list ap;
  va_start(ap, methodID);
  
  // Local object references.
  JavaObject* clazz = *(JavaObject**)_clazz;
  llvm_gcroot(clazz, 0);
  
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromStaticMethod(vm, meth, clazz);
  jshort res = (jshort) meth->invokeIntStaticAP(vm, cl, ap, true);
  va_end(ap);
  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


jshort CallStaticShortMethodV(JNIEnv *env, jclass _clazz, jmethodID methodID,
                              va_list args) {
  
  BEGIN_JNI_EXCEPTION
  
  // Local object references.
  JavaObject* clazz = *(JavaObject**)_clazz;
  llvm_gcroot(clazz, 0);
  
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromStaticMethod(vm, meth, clazz);
  jshort res = (jshort)meth->invokeIntStaticAP(vm, cl, args, true);
  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


jshort CallStaticShortMethodA(JNIEnv *env, jclass clazz, jmethodID methodID,
                              const jvalue *args) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}


jint CallStaticIntMethod(JNIEnv *env, jclass _clazz, jmethodID methodID, ...) {
  
  BEGIN_JNI_EXCEPTION
  
  va_list ap;
  va_start(ap, methodID);
  
  // Local object references.
  JavaObject* clazz = *(JavaObject**)_clazz;
  llvm_gcroot(clazz, 0);
  
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromStaticMethod(vm, meth, clazz);
  jint res = (jint) meth->invokeIntStaticAP(vm, cl, ap, true);
  va_end(ap);
  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


jint CallStaticIntMethodV(JNIEnv *env, jclass _clazz, jmethodID methodID,
                          va_list args) {
  BEGIN_JNI_EXCEPTION
  
  // Local object references.
  JavaObject* clazz = *(JavaObject**)_clazz;
  llvm_gcroot(clazz, 0);
  
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromStaticMethod(vm, meth, clazz);
  jint res = (jint)meth->invokeIntStaticAP(vm, cl, args, true);
  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION
  
  RETURN_FROM_JNI(0);
}


jint CallStaticIntMethodA(JNIEnv *env, jclass clazz, jmethodID methodID,
                          const jvalue *args) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}


jlong CallStaticLongMethod(JNIEnv *env, jclass _clazz, jmethodID methodID, ...) {
  
  BEGIN_JNI_EXCEPTION
  
  va_list ap;
  va_start(ap, methodID);
  
  // Local object references.
  JavaObject* clazz = *(JavaObject**)_clazz;
  llvm_gcroot(clazz, 0);
  
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromStaticMethod(vm, meth, clazz);
  jlong res = (jlong) meth->invokeLongStaticAP(vm, cl, ap, true);
  va_end(ap);
  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


jlong CallStaticLongMethodV(JNIEnv *env, jclass _clazz, jmethodID methodID,
			    va_list args) {

  BEGIN_JNI_EXCEPTION
  
  // Local object references.
  JavaObject* clazz = *(JavaObject**)_clazz;
  llvm_gcroot(clazz, 0);
  
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromStaticMethod(vm, meth, clazz);
  jlong res = (jlong)meth->invokeLongStaticAP(vm, cl, args, true);
  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION
  
  RETURN_FROM_JNI(0);
}


jlong CallStaticLongMethodA(JNIEnv *env, jclass clazz, jmethodID methodID,
                            const jvalue *args) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}



jfloat CallStaticFloatMethod(JNIEnv *env, jclass _clazz, jmethodID methodID,
                             ...) {
  
  BEGIN_JNI_EXCEPTION
  
  va_list ap;
  va_start(ap, methodID);
  
  // Local object references.
  JavaObject* clazz = *(JavaObject**)_clazz;
  llvm_gcroot(clazz, 0);
  
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromStaticMethod(vm, meth, clazz);
  jfloat res = (jfloat) meth->invokeFloatStaticAP(vm, cl, ap, true);
  va_end(ap);
  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0.0f);
}


jfloat CallStaticFloatMethodV(JNIEnv *env, jclass _clazz, jmethodID methodID,
                              va_list args) {
  
  BEGIN_JNI_EXCEPTION
  
  // Local object references.
  JavaObject* clazz = *(JavaObject**)_clazz;
  llvm_gcroot(clazz, 0);
  
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromStaticMethod(vm, meth, clazz);
  jfloat res = (jfloat)meth->invokeFloatStaticAP(vm, cl, args, true);
  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION
  
  RETURN_FROM_JNI(0.0f);
}


jfloat CallStaticFloatMethodA(JNIEnv *env, jclass clazz, jmethodID methodID,
                              const jvalue *args) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}


jdouble CallStaticDoubleMethod(JNIEnv *env, jclass _clazz, jmethodID methodID,
                               ...) {
  
  BEGIN_JNI_EXCEPTION
  
  va_list ap;
  va_start(ap, methodID);
  
  // Local object references.
  JavaObject* clazz = *(JavaObject**)_clazz;
  llvm_gcroot(clazz, 0);
  
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromStaticMethod(vm, meth, clazz);
  jdouble res = (jdouble) meth->invokeDoubleStaticAP(vm, cl, ap, true);
  va_end(ap);
  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0.0);
}


jdouble CallStaticDoubleMethodV(JNIEnv *env, jclass _clazz, jmethodID methodID,
                                va_list args) {
  BEGIN_JNI_EXCEPTION
  
  // Local object references.
  JavaObject* clazz = *(JavaObject**)_clazz;
  llvm_gcroot(clazz, 0);
  
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromStaticMethod(vm, meth, clazz);
  jdouble res = (jdouble)meth->invokeDoubleStaticAP(vm, cl, args, true);
  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION
  
  RETURN_FROM_JNI(0.0);
}


jdouble CallStaticDoubleMethodA(JNIEnv *env, jclass clazz, jmethodID methodID,
                                const jvalue *args) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}


void CallStaticVoidMethod(JNIEnv *env, jclass _clazz, jmethodID methodID, ...) {
  
  BEGIN_JNI_EXCEPTION
  
  va_list ap;
  va_start(ap, methodID);
  
  // Local object references.
  JavaObject* clazz = *(JavaObject**)_clazz;
  llvm_gcroot(clazz, 0);

  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromStaticMethod(vm, meth, clazz);
  meth->invokeIntStaticAP(vm, cl, ap, true);
  va_end(ap);

  RETURN_VOID_FROM_JNI;

  END_JNI_EXCEPTION
  
  RETURN_VOID_FROM_JNI;
}


void CallStaticVoidMethodV(JNIEnv *env, jclass _clazz, jmethodID methodID,
                           va_list args) {
  
  BEGIN_JNI_EXCEPTION
  
  // Local object references.
  JavaObject* clazz = *(JavaObject**)_clazz;
  llvm_gcroot(clazz, 0);
  
  JavaMethod* meth = (JavaMethod*)methodID;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = getClassFromStaticMethod(vm, meth, clazz);
  meth->invokeIntStaticAP(vm, cl, args, true);
  
  RETURN_VOID_FROM_JNI;

  END_JNI_EXCEPTION
  
  RETURN_VOID_FROM_JNI;
}


void CallStaticVoidMethodA(JNIEnv *env, jclass clazz, jmethodID methodID,
                           const jvalue * args) {
  fprintf(stderr, "Implement me\n");
  abort();
}


jfieldID GetStaticFieldID(JNIEnv *env, jclass _clazz, const char *aname,
                          const char *sig) {
  
  BEGIN_JNI_EXCEPTION
  
  // Local object references.
  JavaObject* clazz = *(JavaObject**)_clazz;
  llvm_gcroot(clazz, 0);
  
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserCommonClass* cl = UserCommonClass::resolvedImplClass(vm, clazz, true);
  
  if (cl->isClass()) {
    const UTF8* name = cl->classLoader->hashUTF8->lookupAsciiz(aname);
    if (name) {
      const UTF8* type = cl->classLoader->hashUTF8->lookupAsciiz(sig);
      if (type) {
        JavaField* field = cl->asClass()->lookupField(name, type, true, true,
                                                      0);
        RETURN_FROM_JNI((jfieldID)field);
      }
    }
  }

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


jobject GetStaticObjectField(JNIEnv *env, jclass _clazz, jfieldID fieldID) {

  BEGIN_JNI_EXCEPTION
  
  // Local object references.
  JavaObject* clazz = *(JavaObject**)_clazz;
  JavaObject* obj = 0;
  llvm_gcroot(clazz, 0);
  llvm_gcroot(obj, 0);

  JavaThread* th = JavaThread::get();
  Jnjvm* vm = th->getJVM();
  JavaField* field = (JavaField*)fieldID;
  UserCommonClass* cl = UserCommonClass::resolvedImplClass(vm, clazz, true);
  void* Stat = cl->asClass()->getStaticInstance();
  obj = field->getObjectField(Stat);
  jobject res = (jobject)th->pushJNIRef(obj);
  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


jboolean GetStaticBooleanField(JNIEnv *env, jclass _clazz, jfieldID fieldID) {
  
  BEGIN_JNI_EXCEPTION
  
  // Local object references.
  JavaObject* clazz = *(JavaObject**)_clazz;
  llvm_gcroot(clazz, 0);

  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* field = (JavaField*)fieldID;
  UserCommonClass* cl = UserCommonClass::resolvedImplClass(vm, clazz, true);
  void* Stat = cl->asClass()->getStaticInstance();
  jboolean res = (jboolean)field->getInt8Field(Stat);
  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


jbyte GetStaticByteField(JNIEnv *env, jclass _clazz, jfieldID fieldID) {

  BEGIN_JNI_EXCEPTION
  
  // Local object references.
  JavaObject* clazz = *(JavaObject**)_clazz;
  llvm_gcroot(clazz, 0);

  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* field = (JavaField*)fieldID;
  UserCommonClass* cl = UserCommonClass::resolvedImplClass(vm, clazz, true);
  void* Stat = cl->asClass()->getStaticInstance();
  jbyte res = (jbyte)field->getInt8Field(Stat);
  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


jchar GetStaticCharField(JNIEnv *env, jclass _clazz, jfieldID fieldID) {

  BEGIN_JNI_EXCEPTION

  // Local object references.
  JavaObject* clazz = *(JavaObject**)_clazz;
  llvm_gcroot(clazz, 0);
  
  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* field = (JavaField*)fieldID;
  UserCommonClass* cl = UserCommonClass::resolvedImplClass(vm, clazz, true);
  void* Stat = cl->asClass()->getStaticInstance();
  jchar res = (jchar)field->getInt16Field(Stat);
  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


jshort GetStaticShortField(JNIEnv *env, jclass _clazz, jfieldID fieldID) {

  BEGIN_JNI_EXCEPTION

  // Local object references.
  JavaObject* clazz = *(JavaObject**)_clazz;
  llvm_gcroot(clazz, 0);
  
  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* field = (JavaField*)fieldID;
  UserCommonClass* cl = UserCommonClass::resolvedImplClass(vm, clazz, true);
  void* Stat = cl->asClass()->getStaticInstance();
  jshort res = (jshort)field->getInt16Field(Stat);
  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


jint GetStaticIntField(JNIEnv *env, jclass _clazz, jfieldID fieldID) {

  BEGIN_JNI_EXCEPTION

  // Local object references.
  JavaObject* clazz = *(JavaObject**)_clazz;
  llvm_gcroot(clazz, 0);
  
  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* field = (JavaField*)fieldID;
  UserCommonClass* cl = UserCommonClass::resolvedImplClass(vm, clazz, true);
  void* Stat = cl->asClass()->getStaticInstance();
  jint res = (jint)field->getInt32Field(Stat);
  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


jlong GetStaticLongField(JNIEnv *env, jclass _clazz, jfieldID fieldID) {

  BEGIN_JNI_EXCEPTION
  
  // Local object references.
  JavaObject* clazz = *(JavaObject**)_clazz;
  llvm_gcroot(clazz, 0);

  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* field = (JavaField*)fieldID;
  UserCommonClass* cl = UserCommonClass::resolvedImplClass(vm, clazz, true);
  void* Stat = cl->asClass()->getStaticInstance();
  jlong res = (jlong)field->getLongField(Stat);
  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


jfloat GetStaticFloatField(JNIEnv *env, jclass _clazz, jfieldID fieldID) {

  BEGIN_JNI_EXCEPTION
  
  // Local object references.
  JavaObject* clazz = *(JavaObject**)_clazz;
  llvm_gcroot(clazz, 0);

  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* field = (JavaField*)fieldID;
  UserCommonClass* cl = UserCommonClass::resolvedImplClass(vm, clazz, true);
  void* Stat = cl->asClass()->getStaticInstance();
  jfloat res = (jfloat)field->getFloatField(Stat);
  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


jdouble GetStaticDoubleField(JNIEnv *env, jclass _clazz, jfieldID fieldID) {

  BEGIN_JNI_EXCEPTION

  // Local object references.
  JavaObject* clazz = *(JavaObject**)_clazz;
  llvm_gcroot(clazz, 0);
  
  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* field = (JavaField*)fieldID;
  UserCommonClass* cl = UserCommonClass::resolvedImplClass(vm, clazz, true);
  void* Stat = cl->asClass()->getStaticInstance();
  jdouble res = (jdouble)field->getDoubleField(Stat);
  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


void SetStaticObjectField(JNIEnv *env, jclass _clazz, jfieldID fieldID,
                          jobject _value) {

  BEGIN_JNI_EXCEPTION
  
  // Local object references.
  JavaObject* clazz = *(JavaObject**)_clazz;
  JavaObject* value = *(JavaObject**)_value;
  llvm_gcroot(clazz, 0);
  llvm_gcroot(value, 0);

  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* field = (JavaField*)fieldID;
  UserCommonClass* cl = UserCommonClass::resolvedImplClass(vm, clazz, true);
  void* Stat = cl->asClass()->getStaticInstance();
  field->setObjectField(Stat, value);
  
  RETURN_VOID_FROM_JNI;
  
  END_JNI_EXCEPTION
  
  RETURN_VOID_FROM_JNI;
}


void SetStaticBooleanField(JNIEnv *env, jclass _clazz, jfieldID fieldID,
                           jboolean value) {
  
  BEGIN_JNI_EXCEPTION
  
  // Local object references.
  JavaObject* clazz = *(JavaObject**)_clazz;
  
  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* field = (JavaField*)fieldID;
  UserCommonClass* cl = UserCommonClass::resolvedImplClass(vm, clazz, true);
  void* Stat = cl->asClass()->getStaticInstance();
  field->setInt8Field(Stat, (uint8)value);
  
  RETURN_VOID_FROM_JNI;

  END_JNI_EXCEPTION
  
  RETURN_VOID_FROM_JNI;
}


void SetStaticByteField(JNIEnv *env, jclass _clazz, jfieldID fieldID,
                        jbyte value) {

  BEGIN_JNI_EXCEPTION

  // Local object references.
  JavaObject* clazz = *(JavaObject**)_clazz;
  llvm_gcroot(clazz, 0);
  
  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* field = (JavaField*)fieldID;
  UserCommonClass* cl = UserCommonClass::resolvedImplClass(vm, clazz, true);
  void* Stat = cl->asClass()->getStaticInstance();
  field->setInt8Field(Stat, (sint8)value);
  
  RETURN_VOID_FROM_JNI;

  END_JNI_EXCEPTION
  
  RETURN_VOID_FROM_JNI;
}


void SetStaticCharField(JNIEnv *env, jclass _clazz, jfieldID fieldID,
                        jchar value) {

  BEGIN_JNI_EXCEPTION
  
  // Local object references.
  JavaObject* clazz = *(JavaObject**)_clazz;
  llvm_gcroot(clazz, 0);

  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* field = (JavaField*)fieldID;
  UserCommonClass* cl = UserCommonClass::resolvedImplClass(vm, clazz, true);
  void* Stat = cl->asClass()->getStaticInstance();
  field->setInt16Field(Stat, (uint16)value);
  
  RETURN_VOID_FROM_JNI;

  END_JNI_EXCEPTION
  
  RETURN_VOID_FROM_JNI;
}


void SetStaticShortField(JNIEnv *env, jclass _clazz, jfieldID fieldID,
                         jshort value) {

  BEGIN_JNI_EXCEPTION
  
  // Local object references.
  JavaObject* clazz = *(JavaObject**)_clazz;
  llvm_gcroot(clazz, 0);

  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* field = (JavaField*)fieldID;
  UserCommonClass* cl = UserCommonClass::resolvedImplClass(vm, clazz, true);
  void* Stat = cl->asClass()->getStaticInstance();
  field->setInt16Field(Stat, (sint16)value);
  
  RETURN_VOID_FROM_JNI;

  END_JNI_EXCEPTION
  
  RETURN_VOID_FROM_JNI;
}


void SetStaticIntField(JNIEnv *env, jclass _clazz, jfieldID fieldID,
                       jint value) {

  BEGIN_JNI_EXCEPTION
  
  // Local object references.
  JavaObject* clazz = *(JavaObject**)_clazz;
  llvm_gcroot(clazz, 0);

  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* field = (JavaField*)fieldID;
  UserCommonClass* cl = UserCommonClass::resolvedImplClass(vm, clazz, true);
  void* Stat = cl->asClass()->getStaticInstance();
  field->setInt32Field(Stat, (sint32)value);
  
  RETURN_VOID_FROM_JNI;

  END_JNI_EXCEPTION
  
  RETURN_VOID_FROM_JNI;
}


void SetStaticLongField(JNIEnv *env, jclass _clazz, jfieldID fieldID,
                        jlong value) {

  BEGIN_JNI_EXCEPTION
  
  // Local object references.
  JavaObject* clazz = *(JavaObject**)_clazz;
  llvm_gcroot(clazz, 0);

  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* field = (JavaField*)fieldID;
  UserCommonClass* cl = UserCommonClass::resolvedImplClass(vm, clazz, true);
  void* Stat = cl->asClass()->getStaticInstance();
  field->setLongField(Stat, (sint64)value);
  
  RETURN_VOID_FROM_JNI;

  END_JNI_EXCEPTION
  
  RETURN_VOID_FROM_JNI;
}


void SetStaticFloatField(JNIEnv *env, jclass _clazz, jfieldID fieldID,
                         jfloat value) {

  BEGIN_JNI_EXCEPTION
  
  // Local object references.
  JavaObject* clazz = *(JavaObject**)_clazz;
  llvm_gcroot(clazz, 0);

  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* field = (JavaField*)fieldID;
  UserCommonClass* cl = UserCommonClass::resolvedImplClass(vm, clazz, true);
  void* Stat = cl->asClass()->getStaticInstance();
  field->setFloatField(Stat, (float)value);
  
  RETURN_VOID_FROM_JNI;

  END_JNI_EXCEPTION
  
  RETURN_VOID_FROM_JNI;
}


void SetStaticDoubleField(JNIEnv *env, jclass _clazz, jfieldID fieldID,
                          jdouble value) {

  BEGIN_JNI_EXCEPTION
  
  // Local object references.
  JavaObject* clazz = *(JavaObject**)_clazz;
  llvm_gcroot(clazz, 0);

  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* field = (JavaField*)fieldID;
  UserCommonClass* cl = UserCommonClass::resolvedImplClass(vm, clazz, true);
  void* Stat = cl->asClass()->getStaticInstance();
  field->setDoubleField(Stat, (double)value);
  
  RETURN_VOID_FROM_JNI;

  END_JNI_EXCEPTION

  RETURN_VOID_FROM_JNI;
}


jstring NewString(JNIEnv *env, const jchar *buf, jsize len) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}


jsize GetStringLength(JNIEnv *env, jstring str) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}


const jchar *GetStringChars(JNIEnv *env, jstring str, jboolean *isCopy) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}


void ReleaseStringChars(JNIEnv *env, jstring str, const jchar *chars) {
  fprintf(stderr, "Implement me\n");
  abort();
}


jstring NewStringUTF(JNIEnv *env, const char *bytes) {

  BEGIN_JNI_EXCEPTION

  JavaThread* th = JavaThread::get();
  Jnjvm* vm = th->getJVM();
  JavaObject* obj = vm->asciizToStr(bytes);
  llvm_gcroot(obj, 0);
  jstring ret = (jstring)th->pushJNIRef(obj);
  RETURN_FROM_JNI(ret);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


jsize GetStringUTFLength (JNIEnv *env, jstring string) {   
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}


const char *GetStringUTFChars(JNIEnv *env, jstring _string, jboolean *isCopy) {

  BEGIN_JNI_EXCEPTION
  
  // Local object references.
  JavaString* string = *(JavaString**)_string;
  llvm_gcroot(string, 0);

  if (isCopy != 0) (*isCopy) = true;
  const char* res = string->strToAsciiz();
  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


void ReleaseStringUTFChars(JNIEnv *env, jstring _string, const char *utf) {
  delete[] utf;
}


jsize GetArrayLength(JNIEnv *env, jarray _array) {

  BEGIN_JNI_EXCEPTION
  
  // Local object references.
  JavaArray* array = *(JavaArray**)_array;
  llvm_gcroot(array, 0);

  RETURN_FROM_JNI(array->size);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


jobjectArray NewObjectArray(JNIEnv *env, jsize length, jclass _elementClass,
                            jobject _initialElement) {
  BEGIN_JNI_EXCEPTION
  
  // Local object references.
  JavaObject* elementClass = *(JavaObject**)_elementClass;
  JavaObject* initialElement = _initialElement ? 
    *(JavaObject**)_initialElement : 0;
  ArrayObject* res = 0;
  llvm_gcroot(elementClass, 0);
  llvm_gcroot(initialElement, 0);
  llvm_gcroot(res, 0);

  JavaThread* th = JavaThread::get();
  Jnjvm* vm = th->getJVM();

  if (length < 0) vm->negativeArraySizeException(length);
  
  UserCommonClass* base =
    UserCommonClass::resolvedImplClass(vm, elementClass, true);
  JnjvmClassLoader* loader = base->classLoader;
  const UTF8* name = base->getName();
  const UTF8* arrayName = loader->constructArrayName(1, name);
  UserClassArray* array = loader->constructArray(arrayName, base);
  res = (ArrayObject*)array->doNew(length, vm);
  
  if (initialElement) {
    for (sint32 i = 0; i < length; ++i) {
      res->elements[i] = initialElement;
    }
  }
  
  jobjectArray ret = (jobjectArray)th->pushJNIRef(res);
  RETURN_FROM_JNI(ret);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


jobject GetObjectArrayElement(JNIEnv *env, jobjectArray _array, jsize index) {
  
  BEGIN_JNI_EXCEPTION
  
  // Local object references.
  ArrayObject* array = *(ArrayObject**)_array;
  JavaObject* res = 0;
  llvm_gcroot(array, 0);
  llvm_gcroot(res, 0);
  
  JavaThread* th = JavaThread::get();
  Jnjvm* vm = th->getJVM();
  
  if (index >= array->size) vm->indexOutOfBounds(array, index);
  
  // Store local refererence.
  res = array->elements[index];
  
  jobject ret = (jobject)th->pushJNIRef(res);
  RETURN_FROM_JNI(ret);
  
  END_JNI_EXCEPTION

  RETURN_FROM_JNI(0);
}


void SetObjectArrayElement(JNIEnv *env, jobjectArray _array, jsize index,
                           jobject _val) {
  
  BEGIN_JNI_EXCEPTION
  
  // Local object references.
  ArrayObject* array = *(ArrayObject**)_array;
  JavaObject* val = *(JavaObject**)_val;
  llvm_gcroot(array, 0);
  llvm_gcroot(val, 0);

  if (index >= array->size)
    JavaThread::get()->getJVM()->indexOutOfBounds(array, index);
  
  // Store global reference.
  array->elements[index] = val;
  
  RETURN_VOID_FROM_JNI;

  END_JNI_EXCEPTION
  
  RETURN_VOID_FROM_JNI;
}


jbooleanArray NewBooleanArray(JNIEnv *env, jsize len) {
  
  BEGIN_JNI_EXCEPTION
  
  JavaThread* th = JavaThread::get();
  Jnjvm* vm = th->getJVM();
  JavaObject* res = vm->upcalls->ArrayOfBool->doNew(len, vm);
  llvm_gcroot(res, 0);
  jbooleanArray ret = (jbooleanArray)th->pushJNIRef(res);
  RETURN_FROM_JNI(ret);


  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


jbyteArray NewByteArray(JNIEnv *env, jsize len) {
  
  BEGIN_JNI_EXCEPTION

  JavaThread* th = JavaThread::get();
  Jnjvm* vm = th->getJVM();
  JavaObject* res = vm->upcalls->ArrayOfByte->doNew(len, vm);
  llvm_gcroot(res, 0);
  jbyteArray ret = (jbyteArray)th->pushJNIRef(res);
  RETURN_FROM_JNI(ret);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


jcharArray NewCharArray(JNIEnv *env, jsize len) {
  
  BEGIN_JNI_EXCEPTION
  
  JavaThread* th = JavaThread::get();
  Jnjvm* vm = th->getJVM();
  JavaObject* res = vm->upcalls->ArrayOfChar->doNew(len, vm);
  llvm_gcroot(res, 0);
  jcharArray ret = (jcharArray)th->pushJNIRef(res);
  RETURN_FROM_JNI(ret);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


jshortArray NewShortArray(JNIEnv *env, jsize len) {
  
  BEGIN_JNI_EXCEPTION
  
  JavaThread* th = JavaThread::get();
  Jnjvm* vm = th->getJVM();
  JavaObject* res = vm->upcalls->ArrayOfShort->doNew(len, vm);
  llvm_gcroot(res, 0);
  jshortArray ret = (jshortArray)th->pushJNIRef(res);
  RETURN_FROM_JNI(ret);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


jintArray NewIntArray(JNIEnv *env, jsize len) {
  
  BEGIN_JNI_EXCEPTION
  
  JavaThread* th = JavaThread::get();
  Jnjvm* vm = th->getJVM();
  JavaObject* res = vm->upcalls->ArrayOfInt->doNew(len, vm);
  llvm_gcroot(res, 0);
  jintArray ret = (jintArray)th->pushJNIRef(res);
  RETURN_FROM_JNI(ret);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


jlongArray NewLongArray(JNIEnv *env, jsize len) {
  
  BEGIN_JNI_EXCEPTION
  
  JavaThread* th = JavaThread::get();
  Jnjvm* vm = th->getJVM();
  JavaObject* res = vm->upcalls->ArrayOfLong->doNew(len, vm);
  llvm_gcroot(res, 0);
  jlongArray ret = (jlongArray)th->pushJNIRef(res);
  RETURN_FROM_JNI(ret);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


jfloatArray NewFloatArray(JNIEnv *env, jsize len) {
  
  BEGIN_JNI_EXCEPTION
  
  JavaThread* th = JavaThread::get();
  Jnjvm* vm = th->getJVM();
  JavaObject* res = vm->upcalls->ArrayOfFloat->doNew(len, vm);
  llvm_gcroot(res, 0);
  jfloatArray ret = (jfloatArray)th->pushJNIRef(res);
  RETURN_FROM_JNI(ret);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


jdoubleArray NewDoubleArray(JNIEnv *env, jsize len) {
  
  BEGIN_JNI_EXCEPTION
  
  JavaThread* th = JavaThread::get();
  Jnjvm* vm = th->getJVM();
  JavaObject* res = vm->upcalls->ArrayOfDouble->doNew(len, vm);
  llvm_gcroot(res, 0);
  jdoubleArray ret = (jdoubleArray)th->pushJNIRef(res);
  RETURN_FROM_JNI(ret);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


jboolean* GetBooleanArrayElements(JNIEnv *env, jbooleanArray _array,
				                          jboolean *isCopy) {
  
  BEGIN_JNI_EXCEPTION
 
  // Local object references.
  ArrayUInt8* array = *(ArrayUInt8**)_array;
  llvm_gcroot(array, 0);

  if (isCopy) (*isCopy) = true;

  sint32 len = array->size * sizeof(uint8);
  void* buffer = malloc(len);
  memcpy(buffer, array->elements, len);

  RETURN_FROM_JNI((jboolean*)buffer);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


jbyte *GetByteArrayElements(JNIEnv *env, jbyteArray _array, jboolean *isCopy) {

  BEGIN_JNI_EXCEPTION

  // Local object references.
  ArraySInt8* array = *(ArraySInt8**)_array;
  llvm_gcroot(array, 0);

  if (isCopy) (*isCopy) = true;

  sint32 len = array->size * sizeof(uint8);
  void* buffer = malloc(len);
  memcpy(buffer, array->elements, len);

  RETURN_FROM_JNI((jbyte*)buffer);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


jchar *GetCharArrayElements(JNIEnv *env, jcharArray _array, jboolean *isCopy) {

  BEGIN_JNI_EXCEPTION
  
  // Local object references.
  ArrayUInt16* array = *(ArrayUInt16**)_array;
  llvm_gcroot(array, 0);

  if (isCopy) (*isCopy) = true;

  sint32 len = array->size * sizeof(uint16);
  void* buffer = malloc(len);
  memcpy(buffer, array->elements, len);

  RETURN_FROM_JNI((jchar*)buffer);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


jshort *GetShortArrayElements(JNIEnv *env, jshortArray _array,
                              jboolean *isCopy) {
  
  BEGIN_JNI_EXCEPTION
  
  // Local object references.
  ArraySInt16* array = *(ArraySInt16**)_array;
  llvm_gcroot(array, 0);
  
  if (isCopy) (*isCopy) = true;

  sint32 len = array->size * sizeof(sint16);
  void* buffer = malloc(len);
  memcpy(buffer, array->elements, len);

  RETURN_FROM_JNI((jshort*)buffer);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


jint *GetIntArrayElements(JNIEnv *env, jintArray _array, jboolean *isCopy) {

  BEGIN_JNI_EXCEPTION
  
  // Local object references.
  ArraySInt32* array = *(ArraySInt32**)_array;
  llvm_gcroot(array, 0);

  if (isCopy) (*isCopy) = true;

  sint32 len = array->size * sizeof(sint32);
  void* buffer = malloc(len);
  memcpy(buffer, array->elements, len);

  RETURN_FROM_JNI((jint*)buffer);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


jlong *GetLongArrayElements(JNIEnv *env, jlongArray _array, jboolean *isCopy) {

  BEGIN_JNI_EXCEPTION
  
  // Local object references.
  ArrayLong* array = *(ArrayLong**)_array;
  llvm_gcroot(array, 0);

  if (isCopy) (*isCopy) = true;

  sint32 len = array->size * sizeof(sint64);
  void* buffer = malloc(len);
  memcpy(buffer, array->elements, len);

  RETURN_FROM_JNI((jlong*)buffer);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


jfloat *GetFloatArrayElements(JNIEnv *env, jfloatArray _array,
                              jboolean *isCopy) {

  BEGIN_JNI_EXCEPTION
  
  // Local object references.
  ArrayFloat* array = *(ArrayFloat**)_array;
  llvm_gcroot(array, 0);

  if (isCopy) (*isCopy) = true;

  sint32 len = array->size * sizeof(float);
  void* buffer = malloc(len);
  memcpy(buffer, array->elements, len);

  RETURN_FROM_JNI((jfloat*)buffer);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


jdouble *GetDoubleArrayElements(JNIEnv *env, jdoubleArray _array,
				jboolean *isCopy) {
  
  BEGIN_JNI_EXCEPTION
  
  // Local object references.
  ArrayDouble* array = *(ArrayDouble**)_array;
  llvm_gcroot(array, 0);
  
  if (isCopy) (*isCopy) = true;

  sint32 len = array->size * sizeof(double);
  void* buffer = malloc(len);
  memcpy(buffer, array->elements, len);

  RETURN_FROM_JNI((jdouble*)buffer);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


void ReleaseBooleanArrayElements(JNIEnv *env, jbooleanArray _array,
				 jboolean *elems, jint mode) {
  
  BEGIN_JNI_EXCEPTION
  
  if (mode == JNI_ABORT) {
    free(elems);
  } else {
    JavaArray* array = *(JavaArray**)_array;
    llvm_gcroot(array, 0);

    sint32 len = array->size;
    memcpy(array->elements, elems, len);

    if (mode == 0) free(elems);
  }
  
  END_JNI_EXCEPTION
  
  RETURN_VOID_FROM_JNI;
}


void ReleaseByteArrayElements(JNIEnv *env, jbyteArray _array, jbyte *elems,
			      jint mode) {
  
  BEGIN_JNI_EXCEPTION
  
  if (mode == JNI_ABORT) {
    free(elems);
  } else {
    JavaArray* array = *(JavaArray**)_array;
    llvm_gcroot(array, 0);

    sint32 len = array->size;
    memcpy(array->elements, elems, len);

    if (mode == 0) free(elems);
  }
  
  END_JNI_EXCEPTION
  
  RETURN_VOID_FROM_JNI;
}


void ReleaseCharArrayElements(JNIEnv *env, jcharArray _array, jchar *elems,
			      jint mode) {
  
  BEGIN_JNI_EXCEPTION
  
  if (mode == JNI_ABORT) {
    free(elems);
  } else {
    JavaArray* array = *(JavaArray**)_array;
    llvm_gcroot(array, 0);

    sint32 len = array->size << 1;
    memcpy(array->elements, elems, len);

    if (mode == 0) free(elems);
  }
  
  END_JNI_EXCEPTION
  
  RETURN_VOID_FROM_JNI;
}


void ReleaseShortArrayElements(JNIEnv *env, jshortArray _array, jshort *elems,
			       jint mode) {
  
  BEGIN_JNI_EXCEPTION
  
  if (mode == JNI_ABORT) {
    free(elems);
  } else {
    JavaArray* array = *(JavaArray**)_array;
    llvm_gcroot(array, 0);

    sint32 len = array->size << 1;
    memcpy(array->elements, elems, len);

    if (mode == 0) free(elems);
  }
  
  END_JNI_EXCEPTION
  
  RETURN_VOID_FROM_JNI;
}


void ReleaseIntArrayElements(JNIEnv *env, jintArray _array, jint *elems,
			     jint mode) {
  
  BEGIN_JNI_EXCEPTION
  
  if (mode == JNI_ABORT) {
    free(elems);
  } else {
    JavaArray* array = *(JavaArray**)_array;
    llvm_gcroot(array, 0);

    sint32 len = array->size << 2;
    memcpy(array->elements, elems, len);

    if (mode == 0) free(elems);
  }
  
  END_JNI_EXCEPTION
  
  RETURN_VOID_FROM_JNI;
}


void ReleaseLongArrayElements(JNIEnv *env, jlongArray _array, jlong *elems,
			      jint mode) {
  
  BEGIN_JNI_EXCEPTION
  
  if (mode == JNI_ABORT) {
    free(elems);
  } else {
    JavaArray* array = *(JavaArray**)_array;
    llvm_gcroot(array, 0);

    sint32 len = array->size << 3;
    memcpy(array->elements, elems, len);

    if (mode == 0) free(elems);
  }
  
  END_JNI_EXCEPTION
  
  RETURN_VOID_FROM_JNI;
}


void ReleaseFloatArrayElements(JNIEnv *env, jfloatArray _array, jfloat *elems,
			       jint mode) {
  BEGIN_JNI_EXCEPTION
  
  if (mode == JNI_ABORT) {
    free(elems);
  } else {
    JavaArray* array = *(JavaArray**)_array;
    llvm_gcroot(array, 0);

    sint32 len = array->size << 2;
    memcpy(array->elements, elems, len);

    if (mode == 0) free(elems);
  }
  
  END_JNI_EXCEPTION
  
  RETURN_VOID_FROM_JNI;
}


void ReleaseDoubleArrayElements(JNIEnv *env, jdoubleArray _array,
				jdouble *elems, jint mode) {
  
  BEGIN_JNI_EXCEPTION
  
  if (mode == JNI_ABORT) {
    free(elems);
  } else {
    JavaArray* array = *(JavaArray**)_array;
    llvm_gcroot(array, 0);

    sint32 len = array->size << 3;
    memcpy(array->elements, elems, len);

    if (mode == 0) free(elems);
  }
  
  END_JNI_EXCEPTION
  
  RETURN_VOID_FROM_JNI;
}


void GetBooleanArrayRegion(JNIEnv *env, jbooleanArray array, jsize start,
			   jsize len, jboolean *buf) {
  BEGIN_JNI_EXCEPTION
  
  ArrayUInt8* Array = *(ArrayUInt8**)array;
  llvm_gcroot(Array, 0);
  memcpy(buf, &(Array->elements[start]), len * sizeof(uint8));
  
  END_JNI_EXCEPTION
  
  RETURN_VOID_FROM_JNI;
}


void GetByteArrayRegion(JNIEnv *env, jbyteArray array, jsize start, jsize len,
			jbyte *buf) {
  
  BEGIN_JNI_EXCEPTION
  
  ArraySInt8* Array = *(ArraySInt8**)array;
  llvm_gcroot(Array, 0);
  memcpy(buf, &(Array->elements[start]), len * sizeof(sint8));
  
  END_JNI_EXCEPTION
  
  RETURN_VOID_FROM_JNI;
}


void GetCharArrayRegion(JNIEnv *env, jcharArray array, jsize start, jsize len,
			jchar *buf) {
  
  BEGIN_JNI_EXCEPTION
  
  ArrayUInt16* Array = *(ArrayUInt16**)array;
  llvm_gcroot(Array, 0);
  memcpy(buf, &(Array->elements[start]), len * sizeof(uint16));
  
  END_JNI_EXCEPTION
  
  RETURN_VOID_FROM_JNI;
}


void GetShortArrayRegion(JNIEnv *env, jshortArray array, jsize start,
			 jsize len, jshort *buf) {
  
  BEGIN_JNI_EXCEPTION
  
  ArraySInt16* Array = *(ArraySInt16**)array;
  llvm_gcroot(Array, 0);
  memcpy(buf, &(Array->elements[start]), len * sizeof(sint16));
  
  END_JNI_EXCEPTION
  
  RETURN_VOID_FROM_JNI;
}


void GetIntArrayRegion(JNIEnv *env, jintArray array, jsize start, jsize len,
		       jint *buf) {
  
  BEGIN_JNI_EXCEPTION
  
  ArraySInt32* Array = *(ArraySInt32**)array;
  llvm_gcroot(Array, 0);
  memcpy(buf, &(Array->elements[start]), len * sizeof(sint32));
  
  END_JNI_EXCEPTION
  
  RETURN_VOID_FROM_JNI;
}


void GetLongArrayRegion(JNIEnv *env, jlongArray array, jsize start, jsize len,
		        jlong *buf) {
  
  BEGIN_JNI_EXCEPTION
  
  ArrayLong* Array = *(ArrayLong**)array;
  llvm_gcroot(Array, 0);
  memcpy(buf, &(Array->elements[start]), len * sizeof(sint64));
  
  END_JNI_EXCEPTION
  
  RETURN_VOID_FROM_JNI;
}


void GetFloatArrayRegion(JNIEnv *env, jfloatArray array, jsize start,
			 jsize len, jfloat *buf) {
  
  BEGIN_JNI_EXCEPTION
  
  ArrayFloat* Array = *(ArrayFloat**)array;
  llvm_gcroot(Array, 0);
  memcpy(buf, &(Array->elements[start]), len * sizeof(float));
  
  END_JNI_EXCEPTION
  
  RETURN_VOID_FROM_JNI;
}


void GetDoubleArrayRegion(JNIEnv *env, jdoubleArray array, jsize start,
			  jsize len, jdouble *buf) {
  
  BEGIN_JNI_EXCEPTION
  
  ArrayDouble* Array = *(ArrayDouble**)array;
  llvm_gcroot(Array, 0);
  memcpy(buf, &(Array->elements[start]), len * sizeof(double));
  
  END_JNI_EXCEPTION
  
  RETURN_VOID_FROM_JNI;
}


void SetBooleanArrayRegion(JNIEnv *env, jbooleanArray array, jsize start,
			   jsize len, const jboolean *buf) {
  
  BEGIN_JNI_EXCEPTION
  
  ArrayUInt8* Array = *(ArrayUInt8**)array;
  llvm_gcroot(Array, 0);
  memcpy(&(Array->elements[start]), buf, len * sizeof(uint8));
  
  END_JNI_EXCEPTION
  
  RETURN_VOID_FROM_JNI;
}


void SetByteArrayRegion(JNIEnv *env, jbyteArray array, jsize start, jsize len,
			                  const jbyte *buf) {
  
  BEGIN_JNI_EXCEPTION
  
  ArraySInt8* Array = *(ArraySInt8**)array;
  llvm_gcroot(Array, 0);
  memcpy(&(Array->elements[start]), buf, len * sizeof(sint8));
  
  END_JNI_EXCEPTION
  
  RETURN_VOID_FROM_JNI;
}


void SetCharArrayRegion(JNIEnv *env, jcharArray array, jsize start, jsize len,
			                  const jchar *buf) {
  
  BEGIN_JNI_EXCEPTION
  
  ArrayUInt16* Array = *(ArrayUInt16**)array;
  llvm_gcroot(Array, 0);
  memcpy(&(Array->elements[start]), buf, len * sizeof(uint16));
  
  END_JNI_EXCEPTION
  
  RETURN_VOID_FROM_JNI;
}


void SetShortArrayRegion(JNIEnv *env, jshortArray array, jsize start,
			                   jsize len, const jshort *buf) {
  
  BEGIN_JNI_EXCEPTION
  
  ArraySInt16* Array = *(ArraySInt16**)array;
  llvm_gcroot(Array, 0);
  memcpy(&(Array->elements[start]), buf, len * sizeof(sint16));
  
  END_JNI_EXCEPTION
  
  RETURN_VOID_FROM_JNI;
}


void SetIntArrayRegion(JNIEnv *env, jintArray array, jsize start, jsize len,
		                   const jint *buf) {
  
  BEGIN_JNI_EXCEPTION
  
  ArraySInt32* Array = *(ArraySInt32**)array;
  llvm_gcroot(Array, 0);
  memcpy(&(Array->elements[start]), buf, len * sizeof(sint32));
  
  END_JNI_EXCEPTION
  
  RETURN_VOID_FROM_JNI;
}


void SetLongArrayRegion(JNIEnv* env, jlongArray array, jsize start, jsize len,
			                  const jlong *buf) {
  
  BEGIN_JNI_EXCEPTION
  
  ArrayLong* Array = *(ArrayLong**)array;
  llvm_gcroot(Array, 0);
  memcpy(&(Array->elements[start]), buf, len * sizeof(sint64));
  
  END_JNI_EXCEPTION
  
  RETURN_VOID_FROM_JNI;
}


void SetFloatArrayRegion(JNIEnv *env, jfloatArray array, jsize start,
			                   jsize len, const jfloat *buf) {
  
  BEGIN_JNI_EXCEPTION
  
  ArrayFloat* Array = *(ArrayFloat**)array;
  llvm_gcroot(Array, 0);
  memcpy(&(Array->elements[start]), buf, len * sizeof(float));
  
  END_JNI_EXCEPTION
  
  RETURN_VOID_FROM_JNI;
}


void SetDoubleArrayRegion(JNIEnv *env, jdoubleArray array, jsize start,
			                    jsize len, const jdouble *buf) {
  
  BEGIN_JNI_EXCEPTION
  
  ArrayDouble* Array = *(ArrayDouble**)array;
  llvm_gcroot(Array, 0);
  memcpy(&(Array->elements[start]), buf, len * sizeof(double));
  
  END_JNI_EXCEPTION
  
  RETURN_VOID_FROM_JNI;
}


jint RegisterNatives(JNIEnv *env, jclass clazz, const JNINativeMethod *methods,
		     jint nMethods) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}


jint UnregisterNatives(JNIEnv *env, jclass clazz) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}

jint MonitorEnter(JNIEnv *env, jobject _obj) {
  
  BEGIN_JNI_EXCEPTION
  
  JavaObject* Obj = *(JavaObject**)_obj;
  llvm_gcroot(Obj, 0);
  
  if (Obj) {
    Obj->acquire();
    RETURN_FROM_JNI(0);
  } else {
    RETURN_FROM_JNI(-1);
  }


  END_JNI_EXCEPTION
  RETURN_FROM_JNI(-1);
}


jint MonitorExit(JNIEnv *env, jobject _obj) {

  BEGIN_JNI_EXCEPTION

  JavaObject* Obj = *(JavaObject**)_obj;
  llvm_gcroot(Obj, 0);
 
  if (Obj) {

    if (!Obj->owner()) {
      JavaThread::get()->getJVM()->illegalMonitorStateException(Obj);    
    }
  
    Obj->release();
    RETURN_FROM_JNI(0);
  } else {
    RETURN_FROM_JNI(-1);
  }

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(-1);
}


jint GetJavaVM(JNIEnv *env, JavaVM **vm) {
  BEGIN_JNI_EXCEPTION
  Jnjvm* myvm = JavaThread::get()->getJVM();
  (*vm) = (JavaVM*)(void*)(&(myvm->javavmEnv));
  RETURN_FROM_JNI(0);
  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


void GetStringRegion(JNIEnv* env, jstring str, jsize start, jsize len,
                     jchar *buf) {
  fprintf(stderr, "Implement me\n");
  abort();
}


void GetStringUTFRegion(JNIEnv* env, jstring str, jsize start, jsize len,
                        char *buf) {
  fprintf(stderr, "Implement me\n");
  abort();
}


void *GetPrimitiveArrayCritical(JNIEnv *env, jarray _array, jboolean *isCopy) {
  BEGIN_JNI_EXCEPTION
  
  JavaArray* array = *(JavaArray**)_array;
  llvm_gcroot(array, 0);

  if (isCopy) (*isCopy) = true;

  UserClassArray* cl = array->getClass()->asArrayClass();
  uint32 logSize = cl->baseClass()->asPrimitiveClass()->logSize;
  sint32 len = array->size << logSize;
  void* buffer = malloc(len);
  memcpy(buffer, array->elements, len);

  RETURN_FROM_JNI((jchar*)buffer);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


void ReleasePrimitiveArrayCritical(JNIEnv *env, jarray _array, void *carray,
				   jint mode) {
  
  BEGIN_JNI_EXCEPTION
  
  if (mode == JNI_ABORT) {
    free(carray);
  } else {
    JavaArray* array = *(JavaArray**)_array;
    llvm_gcroot(array, 0);

    UserClassArray* cl = array->getClass()->asArrayClass();
    uint32 logSize = cl->baseClass()->asPrimitiveClass()->logSize;
    sint32 len = array->size << logSize;
    memcpy(array->elements, carray, len);

    if (mode == 0) free(carray);
  }
  
  END_JNI_EXCEPTION
  
  RETURN_VOID_FROM_JNI;
}


const jchar *GetStringCritical(JNIEnv *env, jstring string, jboolean *isCopy) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}


void ReleaseStringCritical(JNIEnv *env, jstring string, const jchar *cstring) {
  fprintf(stderr, "Implement me\n");
  abort();
}


jweak NewWeakGlobalRef(JNIEnv* env, jobject obj) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}


void DeleteWeakGlobalRef(JNIEnv* env, jweak ref) {
  fprintf(stderr, "Implement me\n");
  abort();
}


jobject NewGlobalRef(JNIEnv* env, jobject obj) {
  
  BEGIN_JNI_EXCEPTION
  
  // Local object references.
  if (obj) {
    JavaObject* Obj = *(JavaObject**)obj;
    llvm_gcroot(Obj, 0);

    Jnjvm* vm = JavaThread::get()->getJVM();


    vm->globalRefsLock.lock();
    JavaObject** res = vm->globalRefs.addJNIReference(Obj);
    vm->globalRefsLock.unlock();

    RETURN_FROM_JNI((jobject)res);
  } else {
    RETURN_FROM_JNI(0);
  }
  
  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


void DeleteGlobalRef(JNIEnv* env, jobject globalRef) {
  
  BEGIN_JNI_EXCEPTION
  
  Jnjvm* vm = myVM(env);
  vm->globalRefsLock.lock();
  vm->globalRefs.removeJNIReference((JavaObject**)globalRef);
  vm->globalRefsLock.unlock();
  
  END_JNI_EXCEPTION
  
  RETURN_VOID_FROM_JNI;
}


jboolean ExceptionCheck(JNIEnv *env) {
  BEGIN_JNI_EXCEPTION
  
  if (JavaThread::get()->pendingException) {
    RETURN_FROM_JNI(JNI_TRUE);
  } else {
    RETURN_FROM_JNI(JNI_FALSE);
  }
  
  END_JNI_EXCEPTION
  RETURN_FROM_JNI(false);
}


jobject NewDirectByteBuffer(JNIEnv *env, void *address, jlong capacity) {
  
  BEGIN_JNI_EXCEPTION
  
  JavaObject* res = 0;
  JavaObject* p = 0;
  llvm_gcroot(res, 0);
  llvm_gcroot(p, 0);

  JavaThread* th = JavaThread::get();
  Jnjvm* myvm = th->getJVM();
  UserClass* BB = myvm->upcalls->newDirectByteBuffer;

  res = BB->doNew(myvm);

#if (__WORDSIZE == 32)
  UserClass* PP = myvm->upcalls->newPointer32;
  p = PP->doNew(myvm);
  myvm->upcalls->dataPointer32->setInt32Field(p, (uint32)address);
#else
  UserClass* PP = myvm->upcalls->newPointer64;
  p = PP->doNew(myvm);
  myvm->upcalls->dataPointer64->setLongField(p, (jlong)address);
#endif

  myvm->upcalls->InitDirectByteBuffer->invokeIntSpecial(myvm, BB, res, 0, p,
                                                        (uint32)capacity,
                                                        (uint32)capacity, 0);

  jobject ret = (jobject)th->pushJNIRef(res);
  RETURN_FROM_JNI(ret);
  
  END_JNI_EXCEPTION
  
  RETURN_FROM_JNI(0);
}


void *GetDirectBufferAddress(JNIEnv *env, jobject _buf) {

  BEGIN_JNI_EXCEPTION
 
  // Local object references.
  JavaObject* buf = *(JavaObject**)_buf;
  JavaObject* address = 0;
  llvm_gcroot(buf, 0);
  llvm_gcroot(address, 0);

  Jnjvm* vm = myVM(env);
  address = vm->upcalls->bufferAddress->getObjectField(buf);
  if (address != 0) {
#if (__WORDSIZE == 32)
    int res = vm->upcalls->dataPointer32->getInt32Field(address);
#else
    jlong res = vm->upcalls->dataPointer64->getLongField(address);
#endif
    RETURN_FROM_JNI((void*)res);
  } else {
    RETURN_FROM_JNI(0);
  }

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}


jlong GetDirectBufferCapacity(JNIEnv* env, jobject buf) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}

jobjectRefType GetObjectRefType(JNIEnv* env, jobject obj) {
  fprintf(stderr, "Implement me\n");
  abort();
  return (jobjectRefType)0;
}



jint DestroyJavaVM(JavaVM *vm) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}


jint AttachCurrentThread(JavaVM *vm, void **env, void *thr_args) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}


jint DetachCurrentThread(JavaVM *vm) {
  fprintf(stderr, "Implement me\n");
  abort();
  return 0;
}


jint GetEnv(JavaVM *vm, void **env, jint version) {

  BEGIN_JNI_EXCEPTION

  JavaThread* _th = JavaThread::get();
  JavaObject* obj = _th->currentThread();
  llvm_gcroot(obj, 0);

  Jnjvm* myvm = _th->getJVM();
  if (obj != 0) {
    (*env) = &(myvm->jniEnv);
    RETURN_FROM_JNI(JNI_OK);
  } else {
    (*env) = 0;
    RETURN_FROM_JNI(JNI_EDETACHED);
  }

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}



jint AttachCurrentThreadAsDaemon(JavaVM *vm, void **par1, void *par2) {
  fprintf(stderr, "Implement me\n");
  abort();
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
