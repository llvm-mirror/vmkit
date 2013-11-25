#ifndef _JNI_MD_H_
#define _JNI_MD_H_

#define JNI_TYPES_ALREADY_DEFINED_IN_JNI_MD_H

#include <stdint.h>

namespace j3 {
	class J3ObjectHandle;
	class J3Value;
	class J3ObjectType;
	class J3Field;
	class J3Method;
}

extern "C" {

#ifndef __has_attribute
#define __has_attribute(x) 0
#endif
#if (defined(__GNUC__) && ((__GNUC__ > 4) || (__GNUC__ == 4) && (__GNUC_MINOR__ > 2))) || __has_attribute(visibility)
#define JNIEXPORT     __attribute__((visibility("default")))
#define JNIIMPORT     __attribute__((visibility("default")))
#else
  #define JNIEXPORT
  #define JNIIMPORT
#endif

#define JNICALL

	typedef bool     jboolean;
	typedef int8_t   jbyte;
	typedef uint16_t jchar;
	typedef int16_t  jshort;
	typedef int32_t  jint;
	typedef int64_t  jlong;
	typedef float    jfloat;
	typedef double   jdouble;

	typedef uint32_t jsize;

	typedef j3::J3ObjectHandle* jobject;
	typedef jobject             jclass;
	typedef jobject             jthrowable;
	typedef jobject             jstring;
	typedef jobject             jarray;
	typedef jarray              jbooleanArray;
	typedef jarray              jbyteArray;
	typedef jarray              jcharArray;
	typedef jarray              jshortArray;
	typedef jarray              jintArray;
	typedef jarray              jlongArray;
	typedef jarray              jfloatArray;
	typedef jarray              jdoubleArray;
	typedef jarray              jobjectArray;

	typedef jobject             jweak;
	typedef j3::J3Value         jvalue;

	typedef j3::J3Field*        jfieldID;
	typedef j3::J3Method*       jmethodID;

	/* Return values from jobjectRefType */
	typedef enum _jobjectType {
		JNIInvalidRefType    = 0,
		JNILocalRefType      = 1,
		JNIGlobalRefType     = 2,
		JNIWeakGlobalRefType = 3
	} jobjectRefType;
}

#endif
