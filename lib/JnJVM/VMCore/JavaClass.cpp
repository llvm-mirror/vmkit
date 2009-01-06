//===-------- JavaClass.cpp - Java class representation -------------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#define JNJVM_LOAD 0

#include "debug.h"
#include "types.h"

#include "ClasspathReflect.h"
#include "JavaArray.h"
#include "JavaCache.h"
#include "JavaClass.h"
#include "JavaConstantPool.h"
#include "JavaObject.h"
#include "JavaThread.h"
#include "JavaTypes.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"
#include "JnjvmModule.h"
#include "JnjvmModuleProvider.h"
#include "LockedMap.h"
#include "Reader.h"

#include <cstring>
#include <vector>

using namespace jnjvm;

const UTF8* Attribut::codeAttribut = 0;
const UTF8* Attribut::exceptionsAttribut = 0;
const UTF8* Attribut::constantAttribut = 0;
const UTF8* Attribut::lineNumberTableAttribut = 0;
const UTF8* Attribut::innerClassesAttribut = 0;
const UTF8* Attribut::sourceFileAttribut = 0;

Class* ClassArray::SuperArray;
Class** ClassArray::InterfacesArray;

extern void* JavaArrayVT[];
extern void* ArrayObjectVT[];

Attribut::Attribut(const UTF8* name, uint32 length,
                   uint32 offset) {
  
  this->start    = offset;
  this->nbb      = length;
  this->name     = name;

}

Attribut* Class::lookupAttribut(const UTF8* key ) {
  for (uint32 i = 0; i < nbAttributs; ++i) {
    Attribut* cur = &(attributs[i]);
    if (cur->name->equals(key)) return cur;
  }

  return 0;
}

Attribut* JavaField::lookupAttribut(const UTF8* key ) {
  for (uint32 i = 0; i < nbAttributs; ++i) {
    Attribut* cur = &(attributs[i]);
    if (cur->name->equals(key)) return cur;
  }

  return 0;
}

Attribut* JavaMethod::lookupAttribut(const UTF8* key ) {
  for (uint32 i = 0; i < nbAttributs; ++i) {
    Attribut* cur = &(attributs[i]);
    if (cur->name->equals(key)) return cur;
  }

  return 0;
}

CommonClass::~CommonClass() {
  classLoader->allocator.Deallocate(display);
}

CommonClass::CommonClass() {
  display = 0;
  nbInterfaces = 0;
  access = 0;
}

Class::Class() : CommonClass() {
  virtualVT = 0;
  ctpInfo = 0;
  JInfo = 0;
  outerClass = 0;
  innerOuterResolved = false;
  nbInnerClasses = 0;
  nbVirtualFields = 0;
  nbStaticFields = 0;
  nbVirtualMethods = 0;
  nbStaticMethods = 0;
  ownerClass = 0;
}

Class::~Class() {
  for (uint32 i = 0; i < nbAttributs; ++i) {
    Attribut* cur = &(attributs[i]);
    cur->~Attribut();
    classLoader->allocator.Deallocate(cur);
  }
  
  for (uint32 i = 0; i < nbStaticFields; ++i) {
    JavaField* cur = &(staticFields[i]);
    cur->~JavaField();
    classLoader->allocator.Deallocate(cur);
  }
  
  for (uint32 i = 0; i < nbVirtualFields; ++i) {
    JavaField* cur = &(virtualFields[i]);
    cur->~JavaField();
    classLoader->allocator.Deallocate(cur);
  }
  
  for (uint32 i = 0; i < nbVirtualMethods; ++i) {
    JavaMethod* cur = &(virtualMethods[i]);
    cur->~JavaMethod();
    classLoader->allocator.Deallocate(cur);
  }
  
  for (uint32 i = 0; i < nbStaticMethods; ++i) {
    JavaMethod* cur = &(staticMethods[i]);
    cur->~JavaMethod();
    classLoader->allocator.Deallocate(cur);
  }
 
  if (ctpInfo) {
    ctpInfo->~JavaConstantPool();
    classLoader->allocator.Deallocate(ctpInfo);
  }

  classLoader->allocator.Deallocate(IsolateInfo);
  
  // Currently, only regular classes have a heap allocated virtualVT.
  // Array classes have a C++ allocated virtualVT and primitive classes
  // do not have a virtualVT.
  classLoader->allocator.Deallocate(virtualVT);
}

JavaField::~JavaField() {
  for (uint32 i = 0; i < nbAttributs; ++i) {
    Attribut* cur = &(attributs[i]);
    cur->~Attribut();
    classDef->classLoader->allocator.Deallocate(cur);
  }
}

JavaMethod::~JavaMethod() {
  
  for (uint32 i = 0; i < nbAttributs; ++i) {
    Attribut* cur = &(attributs[i]);
    cur->~Attribut();
    classDef->classLoader->allocator.Deallocate(cur);
  }
  
  for (uint32 i = 0; i < nbEnveloppes; ++i) {
    Enveloppe* cur = &(enveloppes[i]);
    cur->~Enveloppe();
    classDef->classLoader->allocator.Deallocate(cur);
  }
}

