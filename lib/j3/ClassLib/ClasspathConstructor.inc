//===- ClasspathConstructor.cpp -------------------------------------------===//
//===----------- GNU classpath java/lang/reflect/Constructor --------------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "types.h"

#include "Classpath.h"
#include "ClasspathReflect.h"
#include "JavaArray.h"
#include "JavaClass.h"
#include "JavaObject.h"
#include "JavaThread.h"
#include "JavaTypes.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"
#include "JnjvmClassLoader.h"

using namespace j3;

extern "C" {

bool getClassAndMethodPointerByVMConstructor(JavaObjectVMConstructor* vmCons, UserCommonClass** cls, JavaMethod** m)
  __attribute__ ((noinline));

bool getClassAndMethodPointerByVMConstructor(JavaObjectVMConstructor* vmCons, UserCommonClass** cls, JavaMethod** m) {
	JavaObject* res = 0;
	// Local object references
	llvm_gcroot(vmCons, 0);
	llvm_gcroot(res, 0);

	BEGIN_NATIVE_EXCEPTION(0)
		*cls = (UserCommonClass*)JavaObjectVMConstructor::getClass(vmCons);
		*m = JavaObjectVMConstructor::getInternalMethod(vmCons);
	END_NATIVE_EXCEPTION
	return true;
}

JNIEXPORT jobject JNICALL Java_java_lang_reflect_VMConstructor_getParameterTypes(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
JavaObjectVMConstructor* cons) {

	JavaObject* res = 0;
	UserCommonClass* cls = 0;
	JavaMethod* method = 0;
	// Local object references
	llvm_gcroot(cons, 0);
	llvm_gcroot(res, 0);

	BEGIN_NATIVE_EXCEPTION(0)
    // Store local reference

	if (getClassAndMethodPointerByVMConstructor(cons, &cls,&method)) {
		res = method->getParameterTypes(cls->classLoader);
	} // FIXME (INTI) : Throw an exception here

    END_NATIVE_EXCEPTION

    return (jobject)res;
}

JNIEXPORT jint JNICALL Java_java_lang_reflect_VMConstructor_getModifiersInternal(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
JavaObjectVMConstructor* cons) {

	jint res = 0;

	UserCommonClass* cls = 0;
	JavaMethod* method = 0;
	// Local object references
	llvm_gcroot(cons, 0);

	BEGIN_NATIVE_EXCEPTION(0)
	// Store local reference

	if (getClassAndMethodPointerByVMConstructor(cons, &cls,&method)) {
		res = method->access;
	} // FIXME (INTI) : Throw an exception here

	END_NATIVE_EXCEPTION

	return res;
}

JavaObject* proceedRealConstructor(ArrayObject* args,
										UserCommonClass* clazz, JavaMethod* method)
  __attribute__ ((noinline));

JavaObject* proceedRealConstructor(ArrayObject* args,
                               UserCommonClass* clazz, JavaMethod* method) {
  JavaObject* res = 0;
  JavaObject* excp = 0;

  llvm_gcroot(args, 0);
  llvm_gcroot(res, 0);
  llvm_gcroot(excp, 0);

  Jnjvm* vm = JavaThread::get()->getJVM();
  sint32 nbArgs = args ? ArrayObject::getSize(args) : 0;
  Signdef* sign = method->getSignature();
  sint32 size = sign->nbArguments;

  if (isAbstract(clazz->access)) vm->instantiationException(clazz);

  vmkit::ThreadAllocator allocator;
  // Allocate a buffer to store the arguments.
  jvalue* buf = size ?
      (jvalue*)allocator.Allocate(size * sizeof(jvalue)) : NULL;

  if (nbArgs == size) {
    UserClass* cl = clazz->asClass();
    if (cl) {
      cl->initialiseClass(vm);
      res = cl->doNew(vm);
      JavaObject** ptr = (JavaObject**)ArrayObject::getElements(args);

      Typedef* const* arguments = sign->getArgumentsType();
      // Store the arguments, unboxing primitives if necessary.
      for (sint32 i = 0; i < size; ++i) {
        JavaObject::decapsulePrimitive(ptr[i], vm, &buf[i], arguments[i]);
        if (!arguments[i]->isPrimitive()) {
          buf[i].l = reinterpret_cast<jobject>(&ptr[i]);
        }
      }

      JavaThread* th = JavaThread::get();
      TRY {
        method->invokeIntSpecialBuf(vm, cl, res, buf);
      } CATCH {
        excp = th->getJavaException();
      } END_CATCH;
      if (excp) {
        if (JavaObject::getClass(excp)->isSubclassOf(vm->upcalls->newException)) {
          th->clearException();
          // If it's an exception, we encapsule it in an
          // invocationTargetException
          vm->invocationTargetException(excp);
        } else {
          // If it's an error, throw it again.
          th->throwPendingException();
        }
        return NULL;
      }
    } else {
      vm->illegalArgumentException("class is not a regular class");
      return NULL;
    }
  } else {
    vm->illegalArgumentException("wrong number of arguments");
    return NULL;
  }

  return res;
}

JNIEXPORT JavaObject* JNICALL Java_java_lang_reflect_VMConstructor_construct(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
JavaObjectVMConstructor* cons, ArrayObject* args) {

  JavaObject* res = 0;
  JavaMethod* m = 0;
  UserCommonClass* cls = 0;

  llvm_gcroot(res, 0);
  llvm_gcroot(cons, 0);
  llvm_gcroot(args, 0);

  BEGIN_NATIVE_EXCEPTION(0)
  if (getClassAndMethodPointerByVMConstructor(cons,&cls, &m)){
	  res = proceedRealConstructor(args, cls, m);
  } // FIXME (INTI) : Throw an exception here
  END_NATIVE_EXCEPTION
  return res;
}

JNIEXPORT
ArrayObject* JNICALL Java_java_lang_reflect_VMConstructor_getExceptionTypes(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
JavaObjectVMConstructor* cons) {

  ArrayObject* res = 0;
  JavaMethod* m = 0;
  UserCommonClass* cls = 0;
  llvm_gcroot(res, 0);
  llvm_gcroot(cons, 0);

  BEGIN_NATIVE_EXCEPTION(0)

    verifyNull(cons);
    if (getClassAndMethodPointerByVMConstructor(cons, &cls, & m)) {
    	JnjvmClassLoader* loader = cls->classLoader;
    	res = (ArrayObject*)m->getExceptionTypes(loader);
    } // FIXME (INTI) : Throw an exception here

  END_NATIVE_EXCEPTION


  return res;
}

JNIEXPORT JavaObject* JNICALL Java_java_lang_reflect_VMConstructor_getSignature(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
JavaObjectVMConstructor* Meth) {

  JavaObject* result = 0;
  JavaMethod* m = 0;
  UserCommonClass* cls = 0;
  llvm_gcroot(result, 0);
  llvm_gcroot(Meth, 0);

  BEGIN_NATIVE_EXCEPTION(0)

    verifyNull(Meth);
    if (getClassAndMethodPointerByVMConstructor(Meth, &cls, & m)) {
		Jnjvm* vm = JavaThread::get()->getJVM();
		result = vm->internalUTF8ToStr(m->type);
    } // FIXME (INTI) : Throw an exception here

    END_NATIVE_EXCEPTION

  return result;
}


}
