#include "j3/j3jni.h"
#include "j3/j3.h"
#include "j3/j3class.h"
#include "j3/j3classloader.h"
#include "j3/j3object.h"
#include "j3/j3method.h"
#include "j3/j3thread.h"
#include <stdlib.h>

#define enterJVM() try {
#define leaveJVM() } catch(void* e) { J3Thread::get()->setPendingException(J3Thread::get()->push((J3Object*)e)); }

#define NYI() { J3Thread::get()->vm()->internalError(L"not yet implemented: '%s'", __PRETTY_FUNCTION__); }

namespace j3 {

jint JNICALL GetVersion(JNIEnv* env) { enterJVM(); leaveJVM(); NYI(); }

jclass JNICALL DefineClass(JNIEnv* env, const char* name, jobject loader, const jbyte* buf, jsize len) { enterJVM(); leaveJVM(); NYI(); }
jclass JNICALL FindClass(JNIEnv* env, const char* name) { 
	jclass res;

	enterJVM();
	J3Method* m = J3Thread::get()->getJavaCaller();
	J3ClassLoader* loader = m ? m->cl()->loader() : J3Thread::get()->vm()->initialClassLoader;
	J3Class* cl = loader->getClass(loader->vm()->names()->get(name));
	cl->initialise();
	res = cl->javaClass();
	leaveJVM(); 

	return res;
}

jmethodID JNICALL FromReflectedMethod(JNIEnv* env, jobject method) { enterJVM(); leaveJVM(); NYI(); }
jfieldID JNICALL FromReflectedField(JNIEnv* env, jobject field) { enterJVM(); leaveJVM(); NYI(); }

jobject JNICALL ToReflectedMethod(JNIEnv* env, jclass cls, jmethodID methodID, jboolean isStatic) { enterJVM(); leaveJVM(); NYI(); }

jclass JNICALL GetSuperclass(JNIEnv* env, jclass sub) { enterJVM(); leaveJVM(); NYI(); }
jboolean JNICALL IsAssignableFrom(JNIEnv* env, jclass sub, jclass sup) { enterJVM(); leaveJVM(); NYI(); }

jobject JNICALL ToReflectedField(JNIEnv* env, jclass cls, jfieldID fieldID, jboolean isStatic) { enterJVM(); leaveJVM(); NYI(); }

jint JNICALL Throw(JNIEnv* env, jthrowable obj) { enterJVM(); leaveJVM(); NYI(); }
jint JNICALL ThrowNew(JNIEnv* env, jclass clazz, const char* msg) { enterJVM(); leaveJVM(); NYI(); }

jthrowable JNICALL ExceptionOccurred(JNIEnv* env) { 
	return J3Thread::get()->pendingException();
}

void JNICALL ExceptionDescribe(JNIEnv* env) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL ExceptionClear(JNIEnv* env) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL FatalError(JNIEnv* env, const char* msg) { enterJVM(); leaveJVM(); NYI(); }

jint JNICALL PushLocalFrame(JNIEnv* env, jint capacity) { enterJVM(); leaveJVM(); NYI(); }
jobject JNICALL PopLocalFrame(JNIEnv* env, jobject result) { enterJVM(); leaveJVM(); NYI(); }

jobject JNICALL NewGlobalRef(JNIEnv* env, jobject lobj) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL DeleteGlobalRef(JNIEnv* env, jobject gref) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL DeleteLocalRef(JNIEnv* env, jobject obj) { 
	enterJVM(); 
	if(obj) obj->harakiri();
	leaveJVM();
}

jboolean JNICALL IsSameObject(JNIEnv* env, jobject obj1, jobject obj2) { enterJVM(); leaveJVM(); NYI(); }
jobject JNICALL NewLocalRef(JNIEnv* env, jobject ref) { enterJVM(); leaveJVM(); NYI(); }
jint JNICALL EnsureLocalCapacity(JNIEnv* env, jint capacity) { 
	enterJVM(); 
	J3Thread::get()->ensureCapacity(capacity);
	leaveJVM(); 
	return JNI_OK;
}

jobject JNICALL AllocObject(JNIEnv* env, jclass clazz) { enterJVM(); leaveJVM(); NYI(); }
jobject JNICALL NewObject(JNIEnv* env, jclass clazz, jmethodID methodID, ...) { enterJVM(); leaveJVM(); NYI(); }
jobject JNICALL NewObjectV(JNIEnv* env, jclass clazz, jmethodID methodID, va_list args) { enterJVM(); leaveJVM(); NYI(); }
jobject JNICALL NewObjectA(JNIEnv* env, jclass clazz, jmethodID methodID, const jvalue* args) { enterJVM(); leaveJVM(); NYI(); }

jclass JNICALL GetObjectClass(JNIEnv* env, jobject obj) { 
	jclass res;

	enterJVM(); 
	res = obj->vt()->type()->asObjectType()->javaClass();
	leaveJVM(); 

	return res;
}

jboolean JNICALL IsInstanceOf(JNIEnv* env, jobject obj, jclass clazz) { enterJVM(); leaveJVM(); NYI(); }

jmethodID JNICALL GetMethodID(JNIEnv* env, jclass clazz, const char* name, const char* sig) { 
	jmethodID res;

	enterJVM(); 
	J3ObjectType* cl = J3ObjectType::nativeClass(clazz);
	cl->initialise();
	vmkit::Names* n = cl->loader()->vm()->names();
	res = cl->findVirtualMethod(n->get(name), n->get(sig));
	leaveJVM(); 

	return res;
}

#define doInvoke(jtype, id, j3type)																			\
	jtype JNICALL Call##id##Method(JNIEnv* env, jobject obj, jmethodID methodID, ...) { \
		va_list va;																													\
		va_start(va, methodID);																							\
		jobject res = env->Call##id##MethodV(obj, methodID, va);						\
		va_end(va);																													\
		return res;																													\
	}																																			\
																																				\
	jtype JNICALL Call##id##MethodV(JNIEnv* env, jobject obj, jmethodID methodID, va_list args) {	\
		jvalue res;																													\
																																				\
		enterJVM();																													\
		res = methodID->invokeVirtual(obj, args);														\
		leaveJVM();																													\
																																				\
		return res.val##j3type;																							\
	}																																			\
																																				\
	jtype JNICALL Call##id##MethodA(JNIEnv* env, jobject obj, jmethodID methodID, const jvalue* args) { \
		jvalue res;																													\
																																				\
		enterJVM();																													\
		res = methodID->invokeVirtual(obj, args);														\
		leaveJVM();																													\
																																				\
		return res.val##j3type;																							\
	}																																			\
																																				\
	jtype JNICALL CallStatic##id##Method(JNIEnv* env, jclass clazz, jmethodID methodID, ...) { \
		va_list va;																													\
		va_start(va, methodID);																							\
		jobject res = env->CallStatic##id##MethodV(clazz, methodID, va);		\
		va_end(va);																													\
		return res;																													\
	}																																			\
																																				\
	jtype JNICALL CallStatic##id##MethodV(JNIEnv* env, jclass clazz, jmethodID methodID, va_list args) { \
		jvalue res;																													\
																																				\
		enterJVM();																													\
		res = methodID->invokeStatic(args);																	\
		leaveJVM();																													\
																																				\
		return res.val##j3type;																							\
	}																																			\
																																				\
	jtype JNICALL CallStatic##id##MethodA(JNIEnv* env, jclass clazz, jmethodID methodID, const jvalue* args) { \
		jvalue res;																													\
																																				\
		enterJVM();																													\
		res = methodID->invokeStatic(args);																	\
		leaveJVM();																													\
																																				\
		return res.val##j3type;																							\
	}

	doInvoke(jobject, Object, Object);