static void printClassNameIntern(const UTF8* name, unsigned int start,
                                 unsigned int end,  mvm::PrintBuffer* buf) {
  
  uint16 first = name->elements[start];
  if (first == I_TAB) {
    unsigned int stepsEnd = start;
    while (name->elements[stepsEnd] == I_TAB) stepsEnd++;
    if (name->elements[stepsEnd] == I_REF) {
      printClassNameIntern(name, (stepsEnd + 1),(end - 1), buf);
    } else {
      name->printUTF8(buf);
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
  printClassName(name, buf);
}

UserClassPrimitive* CommonClass::toPrimitive(Jnjvm* vm) const {
  if (this == vm->upcalls->voidClass) {
    return vm->upcalls->OfVoid;
  } else if (this == vm->upcalls->intClass) {
    return vm->upcalls->OfInt;
  } else if (this == vm->upcalls->shortClass) {
    return vm->upcalls->OfShort;
  } else if (this == vm->upcalls->charClass) {
    return vm->upcalls->OfChar;
  } else if (this == vm->upcalls->doubleClass) {
    return vm->upcalls->OfDouble;
  } else if (this == vm->upcalls->byteClass) {
    return vm->upcalls->OfByte;
  } else if (this == vm->upcalls->boolClass) {
    return vm->upcalls->OfBool;
  } else if (this == vm->upcalls->longClass) {
    return vm->upcalls->OfLong;
  } else if (this == vm->upcalls->floatClass) {
    return vm->upcalls->OfFloat;
  } else {
    return 0;
  }
}


UserClassPrimitive* 
ClassPrimitive::byteIdToPrimitive(char id, Classpath* upcalls) {
  switch (id) {
    case I_FLOAT :
      return upcalls->OfFloat;
    case I_INT :
      return upcalls->OfInt;
    case I_SHORT :
      return upcalls->OfShort;
    case I_CHAR :
      return upcalls->OfChar;
    case I_DOUBLE :
      return upcalls->OfDouble;
    case I_BYTE :
      return upcalls->OfByte;
    case I_BOOL :
      return upcalls->OfBool;
    case I_LONG :
      return upcalls->OfLong;
    case I_VOID :
      return upcalls->OfVoid;
    default :
      return 0;
  }
}

CommonClass::CommonClass(JnjvmClassLoader* loader, const UTF8* n) {
  name = n;
  classLoader = loader;
  nbInterfaces = 0;
  interfaces = 0;
  access = 0;
  super = 0;
  memset(delegatee, 0, sizeof(JavaObject*) * NR_ISOLATES);
}

ClassPrimitive::ClassPrimitive(JnjvmClassLoader* loader, const UTF8* n,
                               uint32 nb) : 
  CommonClass(loader, n) {
 
  display = (CommonClass**)loader->allocator.Allocate(sizeof(CommonClass*));
  display[0] = this;
  depth = 0;
  access = ACC_ABSTRACT | ACC_FINAL | ACC_PUBLIC | JNJVM_PRIMITIVE;
  primSize = nb;
}

Class::Class(JnjvmClassLoader* loader, const UTF8* n, ArrayUInt8* B) : 
    CommonClass(loader, n) {
  virtualVT = 0;
  bytes = B;
  super = 0;
  ctpInfo = 0;
  JInfo = 0;
  outerClass = 0;
  innerOuterResolved = false;
  display = 0;
  nbInnerClasses = 0;
  staticTracer = 0;
  nbVirtualMethods = 0;
  nbStaticMethods = 0;
  nbStaticFields = 0;
  nbVirtualFields = 0;
  virtualMethods = 0;
  staticMethods = 0;
  virtualFields = 0;
  staticFields = 0;
  ownerClass = 0;
  innerAccess = 0;
  access = JNJVM_CLASS;
  memset(IsolateInfo, 0, sizeof(TaskClassMirror) * NR_ISOLATES);
}

ClassArray::ClassArray(JnjvmClassLoader* loader, const UTF8* n,
                       UserCommonClass* base) : 
    CommonClass(loader, n) {
  _baseClass = base;
  super = ClassArray::SuperArray;
  interfaces = ClassArray::InterfacesArray;
  nbInterfaces = 2;
  depth = 1;
  display = (CommonClass**)loader->allocator.Allocate(2 * sizeof(CommonClass*));
  display[0] = ClassArray::SuperArray;
  display[1] = this;
  access = ACC_FINAL | ACC_ABSTRACT | ACC_PUBLIC | JNJVM_ARRAY;
}

JavaArray* UserClassArray::doNew(sint32 n, Jnjvm* vm) {
  if (n < 0)
    vm->negativeArraySizeException(n);
  else if (n > JavaArray::MaxArraySize)
    vm->outOfMemoryError(n);

  return doNew(n, vm->gcAllocator);
}

JavaArray* UserClassArray::doNew(sint32 n, mvm::Allocator& allocator) {
  UserCommonClass* cl = baseClass();

  uint32 primSize = cl->isPrimitive() ? 
    cl->asPrimitiveClass()->primSize : sizeof(JavaObject*);
  VirtualTable* VT = cl->isPrimitive() ? JavaArrayVT : ArrayObjectVT;
  uint32 size = sizeof(JavaObject) + sizeof(ssize_t) + n * primSize;
  JavaArray* res = (JavaArray*)allocator.allocateManagedObject(size, VT);
  res->initialise(this);
  res->size = n;
  return res;
}

JavaArray* UserClassArray::doNew(sint32 n, mvm::BumpPtrAllocator& allocator) {
  UserCommonClass* cl = baseClass();

  uint32 primSize = cl->isPrimitive() ? 
    cl->asPrimitiveClass()->primSize : sizeof(JavaObject*);
  VirtualTable* VT = cl->isPrimitive() ? JavaArrayVT : ArrayObjectVT;
  uint32 size = sizeof(JavaObject) + sizeof(ssize_t) + n * primSize;
  
  JavaArray* res = (JavaArray*)allocator.Allocate(size);
  ((void**)res)[0] = VT;
  res->initialise(this);
  res->size = n;
  return res;
}

void* JavaMethod::compiledPtr() {
  if (code != 0) return code;
  else {
#ifdef SERVICE
    Jnjvm *vm = classDef->classLoader->getIsolate();
    if (vm && vm->status == 0) {
      JavaThread* th = JavaThread::get();
      th->throwException(th->ServiceException);
    }
#endif
    classDef->acquire();
    if (code == 0) {
      code = 
        classDef->classLoader->getModuleProvider()->materializeFunction(this);
      Jnjvm* vm = JavaThread::get()->getJVM();
      vm->addMethodInFunctionMap(this, code);
    }
    classDef->release();
    return code;
  }
}

void JavaMethod::setCompiledPtr(void* ptr, const char* name) {
  classDef->acquire();
  if (code == 0) {
    code = ptr;
    Jnjvm* vm = JavaThread::get()->getJVM();
    vm->addMethodInFunctionMap(this, code);
    classDef->classLoader->getModule()->setMethod(this, ptr, name);
  }
  access |= ACC_NATIVE;
  classDef->release();
}

const char* JavaMethod::printString() const {
  mvm::PrintBuffer *buf= mvm::PrintBuffer::alloc();
  buf->write("JavaMethod<");
  fprintf(stderr, "name = %s\n", name->printString());
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
  name->printUTF8(buf);
  buf->write(">");
  return buf->contents()->cString();
}

JavaMethod* Class::lookupInterfaceMethodDontThrow(const UTF8* name,
                                                  const UTF8* type) {
  JavaMethod* cur = lookupMethodDontThrow(name, type, false, false, 0);
  if (!cur) {
    for (uint16 i = 0; i < nbInterfaces; ++i) {
      Class* I = interfaces[i];
      cur = I->lookupInterfaceMethodDontThrow(name, type);
      if (cur) return cur;
    }
  }
  return cur;
}

JavaMethod* Class::lookupMethodDontThrow(const UTF8* name,
                                               const UTF8* type,
                                               bool isStatic,
                                               bool recurse,
                                               Class** methodCl) {
  
  JavaMethod* methods = 0;
  uint32 nb = 0;
  if (isStatic) {
    methods = getStaticMethods();
    nb = nbStaticMethods;
  } else {
    methods = getVirtualMethods();
    nb = nbVirtualMethods;
  }
  
  for (uint32 i = 0; i < nb; ++i) {
    JavaMethod& res = methods[i];
    if (res.name->equals(name) && res.type->equals(type)) {
      if (methodCl) *methodCl = (Class*)this;
      return &res;
    }
  }

  JavaMethod *cur = 0;
  
  if (recurse) {
    if (super) cur = super->lookupMethodDontThrow(name, type, isStatic,
                                                  recurse, methodCl);
    if (cur) return cur;
    if (isStatic) {
      for (uint16 i = 0; i < nbInterfaces; ++i) {
        Class* I = interfaces[i];
        cur = I->lookupMethodDontThrow(name, type, isStatic, recurse,
                                       methodCl);
        if (cur) return cur;
      }
    }
  }

  return 0;
}

JavaMethod* Class::lookupMethod(const UTF8* name, const UTF8* type,
                                      bool isStatic, bool recurse,
                                      Class** methodCl) {
  JavaMethod* res = lookupMethodDontThrow(name, type, isStatic, recurse,
                                          methodCl);
  if (!res) {
    JavaThread::get()->getJVM()->noSuchMethodError(this, name);
  }
  return res;
}

JavaField*
Class::lookupFieldDontThrow(const UTF8* name, const UTF8* type,
                                  bool isStatic, bool recurse,
                                  Class** definingClass) {
  JavaField* fields = 0;
  uint32 nb = 0;
  if (isStatic) {
    fields = getStaticFields();
    nb = nbStaticFields;
  } else {
    fields = getVirtualFields();
    nb = nbVirtualFields;
  }
  
  for (uint32 i = 0; i < nb; ++i) {
    JavaField& res = fields[i];
    if (res.name->equals(name) && res.type->equals(type)) {
      if (definingClass) *definingClass = this;
      return &res;
    }
  }

  JavaField *cur = 0;

  if (recurse) {
    if (super) cur = super->lookupFieldDontThrow(name, type, isStatic,
                                                 recurse, definingClass);
    if (cur) return cur;
    if (isStatic) {
      for (uint16 i = 0; i < nbInterfaces; ++i) {
        Class* I = interfaces[i];
        cur = I->lookupFieldDontThrow(name, type, isStatic, recurse,
                                      definingClass);
        if (cur) return cur;
      }
    }
  }

  return 0;
}

JavaField* Class::lookupField(const UTF8* name, const UTF8* type,
                                    bool isStatic, bool recurse,
                                    Class** definingClass) {
  
  JavaField* res = lookupFieldDontThrow(name, type, isStatic, recurse,
                                        definingClass);
  if (!res) {
    JavaThread::get()->getJVM()->noSuchFieldError(this, name);
  }
  return res;
}

JavaObject* UserClass::doNew(Jnjvm* vm) {
  assert(this && "No class when allocating.");
  assert((this->isInitializing() || 
          classLoader->getModule()->isStaticCompiling())
         && "Uninitialized class when allocating.");
  JavaObject* res = 
    (JavaObject*)vm->gcAllocator.allocateManagedObject(getVirtualSize(),
                                                       getVirtualVT());
  res->initialise(this);
  return res;
}

bool UserCommonClass::inheritName(const UTF8* Tname) {
  if (getName()->equals(Tname)) {
    return true;
  } else  if (isPrimitive()) {
    return false;
  } else if (super) {
    if (getSuper()->inheritName(Tname)) return true;
  }
  
  for (uint32 i = 0; i < nbInterfaces; ++i) {
    if (interfaces[i]->inheritName(Tname)) return true;
  }
  return false;
}

bool UserCommonClass::isOfTypeName(Jnjvm* vm, const UTF8* Tname) {
  if (inheritName(Tname)) {
    return true;
  } else if (isArray()) {
    UserCommonClass* curS = this;
    uint32 prof = 0;
    uint32 len = Tname->size;
    bool res = true;
    
    while (res && Tname->elements[prof] == I_TAB) {
      UserCommonClass* cl = ((UserClassArray*)curS)->baseClass();
      ++prof;
      if (cl->isClass()) cl->asClass()->resolveClass();
      res = curS->isArray() && cl && (prof < len);
      curS = cl;
    }
    
    return (Tname->elements[prof] == I_REF) &&  
      (res && curS->inheritName(Tname->extract(vm, prof + 1, len - 1)));
  } else {
    return false;
  }
}

bool UserCommonClass::implements(UserCommonClass* cl) {
  if (this == cl) return true;
  else {
    for (uint16 i = 0; i < nbInterfaces; ++i) {
      Class* I = interfaces[i];
      if (I == cl) return true;
      else if (I->implements(cl)) return true;
    }
    if (super) {
      return super->implements(cl);
    }
  }
  return false;
}

bool UserCommonClass::instantiationOfArray(UserClassArray* cl) {
  if (this == cl) return true;
  else {
    if (isArray()) {
      UserCommonClass* baseThis = ((UserClassArray*)this)->baseClass();
      UserCommonClass* baseCl = ((UserClassArray*)cl)->baseClass();

      if (baseThis->isInterface() && baseCl->isInterface()) {
        return baseThis->implements(baseCl);
      } else {
        return baseThis->isAssignableFrom(baseCl);
      }
    }
  }
  return false;
}

bool UserCommonClass::subclassOf(UserCommonClass* cl) {
  if (cl->depth <= depth) {
    return display[cl->depth] == cl;
  } else {
    return false;
  }
}

bool UserCommonClass::isAssignableFrom(UserCommonClass* cl) {
  if (this == cl) {
    return true;
  } else if (cl->isInterface()) {
    return this->implements(cl);
  } else if (cl->isArray()) {
    return this->instantiationOfArray((UserClassArray*)cl);
  } else {
    return this->subclassOf(cl);
  }
}

void JavaField::InitField(void* obj, uint64 val) {
  
  Typedef* type = getSignature();
  if (!type->isPrimitive()) {
    ((sint32*)((uint64)obj + ptrOffset))[0] = (sint32)val;
    return;
  }

  PrimitiveTypedef* prim = (PrimitiveTypedef*)type;
  if (prim->isLong()) {
    ((sint64*)((uint64)obj + ptrOffset))[0] = val;
  } else if (prim->isInt()) {
    ((sint32*)((uint64)obj + ptrOffset))[0] = (sint32)val;
  } else if (prim->isChar()) {
    ((uint16*)((uint64)obj + ptrOffset))[0] = (uint16)val;
  } else if (prim->isShort()) {
    ((sint16*)((uint64)obj + ptrOffset))[0] = (sint16)val;
  } else if (prim->isByte()) {
    ((sint8*)((uint64)obj + ptrOffset))[0] = (sint8)val;
  } else if (prim->isBool()) {
    ((uint8*)((uint64)obj + ptrOffset))[0] = (uint8)val;
  } else {
    // 0 value for everything else
    ((sint32*)((uint64)obj + ptrOffset))[0] = (sint32)val;
  }
}

void JavaField::InitField(void* obj, JavaObject* val) {
  ((JavaObject**)((uint64)obj + ptrOffset))[0] = val;
}

void JavaField::InitField(void* obj, double val) {
  ((double*)((uint64)obj + ptrOffset))[0] = val;
}

void JavaField::InitField(void* obj, float val) {
  ((float*)((uint64)obj + ptrOffset))[0] = val;
}

void JavaField::initField(void* obj, Jnjvm* vm) {
  const Typedef* type = getSignature();
  Attribut* attribut = lookupAttribut(Attribut::constantAttribut);

  if (!attribut) {
    InitField(obj);
  } else {
    Reader reader(attribut, classDef->bytes);
    JavaConstantPool * ctpInfo = classDef->ctpInfo;
    uint16 idx = reader.readU2();
    if (type->isPrimitive()) {
      UserCommonClass* cl = type->assocClass(vm->bootstrapLoader);
      if (cl == vm->upcalls->OfLong) {
        InitField(obj, (uint64)ctpInfo->LongAt(idx));
      } else if (cl == vm->upcalls->OfDouble) {
        InitField(obj, ctpInfo->DoubleAt(idx));
      } else if (cl == vm->upcalls->OfFloat) {
        InitField(obj, ctpInfo->FloatAt(idx));
      } else {
        InitField(obj, (uint64)ctpInfo->IntegerAt(idx));
      }
    } else if (type->isReference()){
      const UTF8* utf8 = ctpInfo->UTF8At(ctpInfo->ctpDef[idx]);
      InitField(obj, (JavaObject*)ctpInfo->resolveString(utf8, idx));
    } else {
      JavaThread::get()->getJVM()->
        unknownError("unknown constant %s\n", type->printString());
    }
  } 
}

void* UserClass::allocateStaticInstance(Jnjvm* vm) {
  void* val = classLoader->allocator.Allocate(getStaticSize());
  setStaticInstance(val);
  return val;
}


void JavaMethod::initialise(Class* cl, const UTF8* N, const UTF8* T, uint16 A) {
  name = N;
  type = T;
  classDef = cl;
  _signature = 0;
  code = 0;
  access = A;
  canBeInlined = false;
  offset = 0;
  JInfo = 0;
  enveloppes = 0;
}

void JavaField::initialise(Class* cl, const UTF8* N, const UTF8* T, uint16 A) {
  name = N;
  type = T;
  classDef = cl;
  _signature = 0;
  ptrOffset = 0;
  access = A;
  JInfo = 0;
}

void Class::readParents(Reader& reader) {
  uint16 superEntry = reader.readU2();
  const UTF8* superUTF8 = superEntry ? 
        ctpInfo->resolveClassName(superEntry) : 0;

  uint16 nbI = reader.readU2();
  // Use the super field to store the UTF8. since the field is never
  // used before actually loading the super, this is harmless.
  super = (Class*)superUTF8;

  // Use the regular interface array to store the UTF8s. Since this array
  // is never used before actually loading the interfaces, this is harmless.
  interfaces = (Class**)
    classLoader->allocator.Allocate(nbI * sizeof(Class*));
  nbInterfaces = nbI;
  for (int i = 0; i < nbI; i++)
    interfaces[i] = (Class*)ctpInfo->resolveClassName(reader.readU2());

}

void UserClass::loadParents() {
  const UTF8* superUTF8 = (const UTF8*)super;
  if (superUTF8 == 0) {
    depth = 0;
    display = (CommonClass**)
      classLoader->allocator.Allocate(sizeof(CommonClass*));
    display[0] = this;
  } else {
    super = classLoader->loadName(superUTF8, true, true);
    depth = super->depth + 1;
    mvm::BumpPtrAllocator& allocator = classLoader->allocator;
    display = (CommonClass**)
      allocator.Allocate(sizeof(CommonClass*) * (depth + 1));
    memcpy(display, super->display, depth * sizeof(UserCommonClass*));
    display[depth] = this;
  }

  for (unsigned i = 0; i < nbInterfaces; i++)
    interfaces[i] = 
      ((UserClass*)classLoader->loadName((const UTF8*)interfaces[i],
                                         true, true));
}


static void internalLoadExceptions(JavaMethod& meth) {
  
  Attribut* codeAtt = meth.lookupAttribut(Attribut::codeAttribut);
   
  if (codeAtt) {
    Reader reader(codeAtt, meth.classDef->bytes);
    //uint16 maxStack =
    reader.readU2();
    //uint16 maxLocals = 
    reader.readU2();
    uint16 codeLen = reader.readU4();
  
    reader.seek(codeLen, Reader::SeekCur);

    uint16 nbe = reader.readU2();
    for (uint16 i = 0; i < nbe; ++i) {
      //startpc = 
      reader.readU2();
      //endpc=
      reader.readU2();
      //handlerpc =
      reader.readU2();

      uint16 catche = reader.readU2();
      if (catche) meth.classDef->ctpInfo->loadClass(catche);
    }
  }
}

void UserClass::loadExceptions() {
  for (uint32 i = 0; i < nbVirtualMethods; ++i)
    internalLoadExceptions(virtualMethods[i]);
  
  for (uint32 i = 0; i < nbStaticMethods; ++i)
    internalLoadExceptions(staticMethods[i]);
}

Attribut* Class::readAttributs(Reader& reader, uint16& size) {
  uint16 nba = reader.readU2();
  
  Attribut* attributs = new(classLoader->allocator) Attribut[nba];

  for (int i = 0; i < nba; i++) {
    const UTF8* attName = ctpInfo->UTF8At(reader.readU2());
    uint32 attLen = reader.readU4();
    Attribut& att = attributs[i];
    att.start = reader.cursor;
    att.nbb = attLen;
    att.name = attName;
    reader.seek(attLen, Reader::SeekCur);
  }

  size = nba;
  return attributs;
}

void Class::readFields(Reader& reader) {
  uint16 nbFields = reader.readU2();
  virtualFields = new (classLoader->allocator) JavaField[nbFields];
  staticFields = virtualFields + nbFields;
  for (int i = 0; i < nbFields; i++) {
    uint16 access = reader.readU2();
    const UTF8* name = ctpInfo->UTF8At(reader.readU2());
    const UTF8* type = ctpInfo->UTF8At(reader.readU2());
    JavaField* field = 0;
    if (isStatic(access)) {
      --staticFields;
      field = &(staticFields[0]);
      field->initialise(this, name, type, access);
      ++nbStaticFields;
    } else {
      field = &(virtualFields[nbVirtualFields]);
      field->initialise(this, name, type, access);
      ++nbVirtualFields;
    }
    field->attributs = readAttributs(reader, field->nbAttributs);
  }
}

void Class::readMethods(Reader& reader) {
  uint16 nbMethods = reader.readU2();
  virtualMethods = new(classLoader->allocator) JavaMethod[nbMethods];
  staticMethods = virtualMethods + nbMethods;
  for (int i = 0; i < nbMethods; i++) {
    uint16 access = reader.readU2();
    const UTF8* name = ctpInfo->UTF8At(reader.readU2());
    const UTF8* type = ctpInfo->UTF8At(reader.readU2());
    JavaMethod* meth = 0;
    if (isStatic(access)) {
      --staticMethods;
      meth = &(staticMethods[0]);
      meth->initialise(this, name, type, access);
      ++nbStaticMethods;
    } else {
      meth = &(virtualMethods[nbVirtualMethods]);
      meth->initialise(this, name, type, access);
      ++nbVirtualMethods;
    }
    meth->attributs = readAttributs(reader, meth->nbAttributs);
  }
}

void Class::readClass() {

  PRINT_DEBUG(JNJVM_LOAD, 0, COLOR_NORMAL, "; ", 0);
  PRINT_DEBUG(JNJVM_LOAD, 0, LIGHT_GREEN, "reading ", 0);
  PRINT_DEBUG(JNJVM_LOAD, 0, COLOR_NORMAL, "%s\n", printString());

  Reader reader(bytes);
  uint32 magic = reader.readU4();
  if (magic != Jnjvm::Magic) {
    JavaThread::get()->getJVM()->classFormatError("bad magic number %p", magic);
  }
  /* uint16 minor = */ reader.readU2();
  /* uint16 major = */ reader.readU2();
  uint32 ctpSize = reader.readU2();
  ctpInfo = new(classLoader->allocator, ctpSize) JavaConstantPool(this, reader,
                                                                  ctpSize);
  access |= (reader.readU2() & 0x0FFF);
  
  if (!isPublic(access)) access |= ACC_PRIVATE;

  const UTF8* thisClassName = 
    ctpInfo->resolveClassName(reader.readU2());
  
  if (!(thisClassName->equals(name))) {
    JavaThread::get()->getJVM()->classFormatError(
        "try to load %s and found class named %s",
        printString(), thisClassName->printString());
  }

  readParents(reader);
  readFields(reader);
  readMethods(reader);
  attributs = readAttributs(reader, nbAttributs);
  setIsRead();
}

#ifndef ISOLATE_SHARING
void Class::resolveClass() {
  if (!isResolved()) {
    acquire();
    if (isResolved()) {
      release();
    } else if (!isResolving()) {
      setOwnerClass(JavaThread::get());
      readClass();
      release();
      loadParents();
      loadExceptions();
      acquire();
      JnjvmModule *Mod = classLoader->getModule();
      Mod->resolveVirtualClass(this);
      Mod->resolveStaticClass(this);
      setResolved();
      if (!needsInitialisationCheck()) {
        setInitializationState(ready);
      }
      setOwnerClass(0);
      broadcastClass();
      release();
    } else if (JavaThread::get() != getOwnerClass()) {
      while (!isResolved()) {
        waitClass();
      }
      release();
    }
  }
}
#else
void Class::resolveClass() {
  assert(status >= resolved && 
         "Asking to resolve a not resolved-class in a isolate environment");
}
#endif

void UserClass::resolveInnerOuterClasses() {
  if (!innerOuterResolved) {
    Attribut* attribut = lookupAttribut(Attribut::innerClassesAttribut);
    if (attribut != 0) {
      Reader reader(attribut, getBytes());
      uint16 temp = 0;
      uint16 nbi = reader.readU2();
      for (uint16 i = 0; i < nbi; ++i) {
        uint16 inner = reader.readU2();
        uint16 outer = reader.readU2();
        //uint16 innerName = 
        reader.readU2();
        uint16 accessFlags = reader.readU2();
        UserClass* clInner = (UserClass*)ctpInfo->loadClass(inner);
        UserClass* clOuter = (UserClass*)ctpInfo->loadClass(outer);

        if (clInner == this) {
          outerClass = clOuter;
        } else if (clOuter == this) {
          if (!temp) {
            innerClasses = (Class**)
              classLoader->allocator.Allocate(nbi * sizeof(Class*));
          }
          clInner->setInnerAccess(accessFlags);
          innerClasses[nbInnerClasses++] = clInner;
        }
      }
    }
    innerOuterResolved = true;
  }
}

void Class::getDeclaredConstructors(std::vector<JavaMethod*>& res,
                                          bool publicOnly) {
  for (uint32 i = 0; i < nbVirtualMethods; ++i) {
    JavaMethod* meth = &virtualMethods[i];
    bool pub = isPublic(meth->access);
    if (meth->name->equals(classLoader->bootstrapLoader->initName) && 
        (!publicOnly || pub)) {
      res.push_back(meth);
    }
  }
}

void Class::getDeclaredMethods(std::vector<JavaMethod*>& res,
                                     bool publicOnly) {
  for (uint32 i = 0; i < nbVirtualMethods; ++i) {
    JavaMethod* meth = &virtualMethods[i];
    bool pub = isPublic(meth->access);
    if (!(meth->name->equals(classLoader->bootstrapLoader->initName)) && 
        (!publicOnly || pub)) {
      res.push_back(meth);
    }
  }
  
  for (uint32 i = 0; i < nbStaticMethods; ++i) {
    JavaMethod* meth = &staticMethods[i];
    bool pub = isPublic(meth->access);
    if (!(meth->name->equals(classLoader->bootstrapLoader->clinitName)) && 
        (!publicOnly || pub)) {
      res.push_back(meth);
    }
  }
}

void Class::getDeclaredFields(std::vector<JavaField*>& res,
                                    bool publicOnly) {
  for (uint32 i = 0; i < nbVirtualFields; ++i) {
    JavaField* field = &virtualFields[i];
    if (!publicOnly || isPublic(field->access)) {
      res.push_back(field);
    }
  }
  
  for (uint32 i = 0; i < nbStaticFields; ++i) {
    JavaField* field = &staticFields[i];
    if (!publicOnly || isPublic(field->access)) {
      res.push_back(field);
    }
  }
}

static JavaObject* getClassType(Jnjvm* vm, JnjvmClassLoader* loader,
                                Typedef* type) {
  UserCommonClass* res = type->assocClass(loader);
  assert(res && "No associated class");
  return res->getClassDelegatee(vm);
}

ArrayObject* JavaMethod::getParameterTypes(JnjvmClassLoader* loader) {
  Jnjvm* vm = JavaThread::get()->getJVM();
  Signdef* sign = getSignature();
  Typedef* const* arguments = sign->getArgumentsType();
  ArrayObject* res = 
    (ArrayObject*)vm->upcalls->classArrayClass->doNew(sign->nbArguments, vm);

  for (uint32 index = 0; index < sign->nbArguments; ++index) {
    res->elements[index] = getClassType(vm, loader, arguments[index]);
  }

  return res;

}

JavaObject* JavaMethod::getReturnType(JnjvmClassLoader* loader) {
  Jnjvm* vm = JavaThread::get()->getJVM();
  Typedef* ret = getSignature()->getReturnType();
  return getClassType(vm, loader, ret);
}

ArrayObject* JavaMethod::getExceptionTypes(JnjvmClassLoader* loader) {
  Attribut* exceptionAtt = lookupAttribut(Attribut::exceptionsAttribut);
  Jnjvm* vm = JavaThread::get()->getJVM();
  if (exceptionAtt == 0) {
    return (ArrayObject*)vm->upcalls->classArrayClass->doNew(0, vm);
  } else {
    UserConstantPool* ctp = classDef->getConstantPool();
    Reader reader(exceptionAtt, classDef->getBytes());
    uint16 nbe = reader.readU2();
    ArrayObject* res = 
      (ArrayObject*)vm->upcalls->classArrayClass->doNew(nbe, vm);

    for (uint16 i = 0; i < nbe; ++i) {
      uint16 idx = reader.readU2();
      UserCommonClass* cl = ctp->loadClass(idx);
      assert(cl->asClass() && "Wrong exception type");
      cl->asClass()->resolveClass();
      JavaObject* obj = cl->getClassDelegatee(vm);
      res->elements[i] = obj;
    }
    return res;
  }
}


void Class::resolveStaticClass() {
  classLoader->getModule()->resolveStaticClass((Class*)this);
}

#ifdef ISOLATE
TaskClassMirror& Class::getCurrentTaskClassMirror() {
  return IsolateInfo[JavaThread::get()->getJVM()->IsolateID];
}

JavaObject* CommonClass::getDelegatee() {
  return delegatee[JavaThread::get()->getJVM()->IsolateID];
}

JavaObject* CommonClass::setDelegatee(JavaObject* val) {
  JavaObject** obj = &(delegatee[JavaThread::get()->getJVM()->IsolateID]);

  JavaObject* prev = (JavaObject*)
    __sync_val_compare_and_swap((uintptr_t)obj, NULL, val);

  if (!prev) return val;
  else return prev;
}

#else

JavaObject* CommonClass::setDelegatee(JavaObject* val) {
  JavaObject* prev = (JavaObject*)
    __sync_val_compare_and_swap((uintptr_t)&(delegatee[0]), NULL, val);

  if (!prev) return val;
  else return prev;
}

#endif



UserCommonClass* UserCommonClass::resolvedImplClass(Jnjvm* vm,
                                                    JavaObject* clazz,
                                                    bool doClinit) {
  UserCommonClass* cl = ((JavaObjectClass*)clazz)->getClass();
  if (cl->asClass()) {
    cl->asClass()->resolveClass();
    if (doClinit) cl->asClass()->initialiseClass(vm);
  }
  return cl;
}

void JavaMethod::jniConsFromMeth(char* buf) const {
  const UTF8* jniConsClName = classDef->name;
  const UTF8* jniConsName = name;
  sint32 clen = jniConsClName->size;
  sint32 mnlen = jniConsName->size;

  uint32 cur = 0;
  char* ptr = &(buf[JNI_NAME_PRE_LEN]);
  
  memcpy(buf, JNI_NAME_PRE, JNI_NAME_PRE_LEN);
  
  for (sint32 i =0; i < clen; ++i) {
    cur = jniConsClName->elements[i];
    if (cur == '/') ptr[0] = '_';
    else if (cur == '_') {
      ptr[0] = '_';
      ptr[1] = '1';
      ++ptr;
    }
    else ptr[0] = (uint8)cur;
    ++ptr;
  }
  
  ptr[0] = '_';
  ++ptr;
  
  for (sint32 i =0; i < mnlen; ++i) {
    cur = jniConsName->elements[i];
    if (cur == '/') ptr[0] = '_';
    else if (cur == '_') {
      ptr[0] = '_';
      ptr[1] = '1';
      ++ptr;
    }
    else ptr[0] = (uint8)cur;
    ++ptr;
  }
  
  ptr[0] = 0;


}

void JavaMethod::jniConsFromMethOverloaded(char* buf) const {
  const UTF8* jniConsClName = classDef->name;
  const UTF8* jniConsName = name;
  const UTF8* jniConsType = type;
  sint32 clen = jniConsClName->size;
  sint32 mnlen = jniConsName->size;

  uint32 cur = 0;
  char* ptr = &(buf[JNI_NAME_PRE_LEN]);
  
  memcpy(buf, JNI_NAME_PRE, JNI_NAME_PRE_LEN);
  
  for (sint32 i =0; i < clen; ++i) {
    cur = jniConsClName->elements[i];
    if (cur == '/') ptr[0] = '_';
    else if (cur == '_') {
      ptr[0] = '_';
      ptr[1] = '1';
      ++ptr;
    }
    else ptr[0] = (uint8)cur;
    ++ptr;
  }
  
  ptr[0] = '_';
  ++ptr;

  for (sint32 i =0; i < mnlen; ++i) {
    cur = jniConsName->elements[i];
    if (cur == '/') ptr[0] = '_';
    else if (cur == '_') {
      ptr[0] = '_';
      ptr[1] = '1';
      ++ptr;
    }
    else ptr[0] = (uint8)cur;
    ++ptr;
  }
  
  sint32 i = 0;
  while (i < jniConsType->size) {
    char c = jniConsType->elements[i++];
    if (c == I_PARG) {
      ptr[0] = '_';
      ptr[1] = '_';
      ptr += 2;
    } else if (c == '/') {
      ptr[0] = '_';
      ++ptr;
    } else if (c == '_') {
      ptr[0] = '_';
      ptr[1] = '1';
      ptr += 2;
    } else if (c == I_END_REF) {
      ptr[0] = '_';
      ptr[1] = '2';
      ptr += 2;
    } else if (c == I_TAB) {
      ptr[0] = '_';
      ptr[1] = '3';
      ptr += 2;
    } else if (c == I_PARD) {
      break;
    } else {
      ptr[0] = c;
      ++ptr;
    }
  }

  ptr[0] = 0;

}

bool UserClass::isNativeOverloaded(JavaMethod* meth) {
  
  for (uint32 i = 0; i < nbVirtualMethods; ++i) {
    JavaMethod& cur = virtualMethods[i];
    if (&cur != meth && isNative(cur.access) && cur.name->equals(meth->name))
      return true;
  }
  
  for (uint32 i = 0; i < nbStaticMethods; ++i) {
    JavaMethod& cur = staticMethods[i];
    if (&cur != meth && isNative(cur.access) && cur.name->equals(meth->name))
      return true;
  }

  return false;
}

bool UserClass::needsInitialisationCheck() {
  
  if (!isClassRead()) return true;

  if (isReady()) return false;

  if (super && super->needsInitialisationCheck())
    return true;

  if (nbStaticFields) return true;

  JavaMethod* meth = 
    lookupMethodDontThrow(classLoader->bootstrapLoader->clinitName,
                          classLoader->bootstrapLoader->clinitType, 
                          true, false, 0);
  
  if (meth) return true;

  return false;
}


void ClassArray::initialiseVT(Class* cl) {
  uint32 size =  (cl->virtualTableSize - VT_NB_FUNCS) * sizeof(void*);
  
  #define COPY(CLASS) \
    memcpy((void*)((uintptr_t)CLASS + VT_SIZE), \
           (void*)((uintptr_t)cl->virtualVT + VT_SIZE), size);

    COPY(JavaArrayVT)
    COPY(ArrayObjectVT)

#undef COPY
}
