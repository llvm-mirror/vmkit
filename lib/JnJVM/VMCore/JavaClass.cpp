//===-------- JavaClass.cpp - Java class representation -------------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <vector>

#include <string.h>

#include "mvm/JIT.h"
#include "types.h"

#include "JavaArray.h"
#include "JavaCache.h"
#include "JavaClass.h"
#include "JavaConstantPool.h"
#include "JavaJIT.h"
#include "JavaObject.h"
#include "JavaThread.h"
#include "JavaTypes.h"
#include "Jnjvm.h"
#include "JnjvmModuleProvider.h"
#include "Reader.h"

using namespace jnjvm;

const UTF8* Attribut::codeAttribut = 0;
const UTF8* Attribut::exceptionsAttribut = 0;
const UTF8* Attribut::constantAttribut = 0;
const UTF8* Attribut::lineNumberTableAttribut = 0;
const UTF8* Attribut::innerClassesAttribut = 0;
const UTF8* Attribut::sourceFileAttribut = 0;

JavaObject* CommonClass::jnjvmClassLoader = 0;

CommonClass* ClassArray::SuperArray = 0;
std::vector<Class*> ClassArray::InterfacesArray;


Attribut::Attribut(const UTF8* name, uint32 length,
                   uint32 offset) {
  
  this->start    = offset;
  this->nbb      = length;
  this->name     = name;

}

Attribut* Class::lookupAttribut(const UTF8* key ) {
  for (std::vector<Attribut*>::iterator i = attributs.begin(), 
       e = attributs.end(); i!= e; ++i) {
    Attribut* cur = *i;
    if (cur->name->equals(key)) return cur;
  }

  return 0;
}

Attribut* JavaField::lookupAttribut(const UTF8* key ) {
  for (std::vector<Attribut*>::iterator i = attributs.begin(), 
       e = attributs.end(); i!= e;++i) {
    Attribut* cur = *i;
    if (cur->name->equals(key)) return cur;
  }

  return 0;
}

Attribut* JavaMethod::lookupAttribut(const UTF8* key ) {
  for (std::vector<Attribut*>::iterator i = attributs.begin(), 
       e = attributs.end(); i!= e; ++i) {
    Attribut* cur = *i;
    if (cur->name->equals(key)) return cur;
  }

  return 0;
}

CommonClass::~CommonClass() {
  free(display);
  free(virtualVT);
  if (isolate)
    isolate->TheModule->removeClass(this);
  delete lockVar;
  delete condVar;
}

CommonClass::CommonClass() {
  display = 0;
  isolate = 0;
  lockVar = 0;
  condVar = 0;
  virtualVT = 0;
}

Class::Class() {
  ctpInfo = 0;
  staticVT = 0;
}

Class::~Class() {
  for (std::vector<Attribut*>::iterator i = attributs.begin(), 
       e = attributs.end(); i!= e; ++i) {
    Attribut* cur = *i;
    delete cur;
  }
  
  for (field_iterator i = staticFields.begin(), 
       e = staticFields.end(); i!= e; ++i) {
    JavaField* cur = i->second;
    delete cur;
  }
  
  for (field_iterator i = virtualFields.begin(), 
       e = virtualFields.end(); i!= e; ++i) {
    JavaField* cur = i->second;
    delete cur;
  }
  
  for (method_iterator i = virtualMethods.begin(), 
       e = virtualMethods.end(); i!= e; ++i) {
    JavaMethod* cur = i->second;
    delete cur;
  }
  
  for (method_iterator i = staticMethods.begin(), 
       e = staticMethods.end(); i!= e; ++i) {
    JavaMethod* cur = i->second;
    delete cur;
  }
  
  delete ctpInfo;
  free(staticVT);
}

JavaField::~JavaField() {
  for (std::vector<Attribut*>::iterator i = attributs.begin(), 
       e = attributs.end(); i!= e; ++i) {
    Attribut* cur = *i;
    delete cur;
  }
}