jboolean JNICALL CallBooleanMethod(JNIEnv* env, jobject obj, jmethodID methodID, ...) { enterJVM(); leaveJVM(); NYI(); }
jboolean JNICALL CallBooleanMethodV(JNIEnv* env, jobject obj, jmethodID methodID, va_list args) { enterJVM(); leaveJVM(); NYI(); }
jboolean JNICALL CallBooleanMethodA(JNIEnv* env, jobject obj, jmethodID methodID, const jvalue* args) { enterJVM(); leaveJVM(); NYI(); }

jbyte JNICALL CallByteMethod(JNIEnv* env, jobject obj, jmethodID methodID, ...) { enterJVM(); leaveJVM(); NYI(); }
jbyte JNICALL CallByteMethodV(JNIEnv* env, jobject obj, jmethodID methodID, va_list args) { enterJVM(); leaveJVM(); NYI(); }
jbyte JNICALL CallByteMethodA(JNIEnv* env, jobject obj, jmethodID methodID, const jvalue* args) { enterJVM(); leaveJVM(); NYI(); }

jchar JNICALL CallCharMethod(JNIEnv* env, jobject obj, jmethodID methodID, ...) { enterJVM(); leaveJVM(); NYI(); }
jchar JNICALL CallCharMethodV(JNIEnv* env, jobject obj, jmethodID methodID, va_list args) { enterJVM(); leaveJVM(); NYI(); }
jchar JNICALL CallCharMethodA(JNIEnv* env, jobject obj, jmethodID methodID, const jvalue* args) { enterJVM(); leaveJVM(); NYI(); }

jshort JNICALL CallShortMethod(JNIEnv* env, jobject obj, jmethodID methodID, ...) { enterJVM(); leaveJVM(); NYI(); }
jshort JNICALL CallShortMethodV(JNIEnv* env, jobject obj, jmethodID methodID, va_list args) { enterJVM(); leaveJVM(); NYI(); }
jshort JNICALL CallShortMethodA(JNIEnv* env, jobject obj, jmethodID methodID, const jvalue* args) { enterJVM(); leaveJVM(); NYI(); }

jint JNICALL CallIntMethod(JNIEnv* env, jobject obj, jmethodID methodID, ...) { enterJVM(); leaveJVM(); NYI(); }
jint JNICALL CallIntMethodV(JNIEnv* env, jobject obj, jmethodID methodID, va_list args) { enterJVM(); leaveJVM(); NYI(); }
jint JNICALL CallIntMethodA(JNIEnv* env, jobject obj, jmethodID methodID, const jvalue* args) { enterJVM(); leaveJVM(); NYI(); }

jlong JNICALL CallLongMethod(JNIEnv* env, jobject obj, jmethodID methodID, ...) { enterJVM(); leaveJVM(); NYI(); }
jlong JNICALL CallLongMethodV(JNIEnv* env, jobject obj, jmethodID methodID, va_list args) { enterJVM(); leaveJVM(); NYI(); }
jlong JNICALL CallLongMethodA(JNIEnv* env, jobject obj, jmethodID methodID, const jvalue* args) { enterJVM(); leaveJVM(); NYI(); }

jfloat JNICALL CallFloatMethod(JNIEnv* env, jobject obj, jmethodID methodID, ...) { enterJVM(); leaveJVM(); NYI(); }
jfloat JNICALL CallFloatMethodV(JNIEnv* env, jobject obj, jmethodID methodID, va_list args) { enterJVM(); leaveJVM(); NYI(); }
jfloat JNICALL CallFloatMethodA(JNIEnv* env, jobject obj, jmethodID methodID, const jvalue* args) { enterJVM(); leaveJVM(); NYI(); }

jdouble JNICALL CallDoubleMethod(JNIEnv* env, jobject obj, jmethodID methodID, ...) { enterJVM(); leaveJVM(); NYI(); }
jdouble JNICALL CallDoubleMethodV(JNIEnv* env, jobject obj, jmethodID methodID, va_list args) { enterJVM(); leaveJVM(); NYI(); }
jdouble JNICALL CallDoubleMethodA(JNIEnv* env, jobject obj, jmethodID methodID, const jvalue* args) { enterJVM(); leaveJVM(); NYI(); }

