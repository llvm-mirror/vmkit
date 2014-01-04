#include "j3/j3jni.h"
#include "j3/j3.h"
#include "j3/j3class.h"
#include "j3/j3classloader.h"
#include "j3/j3object.h"
#include "j3/j3method.h"
#include "j3/j3thread.h"
#include "j3/j3utf16.h"
#include <stdlib.h>

#define enterJVM() try {
#define leaveJVM() } catch(void* e) { J3Thread::get()->setPendingException(J3Thread::get()->push((J3Object*)e)); }

#define NYI() { J3Thread::get()->vm()->internalError("not yet implemented: '%s'", __PRETTY_FUNCTION__); }

namespace j3 {

jint JNICALL GetVersion(JNIEnv* env) { enterJVM(); leaveJVM(); NYI(); }

jclass JNICALL DefineClass(JNIEnv* env, const char* name, jobject loader, const jbyte* buf, jsize len) { enterJVM(); leaveJVM(); NYI(); }
jclass JNICALL FindClass(JNIEnv* env, const char* name) { 
	jclass res;

	enterJVM();
	J3Method* m = J3Thread::get()->getJavaCaller();
	J3* vm = J3Thread::get()->vm();
	J3ClassLoader* loader = m ? m->cl()->loader() : vm->initialClassLoader;
	J3Class* cl = loader->loadClass(vm->names()->get(name));
	cl->initialise();
	res = cl->javaClass();
	leaveJVM(); 

	return res;
}

jmethodID JNICALL FromReflectedMethod(JNIEnv* env, jobject method) { enterJVM(); leaveJVM(); NYI(); }
jfieldID JNICALL FromReflectedField(JNIEnv* env, jobject field) { enterJVM(); leaveJVM(); NYI(); }

jobject JNICALL ToReflectedMethod(JNIEnv* env, jclass cls, jmethodID methodID, jboolean isStatic) { enterJVM(); leaveJVM(); NYI(); }

jclass JNICALL GetSuperclass(JNIEnv* env, jclass sub) { 
	jclass res;
	enterJVM();
	J3ObjectType* cl = J3ObjectType::nativeClass(sub);
	res = J3Thread::get()->vm()->objectClass ? 0 : cl->javaClass();
	leaveJVM(); 
	return res;
}

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

jobject JNICALL NewGlobalRef(JNIEnv* env, jobject lobj) { 
	jobject res;
	enterJVM(); 
	J3Method* m = J3Thread::get()->getJavaCaller();
	J3ClassLoader* loader = m ? m->cl()->loader() : J3Thread::get()->vm()->initialClassLoader;
	res = loader->globalReferences()->add(lobj);
	leaveJVM(); 
	return res;
}

void JNICALL DeleteGlobalRef(JNIEnv* env, jobject gref) { 
	enterJVM(); 
	J3Method* m = J3Thread::get()->getJavaCaller();
	J3ClassLoader* loader = m ? m->cl()->loader() : J3Thread::get()->vm()->initialClassLoader;
	loader->globalReferences()->del(gref);
	leaveJVM(); 
}

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
jobject JNICALL NewObject(JNIEnv* env, jclass clazz, jmethodID methodID, ...) { 
	jobject res;
	enterJVM();
	va_list va;
	va_start(va, methodID);
	res = env->NewObjectV(clazz, methodID, va);
	va_end(va);
	leaveJVM(); 
	return res;
}

jobject JNICALL NewObjectV(JNIEnv* env, jclass clazz, jmethodID methodID, va_list args) { 
	jobject res;
	enterJVM(); 
	res = J3ObjectHandle::doNewObject(methodID->cl());
	methodID->invokeSpecial(res, args);
	leaveJVM(); 
	return res;
}

jobject JNICALL NewObjectA(JNIEnv* env, jclass clazz, jmethodID methodID, const jvalue* args) { 
	jobject res;
	enterJVM(); 
	res = J3ObjectHandle::doNewObject(methodID->cl());
	methodID->invokeSpecial(res, args);
	leaveJVM(); 
	return res;
}

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
	vmkit::Names* n = J3Thread::get()->vm()->names();
	res = cl->findMethod(0, n->get(name), cl->loader()->getSignature(cl, n->get(sig)), 0);
	leaveJVM(); 

	return res;
}

