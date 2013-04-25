//===------ ClasspathVMObject.cpp - GNU classpath java/lang/VMObject ------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "types.h"

#include "Classpath.h"
#include "Jnjvm.h"

#include <sstream>

using namespace j3;

extern "C" {

JNIEXPORT JavaObject* JNICALL Java_java_lang_VMObject_clone(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz, 
#endif
JavaObject* src) {
  
  JavaObject* res = NULL;
  llvm_gcroot(res, 0);
  llvm_gcroot(src, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  res = JavaObject::clone(src);

  END_NATIVE_EXCEPTION

  return res;
} 

JNIEXPORT JavaObject* JNICALL Java_java_lang_VMObject_getClass(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
JavaObject* obj) {
  
  JavaObject* res = 0;
  llvm_gcroot(res, 0);
  llvm_gcroot(obj, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  res = JavaObject::getClass(obj)->getClassDelegatee(vm);

  END_NATIVE_EXCEPTION

  return res;
}

void printStackTrace(JavaObject *obj, const char *methName, int skip = 3, int level_deep = 1)
{
	j3::JavaThread *thread = j3::JavaThread::get();

	std::ostringstream log;
	log << '[' << *thread << "] " << methName << " obj=" << obj << " class=" << *JavaObject::getClass(obj)->name;

	const UTF8* clName = JavaObject::getClass(obj)->name;
	if (!clName->compare("java/lang/VMThread") || !clName->compare("java/lang/Thread"))
		log << '(' << *static_cast<JavaObjectVMThread*>(obj) << ')';

	log << std::endl;
	std::cerr << log.str();

	j3::Jnjvm *vm = thread->getJVM();
	vmkit::StackWalker Walker(thread);
	for (vmkit::FrameInfo* FI = NULL; (level_deep > 0) && ((FI = Walker.get()) != NULL); ++Walker) {
		if (!FI->Metadata) continue;
		if (skip > 0) {--skip; continue;}

		vm->printMethod(FI, Walker.getReturnAddress(), Walker.getCallFrame());
		--level_deep;
	}
}

JNIEXPORT void JNICALL Java_java_lang_VMObject_notifyAll(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
JavaObject* obj) {

  llvm_gcroot(obj, 0);

  BEGIN_NATIVE_EXCEPTION(0)

//  printStackTrace(obj, "notifyAll", 3, 0);
  JavaObject::notifyAll(obj);

  END_NATIVE_EXCEPTION
}


JNIEXPORT void JNICALL Java_java_lang_VMObject_wait(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
JavaObject* obj, jlong ms, jint ns) {

  llvm_gcroot(obj, 0);

  BEGIN_NATIVE_EXCEPTION(0)
/*
  printStackTrace(obj, "wait", 3, 1);
  bool notified = JavaObject::wait(obj, ms, ns);
  if (notified)
    printStackTrace(obj, "wait_notified", 3, 1);
*/
  JavaObject::wait(obj, ms, ns);
  END_NATIVE_EXCEPTION
}

JNIEXPORT void JNICALL Java_java_lang_VMObject_notify(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
JavaObject* obj) {

  llvm_gcroot(obj, 0);

  BEGIN_NATIVE_EXCEPTION(0)

//  printStackTrace(obj, "notify", 3, 0);
  JavaObject::notify(obj);

  END_NATIVE_EXCEPTION
}

}