void JNICALL CallVoidMethod(JNIEnv* env, jobject obj, jmethodID methodID, ...) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL CallVoidMethodV(JNIEnv* env, jobject obj, jmethodID methodID, va_list args) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL CallVoidMethodA(JNIEnv* env, jobject obj, jmethodID methodID, const jvalue* args) { enterJVM(); leaveJVM(); NYI(); }

jobject JNICALL CallNonvirtualObjectMethod(JNIEnv* env, jobject obj, jclass clazz, jmethodID methodID, ...) { enterJVM(); leaveJVM(); NYI(); }
jobject JNICALL CallNonvirtualObjectMethodV(JNIEnv* env, jobject obj, jclass clazz, jmethodID methodID, va_list args) { enterJVM(); leaveJVM(); NYI(); }
jobject JNICALL CallNonvirtualObjectMethodA(JNIEnv* env, jobject obj, jclass clazz, jmethodID methodID, const jvalue* args) { enterJVM(); leaveJVM(); NYI(); }

jboolean JNICALL CallNonvirtualBooleanMethod(JNIEnv* env, jobject obj, jclass clazz, jmethodID methodID, ...) { enterJVM(); leaveJVM(); NYI(); }
jboolean JNICALL CallNonvirtualBooleanMethodV(JNIEnv* env, jobject obj, jclass clazz, jmethodID methodID, va_list args) { enterJVM(); leaveJVM(); NYI(); }
jboolean JNICALL CallNonvirtualBooleanMethodA(JNIEnv* env, jobject obj, jclass clazz, jmethodID methodID, const jvalue* args) { enterJVM(); leaveJVM(); NYI(); }

jbyte JNICALL CallNonvirtualByteMethod(JNIEnv* env, jobject obj, jclass clazz, jmethodID methodID, ...) { enterJVM(); leaveJVM(); NYI(); }
jbyte JNICALL CallNonvirtualByteMethodV(JNIEnv* env, jobject obj, jclass clazz, jmethodID methodID, va_list args) { enterJVM(); leaveJVM(); NYI(); }
jbyte JNICALL CallNonvirtualByteMethodA(JNIEnv* env, jobject obj, jclass clazz, jmethodID methodID, const jvalue* args) { enterJVM(); leaveJVM(); NYI(); }

jchar JNICALL CallNonvirtualCharMethod(JNIEnv* env, jobject obj, jclass clazz, jmethodID methodID, ...) { enterJVM(); leaveJVM(); NYI(); }
jchar JNICALL CallNonvirtualCharMethodV(JNIEnv* env, jobject obj, jclass clazz, jmethodID methodID, va_list args) { enterJVM(); leaveJVM(); NYI(); }
jchar CallNonvirtualCharMethodA(JNIEnv* env, jobject obj, jclass clazz, jmethodID methodID, const jvalue* args) { enterJVM(); leaveJVM(); NYI(); }

jshort JNICALL CallNonvirtualShortMethod(JNIEnv* env, jobject obj, jclass clazz, jmethodID methodID, ...) { enterJVM(); leaveJVM(); NYI(); }
jshort JNICALL CallNonvirtualShortMethodV(JNIEnv* env, jobject obj, jclass clazz, jmethodID methodID, va_list args) { enterJVM(); leaveJVM(); NYI(); }
jshort JNICALL CallNonvirtualShortMethodA(JNIEnv* env, jobject obj, jclass clazz, jmethodID methodID, const jvalue* args) { enterJVM(); leaveJVM(); NYI(); }

jint JNICALL CallNonvirtualIntMethod(JNIEnv* env, jobject obj, jclass clazz, jmethodID methodID, ...) { enterJVM(); leaveJVM(); NYI(); }
jint JNICALL CallNonvirtualIntMethodV(JNIEnv* env, jobject obj, jclass clazz, jmethodID methodID, va_list args) { enterJVM(); leaveJVM(); NYI(); }
jint JNICALL CallNonvirtualIntMethodA(JNIEnv* env, jobject obj, jclass clazz, jmethodID methodID, const jvalue* args) { enterJVM(); leaveJVM(); NYI(); }

jlong JNICALL CallNonvirtualLongMethod(JNIEnv* env, jobject obj, jclass clazz, jmethodID methodID, ...) { enterJVM(); leaveJVM(); NYI(); }
jlong JNICALL CallNonvirtualLongMethodV(JNIEnv* env, jobject obj, jclass clazz, jmethodID methodID, va_list args) { enterJVM(); leaveJVM(); NYI(); }
jlong JNICALL CallNonvirtualLongMethodA(JNIEnv* env, jobject obj, jclass clazz, jmethodID methodID, const jvalue* args) { enterJVM(); leaveJVM(); NYI(); }

jfloat JNICALL CallNonvirtualFloatMethod(JNIEnv* env, jobject obj, jclass clazz, jmethodID methodID, ...) { enterJVM(); leaveJVM(); NYI(); }
jfloat JNICALL CallNonvirtualFloatMethodV(JNIEnv* env, jobject obj, jclass clazz, jmethodID methodID, va_list args) { enterJVM(); leaveJVM(); NYI(); }
jfloat JNICALL CallNonvirtualFloatMethodA(JNIEnv* env, jobject obj, jclass clazz, jmethodID methodID, const jvalue* args) { enterJVM(); leaveJVM(); NYI(); }