JavaMethod::~JavaMethod() {
  
  for (std::vector<Attribut*>::iterator i = attributs.begin(), 
       e = attributs.end(); i!= e; ++i) {
    Attribut* cur = *i;
    delete cur;
  }

  for (std::vector<Enveloppe*>::iterator i = caches.begin(), 
       e = caches.end(); i!= e; ++i) {
    Enveloppe* cur = *i;
    delete cur;
  }
}

static void printClassNameIntern(const UTF8* name, unsigned int start,
                                 unsigned int end,  mvm::PrintBuffer* buf) {
  
  uint16 first = name->elements[start];
  if (first == AssessorDesc::I_TAB) {
    unsigned int stepsEnd = start;
    while (name->elements[stepsEnd] == AssessorDesc::I_TAB) stepsEnd++;
    if (name->elements[stepsEnd] == AssessorDesc::I_REF) {
      printClassNameIntern(name, (stepsEnd + 1),(end - 1), buf);
    } else {
      AssessorDesc * funcs = 0;
      uint32 next = 0;
      AssessorDesc::analyseIntern(name, stepsEnd, 0, funcs, next);
      buf->write(funcs->asciizName);
    }
    buf->write(" ");
    for (uint32 i = start; i < stepsEnd; i++)
      buf->write("[]");
  } else {
    char* tmp = (char*)(alloca(1 + (end - start)));
    for (uint32 i = start; i < end; i++) {
      short int cur = name->elements[i];
      tmp[i - start] = (cur == '/' ? '.' : cur);
    }
    tmp[end - start] = 0;
    buf->write(tmp);
  }
}

void CommonClass::printClassName(const UTF8* name, mvm::PrintBuffer* buf) {
  printClassNameIntern(name, 0, name->size, buf);
}

void CommonClass::print(mvm::PrintBuffer* buf) const {
  buf->write("CommonClass<");
  printClassName(name, buf);
  buf->write(">");
}

CommonClass::CommonClass(Jnjvm* vm, const UTF8* n, bool isArray) {
  name = n;
  this->lockVar = mvm::Lock::allocRecursive();
  this->condVar = mvm::Cond::allocCond();
  this->status = hashed;
  this->isolate = vm;
  this->isArray = isArray;
  this->isPrimitive = false;
#ifndef MULTIPLE_VM
  this->delegatee = 0;
#endif
}

ClassPrimitive::ClassPrimitive(Jnjvm* vm, const UTF8* n) : 
  CommonClass(vm, n, false) {
  
  display = (CommonClass**)malloc(sizeof(CommonClass*));
  display[0] = this;
  isPrimitive = true;
  status = ready;
  access = ACC_ABSTRACT | ACC_FINAL | ACC_PUBLIC;
}

Class::Class(Jnjvm* vm, const UTF8* n) : CommonClass(vm, n, false) {
  classLoader = 0;
  bytes = 0;
  super = 0;
  ctpInfo = 0;
#ifndef MULTIPLE_VM
  _staticInstance = 0;
#endif
}

ClassArray::ClassArray(Jnjvm* vm, const UTF8* n) : CommonClass(vm, n, true) {
  classLoader = 0;
  _funcs = 0;
  _baseClass = 0;
  super = ClassArray::SuperArray;
  interfaces = ClassArray::InterfacesArray;
  depth = 1;
  display = (CommonClass**)malloc(2 * sizeof(CommonClass*));
  display[0] = ClassArray::SuperArray;
  display[1] = this;
  access = ACC_FINAL | ACC_ABSTRACT;
  status = loaded;
}

void Class::print(mvm::PrintBuffer* buf) const {
  buf->write("Class<");
  printClassName(name, buf);
  buf->write(">");
}

void ClassArray::print(mvm::PrintBuffer* buf) const {
  buf->write("ClassArray<");
  printClassName(name, buf);
  buf->write(">");
}

void ClassArray::resolveComponent() {
  AssessorDesc::introspectArray(isolate, classLoader, name, 0, _funcs,
                                _baseClass);
}