jmethodID JNICALL GetStaticMethodID(JNIEnv* env, jclass clazz, const char* name, const char* sig) { 
	jmethodID res;

	enterJVM(); 
	
	J3ObjectType* cl = J3ObjectType::nativeClass(clazz);
	cl->initialise();
	vmkit::Names* n = J3Thread::get()->vm()->names();
	res = cl->findMethod(J3Cst::ACC_STATIC, n->get(name), cl->loader()->getSignature(cl, n->get(sig)), 0);

	leaveJVM(); 

	return res;
}

#define defCall(jtype, id, j3type)																			\
	jtype JNICALL Call##id##Method(JNIEnv* env, jobject obj, jmethodID methodID, ...) { \
		va_list va;																													\
		va_start(va, methodID);																							\
		jtype res = env->Call##id##MethodV(obj, methodID, va);							\
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
	}

#define defStaticCall(jtype, id, j3type)																\
	jtype JNICALL CallStatic##id##Method(JNIEnv* env, jclass clazz, jmethodID methodID, ...) { \
		va_list va;																													\
		va_start(va, methodID);																							\
		jtype res = env->CallStatic##id##MethodV(clazz, methodID, va);			\
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

#define defNonVirtualCall(jtype, id, j3type)														\
	jtype JNICALL CallNonvirtual##id##Method(JNIEnv* env, jobject obj, jclass clazz, jmethodID methodID, ...) { \
		va_list va;																													\
		va_start(va, methodID);																							\
		jtype res = env->CallNonvirtual##id##MethodV(obj, clazz, methodID, va); \
		va_end(va);																													\
		return res;																													\
	}																																			\
																																				\
	jtype JNICALL CallNonvirtual##id##MethodV(JNIEnv* env, jobject obj, jclass clazz, jmethodID methodID, va_list args) { \
		jvalue res;																													\
																																				\
		enterJVM();																													\
		res = methodID->invokeSpecial(obj, args);														\
		leaveJVM();																													\
																																				\
		return res.val##j3type;																							\
	}																																			\
																																				\
	jtype JNICALL CallNonvirtual##id##MethodA(JNIEnv* env, jobject obj, jclass clazz, jmethodID methodID, const jvalue* args) { \
		jvalue res;																													\
																																				\
		enterJVM();																													\
		res = methodID->invokeSpecial(obj, args);														\
		leaveJVM();																													\
																																				\
		return res.val##j3type;																							\
	}

#define defSetField(jtype, id, j3type)																	\
	void JNICALL SetStatic##id##Field(JNIEnv* env, jclass clazz, jfieldID fieldID, jtype value) { \
		enterJVM();																													\
		J3ObjectType::nativeClass(clazz)->asClass()->staticInstance()->set##j3type(fieldID, value); \
		leaveJVM();																													\
	}

#define defNewArray(jtype, id, j3type)																	\
	jtype##Array JNICALL New##id##Array(JNIEnv* env, jsize len) {					\
		jtype##Array res;																										\
		enterJVM();																													\
		res = J3ObjectHandle::doNewArray(J3Thread::get()->vm()->type##j3type->getArray(), len); \
		leaveJVM();																													\
		return res;																													\
	}

#define defArrayRegion(jtype, id, j3type)																\
	void JNICALL Set##id##ArrayRegion(JNIEnv* env, jtype##Array array, jsize start, jsize len, const jtype* buf) { \
		enterJVM();																													\
		array->setRegion##j3type(0, buf, start, len);												\
		leaveJVM();																													\
	}

#define defJNIObj(jtype, id, j3type)						\
	defCall(jtype, id, j3type)										\
	defNonVirtualCall(jtype, id, j3type)					\
	defStaticCall(jtype, id, j3type)							\
	defSetField(jtype, id, j3type)

#define defJNI(jtype, id, j3type)								\
	defJNIObj(jtype, id, j3type)									\
	defNewArray(jtype, id, j3type)								\
	defArrayRegion(jtype, id, j3type)							

	defJNIObj(jobject,  Object,  Object);
	defJNI   (jboolean, Boolean, Boolean);
	defJNI   (jbyte,    Byte,    Byte);
	defJNI   (jchar,    Char,    Char);
	defJNI   (jshort,   Short,   Short);
	defJNI   (jint,     Int,     Integer);
	defJNI   (jlong,    Long,    Long);
	defJNI   (jfloat,   Float,   Float);
	defJNI   (jdouble,  Double,  Double);

void JNICALL CallVoidMethod(JNIEnv* env, jobject obj, jmethodID methodID, ...) { 
	va_list va;
	va_start(va, methodID);
	env->CallVoidMethodV(obj, methodID, va);
	va_end(va);
}