jdouble JNICALL CallNonvirtualDoubleMethod(JNIEnv* env, jobject obj, jclass clazz, jmethodID methodID, ...) { enterJVM(); leaveJVM(); NYI(); }
jdouble JNICALL CallNonvirtualDoubleMethodV(JNIEnv* env, jobject obj, jclass clazz, jmethodID methodID, va_list args) { enterJVM(); leaveJVM(); NYI(); }
jdouble JNICALL CallNonvirtualDoubleMethodA(JNIEnv* env, jobject obj, jclass clazz, jmethodID methodID, const jvalue* args) { enterJVM(); leaveJVM(); NYI(); }

void JNICALL CallNonvirtualVoidMethod(JNIEnv* env, jobject obj, jclass clazz, jmethodID methodID, ...) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL CallNonvirtualVoidMethodV(JNIEnv* env, jobject obj, jclass clazz, jmethodID methodID, va_list args) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL CallNonvirtualVoidMethodA(JNIEnv* env, jobject obj, jclass clazz, jmethodID methodID, const jvalue* args) { enterJVM(); leaveJVM(); NYI(); }

jfieldID JNICALL GetFieldID(JNIEnv* env, jclass clazz, const char* name, const char* sig) { enterJVM(); leaveJVM(); NYI(); }

jobject JNICALL GetObjectField(JNIEnv* env, jobject obj, jfieldID fieldID) { enterJVM(); leaveJVM(); NYI(); }
jboolean JNICALL GetBooleanField(JNIEnv* env, jobject obj, jfieldID fieldID) { enterJVM(); leaveJVM(); NYI(); }
jbyte JNICALL GetByteField(JNIEnv* env, jobject obj, jfieldID fieldID) { enterJVM(); leaveJVM(); NYI(); }
jchar JNICALL GetCharField(JNIEnv* env, jobject obj, jfieldID fieldID) { enterJVM(); leaveJVM(); NYI(); }
jshort JNICALL GetShortField(JNIEnv* env, jobject obj, jfieldID fieldID) { enterJVM(); leaveJVM(); NYI(); }
jint JNICALL GetIntField(JNIEnv* env, jobject obj, jfieldID fieldID) { enterJVM(); leaveJVM(); NYI(); }
jlong JNICALL GetLongField(JNIEnv* env, jobject obj, jfieldID fieldID) { enterJVM(); leaveJVM(); NYI(); }
jfloat JNICALL GetFloatField(JNIEnv* env, jobject obj, jfieldID fieldID) { enterJVM(); leaveJVM(); NYI(); }
jdouble JNICALL GetDoubleField(JNIEnv* env, jobject obj, jfieldID fieldID) { enterJVM(); leaveJVM(); NYI(); }

void JNICALL SetObjectField(JNIEnv* env, jobject obj, jfieldID fieldID, jobject val) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL SetBooleanField(JNIEnv* env, jobject obj, jfieldID fieldID, jboolean val) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL SetByteField(JNIEnv* env, jobject obj, jfieldID fieldID, jbyte val) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL SetCharField(JNIEnv* env, jobject obj, jfieldID fieldID, jchar val) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL SetShortField(JNIEnv* env, jobject obj, jfieldID fieldID, jshort val) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL SetIntField(JNIEnv* env, jobject obj, jfieldID fieldID, jint val) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL SetLongField(JNIEnv* env, jobject obj, jfieldID fieldID, jlong val) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL SetFloatField(JNIEnv* env, jobject obj, jfieldID fieldID, jfloat val) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL SetDoubleField(JNIEnv* env, jobject obj, jfieldID fieldID, jdouble val) { enterJVM(); leaveJVM(); NYI(); }

jmethodID JNICALL GetStaticMethodID(JNIEnv* env, jclass clazz, const char* name, const char* sig) { 
	jmethodID res;

	enterJVM(); 
	
	J3ObjectType* cl = J3ObjectType::nativeClass(clazz);
	cl->initialise();
	vmkit::Names* n = cl->loader()->vm()->names();
	res = cl->findStaticMethod(n->get(name), n->get(sig));

	leaveJVM(); 

	return res;
}

jboolean JNICALL CallStaticBooleanMethod(JNIEnv* env, jclass clazz, jmethodID methodID, ...) { enterJVM(); leaveJVM(); NYI(); }
jboolean JNICALL CallStaticBooleanMethodV(JNIEnv* env, jclass clazz, jmethodID methodID, va_list args) { enterJVM(); leaveJVM(); NYI(); }
jboolean JNICALL CallStaticBooleanMethodA(JNIEnv* env, jclass clazz, jmethodID methodID, const jvalue* args) { enterJVM(); leaveJVM(); NYI(); }

jbyte JNICALL CallStaticByteMethod(JNIEnv* env, jclass clazz, jmethodID methodID, ...) { enterJVM(); leaveJVM(); NYI(); }
jbyte JNICALL CallStaticByteMethodV(JNIEnv* env, jclass clazz, jmethodID methodID, va_list args) { enterJVM(); leaveJVM(); NYI(); }
jbyte JNICALL CallStaticByteMethodA(JNIEnv* env, jclass clazz, jmethodID methodID, const jvalue* args) { enterJVM(); leaveJVM(); NYI(); }

jchar JNICALL CallStaticCharMethod(JNIEnv* env, jclass clazz, jmethodID methodID, ...) { enterJVM(); leaveJVM(); NYI(); }
jchar JNICALL CallStaticCharMethodV(JNIEnv* env, jclass clazz, jmethodID methodID, va_list args) { enterJVM(); leaveJVM(); NYI(); }
jchar JNICALL CallStaticCharMethodA(JNIEnv* env, jclass clazz, jmethodID methodID, const jvalue* args) { enterJVM(); leaveJVM(); NYI(); }

jshort JNICALL CallStaticShortMethod(JNIEnv* env, jclass clazz, jmethodID methodID, ...) { enterJVM(); leaveJVM(); NYI(); }
jshort JNICALL CallStaticShortMethodV(JNIEnv* env, jclass clazz, jmethodID methodID, va_list args) { enterJVM(); leaveJVM(); NYI(); }
jshort JNICALL CallStaticShortMethodA(JNIEnv* env, jclass clazz, jmethodID methodID, const jvalue* args) { enterJVM(); leaveJVM(); NYI(); }

