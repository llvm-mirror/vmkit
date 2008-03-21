//===----- ClasspathVMSystemProperties.h - Classpath methods --------------===//
//
//                          JnJVM
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef _GNU_CLASSPATH_VMSYSTEMPROPERTIES_H
#define _GNU_CLASSPATH_VMSYSTEMPROPERTIES_H

extern "C" {
/*
 * Class:     gnu/classpath/VMSystemProperties
 * Method:    preInit
 * Signature: (Ljava/util/Properties;)V
 */
JNIEXPORT void JNICALL Java_gnu_classpath_VMSystemProperties_preInit(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject par1);

}

#endif