JavaObject* ClassArray::arrayLoader(Jnjvm* isolate, const UTF8* name,
                                    JavaObject* loader,
                                    unsigned int start, unsigned int len) {
  
  if (name->elements[start] == AssessorDesc::I_TAB) {
    return arrayLoader(isolate, name, loader, start + 1, len - 1);
  } else if (name->elements[start] == AssessorDesc::I_REF) {
    const UTF8* componentName = name->javaToInternal(isolate, start + 1,
                                                     len - 2);
    CommonClass* cl = isolate->loadName(componentName, loader, false, false,
                                        true);
    return cl->classLoader;
  } else {
    return 0;
  }
}

void* JavaMethod::compiledPtr() {
  if (code != 0) return code;
  else {
    classDef->acquire();
    if (code == 0) {
      code = 
        classDef->isolate->TheModuleProvider->materializeFunction(this);
    }
    classDef->release();
    return code;
  }
}

const char* JavaMethod::printString() const {
  mvm::PrintBuffer *buf= mvm::PrintBuffer::alloc();
  buf->write("JavaMethod<");
  ((JavaMethod*)this)->getSignature()->printWithSign(classDef, name, buf);
  buf->write(">");
  return buf->contents()->cString();
}

const char* JavaField::printString() const {
  mvm::PrintBuffer *buf= mvm::PrintBuffer::alloc();
  buf->write("JavaField<");
  if (isStatic(access))
    buf->write("static ");
  else
    buf->write("virtual ");
  ((JavaField*)this)->getSignature()->tPrintBuf(buf);
  buf->write(" ");
  classDef->print(buf);
  buf->write("::");
  name->print(buf);
  buf->write(">");
  return buf->contents()->cString();
}

JavaMethod* CommonClass::lookupMethodDontThrow(const UTF8* name,
                                               const UTF8* type, bool isStatic,
                                               bool recurse) {
  
  FieldCmp CC(name, type);
  method_map& map = isStatic ? staticMethods : virtualMethods;
  method_iterator End = map.end();
  method_iterator I = map.find(CC);
  if (I != End) return I->second;
  
  JavaMethod *cur = 0;
  
  if (recurse) {
    if (super) cur = super->lookupMethodDontThrow(name, type, isStatic,
                                                  recurse);
    if (cur) return cur;
    if (isStatic) {
      for (std::vector<Class*>::iterator i = interfaces.begin(),
           e = interfaces.end(); i!= e; i++) {
        cur = (*i)->lookupMethodDontThrow(name, type, isStatic, recurse);
        if (cur) return cur;
      }
    }
  }

  return 0;
}

JavaMethod* CommonClass::lookupMethod(const UTF8* name, const UTF8* type,
                                      bool isStatic, bool recurse) {
  JavaMethod* res = lookupMethodDontThrow(name, type, isStatic, recurse);
  if (!res) {
    JavaThread::get()->isolate->error(Jnjvm::NoSuchMethodError, 
                                      "unable to find %s in %s",
                                      name->printString(), this->printString());
  }
  return res;
}

JavaField* CommonClass::lookupFieldDontThrow(const UTF8* name,
                                             const UTF8* type, bool isStatic,
                                             bool recurse) {

  FieldCmp CC(name, type);
  field_map& map = isStatic ? staticFields : virtualFields;
  field_iterator End = map.end();
  field_iterator I = map.find(CC);
  if (I != End) return I->second;
  
  JavaField *cur = 0;

  if (recurse) {
    if (super) cur = super->lookupFieldDontThrow(name, type, isStatic,
                                                 recurse);
    if (cur) return cur;
    if (isStatic) {
      for (std::vector<Class*>::iterator i = interfaces.begin(),
           e = interfaces.end(); i!= e; i++) {
        cur = (*i)->lookupFieldDontThrow(name, type, isStatic, recurse);
        if (cur) return cur;
      }
    }
  }

  return 0;
}