jint JNICALL CallStaticIntMethod(JNIEnv* env, jclass clazz, jmethodID methodID, ...) { enterJVM(); leaveJVM(); NYI(); }
jint JNICALL CallStaticIntMethodV(JNIEnv* env, jclass clazz, jmethodID methodID, va_list args) { enterJVM(); leaveJVM(); NYI(); }
jint JNICALL CallStaticIntMethodA(JNIEnv* env, jclass clazz, jmethodID methodID, const jvalue* args) { enterJVM(); leaveJVM(); NYI(); }

jlong JNICALL CallStaticLongMethod(JNIEnv* env, jclass clazz, jmethodID methodID, ...) { enterJVM(); leaveJVM(); NYI(); }
jlong JNICALL CallStaticLongMethodV(JNIEnv* env, jclass clazz, jmethodID methodID, va_list args) { enterJVM(); leaveJVM(); NYI(); }
jlong JNICALL CallStaticLongMethodA(JNIEnv* env, jclass clazz, jmethodID methodID, const jvalue* args) { enterJVM(); leaveJVM(); NYI(); }

jfloat JNICALL CallStaticFloatMethod(JNIEnv* env, jclass clazz, jmethodID methodID, ...) { enterJVM(); leaveJVM(); NYI(); }
jfloat JNICALL CallStaticFloatMethodV(JNIEnv* env, jclass clazz, jmethodID methodID, va_list args) { enterJVM(); leaveJVM(); NYI(); }
jfloat JNICALL CallStaticFloatMethodA(JNIEnv* env, jclass clazz, jmethodID methodID, const jvalue* args) { enterJVM(); leaveJVM(); NYI(); }

jdouble JNICALL CallStaticDoubleMethod(JNIEnv* env, jclass clazz, jmethodID methodID, ...) { enterJVM(); leaveJVM(); NYI(); }
jdouble JNICALL CallStaticDoubleMethodV(JNIEnv* env, jclass clazz, jmethodID methodID, va_list args) { enterJVM(); leaveJVM(); NYI(); }
jdouble JNICALL CallStaticDoubleMethodA(JNIEnv* env, jclass clazz, jmethodID methodID, const jvalue* args) { enterJVM(); leaveJVM(); NYI(); }

void JNICALL CallStaticVoidMethod(JNIEnv* env, jclass cls, jmethodID methodID, ...) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL CallStaticVoidMethodV(JNIEnv* env, jclass cls, jmethodID methodID, va_list args) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL CallStaticVoidMethodA(JNIEnv* env, jclass cls, jmethodID methodID, const jvalue* args) { enterJVM(); leaveJVM(); NYI(); }

jfieldID JNICALL GetStaticFieldID(JNIEnv* env, jclass clazz, const char* name, const char* sig) { enterJVM(); leaveJVM(); NYI(); }
jobject JNICALL GetStaticObjectField(JNIEnv* env, jclass clazz, jfieldID fieldID) { enterJVM(); leaveJVM(); NYI(); }
jboolean JNICALL GetStaticBooleanField(JNIEnv* env, jclass clazz, jfieldID fieldID) { enterJVM(); leaveJVM(); NYI(); }
jbyte JNICALL GetStaticByteField(JNIEnv* env, jclass clazz, jfieldID fieldID) { enterJVM(); leaveJVM(); NYI(); }
jchar JNICALL GetStaticCharField(JNIEnv* env, jclass clazz, jfieldID fieldID) { enterJVM(); leaveJVM(); NYI(); }
jshort JNICALL GetStaticShortField(JNIEnv* env, jclass clazz, jfieldID fieldID) { enterJVM(); leaveJVM(); NYI(); }
jint JNICALL GetStaticIntField(JNIEnv* env, jclass clazz, jfieldID fieldID) { enterJVM(); leaveJVM(); NYI(); }
jlong JNICALL GetStaticLongField(JNIEnv* env, jclass clazz, jfieldID fieldID) { enterJVM(); leaveJVM(); NYI(); }
jfloat JNICALL GetStaticFloatField(JNIEnv* env, jclass clazz, jfieldID fieldID) { enterJVM(); leaveJVM(); NYI(); }
jdouble JNICALL GetStaticDoubleField(JNIEnv* env, jclass clazz, jfieldID fieldID) { enterJVM(); leaveJVM(); NYI(); }

void JNICALL SetStaticObjectField(JNIEnv* env, jclass clazz, jfieldID fieldID, jobject value) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL SetStaticBooleanField(JNIEnv* env, jclass clazz, jfieldID fieldID, jboolean value) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL SetStaticByteField(JNIEnv* env, jclass clazz, jfieldID fieldID, jbyte value) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL SetStaticCharField(JNIEnv* env, jclass clazz, jfieldID fieldID, jchar value) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL SetStaticShortField(JNIEnv* env, jclass clazz, jfieldID fieldID, jshort value) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL SetStaticIntField(JNIEnv* env, jclass clazz, jfieldID fieldID, jint value) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL SetStaticLongField(JNIEnv* env, jclass clazz, jfieldID fieldID, jlong value) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL SetStaticFloatField(JNIEnv* env, jclass clazz, jfieldID fieldID, jfloat value) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL SetStaticDoubleField(JNIEnv* env, jclass clazz, jfieldID fieldID, jdouble value) { enterJVM(); leaveJVM(); NYI(); }

jstring JNICALL NewString(JNIEnv* env, const jchar* unicode, jsize len) { enterJVM(); leaveJVM(); NYI(); }
jsize JNICALL GetStringLength(JNIEnv* env, jstring str) { enterJVM(); leaveJVM(); NYI(); }
const jchar* JNICALL GetStringChars(JNIEnv* env, jstring str, jboolean* isCopy) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL ReleaseStringChars(JNIEnv* env, jstring str, const jchar* chars) { enterJVM(); leaveJVM(); NYI(); }

