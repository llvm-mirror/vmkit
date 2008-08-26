//===---- IsolateSharedLoader.cpp - Shared loader for isolates ------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "IsolateSharedLoader.h"
#include "JavaAllocator.h"
#include "JavaClass.h"
#include "JnjvmClassLoader.h"
#include "JnjvmModule.h"
#include "JnjvmModuleProvider.h"
#include "LockedMap.h"

using namespace jnjvm;

JnjvmSharedLoader* JnjvmSharedLoader::createSharedLoader() {
  
  JnjvmSharedLoader* JCL = gc_new(JnjvmSharedLoader)();
  JCL->TheModule = new JnjvmModule("Bootstrap JnJVM");
  JCL->TheModuleProvider = new JnjvmModuleProvider(JCL->TheModule);
  
  JCL->allocator = new JavaAllocator();
  
  JCL->hashUTF8 = new UTF8Map(JCL->allocator, 0);
  JCL->classes = allocator_new(allocator, ClassMap)();
  JCL->nameClasses = allocator_new(allocator, SharedClassNameMap)();
  JCL->byteClasses = allocator_new(allocator, SharedClassByteMap)();
  JCL->javaTypes = new TypeMap(); 
  JCL->javaSignatures = new SignMap(); 
  
  return JCL;
}

Class* JnjvmSharedLoader::constructSharedClass(const UTF8* name,
                                               ArrayUInt8* bytes) {
  byteClasses->lock->lock();
  SharedClassByteMap::iterator End = byteClasses->map.end();
  SharedClassByteMap::iterator I = byteClasses->map.find(bytes);
  Class* res = 0;
  if (I == End) {
    res = allocator_new(allocator, Class)(this, name, bytes);
    byteClasses->map.insert(std::make_pair(bytes, res));
  } else {
    res = ((Class*)(I->second));
  }
  byteClasses->lock->unlock();
  return res;
}

ClassArray* JnjvmSharedLoader::constructSharedClassArray(const UTF8* name) {
  nameClasses->lock->lock();
  SharedClassNameMap::iterator End = nameClasses->map.end();
  SharedClassNameMap::iterator I = nameClasses->map.find(name);
  ClassArray* res = 0;
  if (I == End) {
    res = allocator_new(allocator, ClassArray)(this, name);
    nameClasses->map.insert(std::make_pair(name, res));
  } else {
    res = ((ClassArray*)(I->second));
  }
  nameClasses->lock->unlock();
  return res;
}

ClassPrimitive* JnjvmSharedLoader::constructSharedClassPrimitive(const UTF8* name) {
  nameClasses->lock->lock();
  SharedClassNameMap::iterator End = nameClasses->map.end();
  SharedClassNameMap::iterator I = nameClasses->map.find(name);
  ClassPrimitive* res = 0;
  if (I == End) {
    res = new ClassPrimitive(this, name);
    nameClasses->map.insert(std::make_pair(name, res));
  } else {
    res = ((ClassPrimitive*)(I->second));
  }
  nameClasses->lock->unlock();
  return res;
}
