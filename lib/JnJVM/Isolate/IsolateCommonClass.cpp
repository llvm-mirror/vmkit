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