jstring JNICALL NewStringUTF(JNIEnv* env, const char* utf) { 
	jstring res;

	enterJVM(); 
	res = J3Thread::get()->push(J3Thread::get()->vm()->utfToString(utf)); /* harakiri want to kill me */
	leaveJVM(); 

	return res;
}

jsize JNICALL GetStringUTFLength(JNIEnv* env, jstring str) { enterJVM(); leaveJVM(); NYI(); }

const char* JNICALL GetStringUTFChars(JNIEnv* env, jstring str, jboolean* isCopy) { 
	char* res;

	enterJVM(); 
	J3* vm = str->vt()->type()->loader()->vm();
	jobject content = str->getObject(vm->stringValue);
	uint32_t length = content->arrayLength();
	res = new char[length+1];

	for(uint32_t i=0; i<length; i++)
		res[i] = content->getCharAt(i);

	res[length] = 0;

	if(isCopy)
		*isCopy = 1;

	leaveJVM(); 

	return res;
}

void JNICALL ReleaseStringUTFChars(JNIEnv* env, jstring str, const char* chars) { 
	enterJVM(); 
	delete[] chars;
	leaveJVM(); 
}


jsize JNICALL GetArrayLength(JNIEnv* env, jarray array) { enterJVM(); leaveJVM(); NYI(); }

jobjectArray JNICALL NewObjectArray(JNIEnv* env, jsize len, jclass clazz, jobject init) { enterJVM(); leaveJVM(); NYI(); }
jobject JNICALL GetObjectArrayElement(JNIEnv* env, jobjectArray array, jsize index) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL SetObjectArrayElement(JNIEnv* env, jobjectArray array, jsize index, jobject val) { enterJVM(); leaveJVM(); NYI(); }

jbooleanArray JNICALL NewBooleanArray(JNIEnv* env, jsize len) { enterJVM(); leaveJVM(); NYI(); }
jbyteArray JNICALL NewByteArray(JNIEnv* env, jsize len) { enterJVM(); leaveJVM(); NYI(); }
jcharArray JNICALL NewCharArray(JNIEnv* env, jsize len) { enterJVM(); leaveJVM(); NYI(); }
jshortArray JNICALL NewShortArray(JNIEnv* env, jsize len) { enterJVM(); leaveJVM(); NYI(); }
jintArray JNICALL NewIntArray(JNIEnv* env, jsize len) { enterJVM(); leaveJVM(); NYI(); }
jlongArray JNICALL NewLongArray(JNIEnv* env, jsize len) { enterJVM(); leaveJVM(); NYI(); }
jfloatArray JNICALL NewFloatArray(JNIEnv* env, jsize len) { enterJVM(); leaveJVM(); NYI(); }
jdoubleArray JNICALL NewDoubleArray(JNIEnv* env, jsize len) { enterJVM(); leaveJVM(); NYI(); }

jboolean* JNICALL GetBooleanArrayElements(JNIEnv* env, jbooleanArray array, jboolean* isCopy) { enterJVM(); leaveJVM(); NYI(); }
jbyte* JNICALL GetByteArrayElements(JNIEnv* env, jbyteArray array, jboolean* isCopy) { enterJVM(); leaveJVM(); NYI(); }
jchar* JNICALL GetCharArrayElements(JNIEnv* env, jcharArray array, jboolean* isCopy) { enterJVM(); leaveJVM(); NYI(); }
jshort* JNICALL GetShortArrayElements(JNIEnv* env, jshortArray array, jboolean* isCopy) { enterJVM(); leaveJVM(); NYI(); }
jint* JNICALL GetIntArrayElements(JNIEnv* env, jintArray array, jboolean* isCopy) { enterJVM(); leaveJVM(); NYI(); }
jlong* JNICALL GetLongArrayElements(JNIEnv* env, jlongArray array, jboolean* isCopy) { enterJVM(); leaveJVM(); NYI(); }
jfloat* JNICALL GetFloatArrayElements(JNIEnv* env, jfloatArray array, jboolean* isCopy) { enterJVM(); leaveJVM(); NYI(); }
jdouble* JNICALL GetDoubleArrayElements(JNIEnv* env, jdoubleArray array, jboolean* isCopy) { enterJVM(); leaveJVM(); NYI(); }

void JNICALL ReleaseBooleanArrayElements(JNIEnv* env, jbooleanArray array, jboolean* elems, jint mode) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL ReleaseByteArrayElements(JNIEnv* env, jbyteArray array, jbyte* elems, jint mode) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL ReleaseCharArrayElements(JNIEnv* env, jcharArray array, jchar* elems, jint mode) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL ReleaseShortArrayElements(JNIEnv* env, jshortArray array, jshort* elems, jint mode) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL ReleaseIntArrayElements(JNIEnv* env, jintArray array, jint* elems, jint mode) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL ReleaseLongArrayElements(JNIEnv* env, jlongArray array, jlong* elems, jint mode) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL ReleaseFloatArrayElements(JNIEnv* env, jfloatArray array, jfloat* elems, jint mode) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL ReleaseDoubleArrayElements(JNIEnv* env, jdoubleArray array, jdouble* elems, jint mode) { enterJVM(); leaveJVM(); NYI(); }

