//===--- IsolateCommonClass.cpp - User visible classes with isolates -------===//
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
#include "JavaThread.h"
#include "Jnjvm.h"
#include "JnjvmModule.h"

using namespace jnjvm;

UserCommonClass::UserCommonClass() {
  this->lockVar = mvm::Lock::allocRecursive();
  this->condVar = mvm::Cond::allocCond();
  this->status = loaded;
}

UserClass::UserClass(JnjvmClassLoader* JCL, const UTF8* name,
                     ArrayUInt8* bytes) {
  Class* cl = JnjvmSharedLoader::sharedLoader->constructSharedClass(name,
                                                                    bytes);
  if (!cl) {
    cl = allocator_new(JCL->allocator, Class)(JCL, name, bytes);
  }

  classDef = cl;
  classLoader = JCL;
  delegatee = 0;
  staticInstance = 0;
  ctpInfo = new(JCL->allocator, cl->ctpInfo->ctpSize) UserConstantPool(this);
}

UserClassArray::UserClassArray(JnjvmClassLoader* JCL, const UTF8* name) {
  ClassArray* cl = JnjvmSharedLoader::sharedLoader->constructSharedClassArray(name);
  if (!cl) {
    cl = allocator_new(JCL->allocator, ClassArray)(JCL, name);
  }
  classDef = cl;
  classLoader = JCL;
  delegatee = 0;
  _baseClass = 0;
  _funcs = 0;
}

UserClassPrimitive::UserClassPrimitive(JnjvmClassLoader* JCL, const UTF8* name,
                                       uint32 nb) {
  ClassPrimitive* cl = 
    JnjvmSharedLoader::sharedLoader->constructSharedClassPrimitive(name, nb);
  if (!cl) {
    cl = new ClassPrimitive(JCL, name, nb);
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
            def->classLoader->TheModule->resolveVirtualClass(def);
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

UserClass* UserCommonClass::lookupClassFromMethod(JavaMethod* meth) {
  fprintf(stderr, "implement me");
  abort();
  return 0;
}

UserCommonClass* UserCommonClass::getUserClass(CommonClass* cl) {
  fprintf(stderr, "implement me");
  abort();
  return 0;
}

UserCommonClass* UserConstantPool::loadClass(uint32 index) {
  fprintf(stderr, "implement me");
  abort();
  return 0;
}

void UserConstantPool::resolveMethod(uint32 index, UserCommonClass*& cl,
                                     const UTF8*& utf8, Signdef*& sign) {
  fprintf(stderr, "implement me");
  abort();
}
  
void UserConstantPool::resolveField(uint32 index, UserCommonClass*& cl,
                                    const UTF8*& utf8, Typedef*& sign) {
  fprintf(stderr, "implement me");
  abort();
}

const UTF8* UserConstantPool::UTF8AtForString(uint32 entry) {
  fprintf(stderr, "implement me");
  abort();
}

void* UserConstantPool::operator new(size_t sz, JavaAllocator* alloc,
                                     uint32 size){
  return alloc->allocateObject(sz + size * sizeof(void*), VT);
}

AssessorDesc* UserClassArray::funcs() {
  fprintf(stderr, "implement me");
  abort();
  return 0;
}


UserClassPrimitive* AssessorDesc::getPrimitiveClass() const {
  fprintf(stderr, "implement me");
  abort();
  return 0;
}

UserClassArray* AssessorDesc::getArrayClass() const {
  fprintf(stderr, "implement me");
  abort();
  return 0;
}

JavaMethod* UserCommonClass::lookupMethodDontThrow(const UTF8* name,
                                                   const UTF8* type,
                                                   bool isStatic,
                                                   bool recurse) {
  
  CommonClass::FieldCmp CC(name, type);
  CommonClass::method_map* map = isStatic ? getStaticMethods() :
                                            getVirtualMethods();
  CommonClass::method_iterator End = map->end();
  CommonClass::method_iterator I = map->find(CC);
  if (I != End) return I->second;
  
  JavaMethod *cur = 0;
  
  if (recurse) {
    if (super) cur = super->lookupMethodDontThrow(name, type, isStatic,
                                                  recurse);
    if (cur) return cur;
    if (isStatic) {
      std::vector<UserClass*>* interfaces = getInterfaces();
      for (std::vector<UserClass*>::iterator i = interfaces->begin(),
           e = interfaces->end(); i!= e; i++) {
        cur = (*i)->lookupMethodDontThrow(name, type, isStatic, recurse);
        if (cur) return cur;
      }
    }
  }

  return 0;
}

JavaMethod* UserCommonClass::lookupMethod(const UTF8* name, const UTF8* type,
                                          bool isStatic, bool recurse) {
  JavaMethod* res = lookupMethodDontThrow(name, type, isStatic, recurse);
  if (!res) {
    JavaThread::get()->isolate->noSuchMethodError(this->classDef, name);
  }
  return res;
}

JavaField*
UserCommonClass::lookupFieldDontThrow(const UTF8* name, const UTF8* type,
                                      bool isStatic, bool recurse,
                                      UserCommonClass*& definingClass) {

  CommonClass::FieldCmp CC(name, type);
  CommonClass::field_map* map = isStatic ? getStaticFields() :
                                           getVirtualFields();
  CommonClass::field_iterator End = map->end();
  CommonClass::field_iterator I = map->find(CC);
  if (I != End) {
    definingClass = this;
    return I->second;
  }
  
  JavaField *cur = 0;

  if (recurse) {
    if (super) cur = super->lookupFieldDontThrow(name, type, isStatic,
                                                 recurse, definingClass);
    if (cur) {
      definingClass = (UserClass*)super;
      return cur;
    }
    if (isStatic) {
      std::vector<UserClass*>* interfaces = getInterfaces();
      for (std::vector<UserClass*>::iterator i = interfaces->begin(),
           e = interfaces->end(); i!= e; i++) {
        cur = (*i)->lookupFieldDontThrow(name, type, isStatic, recurse,
                                         definingClass);
        if (cur) {
          definingClass = *i;
          return cur;
        }
      }
    }
  }

  return 0;
}

JavaField* UserCommonClass::lookupField(const UTF8* name, const UTF8* type,
                                        bool isStatic, bool recurse,
                                        UserCommonClass*& definingClass) {
  
  JavaField* res = lookupFieldDontThrow(name, type, isStatic, recurse,
                                        definingClass);
  if (!res) {
    JavaThread::get()->isolate->noSuchFieldError(this->classDef, name);
  }
  return res;
}
