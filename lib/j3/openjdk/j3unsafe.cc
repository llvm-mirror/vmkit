#include "j3/j3jni.h"
#include "j3/j3object.h"
#include "j3/j3thread.h"
#include "j3/j3class.h"
#include "j3/j3classloader.h"
#include "j3/j3.h"

#include "llvm/IR/DataLayout.h"

using namespace j3;

extern "C" {
	JNIEXPORT void JNICALL Java_sun_misc_Unsafe_registerNatives(JNIEnv* env, jclass cls) {
		// Nothing, we define the Unsafe methods with the expected signatures.
	}

	/// arrayBaseOffset - Offset from the array object where the actual
	/// element data begins.
	///
	JNIEXPORT jlong JNICALL Java_sun_misc_Unsafe_arrayBaseOffset(JNIEnv* env, jobject unsafe, jclass clazz) {
		return sizeof(J3ArrayObject);
	}

	/// arrayIndexScale - Indexing scale for the element type in
	/// the specified array.  For use with arrayBaseOffset,
	/// NthElementPtr = ArrayObject + BaseOffset + N*IndexScale
	/// Return '0' if our JVM stores the elements in a way that
	/// makes this type of access impossible or unsupported.
	///
	JNIEXPORT jlong JNICALL Java_sun_misc_Unsafe_arrayIndexScale(JNIEnv* env, jobject unsafe, jclass clazz) {
		return J3ObjectType::nativeClass(clazz)->asArrayClass()->component()->getLogSize();
	}

	JNIEXPORT jint JNICALL Java_sun_misc_Unsafe_addressSize(JNIEnv* env, jobject unsafe) {
		return J3Thread::get()->vm()->objectClass->getSizeInBits()>>3;
	}
}