void JNICALL GetBooleanArrayRegion(JNIEnv* env, jbooleanArray array, jsize start, jsize l, jboolean* buf) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL GetByteArrayRegion(JNIEnv* env, jbyteArray array, jsize start, jsize len, jbyte* buf) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL GetCharArrayRegion(JNIEnv* env, jcharArray array, jsize start, jsize len, jchar* buf) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL GetShortArrayRegion(JNIEnv* env, jshortArray array, jsize start, jsize len, jshort* buf) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL GetIntArrayRegion(JNIEnv* env, jintArray array, jsize start, jsize len, jint* buf) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL GetLongArrayRegion(JNIEnv* env, jlongArray array, jsize start, jsize len, jlong* buf) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL GetFloatArrayRegion(JNIEnv* env, jfloatArray array, jsize start, jsize len, jfloat* buf) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL GetDoubleArrayRegion(JNIEnv* env, jdoubleArray array, jsize start, jsize len, jdouble* buf) { enterJVM(); leaveJVM(); NYI(); }

void JNICALL SetBooleanArrayRegion(JNIEnv* env, jbooleanArray array, jsize start, jsize l, const jboolean* buf) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL SetByteArrayRegion(JNIEnv* env, jbyteArray array, jsize start, jsize len, const jbyte* buf) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL SetCharArrayRegion(JNIEnv* env, jcharArray array, jsize start, jsize len, const jchar* buf) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL SetShortArrayRegion(JNIEnv* env, jshortArray array, jsize start, jsize len, const jshort* buf) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL SetIntArrayRegion(JNIEnv* env, jintArray array, jsize start, jsize len, const jint* buf) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL SetLongArrayRegion(JNIEnv* env, jlongArray array, jsize start, jsize len, const jlong* buf) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL SetFloatArrayRegion(JNIEnv* env, jfloatArray array, jsize start, jsize len, const jfloat* buf) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL SetDoubleArrayRegion(JNIEnv* env, jdoubleArray array, jsize start, jsize len, const jdouble* buf) { enterJVM(); leaveJVM(); NYI(); }

jint JNICALL RegisterNatives(JNIEnv* env, jclass clazz, const JNINativeMethod* methods, jint nMethods) {
	enterJVM();
	J3Class* cl = J3Class::nativeClass(clazz)->asClass();
	J3*      j3 = cl->loader()->vm();

 	for(jint i=0; i<nMethods; i++)
 		cl->registerNative(j3->names()->get(methods[i].name), j3->names()->get(methods[i].signature), methods[i].fnPtr);

	leaveJVM();
	return 0;
}

jint JNICALL UnregisterNatives(JNIEnv* env, jclass clazz) { enterJVM(); leaveJVM(); NYI(); }

jint JNICALL MonitorEnter(JNIEnv* env, jobject obj) { enterJVM(); leaveJVM(); NYI(); }
jint JNICALL MonitorExit(JNIEnv* env, jobject obj) { enterJVM(); leaveJVM(); NYI(); }

jint JNICALL GetJavaVM(JNIEnv* env, JavaVM** vm) { enterJVM(); leaveJVM(); NYI(); }

void JNICALL GetStringRegion(JNIEnv* env, jstring str, jsize start, jsize len, jchar* buf) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL GetStringUTFRegion(JNIEnv* env, jstring str, jsize start, jsize len, char* buf) { enterJVM(); leaveJVM(); NYI(); }

void* JNICALL GetPrimitiveArrayCritical(JNIEnv* env, jarray array, jboolean* isCopy) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL ReleasePrimitiveArrayCritical(JNIEnv* env, jarray array, void* carray, jint mode) { enterJVM(); leaveJVM(); NYI(); }

const jchar* JNICALL GetStringCritical(JNIEnv* env, jstring string, jboolean* isCopy) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL ReleaseStringCritical(JNIEnv* env, jstring string, const jchar* cstring) { enterJVM(); leaveJVM(); NYI(); }

jweak JNICALL NewWeakGlobalRef(JNIEnv* env, jobject obj) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL DeleteWeakGlobalRef(JNIEnv* env, jweak ref) { enterJVM(); leaveJVM(); NYI(); }

jboolean JNICALL ExceptionCheck(JNIEnv* env) { 
	return J3Thread::get()->hasPendingException();
}

jobject JNICALL NewDirectByteBuffer(JNIEnv* env, void* address, jlong capacity) { enterJVM(); leaveJVM(); NYI(); }
void* JNICALL GetDirectBufferAddress(JNIEnv* env, jobject buf) { enterJVM(); leaveJVM(); NYI(); }
jlong JNICALL GetDirectBufferCapacity(JNIEnv* env, jobject buf) { enterJVM(); leaveJVM(); NYI(); }

jobjectRefType JNICALL GetObjectRefType(JNIEnv* env, jobject obj) { enterJVM(); leaveJVM(); NYI(); }

