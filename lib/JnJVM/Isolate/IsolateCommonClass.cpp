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
  ctpInfo = 0;
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
            cl->ctpInfo = 
              new(classLoader->allocator, def->ctpInfo->ctpSize) UserConstantPool(cl);
            cl->loadParents();
            if (cl->super)
              def->super = cl->super->classDef;
            for (std::vector<UserClass*>::iterator i = interfaces.begin(),
                 e = interfaces.end(); i != e; ++i) {
              def->interfaces.push_back((Class*)((*i)->classDef));
            }
            def->status = prepared;
            status = prepared;
            def->classLoader->TheModule->resolveVirtualClass(def);
            virtualSize = def->virtualSize;
            virtualVT = def->virtualVT;
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
          cl->ctpInfo = 
            new(classLoader->allocator, def->ctpInfo->ctpSize) UserConstantPool(cl);
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


UserCommonClass* UserConstantPool::isClassLoaded(uint32 entry) {
  JavaConstantPool* ctpInfo = getSharedPool();
  if (! ((entry > 0) && (entry < ctpInfo->ctpSize) && 
        ctpInfo->typeAt(entry) == JavaConstantPool::ConstantClass)) {
    JavaThread::get()->isolate->classFormatError(
              "bad constant pool number for class at entry %d", entry);
  }
  return (UserCommonClass*)ctpRes[entry];
}

UserCommonClass* UserConstantPool::loadClass(uint32 index) {
  UserCommonClass* temp = isClassLoaded(index);
  if (!temp) {
    JavaConstantPool* ctpInfo = getSharedPool();
    JnjvmClassLoader* loader = getClass()->classLoader;
    const UTF8* name = ctpInfo->UTF8At(ctpInfo->ctpDef[index]);
    if (name->elements[0] == AssessorDesc::I_TAB) {
      temp = loader->constructArray(name);
      temp->resolveClass();
    } else {
      // Put into ctpRes because there is only one representation of the class
      temp = loader->loadName(name, true, false);
    }
    ctpRes[index] = temp;
    ctpInfo->ctpRes[index] = temp->classDef;
  }
  return temp;
}

void UserConstantPool::resolveMethod(uint32 index, UserCommonClass*& cl,
                                     const UTF8*& utf8, Signdef*& sign) {
  JavaConstantPool* ctpInfo = getSharedPool();
  sint32 entry = ctpInfo->ctpDef[index];
  sint32 ntIndex = entry & 0xFFFF;
  sign = (Signdef*)ctpInfo->ctpRes[ntIndex];
  assert(sign && "No cached signature after JITting");
  utf8 = ctpInfo->UTF8At(ctpInfo->ctpDef[ntIndex] >> 16);
  cl = loadClass(entry >> 16);
  assert(cl && cl->isResolved() && "No resolved class when resolving method");
}
  
void UserConstantPool::resolveField(uint32 index, UserCommonClass*& cl,
                                    const UTF8*& utf8, Typedef*& sign) {
  JavaConstantPool* ctpInfo = getSharedPool();
  sint32 entry = ctpInfo->ctpDef[index];
  sint32 ntIndex = entry & 0xFFFF;
  sign = (Typedef*)ctpInfo->ctpRes[ntIndex];
  assert(sign && "No cached Typedef after JITting");
  utf8 = ctpInfo->UTF8At(ctpInfo->ctpDef[ntIndex] >> 16);
  cl = loadClass(entry >> 16);
  assert(cl && cl->isResolved() && "No resolved class when resolving field");
}

void* UserConstantPool::operator new(size_t sz, JavaAllocator* alloc,
                                     uint32 size){
  return alloc->allocateObject(sz + size * sizeof(void*), VT);
}

UserClassPrimitive* AssessorDesc::getPrimitiveClass() const {
  Jnjvm* vm = JavaThread::get()->isolate;
  UserClassArray* arrayCl = vm->arrayClasses[numId];
  UserClassPrimitive* cl = (UserClassPrimitive*)arrayCl->baseClass();
  assert(cl && "Primitive array class does not have a primitive.");
  return cl;
}

UserClassArray* AssessorDesc::getArrayClass() const {
  Jnjvm* vm = JavaThread::get()->isolate;
  UserClassArray* arrayCl = vm->arrayClasses[numId];
  return arrayCl;
}

JavaMethod* UserCommonClass::lookupMethodDontThrow(const UTF8* name,
                                                   const UTF8* type,
                                                   bool isStatic,
                                                   bool recurse,
                                                   UserClass*& methodCl) {
  
  CommonClass::FieldCmp CC(name, type);
  CommonClass::method_map* map = isStatic ? getStaticMethods() :
                                            getVirtualMethods();
  CommonClass::method_iterator End = map->end();
  CommonClass::method_iterator I = map->find(CC);
  if (I != End) {
    methodCl = (UserClass*)this;
    return I->second;
  }
  
  JavaMethod *cur = 0;
  
  if (recurse) {
    if (super) cur = super->lookupMethodDontThrow(name, type, isStatic,
                                                  recurse, methodCl);
    if (cur) return cur;

    if (isStatic) {
      std::vector<UserClass*>* interfaces = getInterfaces();
      for (std::vector<UserClass*>::iterator i = interfaces->begin(),
           e = interfaces->end(); i!= e; i++) {
        cur = (*i)->lookupMethodDontThrow(name, type, isStatic, recurse,
                                          methodCl);
        if (cur) return cur;
      }
    }
  }

  return 0;
}

JavaMethod* UserCommonClass::lookupMethod(const UTF8* name, const UTF8* type,
                                          bool isStatic, bool recurse,
                                          UserClass*& methodCl) {
  JavaMethod* res = lookupMethodDontThrow(name, type, isStatic, recurse,
                                          methodCl);
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
    if (cur) return cur;

    if (isStatic) {
      std::vector<UserClass*>* interfaces = getInterfaces();
      for (std::vector<UserClass*>::iterator i = interfaces->begin(),
           e = interfaces->end(); i!= e; i++) {
        cur = (*i)->lookupFieldDontThrow(name, type, isStatic, recurse,
                                         definingClass);
        if (cur) return cur;
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

void UserConstantPool::print(mvm::PrintBuffer* buf) const {
  buf->write("User constant pool of <");
  getClass()->classDef->print(buf);
  buf->write(">");
}