void JNICALL CallVoidMethodV(JNIEnv* env, jobject obj, jmethodID methodID, va_list args) { 
	enterJVM(); 
	methodID->invokeVirtual(obj, args);
	leaveJVM(); 
}

void JNICALL CallVoidMethodA(JNIEnv* env, jobject obj, jmethodID methodID, const jvalue* args) { 
	enterJVM(); 
	methodID->invokeVirtual(obj, args);
	leaveJVM();
}

void JNICALL CallNonvirtualVoidMethod(JNIEnv* env, jobject obj, jclass clazz, jmethodID methodID, ...) { 
	va_list va;
	va_start(va, methodID);
	env->CallNonvirtualVoidMethodV(obj, clazz, methodID, va);
	va_end(va);
}

void JNICALL CallNonvirtualVoidMethodV(JNIEnv* env, jobject obj, jclass clazz, jmethodID methodID, va_list args) { 
	enterJVM(); 
	methodID->invokeSpecial(obj, args);
	leaveJVM(); 
}

void JNICALL CallNonvirtualVoidMethodA(JNIEnv* env, jobject obj, jclass clazz, jmethodID methodID, const jvalue* args) { 
	enterJVM(); 
	methodID->invokeSpecial(obj, args);
	leaveJVM(); 
}

void JNICALL CallStaticVoidMethod(JNIEnv* env, jclass cls, jmethodID methodID, ...) { 
	va_list va;
	va_start(va, methodID);
	env->CallStaticVoidMethodV(cls, methodID, va);
	va_end(va);
}

void JNICALL CallStaticVoidMethodV(JNIEnv* env, jclass cls, jmethodID methodID, va_list args) { 
	enterJVM(); 
	methodID->invokeStatic(args);
	leaveJVM(); 
}

void JNICALL CallStaticVoidMethodA(JNIEnv* env, jclass cls, jmethodID methodID, const jvalue* args) { 
	enterJVM(); 
	methodID->invokeStatic(args);
	leaveJVM(); 
}

jfieldID JNICALL GetFieldID(JNIEnv* env, jclass clazz, const char* name, const char* sig) { 
	jfieldID res;
	enterJVM(); 
	J3* vm = J3Thread::get()->vm();
	J3Class* cl = J3ObjectType::nativeClass(clazz)->asClass();
	res = cl->findField(0, vm->names()->get(name), cl->loader()->getType(cl, vm->names()->get(sig)), 0);
	leaveJVM(); 
	return res;
}

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

jfieldID JNICALL GetStaticFieldID(JNIEnv* env, jclass clazz, const char* name, const char* sig) { 
	jfieldID res;
	enterJVM(); 
	J3* vm = J3Thread::get()->vm();
	J3Class* cl = J3ObjectType::nativeClass(clazz)->asClass();
	res = cl->findField(J3Cst::ACC_STATIC, vm->names()->get(name), cl->loader()->getType(cl, vm->names()->get(sig)), 0);
	leaveJVM(); 
	return res;
}

jobject JNICALL GetStaticObjectField(JNIEnv* env, jclass clazz, jfieldID fieldID) { enterJVM(); leaveJVM(); NYI(); }
jboolean JNICALL GetStaticBooleanField(JNIEnv* env, jclass clazz, jfieldID fieldID) { enterJVM(); leaveJVM(); NYI(); }
jbyte JNICALL GetStaticByteField(JNIEnv* env, jclass clazz, jfieldID fieldID) { enterJVM(); leaveJVM(); NYI(); }
jchar JNICALL GetStaticCharField(JNIEnv* env, jclass clazz, jfieldID fieldID) { enterJVM(); leaveJVM(); NYI(); }
jshort JNICALL GetStaticShortField(JNIEnv* env, jclass clazz, jfieldID fieldID) { enterJVM(); leaveJVM(); NYI(); }
jint JNICALL GetStaticIntField(JNIEnv* env, jclass clazz, jfieldID fieldID) { enterJVM(); leaveJVM(); NYI(); }
jlong JNICALL GetStaticLongField(JNIEnv* env, jclass clazz, jfieldID fieldID) { enterJVM(); leaveJVM(); NYI(); }
jfloat JNICALL GetStaticFloatField(JNIEnv* env, jclass clazz, jfieldID fieldID) { enterJVM(); leaveJVM(); NYI(); }
jdouble JNICALL GetStaticDoubleField(JNIEnv* env, jclass clazz, jfieldID fieldID) { enterJVM(); leaveJVM(); NYI(); }

