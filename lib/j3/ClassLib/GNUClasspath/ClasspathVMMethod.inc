//===- ClasspathReflect.cpp - Internal representation of core system classes -//
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
#include "JavaTypes.h"
#include "JavaThread.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"
#include "JnjvmClassLoader.h"

using namespace j3;

extern "C" {

JNIEXPORT JavaObject* JNICALL Java_java_lang_reflect_VMMethod_getDefaultValue(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
JavaObjectVMMethod* VMMeth) {
  JavaObject* res = 0;
  llvm_gcroot(VMMeth, 0);
  llvm_gcroot(res, 0);

  BEGIN_NATIVE_EXCEPTION(0)
  dprintf("*************VMMethod_getDefaultValue*************\n");
  Classpath* upcalls = JavaThread::get()->getJVM()->upcalls;
  UserClass* cl = JavaObjectVMMethod::getClass(VMMeth);
  dprintf("JavaClass:%s\n", UTF8Buffer(cl->name).cString());

  // Annotation interface class.
  const UTF8* name = upcalls->newAnnotation->name;

  if (cl->inheritName(name->elements, name->size)) {
    // Find "default attribute"
    JavaMethod* method = JavaObjectVMMethod::getInternalMethod(VMMeth);
    JavaAttribute* annotationsAtt =
      method->lookupAttribute(JavaAttribute::annotationDefaultAttribute);
    dprintf("JavaMethod:%s\n", UTF8Buffer(method->name).cString());

    if (annotationsAtt) {
      Reader reader(annotationsAtt, cl->bytes);
      AnnotationReader AR(reader, cl);

      res = AR.createElementValue();
    }
  }

  END_NATIVE_EXCEPTION

  return res;
}

JNIEXPORT JavaObject* JNICALL Java_java_lang_reflect_VMMethod_getAnnotation(
#ifdef NATIVE_JNI
JNIEnv *env, 
#endif
JavaObjectVMMethod* VMMeth, JavaObjectClass* annotationClass) {
  JavaObject* res = 0;
  JavaObject* newHashMap = 0;
  llvm_gcroot(res, 0);
  llvm_gcroot(newHashMap, 0);
  llvm_gcroot(VMMeth, 0);
  llvm_gcroot(annotationClass, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  Classpath* upcalls = vm->upcalls;

  dprintf("*************VMMethod_getAnnotation*************\n");
  if (annotationClass) {
    /* Retrieving annotationClass classname */
    UserClass* clazz = JavaObjectClass::getClass(annotationClass)->asClass();
    dprintf("searched annotation:%s\n\n", UTF8Buffer(clazz->name).cString());

    /* Retrieving VMMethod.class attributes list */
    UserClass* compilingClass = JavaObjectVMMethod::getClass(VMMeth);
    JavaMethod* method = JavaObjectVMMethod::getInternalMethod(VMMeth);
    JavaAttribute* annotationsAtt =
      method->lookupAttribute(JavaAttribute::annotationsAttribute);
    dprintf("JavaMethod:%s\n", UTF8Buffer(method->name).cString());

    if (annotationsAtt) {
      Reader reader(annotationsAtt, compilingClass->bytes);
      AnnotationReader AR(reader, compilingClass);
      uint16 numAnnotations = reader.readU2();

      for (uint16 i = 0; i < numAnnotations; ++i) {
	uint16 typeIndex = reader.readU2();
	const UTF8* annoType = compilingClass->ctpInfo->UTF8At(typeIndex);

	// Remove the L and ; in the name annotation type (L.....;)
	// for the comparison.
	if (clazz->name->equals(annoType->elements+1, annoType->size-2)) {
	  newHashMap = AR.createAnnotationMapValues();
	  break;

	} else {
	  AR.readAnnotationElementValues();
	}
      } // end for

      if (newHashMap) {
	dprintf("Annotation creation\n");
	res = upcalls->createAnnotation->invokeJavaObjectStatic(vm, upcalls->newAnnotationHandler, &annotationClass, &newHashMap);
      }

    }
  } else {
    vm->nullPointerException();
  }

  END_NATIVE_EXCEPTION

  return res;
}

JNIEXPORT JavaObject* JNICALL Java_java_lang_reflect_VMMethod_getDeclaredAnnotations(
#ifdef NATIVE_JNI
JNIEnv *env, 
#endif
JavaObjectVMMethod* VMMethod) { 
  JavaObject* res = 0;
  JavaObject* annon = 0;
  JavaObject* newHashMap = 0;
  JavaObject* annotationClass = 0;
  llvm_gcroot(res, 0);
  llvm_gcroot(annon, 0);
  llvm_gcroot(newHashMap, 0);
  llvm_gcroot(VMMethod, 0);
  llvm_gcroot(annotationClass, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  Classpath* upcalls = vm->upcalls;

  /* Retrieving VMMethod.class attributes list */
  UserClass* compilingClass = JavaObjectVMMethod::getClass(VMMethod);
  JavaMethod* method = JavaObjectVMMethod::getInternalMethod(VMMethod);
  JavaAttribute* annotationsAtt =
    method->lookupAttribute(JavaAttribute::annotationsAttribute);
  dprintf("JavaMethod : %s\n", UTF8Buffer(method->name).cString());
  
	JnjvmClassLoader* loader = compilingClass->classLoader;

  if (annotationsAtt) {
    Reader reader(annotationsAtt, compilingClass->bytes);
    AnnotationReader AR(reader, compilingClass);
    uint16 numAnnotations = reader.readU2();
    
    UserClassArray* array = upcalls->annotationArrayClass;
    res = array->doNew(numAnnotations, vm);
    for (uint16 i = 0; i < numAnnotations; ++i) {
			uint16 typeIndex = reader.readU2();
			const UTF8* annoType = compilingClass->ctpInfo->UTF8At(typeIndex);
			newHashMap = AR.createAnnotationMapValues();
			
			annoType = annoType->extract(loader->hashUTF8, 1,annoType->size-1);
			dprintf("Annotation type : %s\n", UTF8Buffer(annoType).cString());
			UserClass* AnnonClass = 0;
			UserCommonClass* commClass = loader->lookupClass(annoType);
			
			if (commClass) 
				AnnonClass = commClass -> asClass();
			else
				AnnonClass = loader->loadName(annoType, true, true, NULL);
				
			if (!AnnonClass) {
				assert(AnnonClass && "Panic, imposible conditions");
				abort();
			}
			dprintf("Loaded class : %s \n", UTF8Buffer(AnnonClass->name).cString());
			annotationClass = AnnonClass->getClassDelegatee(vm);
			
			annon = upcalls->createAnnotation->invokeJavaObjectStatic(vm, upcalls->newAnnotationHandler, &annotationClass, &newHashMap);
			ArrayObject::setElement((ArrayObject *)res, annon, i);
		} // end for
  }

  END_NATIVE_EXCEPTION
  return res;
}


JNIEXPORT jint JNICALL Java_java_lang_reflect_VMMethod_getModifiersInternal(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
JavaObjectVMMethod* Meth) {

  jint res = 0;
  llvm_gcroot(Meth, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  JavaMethod* meth = JavaObjectVMMethod::getInternalMethod(Meth);
  res = meth->access;

  END_NATIVE_EXCEPTION

  return res;
}

JNIEXPORT JavaObject* JNICALL Java_java_lang_reflect_VMMethod_getReturnType(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
JavaObjectVMMethod* Meth) {

  JavaObject* res = 0;
  llvm_gcroot(Meth, 0);
  llvm_gcroot(res, 0);

  BEGIN_NATIVE_EXCEPTION(0)
  verifyNull(Meth);

  JavaMethod* meth = JavaObjectVMMethod::getInternalMethod(Meth);
  UserCommonClass* cls =  (UserCommonClass*)JavaObjectVMMethod::getClass(Meth);
  JnjvmClassLoader* loader = cls->classLoader;
  res = meth->getReturnType(loader);

  END_NATIVE_EXCEPTION

  return res;
}


JNIEXPORT JavaObject* JNICALL Java_java_lang_reflect_VMMethod_getParameterTypes(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
JavaObjectVMMethod* Meth) {

  JavaObject* res = 0;
  llvm_gcroot(Meth, 0);
  llvm_gcroot(res, 0);

  BEGIN_NATIVE_EXCEPTION(0)
  verifyNull(Meth);

  JavaMethod* meth = JavaObjectVMMethod::getInternalMethod(Meth);
  UserCommonClass* cls =  (UserCommonClass*)JavaObjectVMMethod::getClass(Meth);
  JnjvmClassLoader* loader = cls->classLoader;
  res = meth->getParameterTypes(loader);

  END_NATIVE_EXCEPTION

  return res;
}


JavaObject* proceedVMMethod(JavaObjectVMMethod* Meth, JavaObject* obj,
                          ArrayObject* args)
  __attribute__((noinline));

JavaObject* proceedVMMethod(JavaObjectVMMethod* Meth, JavaObject* obj,
                          ArrayObject* args) {

  JavaObject* res = 0;
  JavaObject* exc = 0;

  llvm_gcroot(res, 0);
  llvm_gcroot(Meth, 0);
  llvm_gcroot(obj, 0);
  llvm_gcroot(args, 0);
  llvm_gcroot(exc, 0);

  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaMethod* meth = JavaObjectVMMethod::getInternalMethod(Meth);
  UserCommonClass* _cl =  (UserCommonClass*)JavaObjectVMMethod::getClass(Meth);

  sint32 nbArgs = args ? ArrayObject::getSize(args) : 0;
  Signdef* sign = meth->getSignature();
  sint32 size = sign->nbArguments;

  vmkit::ThreadAllocator allocator;
  jvalue* buf = size ?
      (jvalue*)allocator.Allocate(size * sizeof(jvalue)) : NULL;

  if (nbArgs == size) {
    //UserCommonClass* _cl = UserCommonClass::resolvedImplClass(vm, Cl, false);
    UserClass* cl = (UserClass*)_cl;

    if (isVirtual(meth->access)) {
      verifyNull(obj);
      UserCommonClass* objCl = JavaObject::getClass(obj);
      if (!(objCl->isSubclassOf(cl))) {
        vm->illegalArgumentException("<this> is not a valid type");
      }

      if (isInterface(cl->access)) {
        cl->initialiseClass(vm);
        UserClass* methodCl = 0;
        UserClass* lookup = objCl->isArray() ? objCl->super : objCl->asClass();
        meth = lookup->lookupMethod(meth->name, meth->type, false, true,
                                    &methodCl);
      }
    } else {
      cl->initialiseClass(vm);
    }

    JavaObject** ptr = ArrayObject::getElements(args);
    Typedef* const* arguments = sign->getArgumentsType();
    for (sint32 i = 0; i < size; ++i) {
      JavaObject::decapsulePrimitive(ptr[i], vm, &buf[i], arguments[i]);
      if (!arguments[i]->isPrimitive()) {
        buf[i].l = reinterpret_cast<jobject>(&ptr[i]);
      }
    }

    JavaThread* th = JavaThread::get();

#define RUN_METH(TYPE, VAR)                                                    \
    TRY {                                                                      \
      if (isVirtual(meth->access)) {                                           \
        if (isPublic(meth->access) && !isFinal(meth->access) &&                \
            !isFinal(meth->classDef->access)) {                                \
          VAR = meth->invoke##TYPE##VirtualBuf(vm, cl, obj, buf);              \
        } else {                                                               \
          VAR = meth->invoke##TYPE##SpecialBuf(vm, cl, obj, buf);              \
        }                                                                      \
      } else {                                                                 \
        VAR = meth->invoke##TYPE##StaticBuf(vm, cl, buf);                      \
      }                                                                        \
    } CATCH {                                                                  \
      exc = th->getJavaException();                                            \
    } END_CATCH;                                                               \
    if (exc) {                                                                 \
      th->clearException();                                                    \
      th->getJVM()->invocationTargetException(exc);                            \
      return NULL;                                                             \
    }

    Typedef* retType = sign->getReturnType();
    if (retType->isPrimitive()) {
      PrimitiveTypedef* prim = (PrimitiveTypedef*)retType;
      if (prim->isVoid()) {
        res = 0;
        uint32 val = 0;
        RUN_METH(Int, val);
      } else if (prim->isBool()) {
        uint32 val = 0;
        RUN_METH(Int, val);
        res = vm->upcalls->boolClass->doNew(vm);
        vm->upcalls->boolValue->setInstanceInt8Field(res, val);
      } else if (prim->isByte()) {
        uint32 val = 0;
        RUN_METH(Int, val);
        res = vm->upcalls->byteClass->doNew(vm);
        vm->upcalls->byteValue->setInstanceInt8Field(res, val);
      } else if (prim->isChar()) {
        uint32 val = 0;
        RUN_METH(Int, val);
        res = vm->upcalls->charClass->doNew(vm);
        vm->upcalls->charValue->setInstanceInt16Field(res, val);
      } else if (prim->isShort()) {
        uint32 val = 0;
        RUN_METH(Int, val);
        res = vm->upcalls->shortClass->doNew(vm);
        vm->upcalls->shortValue->setInstanceInt16Field(res, val);
      } else if (prim->isInt()) {
        uint32 val = 0;
        RUN_METH(Int, val);
        res = vm->upcalls->intClass->doNew(vm);
        vm->upcalls->intValue->setInstanceInt32Field(res, val);
      } else if (prim->isLong()) {
        sint64 val = 0;
        RUN_METH(Long, val);
        res = vm->upcalls->longClass->doNew(vm);
        vm->upcalls->longValue->setInstanceLongField(res, val);
      } else if (prim->isFloat()) {
        float val = 0;
        RUN_METH(Float, val);
        res = vm->upcalls->floatClass->doNew(vm);
        vm->upcalls->floatValue->setInstanceFloatField(res, val);
      } else if (prim->isDouble()) {
        double val = 0;
        RUN_METH(Double, val);
        res = vm->upcalls->doubleClass->doNew(vm);
        vm->upcalls->doubleValue->setInstanceDoubleField(res, val);
      }
    } else {
      RUN_METH(JavaObject, res);
    }
  } else {
    vm->illegalArgumentException("wrong number of arguments");
    return NULL;
  }

  return res;
}

#undef RUN_METH

JNIEXPORT JavaObject* JNICALL Java_java_lang_reflect_VMMethod_invoke(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
JavaObjectVMMethod* Meth, JavaObject* obj, ArrayObject* args) {

  JavaObject* res = 0;
  llvm_gcroot(res, 0);
  llvm_gcroot(Meth, 0);
  llvm_gcroot(obj, 0);
  llvm_gcroot(args, 0);

  BEGIN_NATIVE_EXCEPTION(0)
  verifyNull(Meth);
  res = proceedVMMethod(Meth, obj, args);

  END_NATIVE_EXCEPTION

  return res;
}

JNIEXPORT ArrayObject* JNICALL Java_java_lang_reflect_VMMethod_getExceptionTypes(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
JavaObjectVMMethod* Meth) {

  ArrayObject* res = 0;
  llvm_gcroot(Meth, 0);
  llvm_gcroot(res, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  verifyNull(Meth);

  JavaMethod* meth = JavaObjectVMMethod::getInternalMethod(Meth);
  UserCommonClass* cls =  (UserCommonClass*)JavaObjectVMMethod::getClass(Meth);
  JnjvmClassLoader* loader = cls->classLoader;
  res = meth->getExceptionTypes(loader);

  END_NATIVE_EXCEPTION

  return res;
}

JNIEXPORT JavaObject* JNICALL Java_java_lang_reflect_VMMethod_getSignature(
#ifdef NATIVE_JNI
JNIEnv *env,
#endif
JavaObjectVMMethod* Meth) {

  JavaObject* result = 0;
  llvm_gcroot(Meth, 0);
  llvm_gcroot(result, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  verifyNull(Meth);
  JavaMethod* meth = JavaObjectVMMethod::getInternalMethod(Meth);
  Jnjvm* vm = JavaThread::get()->getJVM();
  result = vm->internalUTF8ToStr(meth->type);

  END_NATIVE_EXCEPTION

  return result;
}

}
