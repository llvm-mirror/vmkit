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
#include "JavaCompiler.h"
#include "JavaConstantPool.h"
#include "JavaObject.h"
#include "JavaThread.h"
#include "JavaTypes.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"
#include "LockedMap.h"
#include "Reader.h"

#include <cstring>

using namespace jnjvm;

const UTF8* Attribut::codeAttribut = 0;
const UTF8* Attribut::exceptionsAttribut = 0;
const UTF8* Attribut::constantAttribut = 0;
const UTF8* Attribut::lineNumberTableAttribut = 0;
const UTF8* Attribut::innerClassesAttribut = 0;
const UTF8* Attribut::sourceFileAttribut = 0;

Class* ClassArray::SuperArray;
Class** ClassArray::InterfacesArray;

extern "C" void JavaArrayTracer(JavaObject*);
extern "C" void JavaObjectTracer(JavaObject*);
extern "C" void ArrayObjectTracer(JavaObject*);

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
  uint32 size = JavaVirtualTable::getBaseSize();
  virtualVT = new(loader->allocator, size) JavaVirtualTable(this);
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
                       UserCommonClass* base) : CommonClass(loader, n) {
  _baseClass = base;
  super = ClassArray::SuperArray;
  interfaces = ClassArray::InterfacesArray;
  nbInterfaces = 2;
  
  uint32 size = JavaVirtualTable::getBaseSize();
  virtualVT = new(loader->allocator, size) JavaVirtualTable(this);
  
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
  VirtualTable* VT = virtualVT;
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
  VirtualTable* VT = virtualVT;
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
    code = classDef->classLoader->getCompiler()->materializeFunction(this);
    Jnjvm* vm = JavaThread::get()->getJVM();
    vm->addMethodInFunctionMap(this, code);
  }
  
  return code;
}

void JavaMethod::setCompiledPtr(void* ptr, const char* name) {
  classDef->acquire();
  if (code == 0) {
    code = ptr;
    Jnjvm* vm = JavaThread::get()->getJVM();
    vm->addMethodInFunctionMap(this, code);
    classDef->classLoader->getCompiler()->setMethod(this, ptr, name);
  }
  access |= ACC_NATIVE;
  classDef->release();
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
          classLoader->getCompiler()->isStaticCompiling())
         && "Uninitialized class when allocating.");
  assert(getVirtualVT() && "No VT\n");
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

bool UserCommonClass::isAssignableFrom(UserCommonClass* cl) {
  assert(virtualVT && cl->virtualVT);
  return virtualVT->isSubtypeOf(cl->virtualVT);
}

bool JavaVirtualTable::isSubtypeOf(JavaVirtualTable* otherVT) {
  
  if (otherVT == ((JavaVirtualTable**)this)[otherVT->offset]) return true;
  else if (otherVT->offset != getCacheIndex()) return false;
  else if (this == otherVT) return true;
  else {
    for (uint32 i = 0; i < nbSecondaryTypes; ++i) {
      if (secondaryTypes[i] == otherVT) {
        cache = otherVT;
        return true;
      }
    }
  }
  return false;
}

