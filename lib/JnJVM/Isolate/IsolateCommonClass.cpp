//===--- IsolateCommonClass.cpp - User visible classes with isolates -------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "IsolateCommonClass.h"
#include "IsolateSharedLoader.h"
#include "JavaAllocator.h"
#include "JavaClass.h"

using namespace jnjvm;

UserCommonClass::UserCommonClass() {
  this->lockVar = mvm::Lock::allocRecursive();
  this->condVar = mvm::Cond::allocCond();
  this->status = loaded;
}

UserClass::UserClass(JnjvmClassLoader* JCL, const UTF8* name,
                     ArrayUInt8* bytes) {
  Class* cl = JnjvmSharedLoader::sharedLoader->constructSharedClass(name, bytes);
  if (!cl) {
    cl = allocator_new(JCL->allocator, Class)(JCL, name, bytes);
  }

  classDef = cl;
  classLoader = JCL;
  delegatee = 0;
  staticInstance = 0;
}

UserClassArray::UserClassArray(JnjvmClassLoader* JCL, const UTF8* name) {
  ClassArray* cl = JnjvmSharedLoader::sharedLoader->constructSharedClassArray(name);
  if (!cl) {
    cl = allocator_new(JCL->allocator, ClassArray)(JCL, name);
  }
  classDef = cl;
  classLoader = JCL;
  delegatee = 0;
  baseClass = 0;
}

UserClassPrimitive::UserClassPrimitive(JnjvmClassLoader* JCL, const UTF8* name) {
  ClassPrimitive* cl = JnjvmSharedLoader::sharedLoader->constructSharedClassPrimitive(name);
  if (!cl) {
    cl = new ClassPrimitive(JCL, name);
  }
  classDef = cl;
  classLoader = JCL;
  delegatee = 0;
}

void UserCommonClass::resolveClass() {
  if (status < resolved) {
    acquire();
    if (status >= resolved) {
      release();
    } else if (status == loaded) {
      if (isArray()) {
        UserClassArray* arrayCl = (UserClassArray*)this;
        UserCommonClass* baseClass =  arrayCl->baseClass();
        baseClass->resolveClass();
        status = resolved;
      } else {
        UserClass* cl = (UserClass*)this;
        Class* def = (Class*)classDef;
        if (classDef->status < resolved) {
          classDef->acquire();
          if (classDef->status == loaded) {
            def->readClass();
            def->status = classRead;
            status = classRead;
            def->release();
            release();
            cl->loadParents();
            acquire();
            def->acquire();
            def->status = prepared;
            status = prepared;
            def->classLoader->TheModule->resolveVirtualClass(cl);
            def->status = resolved;
            status = resolved;
            classDef->broadcastClass();
          } else {
            while (classDef->status < resolved) {
              classDef->waitClass();
            }
            classDef->release();
          }
        } else {
          release();
          status = classRead,
          cl->loadParents();
          status = resolved;
          broadcastClass();
        }
      }
    } else {
      while (status < resolved) {
        waitClass();
      }
      release();
    }
  }
}
