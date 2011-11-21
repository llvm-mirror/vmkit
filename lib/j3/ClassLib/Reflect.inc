//===---- Reflect.inc - Shared reflecation code ---------------------------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "ClasspathReflect.h"
#include "JavaArray.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"

namespace j3 {

ArrayObject* JavaObjectClass::getDeclaredConstructors(JavaObjectClass* Cl, bool publicOnly) {
  ArrayObject* ret = 0;
  JavaObject* tmp = 0;
  llvm_gcroot(Cl, 0);
  llvm_gcroot(ret, 0);
  llvm_gcroot(tmp, 0);


  Jnjvm* vm = JavaThread::get()->getJVM();
  UserCommonClass* cl = UserCommonClass::resolvedImplClass(vm, Cl, false);

  if (cl->isArray() || cl->isInterface() || cl->isPrimitive()) {
    ret = (ArrayObject*)vm->upcalls->constructorArrayClass->doNew(0, vm);
  } else {
    UserClass* realCl = cl->asClass();;
    JnjvmClassLoader* classLoader = cl->classLoader;
    uint32 size = 0;

    for (uint32 i = 0; i < realCl->nbVirtualMethods; ++i) {
      JavaMethod* meth = &realCl->virtualMethods[i];
      bool pub = isPublic(meth->access);
      if (meth->name->equals(classLoader->bootstrapLoader->initName) &&
          (!publicOnly || pub)) {
        ++size;
      }
    }

    ret = (ArrayObject*)vm->upcalls->constructorArrayClass->doNew(size, vm);

    sint32 index = 0;
    for (uint32 i = 0; i < realCl->nbVirtualMethods; ++i) {
      JavaMethod* meth = &realCl->virtualMethods[i];
      bool pub = isPublic(meth->access);
      if (meth->name->equals(classLoader->bootstrapLoader->initName) &&
          (!publicOnly || pub)) {
        tmp = JavaObjectConstructor::createFromInternalConstructor(meth, i);
        ArrayObject::setElement(ret, tmp, index);
        index++;
      }
    }
  }

  return ret;
}

ArrayObject* JavaObjectClass::getDeclaredMethods(JavaObjectClass* Cl, bool publicOnly) {
  ArrayObject* ret = 0;
  JavaObject* tmp = 0;
  JavaString* str = 0;
  llvm_gcroot(Cl, 0);
  llvm_gcroot(ret, 0);
  llvm_gcroot(tmp, 0);
  llvm_gcroot(str, 0);

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserCommonClass* cl = UserCommonClass::resolvedImplClass(vm, Cl, false);
  Classpath* upcalls = vm->upcalls;

  if (cl->isArray() || cl->isPrimitive()) {
    ret = (ArrayObject*)upcalls->methodArrayClass->doNew(0, vm);
  } else {
    UserClass* realCl = cl->asClass();
    JnjvmClassLoader* classLoader = cl->classLoader;
    uint32 size = 0;

    for (uint32 i = 0; i < realCl->nbVirtualMethods + realCl->nbStaticMethods;
         ++i) {
      JavaMethod* meth = &realCl->virtualMethods[i];
      bool pub = isPublic(meth->access);
      if (!(meth->name->equals(classLoader->bootstrapLoader->initName)) &&
          !(meth->name->equals(classLoader->bootstrapLoader->clinitName)) &&
          (!publicOnly || pub)) {
        ++size;
      }
    }


    ret = (ArrayObject*)upcalls->methodArrayClass->doNew(size, vm);

    sint32 index = 0;
    for (uint32 i = 0; i < realCl->nbVirtualMethods + realCl->nbStaticMethods;
         ++i) {
      JavaMethod* meth = &realCl->virtualMethods[i];
      bool pub = isPublic(meth->access);
      if (!(meth->name->equals(classLoader->bootstrapLoader->initName)) &&
          !(meth->name->equals(classLoader->bootstrapLoader->clinitName)) &&
          (!publicOnly || pub)) {
        tmp = JavaObjectMethod::createFromInternalMethod(meth, i);
        ArrayObject::setElement(ret, tmp, index);
        index++;
      }
    }
  }

  return ret;
}

JavaObject* JavaObjectClass::getDeclaringClass(JavaObjectClass* Cl) {
  JavaObject* res = 0;
  llvm_gcroot(Cl, 0);
  llvm_gcroot(res, 0);

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = UserCommonClass::resolvedImplClass(vm, Cl, false)->asClass();

  if (cl) {
    cl->resolveInnerOuterClasses();
    UserClass* outer = cl->getOuterClass();
    if (outer) {
      res = outer->getClassDelegatee(vm);
    }
  }

  return res;
}

ArrayObject* JavaObjectClass::getDeclaredClasses(JavaObjectClass* Cl, bool publicOnly) {
  ArrayObject* result = 0;
  llvm_gcroot(result, 0);
  llvm_gcroot(Cl, 0);

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = UserCommonClass::resolvedImplClass(vm, Cl, false)->asClass();
  if (cl) {
    cl->resolveInnerOuterClasses();
    UserClassArray* array = vm->upcalls->constructorArrayClass;

    uint16 sizeArray = 0;

    if (publicOnly) {
      for (uint16 i = 0; i < cl->nbInnerClasses; ++i) {
        UserClass* klass = cl->innerClasses[i];
        if (isPublic(klass->innerAccess)) ++sizeArray;
      }
    } else {
      sizeArray = cl->nbInnerClasses;
    }

    result = (ArrayObject*)array->doNew(sizeArray, vm);
    int index = 0;
    for (uint16 i = 0; i < cl->nbInnerClasses; ++i) {
      UserClass* klass = cl->innerClasses[i];
      if (!publicOnly || isPublic(klass->innerAccess))
        ArrayObject::setElement(result, klass->getClassDelegatee(vm), index++);
    }
    assert(index == sizeArray);
  }

  return result;
}

ArrayObject* JavaObjectClass::getDeclaredFields(JavaObjectClass* Cl, bool publicOnly) {
  ArrayObject* ret = 0;
  JavaObject* tmp = 0;
  JavaString* name = 0;
  llvm_gcroot(Cl, 0);
  llvm_gcroot(ret, 0);
  llvm_gcroot(tmp, 0);
  llvm_gcroot(name, 0);

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserCommonClass* cl = UserCommonClass::resolvedImplClass(vm, Cl, false);

  if (!cl->isClass()) {
    ret = (ArrayObject*)vm->upcalls->fieldArrayClass->doNew(0, vm);
  } else {
    UserClass* realCl = cl->asClass();
    uint32 size = 0;
    for (uint32 i = 0; i < realCl->nbVirtualFields + realCl->nbStaticFields;
         ++i) {
      JavaField* field = &realCl->virtualFields[i];
      if (!publicOnly || isPublic(field->access)) {
        ++size;
      }
    }

    ret = (ArrayObject*)vm->upcalls->fieldArrayClass->doNew(size, vm);

    sint32 index = 0;
    for (uint32 i = 0; i < realCl->nbVirtualFields + realCl->nbStaticFields;
         ++i) {
      JavaField* field = &realCl->virtualFields[i];
      if (!publicOnly || isPublic(field->access)) {
        tmp = JavaObjectField::createFromInternalField(field, i);
        ArrayObject::setElement(ret, tmp, index);
        index++;
      }
    }
  }

  return ret;
}

ArrayObject* JavaObjectClass::getInterfaces(JavaObjectClass* Cl) {
  ArrayObject* res = 0;
  llvm_gcroot(Cl, 0);
  llvm_gcroot(res, 0);

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserCommonClass* cl = UserCommonClass::resolvedImplClass(vm, Cl, false);
  res = (ArrayObject*)vm->upcalls->classArrayClass->doNew(cl->nbInterfaces, vm);

  for (uint16 i = 0; i < cl->nbInterfaces; ++i) {
    UserClass* klass = cl->interfaces[i];
    ArrayObject::setElement(res, klass->getClassDelegatee(vm), i);
  }

  return res;
}

ArrayObject* JavaObjectClass::getDeclaredAnnotations(JavaObjectClass* Cl) {
  // TODO: Implement me (for now just returning empty array)
  ArrayObject* res = 0;
  llvm_gcroot(res, 0);

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClassArray* array = vm->upcalls->constructorArrayAnnotation;
  res = (ArrayObject*)array->doNew(0, vm);

  return res;
}

int32_t JavaObjectClass::getModifiers(JavaObjectClass* Cl) {
  llvm_gcroot(Cl, 0);
  int32_t res;

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserCommonClass* cl = UserCommonClass::resolvedImplClass(vm, Cl, false);
  res = cl->getAccess();

  if (isEnum(res) && cl->getSuper() != vm->upcalls->EnumClass) {
    // javac may put that flag to inner classes of enum classes.
    res &= ~ACC_ENUM;
  }

  return res;
}

} // end namespace j3