JavaField* CommonClass::lookupField(const UTF8* name, const UTF8* type,
                                    bool isStatic, bool recurse) {
  
  JavaField* res = lookupFieldDontThrow(name, type, isStatic, recurse);
  if (!res) {
    JavaThread::get()->isolate->error(Jnjvm::NoSuchFieldError, 
                                      "unable to find %s in %s",
                                      name->printString(), this->printString());
  }
  return res;
}

JavaObject* Class::doNew(Jnjvm* vm) {
  JavaObject* res = (JavaObject*)vm->allocateObject(virtualSize, virtualVT);
  res->classOf = this;
  return res;
}

bool CommonClass::inheritName(const UTF8* Tname) {
  if (name->equals(Tname)) {
    return true;
  } else  if (isPrimitive) {
    return false;
  } else if (super) {
    if (super->inheritName(Tname)) return true;
  }
  
  for (uint32 i = 0; i < interfaces.size(); ++i) {
    if (interfaces[i]->inheritName(Tname)) return true;
  }
  return false;
}

bool CommonClass::isOfTypeName(const UTF8* Tname) {
  if (inheritName(Tname)) {
    return true;
  } else if (isArray) {
    CommonClass* curS = this;
    uint32 prof = 0;
    uint32 len = Tname->size;
    bool res = true;
    
    while (res && Tname->elements[prof] == AssessorDesc::I_TAB) {
      CommonClass* cl = ((ClassArray*)curS)->baseClass();
      Jnjvm *vm = cl->isolate;
      ++prof;
      vm->resolveClass(cl, false);
      res = curS->isArray && cl && (prof < len);
      curS = cl;
    }
    
    Jnjvm *vm = this->isolate;
    return (Tname->elements[prof] == AssessorDesc::I_REF) &&  
      (res && curS->inheritName(Tname->extract(vm, prof + 1, len - 1)));
  } else {
    return false;
  }
}

bool CommonClass::implements(CommonClass* cl) {
  if (this == cl) return true;
  else {
    for (std::vector<Class*>::iterator i = interfaces.begin(),
         e = interfaces.end(); i!= e; i++) {
      if (*i == cl) return true;
      else if ((*i)->implements(cl)) return true;
    }
    if (super) {
      return super->implements(cl);
    }
  }
  return false;
}

bool CommonClass::instantiationOfArray(ClassArray* cl) {
  if (this == cl) return true;
  else {
    if (isArray) {
      CommonClass* baseThis = ((ClassArray*)this)->baseClass();
      CommonClass* baseCl = ((ClassArray*)cl)->baseClass();

      if (isInterface(baseThis->access) && isInterface(baseCl->access)) {
        return baseThis->implements(baseCl);
      } else {
        return baseThis->isAssignableFrom(baseCl);
      }
    }
  }
  return false;
}

bool CommonClass::subclassOf(CommonClass* cl) {
  if (cl->depth <= depth) {
    return display[cl->depth] == cl;
  } else {
    return false;
  }
}

bool CommonClass::isAssignableFrom(CommonClass* cl) {
  if (this == cl) {
    return true;
  } else if (isInterface(cl->access)) {
    return this->implements(cl);
  } else if (cl->isArray) {
    return this->instantiationOfArray((ClassArray*)cl);
  } else {
    return this->subclassOf(cl);
  }
}

