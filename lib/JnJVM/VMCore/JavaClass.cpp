//===-------- JavaClass.cpp - Java class representation -------------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#define JNJVM_LOAD 0

#include <vector>

#include <string.h>

#include "mvm/JIT.h"
#include "debug.h"
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
#include "LockedMap.h"
#include "Reader.h"

using namespace jnjvm;

const UTF8* Attribut::codeAttribut = 0;
const UTF8* Attribut::exceptionsAttribut = 0;
const UTF8* Attribut::constantAttribut = 0;
const UTF8* Attribut::lineNumberTableAttribut = 0;
const UTF8* Attribut::innerClassesAttribut = 0;
const UTF8* Attribut::sourceFileAttribut = 0;

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

bool CommonClass::FieldCmp::operator<(const CommonClass::FieldCmp &cmp) const {
  if (name->lessThan(cmp.name)) return true;
  else if (cmp.name->lessThan(name)) return false;
  else return type->lessThan(cmp.type);
}


CommonClass::~CommonClass() {
  free(display);
  free(virtualVT);
  delete lockVar;
  delete condVar;
}

CommonClass::CommonClass() {
  display = 0;
  lockVar = 0;
  condVar = 0;
  virtualVT = 0;
  JInfo = 0;
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

CommonClass::CommonClass(JnjvmClassLoader* loader, const UTF8* n, bool isArray) {
  name = n;
  this->lockVar = mvm::Lock::allocRecursive();
  this->condVar = mvm::Cond::allocCond();
  this->status = hashed;
  this->classLoader = loader;
  this->isArray = isArray;
  this->isPrimitive = false;
#ifndef MULTIPLE_VM
  this->delegatee = 0;
#endif
}

ClassPrimitive::ClassPrimitive(JnjvmClassLoader* loader, const UTF8* n) : 
  CommonClass(loader, n, false) {
  
  display = (CommonClass**)malloc(sizeof(CommonClass*));
  display[0] = this;
  isPrimitive = true;
  status = ready;
  access = ACC_ABSTRACT | ACC_FINAL | ACC_PUBLIC;
}

Class::Class(JnjvmClassLoader* loader, const UTF8* n) : CommonClass(loader, n, false) {
  bytes = 0;
  super = 0;
  ctpInfo = 0;
#ifndef MULTIPLE_VM
  _staticInstance = 0;
#endif
}

ClassArray::ClassArray(JnjvmClassLoader* loader, const UTF8* n) : CommonClass(loader, n, true) {
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
  AssessorDesc::introspectArray(classLoader, name, 0, _funcs,
                                _baseClass);
}

JnjvmClassLoader* ClassArray::arrayLoader(const UTF8* name,
                                          JnjvmClassLoader* loader,
                                          unsigned int start,
                                          unsigned int len) {
  
  if (name->elements[start] == AssessorDesc::I_TAB) {
    return arrayLoader(name, loader, start + 1, len - 1);
  } else if (name->elements[start] == AssessorDesc::I_REF) {
    const UTF8* componentName = name->javaToInternal(loader->hashUTF8, start + 1,
                                                     len - 2);
    CommonClass* cl = loader->loadName(componentName, false, true);
    return cl->classLoader;
  } else {
    return JnjvmClassLoader::bootstrapLoader;
  }
}

void* JavaMethod::compiledPtr() {
  if (code != 0) return code;
  else {
    classDef->acquire();
    if (code == 0) {
      code = 
        classDef->classLoader->TheModuleProvider->materializeFunction(this);
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
    JavaThread::get()->isolate->noSuchMethodError(this, name);
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
    JavaThread::get()->isolate->noSuchFieldError(this, name);
  }
  return res;
}

JavaObject* Class::doNew(Jnjvm* vm) {
  assert(this->isReady() && "Uninitialized class when allocating.");
  JavaObject* res = (JavaObject*)vm->allocator.allocateObject(virtualSize, virtualVT);
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
      ++prof;
      cl->resolveClass();
      res = curS->isArray && cl && (prof < len);
      curS = cl;
    }
    
    return (Tname->elements[prof] == AssessorDesc::I_REF) &&  
      (res && curS->inheritName(Tname->extract(classLoader->hashUTF8, prof + 1, len - 1)));
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
    JavaConstantPool * ctpInfo = classDef->ctpInfo;
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

JavaObject* CommonClass::getClassDelegatee(JavaObject* pd) {
  return JavaThread::get()->isolate->getClassDelegatee(this, pd);
}

#ifdef MULTIPLE_VM
JavaObject* Class::staticInstance() {
  std::pair<JavaState, JavaObject*>* val = 
    JavaThread::get()->isolate->statics->lookup(this);
  assert(val);
  return val->second;
}

void Class::createStaticInstance() {
  JavaAllocator* allocator = &(JavaThread::get()->isolate->allocator);
  JavaObject* val = 
    (JavaObject*)allocator->allocateObject(staticSize, staticVT);

  val->initialise(this);
  for (field_iterator i = this->staticFields.begin(),
            e = this->staticFields.end(); i!= e; ++i) {
    
    JavaField* cur = i->second;
    cur->initField(val);
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
    method->JInfo = 0;
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
    field->JInfo = 0;
    map.insert(std::make_pair(CC, field));
    return field;
  } else {
    return I->second;
  }
}

void Class::readParents(Reader& reader) {
  unsigned short int superEntry = reader.readU2();
  const UTF8* super = superEntry ? 
        ctpInfo->resolveClassName(superEntry) : 0;

  unsigned short int nbI = reader.readU2();
  superUTF8 = super;
  for (int i = 0; i < nbI; i++)
    interfacesUTF8.push_back(ctpInfo->resolveClassName(reader.readU2()));

}

void Class::loadParents() {
  int nbI = interfacesUTF8.size();
  if (superUTF8 == 0) {
    depth = 0;
    display = (CommonClass**)malloc(sizeof(CommonClass*));
    display[0] = this;
    virtualTableSize = VT_SIZE / sizeof(void*);
  } else {
    super = classLoader->loadName(superUTF8, true, true);
    depth = super->depth + 1;
    virtualTableSize = super->virtualTableSize;
    display = (CommonClass**)malloc((depth + 1) * sizeof(CommonClass*));
    memcpy(display, super->display, depth * sizeof(CommonClass*));
    display[depth] = this;
  }

  for (int i = 0; i < nbI; i++)
    interfaces.push_back((Class*)classLoader->loadName(interfacesUTF8[i],
                                                       true, true));
}

void Class::readAttributs(Reader& reader, std::vector<Attribut*>& attr) {
  unsigned short int nba = reader.readU2();
  
  for (int i = 0; i < nba; i++) {
    const UTF8* attName = ctpInfo->UTF8At(reader.readU2());
    uint32 attLen = reader.readU4();
    Attribut* att = new Attribut(attName, attLen, reader.cursor);
    attr.push_back(att);
    reader.seek(attLen, Reader::SeekCur);
  }
}

void Class::readFields(Reader& reader) {
  uint16 nbFields = reader.readU2();
  uint32 sindex = 0;
  uint32 vindex = 0;
  for (int i = 0; i < nbFields; i++) {
    uint16 access = reader.readU2();
    const UTF8* name = ctpInfo->UTF8At(reader.readU2());
    const UTF8* type = ctpInfo->UTF8At(reader.readU2());
    JavaField* field = constructField(name, type, access);
    isStatic(access) ?
      field->num = sindex++ :
      field->num = vindex++;
    readAttributs(reader, field->attributs);
  }
}

void Class::readMethods(Reader& reader) {
  uint16 nbMethods = reader.readU2();
  for (int i = 0; i < nbMethods; i++) {
    uint16 access = reader.readU2();
    const UTF8* name = ctpInfo->UTF8At(reader.readU2());
    const UTF8* type = ctpInfo->UTF8At(reader.readU2());
    JavaMethod* meth = constructMethod(name, type, access);
    readAttributs(reader, meth->attributs);
  }
}

void Class::readClass() {

  PRINT_DEBUG(JNJVM_LOAD, 0, COLOR_NORMAL, "; ", 0);
  PRINT_DEBUG(JNJVM_LOAD, 0, LIGHT_GREEN, "reading ", 0);
  PRINT_DEBUG(JNJVM_LOAD, 0, COLOR_NORMAL, "%s\n", printString());

  Reader reader(bytes);
  uint32 magic = reader.readU4();
  if (magic != Jnjvm::Magic) {
    JavaThread::get()->isolate->classFormatError("bad magic number %p", magic);
  }
  minor = reader.readU2();
  major = reader.readU2();
  ctpInfo = new JavaConstantPool(this, reader);
  access = reader.readU2();
  
  const UTF8* thisClassName = 
    ctpInfo->resolveClassName(reader.readU2());
  
  if (!(thisClassName->equals(name))) {
    JavaThread::get()->isolate->classFormatError("try to load %s and found class named %s",
          printString(), thisClassName->printString());
  }

  readParents(reader);
  readFields(reader);
  readMethods(reader);
  readAttributs(reader, attributs);
}

void CommonClass::resolveClass() {
  if (status < resolved) {
    acquire();
    if (status >= resolved) {
      release();
    } else if (status <  loaded) {
      release();
      JavaThread::get()->isolate->unknownError("try to resolve a not-read class");
    } else if (status == loaded || ownerClass()) {
      if (isArray) {
        ClassArray* arrayCl = (ClassArray*)this;
        CommonClass* baseClass =  arrayCl->baseClass();
        baseClass->resolveClass();
        status = resolved;
      // Primitives are resolved at boot time
      } else {
        Class* cl = (Class*)this;
        cl->readClass();
        cl->status = classRead;
        cl->release();
        cl->loadParents();
        cl->acquire();
        cl->status = prepared;
        classLoader->TheModule->resolveVirtualClass(cl);
        cl->status = resolved;
      }
      release();
      broadcastClass();
    } else {
      while (status < resolved) {
        waitClass();
      }
      release();
    }
  }
}
