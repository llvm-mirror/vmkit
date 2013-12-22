#include "j3/j3jni.h"
#include "j3/j3object.h"

using namespace j3;

extern "C" {
	JNIEXPORT void JNICALL Java_sun_misc_Unsafe_registerNatives(J3Object*) {
		// Nothing, we define the Unsafe methods with the expected signatures.
	}
}