void JavaField::initField(JavaObject* obj) {
  const AssessorDesc* funcs = getSignature()->funcs;
  Attribut* attribut = lookupAttribut(Attribut::constantAttribut);

  if (!attribut) {
    JnjvmModule::InitField(this, obj);
  } else {
    Reader reader(attribut, classDef->bytes);
    JavaCtpInfo * ctpInfo = classDef->ctpInfo;
    uint16 idx = reader.readU2();
    if (funcs == AssessorDesc::dLong) {
      JnjvmModule::InitField(this, obj, (uint64)ctpInfo->LongAt(idx));
    } else if (funcs == AssessorDesc::dDouble) {
      JnjvmModule::InitField(this, obj, ctpInfo->DoubleAt(idx));
    } else if (funcs == AssessorDesc::dFloat) {
      JnjvmModule::InitField(this, obj, ctpInfo->FloatAt(idx));
    } else if (funcs == AssessorDesc::dRef) {
      const UTF8* utf8 = ctpInfo->UTF8At(ctpInfo->ctpDef[idx]);
      JnjvmModule::InitField(this, obj,
                         (JavaObject*)ctpInfo->resolveString(utf8, idx));
    } else if (funcs == AssessorDesc::dInt || funcs == AssessorDesc::dChar ||
               funcs == AssessorDesc::dShort || funcs == AssessorDesc::dByte ||
               funcs == AssessorDesc::dBool) {
      JnjvmModule::InitField(this, obj, (uint64)ctpInfo->IntegerAt(idx));
    } else {
      JavaThread::get()->isolate->
        unknownError("unknown constant %c", funcs->byteId);
    }
  }
  
}

JavaObject* CommonClass::getClassDelegatee() {
  return JavaThread::get()->isolate->getClassDelegatee(this);
}

void CommonClass::resolveClass(bool doClinit) {
  isolate->resolveClass(this, doClinit);
}

void CommonClass::initialiseClass() {
  return isolate->initialiseClass(this);
}

#ifdef MULTIPLE_VM
JavaObject* Class::staticInstance() {
  std::pair<JavaState, JavaObject*>* val = 
    JavaThread::get()->isolate->statics->lookup(this);
  assert(val);
  return val->second;
}

void Class::createStaticInstance() {
  JavaObject* val = 
    (JavaObject*)JavaThread::get()->isolate->allocateObject(staticSize,
                                                            staticVT);
  val->initialise(this);
  for (std::vector<JavaField*>::iterator i = this->staticFields.begin(),
            e = this->staticFields.end(); i!= e; ++i) {
    
    (*i)->initField(val);
  }
  
  Jnjvm* vm = JavaThread::get()->isolate;
  std::pair<JavaState, JavaObject*>* p = vm->statics->lookup(this);
  assert(p);
  assert(!p->second);
  p->second = val;
}

JavaState* CommonClass::getStatus() {
  if (!this->isArray && 
      !this->isPrimitive) {
    Class* cl = (Class*)this;
    Jnjvm* vm = JavaThread::get()->isolate;
    std::pair<JavaState, JavaObject*>* val = vm->statics->lookup(cl);
    if (!val) {
      val = new std::pair<JavaState, JavaObject*>(status, 0);
      JavaThread::get()->isolate->statics->hash(cl, val);
    }
    if (val->first < status) val->first = status;
    return (JavaState*)&(val->first);
  } else {
    return &status;
  }
}
#endif


JavaMethod* CommonClass::constructMethod(const UTF8* name,
                                         const UTF8* type, uint32 access) {
  method_map& map = isStatic(access) ? staticMethods : virtualMethods;
  FieldCmp CC(name, type);
  method_iterator End = map.end();
  method_iterator I = map.find(CC);
  if (I == End) {
    JavaMethod* method = new JavaMethod();
    method->name = name;
    method->type = type;
    method->classDef = (Class*)this;
    method->_signature = 0;
    method->code = 0;
    method->access = access;
    method->canBeInlined = false;
    method->offset = 0;
    map.insert(std::make_pair(CC, method));
    return method;
  } else {
    return I->second;
  }
}

JavaField* CommonClass::constructField(const UTF8* name,
                                       const UTF8* type, uint32 access) {
  field_map& map = isStatic(access) ? staticFields : virtualFields;
  FieldCmp CC(name, type);
  field_iterator End = map.end();
  field_iterator I = map.find(CC);
  if (I == End) {
    JavaField* field = new JavaField();
    field->name = name;
    field->type = type;
    field->classDef = (Class*)this;
    field->_signature = 0;
    field->ptrOffset = 0;
    field->access = access;
    map.insert(std::make_pair(CC, field));
    return field;
  } else {
    return I->second;
  }
}