void JavaField::InitField(void* obj, uint64 val) {
  
  Typedef* type = getSignature();
  if (!type->isPrimitive()) {
    ((JavaObject**)((uint64)obj + ptrOffset))[0] = (JavaObject*)val;
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
#ifdef USE_GC_BOEHM
  void* val = GC_MALLOC(getStaticSize());
#else
  void* val = classLoader->allocator.Allocate(getStaticSize());
#endif
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
  if (superEntry) {
    const UTF8* superUTF8 = ctpInfo->resolveClassName(superEntry);
    super = classLoader->loadName(superUTF8, false, true);
    depth = super->depth + 1;
    mvm::BumpPtrAllocator& allocator = classLoader->allocator;
    display = (CommonClass**)
      allocator.Allocate(sizeof(CommonClass*) * (depth + 1));
    memcpy(display, super->display, depth * sizeof(UserCommonClass*));
    display[depth] = this;
    virtualTableSize = super->virtualTableSize;
  } else {
    depth = 0;
    display = (CommonClass**)
      classLoader->allocator.Allocate(sizeof(CommonClass*));
    display[0] = this;
    virtualTableSize = JavaVirtualTable::getFirstJavaMethodIndex();
  }

  uint16 nbI = reader.readU2();

  interfaces = (Class**)
    classLoader->allocator.Allocate(nbI * sizeof(Class*));
  
  // Do not set nbInterfaces yet, we may be interrupted by the GC
  // in anon-cooperative environment.
  for (int i = 0; i < nbI; i++) {
    const UTF8* name = ctpInfo->resolveClassName(reader.readU2());
    interfaces[i] = classLoader->loadName(name, false, true);
  }
  nbInterfaces = nbI;

}

void UserClass::loadParents() {
  if (super)
    super->resolveClass();

  for (unsigned i = 0; i < nbInterfaces; i++)
    interfaces[i]->resolveClass();
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
      if (catche) {
        meth.classDef->ctpInfo->loadClass(catche, false);
      }
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

void Class::makeVT() {
  
  for (uint32 i = 0; i < nbVirtualMethods; ++i) {
    JavaMethod& meth = virtualMethods[i];
    if (meth.name->equals(classLoader->bootstrapLoader->finalize)) {
      meth.offset = 0;
    } else {
      JavaMethod* parent = super? 
        super->lookupMethodDontThrow(meth.name, meth.type, false, true, 0) :
        0;

      uint64_t offset = 0;
      if (!parent) {
        offset = virtualTableSize++;
        meth.offset = offset;
      } else {
        offset = parent->offset;
        meth.offset = parent->offset;
      }
    }
  }

  if (super) {
    assert(virtualTableSize >= super->virtualTableSize &&
           "Size of virtual table less than super!");
  }

  mvm::BumpPtrAllocator& allocator = classLoader->allocator;
  virtualVT = new(allocator, virtualTableSize) JavaVirtualTable(this);
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
  makeVT();
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
      setIsResolving();
      release();

      loadParents();
      JavaCompiler *Comp = classLoader->getCompiler();
      Comp->resolveVirtualClass(this);
      Comp->resolveStaticClass(this);
      loadExceptions();
      if (!super) ClassArray::initialiseVT(this);
      
      bool init = needsInitialisationCheck();
      
      acquire();
      if (!init) {
        setInitializationState(ready);
      } else {
        setResolved();
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

void JavaMethod::jniConsFromMeth(char* buf, const UTF8* jniConsClName,
                                 const UTF8* jniConsName,
                                 const UTF8* jniConsType,
                                 bool synthetic) {
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

void JavaMethod::jniConsFromMethOverloaded(char* buf, const UTF8* jniConsClName,
                                           const UTF8* jniConsName,
                                           const UTF8* jniConsType,
                                           bool synthetic) {
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

  if (synthetic) {
    ptr[0] = 'S';
    ++ptr;
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
  
  if (!isResolved()) return true;

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

void ClassArray::initialiseVT(Class* javaLangObject) {

  ClassArray::SuperArray = javaLangObject;
  JnjvmClassLoader* JCL = javaLangObject->classLoader;
  Classpath* upcalls = JCL->bootstrapLoader->upcalls;
  
  assert(javaLangObject->virtualVT->init && 
         "Initializing array VT before JavaObjectVT");
  
  // Load and resolve interfaces of array classes. We resolve them now
  // so that the secondary type list of array VTs can reference them.
  ClassArray::InterfacesArray[0] = 
    JCL->loadName(JCL->asciizConstructUTF8("java/lang/Cloneable"),
                  true, false);
  
  ClassArray::InterfacesArray[1] = 
    JCL->loadName(JCL->asciizConstructUTF8("java/io/Serializable"),
                  true, false);
   
  // Load base array classes that JnJVM internally uses. Now that the interfaces
  // have been loaded, the secondary type can be safely created.
  upcalls->ArrayOfObject = 
    JCL->constructArray(JCL->asciizConstructUTF8("[Ljava/lang/Object;"));
  
  upcalls->ArrayOfString = 
    JCL->constructArray(JCL->asciizConstructUTF8("[Ljava/lang/String;"));
  
  // Update native array classes. A few things have not been set properly
  // when loading these classes because java.lang.Object and java.lang.Object[]
  // were not loaded yet. Correct that now by updating these classes.
  #define COPY(CLASS) \
    memcpy(CLASS->virtualVT->getFirstJavaMethod(), \
           javaLangObject->virtualVT->getFirstJavaMethod(), \
           sizeof(uintptr_t) * JavaVirtualTable::getNumJavaMethods()); \
    CLASS->super = javaLangObject; \
    CLASS->display[0] = javaLangObject; \
    CLASS->display[1] = CLASS; \
    CLASS->virtualVT->display[0] = javaLangObject->virtualVT; \
    CLASS->virtualVT->secondaryTypes = \
      upcalls->ArrayOfObject->virtualVT->secondaryTypes; \

    COPY(upcalls->ArrayOfBool)
    COPY(upcalls->ArrayOfByte)
    COPY(upcalls->ArrayOfChar)
    COPY(upcalls->ArrayOfShort)
    COPY(upcalls->ArrayOfInt)
    COPY(upcalls->ArrayOfFloat)
    COPY(upcalls->ArrayOfDouble)
    COPY(upcalls->ArrayOfLong)

#undef COPY
 
}

JavaVirtualTable::JavaVirtualTable(Class* C) {
   
  if (C->super) {
    // Set the class of this VT.
    cl = C;
    
    // Set depth and display for fast dynamic type checking.
    JavaVirtualTable* superVT = C->super->virtualVT;
    depth = superVT->depth + 1;
    nbSecondaryTypes = superVT->nbSecondaryTypes + cl->nbInterfaces;

    for (uint32 i = 0; i < cl->nbInterfaces; ++i) {
      nbSecondaryTypes += cl->interfaces[i]->virtualVT->nbSecondaryTypes;
    }
    
    uint32 length = getDisplayLength() < depth ? getDisplayLength() : depth;
    memcpy(display, superVT->display, length * sizeof(JavaVirtualTable*)); 
    uint32 outOfDepth = 0;
    if (C->isInterface()) {
      offset = getCacheIndex();
    } else if (depth < getDisplayLength()) {
      display[depth] = this;
      offset = getCacheIndex() + depth + 1;
    } else {
      offset = getCacheIndex();
      ++nbSecondaryTypes;
      outOfDepth = 1;
    }

    mvm::BumpPtrAllocator& allocator = C->classLoader->allocator;
    secondaryTypes = (JavaVirtualTable**)
      allocator.Allocate(sizeof(JavaVirtualTable*) * nbSecondaryTypes);  
    
    if (outOfDepth) {
      secondaryTypes[0] = this;
    }

    if (superVT->nbSecondaryTypes) {
      memcpy(secondaryTypes + outOfDepth, superVT->secondaryTypes,
             sizeof(JavaVirtualTable*) * superVT->nbSecondaryTypes);
    }

    for (uint32 i = 0; i < cl->nbInterfaces; ++i) {
      JavaVirtualTable* cur = cl->interfaces[i]->virtualVT;
      assert(cur && "Interface not resolved!\n");
      uint32 index = superVT->nbSecondaryTypes + outOfDepth + i;
      secondaryTypes[index] = cur;
    }
   
    uint32 lastIndex = superVT->nbSecondaryTypes + cl->nbInterfaces +
                        outOfDepth;

    for (uint32 i = 0; i < cl->nbInterfaces; ++i) {
      JavaVirtualTable* cur = cl->interfaces[i]->virtualVT;
      memcpy(secondaryTypes + lastIndex, cur->secondaryTypes,
             sizeof(JavaVirtualTable*) * cur->nbSecondaryTypes);
      lastIndex += cur->nbSecondaryTypes;
    }

  } else {
    // Set the tracer, destructor and delete
    tracer = (uintptr_t)JavaObjectTracer;
    destructor = 0;
    operatorDelete = 0;
    
    // Set the class of this VT.
    cl = C;
    
    // Set depth and display for fast dynamic type checking.
    // java.lang.Object does not have any secondary types.
    offset = getCacheIndex() + 1;
    depth = 0;
    display[0] = this;
    destructor = 0;
    nbSecondaryTypes = 0;
  }
    

}
  
JavaVirtualTable::JavaVirtualTable(ClassArray* C) {
  
  baseClassVT = C->baseClass()->virtualVT;
  assert(baseClassVT && "Not base VT when creating an array");

  if (!C->baseClass()->isPrimitive()) {
    // Copy the super VT into the current VT.
    uint32 size = (getBaseSize() - getFirstJavaMethodIndex());
    memcpy(this->getFirstJavaMethod(),
           C->super->virtualVT->getFirstJavaMethod(),
           size * sizeof(uintptr_t));
    tracer = (uintptr_t)ArrayObjectTracer;
    
    // Set the class of this VT.
    cl = C;

    // Set depth and display for fast dynamic type checking.
    JnjvmClassLoader* JCL = cl->classLoader;
    Classpath* upcalls = JCL->bootstrapLoader->upcalls;
    
    if (upcalls->ArrayOfObject) {
      UserCommonClass* temp = C->baseClass();
      uint32 dim = 1;
      while (temp->isArray()) {
        temp = temp->asArrayClass()->baseClass();
        ++dim;
      }
     
      bool newSecondaryTypes = false;
      bool intf = temp->isInterface();
      if (temp->isPrimitive()) {
        --dim;
        temp = C->super;
      } else if (temp == C->super) {
        --dim;
        newSecondaryTypes = true;
      } else {
        temp = temp->super;
      }
      
      const UTF8* name = JCL->constructArrayName(dim, temp->name);
      ClassArray* super = JCL->constructArray(name);
      JavaVirtualTable* superVT = super->virtualVT;
      depth = superVT->depth + 1;

      uint32 length = getDisplayLength() < depth ? getDisplayLength() : depth;
      memcpy(display, superVT->display, length * sizeof(JavaVirtualTable*)); 
      if (depth < getDisplayLength() && !intf) {
        display[depth] = this;
        offset = getCacheIndex() + depth + 1;
      } else {
        offset = getCacheIndex();
      }
        
      mvm::BumpPtrAllocator& allocator = JCL->allocator;

      if (!newSecondaryTypes) {
        if (depth < getDisplayLength()) {
          nbSecondaryTypes = superVT->nbSecondaryTypes;
          secondaryTypes = superVT->secondaryTypes;
        } else {
          nbSecondaryTypes = superVT->nbSecondaryTypes + 1;
          secondaryTypes = (JavaVirtualTable**)
            allocator.Allocate(sizeof(JavaVirtualTable*) * nbSecondaryTypes);
          secondaryTypes[0] = this;
          memcpy(secondaryTypes + 1 , superVT->secondaryTypes,
                 superVT->nbSecondaryTypes * sizeof(JavaVirtualTable*));
        }
      } else {

        // This is an Object[....] array class. It will create the list of
        // secondary types and all array classes of the same dimension will
        // point to this array.

        // If we're superior than the display limit, we must make room for one
        // slot that will contain the current VT.
        uint32 outOfDepth = 0;
        if (depth >= getDisplayLength()) outOfDepth = 1;
        
        assert(cl->nbInterfaces == 2 && "Arrays have more than 2 interface?");

        // The list of secondary types is composed of:
        // (1) The list of secondary types of super array.
        // (2) The array of inherited interfaces with the same dimensions.
        // (3) This VT, if its depth is superior than the display size.
        nbSecondaryTypes = superVT->nbSecondaryTypes + 2 + outOfDepth;

        secondaryTypes = (JavaVirtualTable**)
          allocator.Allocate(sizeof(JavaVirtualTable*) * nbSecondaryTypes);
        
        // First, copy the list of secondary types from super array.
        memcpy(secondaryTypes + outOfDepth, superVT->secondaryTypes,
               superVT->nbSecondaryTypes * sizeof(JavaVirtualTable*));

        // If the depth is superior than the display size, put the current VT
        // at the beginning of the list.
        if (outOfDepth) secondaryTypes[0] = this;
        
        // Load Cloneable[...] and Serializable[...]
        const UTF8* name = JCL->constructArrayName(dim, cl->interfaces[0]->name);
        ClassArray* firstInterface = JCL->constructArray(name);
        name = JCL->constructArrayName(dim, cl->interfaces[1]->name);
        ClassArray* secondInterface = JCL->constructArray(name);

        uint32 index = superVT->nbSecondaryTypes + outOfDepth;

        // Put Cloneable[...] and Serializable[...] at the end of the list.
        secondaryTypes[index] = firstInterface->virtualVT;
        secondaryTypes[index + 1] = secondInterface->virtualVT;

        // If the depth is greater than the display size, 
        // Cloneable[...] and Serializable[...] have their own list of
        // secondary types, and we must therefore tell them that they
        // implement themselves.
        // If the depth is less than than the display size, there is nothing
        // to do: the array of secondary types has been created before loading
        // the interface arrays, so the interface arrays already reference
        // the array.
        if (outOfDepth) {
          firstInterface->virtualVT->secondaryTypes[index] =
            firstInterface->virtualVT;
          firstInterface->virtualVT->secondaryTypes[index + 1] =
            secondInterface->virtualVT;
          secondInterface->virtualVT->secondaryTypes[index] =
            firstInterface->virtualVT;
          secondInterface->virtualVT->secondaryTypes[index + 1] =
            secondInterface->virtualVT;
        }
      }

    } else {
      // This is java.lang.Object[].
      depth = 1;
      display[0] = C->super->virtualVT;
      display[1] = this;
      offset = getCacheIndex() + 2;
      nbSecondaryTypes = 2;
      
      mvm::BumpPtrAllocator& allocator = JCL->allocator;
      secondaryTypes = (JavaVirtualTable**)
        allocator.Allocate(sizeof(JavaVirtualTable*) * nbSecondaryTypes);

      // The interfaces have already been resolved.
      secondaryTypes[0] = cl->interfaces[0]->virtualVT;
      secondaryTypes[1] = cl->interfaces[1]->virtualVT;
    }

  } else {
    // Set the tracer, destructor and delete
    tracer = (uintptr_t)JavaArrayTracer;
    destructor = 0;
    operatorDelete = 0;
    
    // Set the class of this VT.
    cl = C;
    
    // Set depth and display for fast dynamic type checking. Since
    // JavaObject has not been loaded yet, don't use super.
    depth = 1;
    display[0] = 0;
    display[1] = this;
    nbSecondaryTypes = 2;
    offset = getCacheIndex() + 2;

    // The list of secondary types has not been allocated yet by
    // java.lang.Object[]. The initialiseVT function will update the current
    // array to point to java.lang.Object[]'s secondary list.
  }
}


JavaVirtualTable::JavaVirtualTable(ClassPrimitive* C) {
  // Only used for subtype checking
  cl = C;
  depth = 0;
  display[0] = this;
  nbSecondaryTypes = 0;
  offset = getCacheIndex() + 1;
}