jstring JNICALL NewString(JNIEnv* env, const jchar* unicode, jsize len) { 
	jstring res;
	enterJVM(); 
	J3* vm = J3Thread::get()->vm();
	J3ObjectHandle* content = J3ObjectHandle::doNewArray(vm->typeChar->getArray(), len);
	content->setRegionChar(0, unicode, 0, len);
	res = J3ObjectHandle::doNewObject(vm->stringClass);
	vm->stringClassInit->invokeSpecial(res, content, 0);
	leaveJVM(); 
	return res;
}

jsize JNICALL GetStringLength(JNIEnv* env, jstring str) { 
	jsize res;
	enterJVM(); 
	res = str->getObject(J3Thread::get()->vm()->stringClassValue)->arrayLength();
	leaveJVM(); 
	return res;
}

const jchar* JNICALL GetStringChars(JNIEnv* env, jstring str, jboolean* isCopy) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL ReleaseStringChars(JNIEnv* env, jstring str, const jchar* chars) { enterJVM(); leaveJVM(); NYI(); }

jstring JNICALL NewStringUTF(JNIEnv* env, const char* utf) { 
	jstring res;

	enterJVM(); 
	res = J3Thread::get()->vm()->utfToString(utf);
	leaveJVM(); 

	return res;
}

jsize JNICALL GetStringUTFLength(JNIEnv* env, jstring str) { 
	jsize res;
	enterJVM(); 
	jobject content = str->getObject(J3Thread::get()->vm()->stringClassValue);
	char buf[J3Utf16Decoder::maxSize(content)];
	res = J3Utf16Decoder::decode(content, buf);
	leaveJVM(); 
	return res;
}

const char* JNICALL GetStringUTFChars(JNIEnv* env, jstring str, jboolean* isCopy) { 
	char* res;

	enterJVM(); 
	J3* vm = J3Thread::get()->vm();
	jobject content = str->getObject(vm->stringClassValue);
	res = new char[J3Utf16Decoder::maxSize(content)];
	J3Utf16Decoder::decode(content, res);

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


jsize JNICALL GetArrayLength(JNIEnv* env, jarray array) { 
	jsize res;
	enterJVM();
	res = array->arrayLength();
	leaveJVM(); 
	return res;
}

jobjectArray JNICALL NewObjectArray(JNIEnv* env, jsize len, jclass clazz, jobject init) { enterJVM(); leaveJVM(); NYI(); }
jobject JNICALL GetObjectArrayElement(JNIEnv* env, jobjectArray array, jsize index) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL SetObjectArrayElement(JNIEnv* env, jobjectArray array, jsize index, jobject val) { enterJVM(); leaveJVM(); NYI(); }

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

jint JNICALL RegisterNatives(JNIEnv* env, jclass clazz, const JNINativeMethod* methods, jint nMethods) {
	enterJVM();
	J3Class* cl = J3Class::nativeClass(clazz)->asClass();
	J3*      j3 = J3Thread::get()->vm();

 	for(jint i=0; i<nMethods; i++)
 		cl->registerNative(j3->names()->get(methods[i].name), j3->names()->get(methods[i].signature), methods[i].fnPtr);

	leaveJVM();
	return 0;
}

jint JNICALL UnregisterNatives(JNIEnv* env, jclass clazz) { enterJVM(); leaveJVM(); NYI(); }

jint JNICALL MonitorEnter(JNIEnv* env, jobject obj) { enterJVM(); leaveJVM(); NYI(); }
jint JNICALL MonitorExit(JNIEnv* env, jobject obj) { enterJVM(); leaveJVM(); NYI(); }

jint JNICALL GetJavaVM(JNIEnv* env, JavaVM** vm) { enterJVM(); leaveJVM(); NYI(); }

void JNICALL GetStringRegion(JNIEnv* env, jstring str, jsize start, jsize len, jchar* buf) { 
	enterJVM(); 
	str->getObject(J3Thread::get()->vm()->stringClassValue)->getRegionChar(start, buf, 0, len);
	leaveJVM(); 
}

void JNICALL GetStringUTFRegion(JNIEnv* env, jstring str, jsize start, jsize len, char* buf) { 
	enterJVM(); 
	J3* vm = J3Thread::get()->vm();
	jobject content = str->getObject(vm->stringClassValue);
	J3Utf16Decoder::decode(content, buf);
	leaveJVM(); 
}

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