struct JNINativeInterface_ jniEnvTable = {
	0,
	0,
	0,
	0,
	GetVersion,
	DefineClass,
	FindClass,
	FromReflectedMethod,
	FromReflectedField,
	ToReflectedMethod,
	GetSuperclass,
	IsAssignableFrom,
	ToReflectedField,
	Throw,
	ThrowNew,
	ExceptionOccurred,
	ExceptionDescribe,
	ExceptionClear,
	FatalError,
	PushLocalFrame,
	PopLocalFrame,
	NewGlobalRef,
	DeleteGlobalRef,
	DeleteLocalRef,
	IsSameObject,
	NewLocalRef,
	EnsureLocalCapacity,
	AllocObject,
	NewObject,
	NewObjectV,
	NewObjectA,
	GetObjectClass,
	IsInstanceOf,
	GetMethodID,
	CallObjectMethod,
	CallObjectMethodV,
	CallObjectMethodA,
	CallBooleanMethod,
	CallBooleanMethodV,
	CallBooleanMethodA,
	CallByteMethod,
	CallByteMethodV,
	CallByteMethodA,
	CallCharMethod,
	CallCharMethodV,
	CallCharMethodA,
	CallShortMethod,
	CallShortMethodV,
	CallShortMethodA,
	CallIntMethod,
	CallIntMethodV,
	CallIntMethodA,
	CallLongMethod,
	CallLongMethodV,
	CallLongMethodA,
	CallFloatMethod,
	CallFloatMethodV,
	CallFloatMethodA,
	CallDoubleMethod,
	CallDoubleMethodV,
	CallDoubleMethodA,
	CallVoidMethod,
	CallVoidMethodV,
	CallVoidMethodA,
	CallNonvirtualObjectMethod,
	CallNonvirtualObjectMethodV,
	CallNonvirtualObjectMethodA,
	CallNonvirtualBooleanMethod,
	CallNonvirtualBooleanMethodV,
	CallNonvirtualBooleanMethodA,
	CallNonvirtualByteMethod,
	CallNonvirtualByteMethodV,
	CallNonvirtualByteMethodA,
	CallNonvirtualCharMethod,
	CallNonvirtualCharMethodV,
	CallNonvirtualCharMethodA,
	CallNonvirtualShortMethod,
	CallNonvirtualShortMethodV,
	CallNonvirtualShortMethodA,
	CallNonvirtualIntMethod,
	CallNonvirtualIntMethodV,
	CallNonvirtualIntMethodA,
	CallNonvirtualLongMethod,
	CallNonvirtualLongMethodV,
	CallNonvirtualLongMethodA,
	CallNonvirtualFloatMethod,
	CallNonvirtualFloatMethodV,
	CallNonvirtualFloatMethodA,
	CallNonvirtualDoubleMethod,
	CallNonvirtualDoubleMethodV,
	CallNonvirtualDoubleMethodA,
	CallNonvirtualVoidMethod,
	CallNonvirtualVoidMethodV,
	CallNonvirtualVoidMethodA,
	GetFieldID,
	GetObjectField,
	GetBooleanField,
	GetByteField,
	GetCharField,
	GetShortField,
	GetIntField,
	GetLongField,
	GetFloatField,
	GetDoubleField,
	SetObjectField,
	SetBooleanField,
	SetByteField,
	SetCharField,
	SetShortField,
	SetIntField,
	SetLongField,
	SetFloatField,
	SetDoubleField,
	GetStaticMethodID,
	CallStaticObjectMethod,
	CallStaticObjectMethodV,
	CallStaticObjectMethodA,
	CallStaticBooleanMethod,
	CallStaticBooleanMethodV,
	CallStaticBooleanMethodA,
	CallStaticByteMethod,
	CallStaticByteMethodV,
	CallStaticByteMethodA,
	CallStaticCharMethod,
	CallStaticCharMethodV,
	CallStaticCharMethodA,
	CallStaticShortMethod,
	CallStaticShortMethodV,
	CallStaticShortMethodA,
	CallStaticIntMethod,
	CallStaticIntMethodV,
	CallStaticIntMethodA,
	CallStaticLongMethod,
	CallStaticLongMethodV,
	CallStaticLongMethodA,
	CallStaticFloatMethod,
	CallStaticFloatMethodV,
	CallStaticFloatMethodA,
	CallStaticDoubleMethod,
	CallStaticDoubleMethodV,
	CallStaticDoubleMethodA,
	CallStaticVoidMethod,
	CallStaticVoidMethodV,
	CallStaticVoidMethodA,
	GetStaticFieldID,
	GetStaticObjectField,
	GetStaticBooleanField,
	GetStaticByteField,
	GetStaticCharField,
	GetStaticShortField,
	GetStaticIntField,
	GetStaticLongField,
	GetStaticFloatField,
	GetStaticDoubleField,
	SetStaticObjectField,
	SetStaticBooleanField,
	SetStaticByteField,
	SetStaticCharField,
	SetStaticShortField,
	SetStaticIntField,
	SetStaticLongField,
	SetStaticFloatField,
	SetStaticDoubleField,
	NewString,
	GetStringLength,
	GetStringChars,
	ReleaseStringChars,
	NewStringUTF,
	GetStringUTFLength,
	GetStringUTFChars,
	ReleaseStringUTFChars,
	GetArrayLength,
	NewObjectArray,
	GetObjectArrayElement,
	SetObjectArrayElement,
	NewBooleanArray,
	NewByteArray,
	NewCharArray,
	NewShortArray,
	NewIntArray,
	NewLongArray,
	NewFloatArray,
	NewDoubleArray,
	GetBooleanArrayElements,
	GetByteArrayElements,
	GetCharArrayElements,
	GetShortArrayElements,
	GetIntArrayElements,
	GetLongArrayElements,
	GetFloatArrayElements,
	GetDoubleArrayElements,
	ReleaseBooleanArrayElements,
	ReleaseByteArrayElements,
	ReleaseCharArrayElements,
	ReleaseShortArrayElements,
	ReleaseIntArrayElements,
	ReleaseLongArrayElements,
	ReleaseFloatArrayElements,
	ReleaseDoubleArrayElements,
	GetBooleanArrayRegion,
	GetByteArrayRegion,
	GetCharArrayRegion,
	GetShortArrayRegion,
	GetIntArrayRegion,
	GetLongArrayRegion,
	GetFloatArrayRegion,
	GetDoubleArrayRegion,
	SetBooleanArrayRegion,
	SetByteArrayRegion,
	SetCharArrayRegion,
	SetShortArrayRegion,
	SetIntArrayRegion,
	SetLongArrayRegion,
	SetFloatArrayRegion,
	SetDoubleArrayRegion,
	RegisterNatives,
	UnregisterNatives,
	MonitorEnter,
	MonitorExit,
	GetJavaVM,
	GetStringRegion,
	GetStringUTFRegion,
	GetPrimitiveArrayCritical,
	ReleasePrimitiveArrayCritical,
	GetStringCritical,
	ReleaseStringCritical,
	NewWeakGlobalRef,
	DeleteWeakGlobalRef,
	ExceptionCheck,
	NewDirectByteBuffer,
	GetDirectBufferAddress,
	GetDirectBufferCapacity,
	GetObjectRefType
};

}
