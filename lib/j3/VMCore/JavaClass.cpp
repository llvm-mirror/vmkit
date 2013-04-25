//===-------- JavaClass.cpp - Java class representation -------------------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#define JNJVM_LOAD 1

#include "debug.h"
#include "types.h"

#include "vmkit/System.h"

#include "ClasspathReflect.h"
#include "JavaArray.h"
#include "JavaClass.h"
#include "j3/JavaCompiler.h"
#include "JavaString.h"
#include "JavaConstantPool.h"
#include "JavaObject.h"
#include "JavaThread.h"
#include "JavaTypes.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"
#include "LockedMap.h"
#include "Reader.h"

#include <cstring>
#include <cstdlib>
#include <algorithm>

#if 0
using namespace vmkit;
#define dprintf(...) do { printf("JavaClass: "); printf(__VA_ARGS__); } while(0)
#define ddprintf(...) do { printf(__VA_ARGS__); } while(0)
#else
#define dprintf(...)
#define ddprintf(...)
#endif

using namespace j3;
using namespace std;

const UTF8* JavaAttribute::annotationsAttribute = 0;
const UTF8* JavaAttribute::codeAttribute = 0;
const UTF8* JavaAttribute::exceptionsAttribute = 0;
const UTF8* JavaAttribute::constantAttribute = 0;
const UTF8* JavaAttribute::lineNumberTableAttribute = 0;
const UTF8* JavaAttribute::innerClassesAttribute = 0;
const UTF8* JavaAttribute::sourceFileAttribute = 0;
const UTF8* JavaAttribute::signatureAttribute = 0;
const UTF8* JavaAttribute::enclosingMethodAttribute = 0;
const UTF8* JavaAttribute::paramAnnotationsAttribute = 0;
const UTF8* JavaAttribute::annotationDefaultAttribute = 0;

Class* ClassArray::SuperArray;
Class** ClassArray::InterfacesArray;

extern "C" void JavaObjectTracer(JavaObject*);
extern "C" void ArrayObjectTracer(JavaObject*);
extern "C" void RegularObjectTracer(JavaObject*);
extern "C" void ReferenceObjectTracer(JavaObject*);


extern "C" bool CheckIfObjectIsAssignableToArrayPosition(JavaObject * obj, JavaObject* array) {
	llvm_gcroot(obj, 0);
	llvm_gcroot(array, 0);
	if (obj == 0)
		return true;
	bool b = obj->getVirtualTable()->isSubtypeOf(array->getVirtualTable()->baseClassVT);
	if (!b) {
		BEGIN_NATIVE_EXCEPTION(0)
		Jnjvm* vm = JavaThread::get()->getJVM();
		vm->CreateArrayStoreException(obj->getVirtualTable());
		END_NATIVE_EXCEPTION
	}
	return b;
}

extern "C" bool IsSubtypeIntrinsic(JavaVirtualTable* obj, JavaVirtualTable* clazz){
	 //printf("2 + 1 \n");
	 return obj->isSubtypeOf(clazz);
}

static bool LessThanPtrVirtualTable (JavaVirtualTable * elem1, JavaVirtualTable * elem2) {
	return elem1 < elem2;
}

static bool equalsPtrVirtualTable (JavaVirtualTable * elem1, JavaVirtualTable * elem2) {
	return elem1 == elem2;
}

JavaAttribute::JavaAttribute(const UTF8* name, uint32 length,
                   uint32 offset) {
  this->start    = offset;
  this->nbb      = length;
  this->name     = name;
}

JavaAttribute* Class::lookupAttribute(const UTF8* key) {
  for (uint32 i = 0; i < nbAttributes; ++i) {
    JavaAttribute* cur = &(attributes[i]);
    if (cur->name->equals(key)) return cur;
  }

  return 0;
}

JavaAttribute* JavaField::lookupAttribute(const UTF8* key) {
  for (uint32 i = 0; i < nbAttributes; ++i) {
    JavaAttribute* cur = &(attributes[i]);
    if (cur->name->equals(key)) return cur;
  }

  return 0;
}

JavaAttribute* JavaMethod::lookupAttribute(const UTF8* key) {
  for (uint32 i = 0; i < nbAttributes; ++i) {
    JavaAttribute* cur = &(attributes[i]);
    if (cur->name->equals(key)) return cur;
  }

  return 0;
}

CommonClass::~CommonClass() {
}

Class::~Class() {
  for (uint32 i = 0; i < nbAttributes; ++i) {
    JavaAttribute* cur = &(attributes[i]);
    cur->~JavaAttribute();
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
  for (uint32 i = 0; i < nbAttributes; ++i) {
    JavaAttribute* cur = &(attributes[i]);
    cur->~JavaAttribute();
    classDef->classLoader->allocator.Deallocate(cur);
  }
}

JavaMethod::~JavaMethod() { 
  for (uint32 i = 0; i < nbAttributes; ++i) {
    JavaAttribute* cur = &(attributes[i]);
    cur->~JavaAttribute();
    classDef->classLoader->allocator.Deallocate(cur);
  }
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
 
  uint32 size = JavaVirtualTable::getBaseSize();
  virtualVT = new(loader->allocator, size) JavaVirtualTable(this);
  access = ACC_ABSTRACT | ACC_FINAL | ACC_PUBLIC | JNJVM_PRIMITIVE;
  logSize = nb;
}

Class::Class(JnjvmClassLoader* loader, const UTF8* n, ClassBytes* B) : 
    CommonClass(loader, n) {
  virtualVT = 0;
  bytes = B;
  super = 0;
  ctpInfo = 0;
  outerClass = 0;
  innerOuterResolved = false;
  nbInnerClasses = 0;
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
  
  access = ACC_FINAL | ACC_ABSTRACT | JNJVM_ARRAY | base->getAccess();
  access &= ~ACC_INTERFACE;
}

JavaObject* UserClassArray::doNew(sint32 n, Jnjvm* vm) {
  JavaObject* res = NULL;
  llvm_gcroot(res, 0);
  if (n < 0) {
    vm->negativeArraySizeException(n);
  } else if (n > JavaArray::MaxArraySize) {
    vm->outOfMemoryError();
  }
  UserCommonClass* cl = baseClass();
  uint32 logSize = cl->isPrimitive() ? 
    cl->asPrimitiveClass()->logSize : (sizeof(JavaObject*) == 8 ? 3 : 2);
  VirtualTable* VT = virtualVT;
  uint32 size = sizeof(JavaObject) + sizeof(ssize_t) + (n << logSize);
  res = (JavaObject*)JavaObject::operator new(size, VT);
  JavaArray::setSize(res, n);
  return res;
}

void* JavaMethod::compiledPtr(Class* customizeFor) {
  if ((isCustomizable && customizeFor != NULL) || code == 0) {
    return classDef->classLoader->getCompiler()->materializeFunction(this, customizeFor);
  }
  return code;
}

void JavaMethod::setNative() {
  access |= ACC_NATIVE;
}

void JavaVirtualTable::setNativeTracer(word_t ptr, const char* name) {
  tracer = ptr;
}

void JavaVirtualTable::setNativeDestructor(word_t ptr, const char* name) {
	if (!cl->classLoader->getCompiler()->isStaticCompiling()) {
	  destructor = ptr;
  	operatorDelete = ptr;
	}
}

JavaMethod* Class::lookupInterfaceMethodDontThrow(const UTF8* name,
                                                  const UTF8* type) {
  JavaMethod* cur = lookupMethodDontThrow(name, type, false, false, 0);
  if (cur == NULL) {
    for (uint16 i = 0; i < nbInterfaces; ++i) {
      Class* I = interfaces[i];
      cur = I->lookupInterfaceMethodDontThrow(name, type);
      if (cur) return cur;
    }
  }
  if (cur == NULL && super != NULL) {
    cur = super->lookupInterfaceMethodDontThrow(name, type);
  }
  return cur;
}

JavaMethod* Class::lookupSpecialMethodDontThrow(const UTF8* name,
                                                const UTF8* type,
                                                Class* current) {
  JavaMethod* meth = lookupMethodDontThrow(name, type, false, true, NULL);

  if (meth &&
      isSuper(current->access) &&
      current != meth->classDef &&
      meth->classDef->isSubclassOf(current) &&
      !name->equals(classLoader->bootstrapLoader->initName)) {
    meth = current->super->lookupMethodDontThrow(name, type, false, true, NULL);
  }

  return meth;
}

JavaMethod* Class::lookupMethodDontThrow(const UTF8* name, const UTF8* type,
                                         bool isStatic, bool recurse,
                                         Class** methodCl) {
  // This is a dirty hack because of a dirty usage pattern. See UPCALL_METHOD macro...
  if (this == NULL) return NULL;
  
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

JavaMethod* Class::lookupInterfaceMethod(const UTF8* name, const UTF8* type) {
  JavaMethod* res = lookupInterfaceMethodDontThrow(name, type);

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
  JavaObject* res = NULL;
  llvm_gcroot(res, 0);
  assert(this && "No class when allocating.");
  assert((this->isInitializing() || 
          classLoader->getCompiler()->isStaticCompiling() ||
          this == classLoader->bootstrapLoader->upcalls->newClass)
         && "Uninitialized class when allocating.");
  assert(getVirtualVT() && "No VT\n");
  res = (JavaObject*)JavaObject::operator new(getVirtualSize(), getVirtualVT());
  return res;
}

bool UserCommonClass::inheritName(const uint16* buf, uint32 len) {
  if (getName()->equals(buf, len)) {
    return true;
  } else  if (isPrimitive()) {
    return false;
  } else if (super) {
    if (getSuper()->inheritName(buf, len)) return true;
  }
  
  for (uint32 i = 0; i < nbInterfaces; ++i) {
    if (interfaces[i]->inheritName(buf, len)) return true;
  }
  return false;
}

bool UserCommonClass::isOfTypeName(const UTF8* Tname) {
  if (inheritName(Tname->elements, Tname->size)) {
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
      (res && curS->inheritName(&(Tname->elements[prof + 1]), len - 1));
  } else {
    return false;
  }
}

bool UserCommonClass::isSubclassOf(UserCommonClass* cl) {
  assert(virtualVT && cl->virtualVT);
  return virtualVT->isSubtypeOf(cl->virtualVT);
}

bool JavaVirtualTable::isSubtypeOf(JavaVirtualTable* otherVT) {
 
  assert(this);
  assert(otherVT);
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
    if (cl->isArray() && otherVT->cl->isArray()) {
    	return baseClassVT->isSubtypeOf(otherVT->baseClassVT);
    }
  }
  return false;
}

void JavaField::InitNullStaticField() {
  
  Typedef* type = getSignature();
  void* obj = classDef->getStaticInstance();
  if (!type->isPrimitive()) {
    ((JavaObject**)((uint64)obj + ptrOffset))[0] = NULL;
    return;
  }

  PrimitiveTypedef* prim = (PrimitiveTypedef*)type;
  if (prim->isLong()) {
    ((sint64*)((uint64)obj + ptrOffset))[0] = 0;
  } else if (prim->isInt()) {
    ((sint32*)((uint64)obj + ptrOffset))[0] = 0;
  } else if (prim->isChar()) {
    ((uint16*)((uint64)obj + ptrOffset))[0] = 0;
  } else if (prim->isShort()) {
    ((sint16*)((uint64)obj + ptrOffset))[0] = 0;
  } else if (prim->isByte()) {
    ((sint8*)((uint64)obj + ptrOffset))[0] = 0;
  } else if (prim->isBool()) {
    ((uint8*)((uint64)obj + ptrOffset))[0] = 0;
  } else if (prim->isDouble()) {
    ((double*)((uint64)obj + ptrOffset))[0] = 0.0;
  } else if (prim->isFloat()) {
    ((float*)((uint64)obj + ptrOffset))[0] = 0.0;
  } else {
    abort();
  }
}

void JavaField::InitStaticField(uint64 val) { 
  Typedef* type = getSignature();
  void* obj = classDef->getStaticInstance();
  assert(type->isPrimitive() && "Non primitive field");
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
    // Should never be here.
    abort();
  }
}

void JavaField::InitStaticField(JavaObject* val) {
  llvm_gcroot(val, 0);
  void* obj = classDef->getStaticInstance();
  assert(isReference());
  JavaObject** ptr = (JavaObject**)((uint64)obj + ptrOffset);
  vmkit::Collector::objectReferenceNonHeapWriteBarrier((gc**)ptr, (gc*)val);
}

void JavaField::InitStaticField(double val) {
  void* obj = classDef->getStaticInstance();
  ((double*)((uint64)obj + ptrOffset))[0] = val;
}

void JavaField::InitStaticField(float val) {
  void* obj = classDef->getStaticInstance();
  ((float*)((uint64)obj + ptrOffset))[0] = val;
}

void JavaField::InitStaticField(Jnjvm* vm) {
  JavaString* obj = 0;
  llvm_gcroot(obj, 0);

  const Typedef* type = getSignature();
  JavaAttribute* attribute = lookupAttribute(JavaAttribute::constantAttribute);

  if (!attribute) {
    InitNullStaticField();
  } else {
    Reader reader(attribute, classDef->bytes);
    JavaConstantPool * ctpInfo = classDef->ctpInfo;
    uint16 idx = reader.readU2();
    if (type->isPrimitive()) {
      UserCommonClass* cl = type->assocClass(vm->bootstrapLoader);
      if (cl == vm->upcalls->OfLong) {
        InitStaticField((uint64)ctpInfo->LongAt(idx));
      } else if (cl == vm->upcalls->OfDouble) {
        InitStaticField(ctpInfo->DoubleAt(idx));
      } else if (cl == vm->upcalls->OfFloat) {
        InitStaticField(ctpInfo->FloatAt(idx));
      } else {
        InitStaticField((uint64)ctpInfo->IntegerAt(idx));
      }
    } else if (type->isReference()) {
      const UTF8* utf8 = ctpInfo->UTF8At(ctpInfo->ctpDef[idx]);
      obj = ctpInfo->resolveString(utf8, idx);
      InitStaticField(obj);
    } else {
      fprintf(stderr, "I haven't verified your class file and it's malformed:"
                      " unknown constant %s!\n",
                      UTF8Buffer(type->keyName).cString());
      abort();
    }
  } 
}

void* UserClass::allocateStaticInstance(Jnjvm* vm) {
  void* val = classLoader->allocator.Allocate(getStaticSize(),
                                              "Static instance");
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
  isCustomizable = false;
  offset = 0;
}

void JavaField::initialise(Class* cl, const UTF8* N, const UTF8* T, uint16 A) {
  name = N;
  type = T;
  classDef = cl;
  _signature = 0;
  ptrOffset = 0;
  access = A;
}

void Class::readParents(Reader& reader) {
  uint16 superEntry = reader.readU2();
  if (superEntry) {
    const UTF8* superUTF8 = ctpInfo->resolveClassName(superEntry);
    super = classLoader->loadName(superUTF8, false, true, NULL);
  }

  uint16 nbI = reader.readU2();

  interfaces = (Class**)
    classLoader->allocator.Allocate(nbI * sizeof(Class*), "Interfaces");
  
  // Do not set nbInterfaces yet, we may be interrupted by the GC
  // in anon-cooperative environment.
  for (int i = 0; i < nbI; i++) {
    const UTF8* name = ctpInfo->resolveClassName(reader.readU2());
    interfaces[i] = classLoader->loadName(name, false, true, NULL);
  }
  nbInterfaces = nbI;

}

void internalLoadExceptions(JavaMethod& meth) {
  
  JavaAttribute* codeAtt = meth.lookupAttribute(JavaAttribute::codeAttribute);
   
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
      if (catche) meth.classDef->ctpInfo->loadClass(catche, false);
    }
  }
}

void UserClass::loadExceptions() {
  for (uint32 i = 0; i < nbVirtualMethods; ++i)
    internalLoadExceptions(virtualMethods[i]);
  
  for (uint32 i = 0; i < nbStaticMethods; ++i)
    internalLoadExceptions(staticMethods[i]);
}

JavaAttribute* Class::readAttributes(Reader& reader, uint16& size) {
  uint16 nba = reader.readU2();
 
  JavaAttribute* attributes = new(classLoader->allocator, "Attributes") JavaAttribute[nba];

  for (int i = 0; i < nba; i++) {
    const UTF8* attName = ctpInfo->UTF8At(reader.readU2());
    uint32 attLen = reader.readU4();
    JavaAttribute& att = attributes[i];
    att.start = reader.cursor;
    att.nbb = attLen;
    att.name = attName;
    reader.seek(attLen, Reader::SeekCur);
  }

  size = nba;
  return attributes;
}

void Class::readFields(Reader& reader) {
  uint16 nbFields = reader.readU2();
  virtualFields = new (classLoader->allocator, "Fields") JavaField[nbFields];
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
    field->attributes = readAttributes(reader, field->nbAttributes);
  }
}

void Class::fillIMT(std::set<JavaMethod*>* meths) {
  for (uint32 i = 0; i < nbInterfaces; ++i) {
    interfaces[i]->fillIMT(meths);
  }

  if (super != NULL) {
    super->fillIMT(meths);
  }

  // Specification says that an invokeinterface also looks at j.l.Object.
  if (isInterface() || (super == NULL)) {
    for (uint32 i = 0; i < nbVirtualMethods; ++i) {
      JavaMethod& meth = virtualMethods[i];
      uint32_t index = InterfaceMethodTable::getIndex(meth.name, meth.type);
      meths[index].insert(&meth);
    }
  }
}

void Class::makeVT() {
  if (super == NULL) {
    virtualTableSize = JavaVirtualTable::getFirstJavaMethodIndex();
  } else  {
    virtualTableSize = super->virtualTableSize;
  }
  
  for (uint32 i = 0; i < nbVirtualMethods; ++i) {
    JavaMethod& meth = virtualMethods[i];
    if (meth.name->equals(classLoader->bootstrapLoader->finalize) &&
        meth.type->equals(classLoader->bootstrapLoader->clinitType)) {
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

  vmkit::BumpPtrAllocator& allocator = classLoader->allocator;
  virtualVT = new(allocator, virtualTableSize) JavaVirtualTable(this);
}

static void computeMirandaMethods(Class* current,
    Class* baseClass, std::vector<JavaMethod*>& mirandaMethods) {
  for (uint32 i = 0; i < current->nbInterfaces; i++) {
    Class* I = current->interfaces[i];
		// TODO: At this point, the interface may have not been read, so there
		// is no methods yet.
    for (uint32 j = 0; j < I->nbVirtualMethods; j++) {
      JavaMethod& orig = I->virtualMethods[j];
      JavaMethod* meth = baseClass->lookupMethodDontThrow(orig.name, orig.type,
                                                          false, true, 0);
      if (meth == NULL) {
        mirandaMethods.push_back(&orig);
      }
    }
    computeMirandaMethods(I, baseClass, mirandaMethods);
  }
}

static bool lessThanJavaMethods (JavaMethod* a,JavaMethod* b) {
	bool flag = a->name->lessThan(b->name);
	if (!flag) {
		flag = a->name->equals(b->name);
		if (flag) {
			flag = a->type->lessThan(b->type);
		}
	}
	return flag;
}

static bool cmpJavaMethods (JavaMethod* a,JavaMethod* b) {
	bool flag = a->name->equals(b->name) && a->type->equals(b->type);
	return flag;
}

static void cleanMirandaMethods(std::vector<JavaMethod*>& mirandaMethods) {
	std::sort(mirandaMethods.begin(), mirandaMethods.end(), lessThanJavaMethods);
	// using predicate comparison:
	std::vector<JavaMethod*>::iterator it;
	it = std::unique (mirandaMethods.begin(), mirandaMethods.end(), cmpJavaMethods);   // (no changes)
	mirandaMethods.resize( std::distance(mirandaMethods.begin(),it) ); // 10 20 30 20 10
}

void Class::readMethods(Reader& reader) {

  uint32 nbMethods = reader.readU2();
  vmkit::ThreadAllocator allocator;
  if (isAbstract(access)) {
    virtualMethods = (JavaMethod*)
      allocator.Allocate(nbMethods * sizeof(JavaMethod));
  } else {
    virtualMethods =
      new(classLoader->allocator, "Methods") JavaMethod[nbMethods];
  }
  staticMethods = virtualMethods + nbMethods;
  for (uint32 i = 0; i < nbMethods; i++) {
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
    meth->attributes = readAttributes(reader, meth->nbAttributes);
  }

  if (isAbstract(access)) {
    std::vector<JavaMethod*> mirandaMethods;
    computeMirandaMethods(this, this, mirandaMethods);
    uint32 size = mirandaMethods.size();
    if (size > 100) {
    	cleanMirandaMethods(mirandaMethods);
    	size = mirandaMethods.size();
    }
    nbMethods += size;
    JavaMethod* realMethods =
      new(classLoader->allocator, "Methods") JavaMethod[nbMethods];

    memcpy(realMethods + size, virtualMethods,
           sizeof(JavaMethod) * (nbMethods - size));

    nbVirtualMethods += size;
    staticMethods = realMethods + nbVirtualMethods;
    if (size != 0) {
      int j = 0;
      for (std::vector<JavaMethod*>::iterator i = mirandaMethods.begin(),
           e = mirandaMethods.end(); i != e; i++) {
        JavaMethod* cur = *i;
        realMethods[j++].initialise(this, cur->name, cur->type, cur->access);
      }
    }
    virtualMethods = realMethods;
  }
}

void Class::readClass() {
  PRINT_DEBUG(JNJVM_LOAD, 0, LIGHT_GREEN, "readClass \t");
  PRINT_DEBUG(JNJVM_LOAD, 0, DARK_MAGENTA, "%s\n", UTF8Buffer(this->name).cString());

  assert(getInitializationState() == loaded && "Wrong init state");

  Reader reader(bytes);
  uint32 magic;
  magic = reader.readU4();
  assert(magic == Jnjvm::Magic && "I've created a class but magic is no good!");

  uint16 minor = reader.readU2();
  uint16 major = reader.readU2();
  this->isClassVersionSupported(major, minor);

  uint32 ctpSize = reader.readU2();
  ctpInfo = new(classLoader->allocator, ctpSize) JavaConstantPool(this, reader,
                                                                  ctpSize);
  access |= reader.readU2();
  
  if (!isPublic(access)) access |= ACC_PRIVATE;

  const UTF8* thisClassName = 
    ctpInfo->resolveClassName(reader.readU2());
  
  if (!(thisClassName->equals(name))) {
    JavaThread::get()->getJVM()->noClassDefFoundError(this, thisClassName);
  }

  readParents(reader);
  readFields(reader);
  readMethods(reader);
  attributes = readAttributes(reader, nbAttributes);
}

void Class::getMinimalJDKVersion(uint16_t major, uint16_t minor, uint16_t& JDKMajor, uint16_t& JDKMinor, uint16_t& JDKBuild)
{
	JDKMajor = 1;
	JDKBuild = 0;
	if (major == 45 && minor <= 3) {			// Supported by Java 1.0.2
		JDKMinor = 0;
		JDKBuild = 2;
	} else if (major == 45 && minor <= 65535) {	// Supported by Java 1.1
		JDKMinor = 1;
	} else {									// Supported by Java 1.x (x >= 2)
		JDKMinor = major - 43;
		if (minor == 0) --JDKMinor;
	}
}

bool Class::isClassVersionSupported(uint16_t major, uint16_t minor)
{
	const uint16_t supportedJavaMinorVersion = 6;	// Java 1.6

	Class::getMinimalJDKVersion(major, minor, minJDKVersionMajor, minJDKVersionMinor, minJDKVersionBuild);

	bool res = (minJDKVersionMajor <= 1) && (minJDKVersionMinor <= supportedJavaMinorVersion);
	if (!res) {
		cerr << "WARNING: Class file '" << *name << "' requires Java version " << minJDKVersionMajor << '.' << minJDKVersionMinor <<
			". This JVM only supports Java versions up to 1." << supportedJavaMinorVersion << '.' << endl;
	}
	return res;
}

void UserClass::resolveParents() {
  if (super != NULL) {
    super->resolveClass();
  }

  for (unsigned i = 0; i < nbInterfaces; i++)
    interfaces[i]->resolveClass(); 
}

void Class::resolveClass() {
  if (isResolved() || isErroneous()) return;
  resolveParents();
  loadExceptions();
  // Do a compare and swap in case another thread initialized the class.
  __sync_val_compare_and_swap(
      &(getCurrentTaskClassMirror().status), loaded, resolved);
  assert(isResolved() || isErroneous());
}

void UserClass::resolveInnerOuterClasses() {
  if (!innerOuterResolved) {
    JavaAttribute* attribute = lookupAttribute(JavaAttribute::innerClassesAttribute);
    if (attribute != 0) {
      Reader reader(attribute, bytes);
      uint16 nbi = reader.readU2();
      for (uint16 i = 0; i < nbi; ++i) {
        uint16 inner = reader.readU2();
        uint16 outer = reader.readU2();
        uint16 innerName = reader.readU2();
        uint16 accessFlags = reader.readU2();
        UserClass* clInner = 0;
        UserClass* clOuter = 0;
        if (inner) clInner = (UserClass*)ctpInfo->loadClass(inner);
        if (outer) clOuter = (UserClass*)ctpInfo->loadClass(outer);

        if (clInner == this) {
          outerClass = clOuter;
          if (!innerName) isAnonymous = true;
        } else if (clOuter == this) {
          if (!innerClasses) {
            innerClasses = (Class**)
              classLoader->allocator.Allocate(nbi * sizeof(Class*),
                                              "Inner classes");
          }
          clInner->setInnerAccess(accessFlags);
          if (!innerName) clInner->isAnonymous = true;
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
  
  ArrayObject* res = NULL;
  JavaObject* delegatee = NULL;
  llvm_gcroot(res, 0);
  llvm_gcroot(delegatee, 0);
  
  Jnjvm* vm = JavaThread::get()->getJVM();
  Signdef* sign = getSignature();
  Typedef* const* arguments = sign->getArgumentsType();
  res = (ArrayObject*)vm->upcalls->classArrayClass->doNew(sign->nbArguments,vm);

  for (uint32 index = 0; index < sign->nbArguments; ++index) {
    delegatee = getClassType(vm, loader, arguments[index]);
    ArrayObject::setElement(res, delegatee, index);
  }

  return res;

}

JavaObject* JavaMethod::getReturnType(JnjvmClassLoader* loader) {
  JavaObject* obj = 0;
  llvm_gcroot(obj, 0);
  Jnjvm* vm = JavaThread::get()->getJVM();
  Typedef* ret = getSignature()->getReturnType();
  obj = getClassType(vm, loader, ret);
  return obj;
}

ArrayObject* JavaMethod::getExceptionTypes(JnjvmClassLoader* loader) {
  
  ArrayObject* res = NULL;
  JavaObject* delegatee = NULL;
  llvm_gcroot(res, 0);
  llvm_gcroot(delegatee, 0);
  
  JavaAttribute* exceptionAtt = lookupAttribute(JavaAttribute::exceptionsAttribute);
  Jnjvm* vm = JavaThread::get()->getJVM();
  if (exceptionAtt == 0) {
    return (ArrayObject*)vm->upcalls->classArrayClass->doNew(0, vm);
  } else {
    UserConstantPool* ctp = classDef->getConstantPool();
    Reader reader(exceptionAtt, classDef->bytes);
    uint16 nbe = reader.readU2();
    res = (ArrayObject*)vm->upcalls->classArrayClass->doNew(nbe, vm);

    for (uint16 i = 0; i < nbe; ++i) {
      uint16 idx = reader.readU2();
      UserCommonClass* cl = ctp->loadClass(idx);
      assert(cl->asClass() && "Wrong exception type");
      cl->asClass()->resolveClass();
      delegatee = cl->getClassDelegatee(vm);
      ArrayObject::setElement(res, delegatee, i);
    }
    return res;
  }
}


JavaObject* CommonClass::setDelegatee(JavaObject* val) {
  llvm_gcroot(val, 0);
  JavaObject** obj = &(delegatee[0]);
  if (*obj == NULL) {
    vmkit::Collector::objectReferenceNonHeapWriteBarrier((gc**)obj, (gc*)val);
  }
  return getDelegatee();
}


UserCommonClass* UserCommonClass::resolvedImplClass(Jnjvm* vm,
                                                    JavaObject* clazz,
                                                    bool doClinit) {
  JavaObjectClass* jcl = 0;
  llvm_gcroot(clazz, 0);
  llvm_gcroot(jcl, 0);

  UserCommonClass* cl = JavaObjectClass::getClass(jcl = (JavaObjectClass*)clazz);
  assert(cl && "No class in Class object");
  if (cl->isClass()) {
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
  
  for (sint32 i = 0; i < clen; ++i) {
    cur = jniConsClName->elements[i];
    if (cur == '/') {
      ptr[0] = '_';
      ptr++;
    } else if (cur == '_') {
      ptr[0] = '_';
      ptr[1] = '1';
      ptr += 2;
    } else if (cur == '$') {
      ptr[0] = '_';
      ptr[1] = '0';
      ptr[2] = '0';
      ptr[3] = '0';
      ptr[4] = '2';
      ptr[5] = '4';
      ptr += 6;
    } else {
      ptr[0] = (uint8)cur;
      ptr++;
    }
  }
  
  ptr[0] = '_';
  ++ptr;
  
  for (sint32 i = 0; i < mnlen; ++i) {
    cur = jniConsName->elements[i];
    if (cur == '/') {
      ptr[0] = '_';
      ++ptr;
    } else if (cur == '_') {
      ptr[0] = '_';
      ptr[1] = '1';
      ptr += 2;
    } else if (cur == '<') {
      ptr[0] = '_';
      ptr[1] = '0';
      ptr[2] = '0';
      ptr[3] = '0';
      ptr[4] = '3';
      ptr[5] = 'C';
      ptr += 6;
    } else if (cur == '>') {
      ptr[0] = '_';
      ptr[1] = '0';
      ptr[2] = '0';
      ptr[3] = '0';
      ptr[4] = '3';
      ptr[5] = 'E';
      ptr += 6;
    } else {
      ptr[0] = (uint8)cur;
      ++ptr;
    }
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
  
  for (sint32 i = 0; i < clen; ++i) {
    cur = jniConsClName->elements[i];
    if (cur == '/') {
      ptr[0] = '_';
      ++ptr;
    } else if (cur == '_') {
      ptr[0] = '_';
      ptr[1] = '1';
      ptr += 2;
    } else if (cur == '$') {
      ptr[0] = '_';
      ptr[1] = '0';
      ptr[2] = '0';
      ptr[3] = '0';
      ptr[4] = '2';
      ptr[5] = '4';
      ptr += 6;
    } else {
      ptr[0] = (uint8)cur;
      ++ptr;
    }
  }
  
  ptr[0] = '_';
  ++ptr;

  for (sint32 i = 0; i < mnlen; ++i) {
    cur = jniConsName->elements[i];
    if (cur == '/') ptr[0] = '_';
    else if (cur == '_') {
      ptr[0] = '_';
      ptr[1] = '1';
      ptr += 2;
    } else if (cur == '<') {
      ptr[0] = '_';
      ptr[1] = '0';
      ptr[2] = '0';
      ptr[3] = '0';
      ptr[4] = '3';
      ptr[5] = 'C';
      ptr += 6;
    } else if (cur == '>') {
      ptr[0] = '_';
      ptr[1] = '0';
      ptr[2] = '0';
      ptr[3] = '0';
      ptr[4] = '3';
      ptr[5] = 'E';
      ptr += 6;
    } else {
      ptr[0] = (uint8)cur;
      ++ptr;
    }
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
    } else if (c == '$') {
      ptr[0] = '_';
      ptr[1] = '0';
      ptr[2] = '0';
      ptr[3] = '0';
      ptr[4] = '2';
      ptr[5] = '4';
      ptr += 6;
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


ArrayUInt16* JavaMethod::toString() const { 
  Jnjvm* vm = JavaThread::get()->getJVM();
  uint32 size = classDef->name->size + name->size + type->size + 1;
  ArrayUInt16* res = (ArrayUInt16*)vm->upcalls->ArrayOfChar->doNew(size, vm);
  llvm_gcroot(res, 0);

  uint32 i = 0;
 
  for (sint32 j = 0; j < classDef->name->size; ++j) {
    if (classDef->name->elements[j] == '/') {
      ArrayUInt16::setElement(res, '.', i);
    } else {
      ArrayUInt16::setElement(res, classDef->name->elements[j], i);
    }
    i++;
  }

  ArrayUInt16::setElement(res, '.', i);
  i++;
  
  for (sint32 j = 0; j < name->size; ++j) {
    ArrayUInt16::setElement(res, name->elements[j], i);
    i++;
  }
  
  for (sint32 j = 0; j < type->size; ++j) {
    ArrayUInt16::setElement(res, type->elements[j], i);
    i++;
  }

  return res;
}


bool UserClass::needsInitialisationCheck() {
  
  if (isReady()) return false;

  if (super && super->needsInitialisationCheck())
    return true;

  if (nbStaticFields > 0) return true;

  JavaMethod* meth = 
    lookupMethodDontThrow(classLoader->bootstrapLoader->clinitName,
                          classLoader->bootstrapLoader->clinitType, 
                          true, false, 0);
  
  if (meth) return true;

  setInitializationState(ready);
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
                  true, false, NULL);
  
  ClassArray::InterfacesArray[1] = 
    JCL->loadName(JCL->asciizConstructUTF8("java/io/Serializable"),
                  true, false, NULL);
   
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
           sizeof(word_t) * JavaVirtualTable::getNumJavaMethods()); \
    CLASS->super = javaLangObject; \
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

    Class* referenceClass = 
        C->classLoader->bootstrapLoader->upcalls->newReference;
    if (referenceClass != NULL && C->super->isSubclassOf(referenceClass)) {
      tracer = (word_t)ReferenceObjectTracer;
    } else {
      tracer = (word_t)RegularObjectTracer;
    }
    operatorDelete = 0;
    
    // Set IMT.
    if (!isAbstract(C->access)) {
      IMT = new (C->classLoader->allocator, "IMT") InterfaceMethodTable();
    }
    
    // Set the class of this VT.
    cl = C;
    
    // Set depth and display for fast dynamic type checking.
    JavaVirtualTable* superVT = C->super->virtualVT; 
    assert(superVT && "Super has no VT");
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

    vmkit::BumpPtrAllocator& allocator = C->classLoader->allocator;
    secondaryTypes = (JavaVirtualTable**)
      allocator.Allocate(sizeof(JavaVirtualTable*) * nbSecondaryTypes,
                         "Secondary types");  
    
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

    if (nbSecondaryTypes) {
    	std::sort(secondaryTypes, &secondaryTypes[nbSecondaryTypes],LessThanPtrVirtualTable);
    	JavaVirtualTable** it = std::unique (secondaryTypes, &secondaryTypes[nbSecondaryTypes], equalsPtrVirtualTable);   // (no changes)
    	nbSecondaryTypes = std::distance(secondaryTypes,it);
    	//isSortedSecondaryTypes = true;
    }


  } else {
    // Set the tracer, destructor and delete.
    tracer = (word_t)JavaObjectTracer;
    operatorDelete = 0;
    
    // Set the class of this VT.
    cl = C;
    
    // Set depth and display for fast dynamic type checking.
    // java.lang.Object does not have any secondary types.
    offset = getCacheIndex() + 1;
    depth = 0;
    display[0] = this;
    nbSecondaryTypes = 0;
  }
}
  
JavaVirtualTable::JavaVirtualTable(ClassArray* C) {
  
  if (C->baseClass()->isClass())
    C->baseClass()->asClass()->resolveClass();

  if (!C->baseClass()->isPrimitive()) {
    baseClassVT = C->baseClass()->virtualVT;

    // Copy the super VT into the current VT.
    uint32 size = (getBaseSize() - getFirstJavaMethodIndex());
    memcpy(this->getFirstJavaMethod(),
           C->super->virtualVT->getFirstJavaMethod(),
           size * sizeof(word_t));
    tracer = (word_t)ArrayObjectTracer;
    
    // Set the class of this VT.
    cl = C;

    // Set depth and display for fast dynamic type checking.
    JnjvmClassLoader* JCL = cl->classLoader;
    Classpath* upcalls = JCL->bootstrapLoader->upcalls;
    
    if (upcalls->ArrayOfObject) {
      UserCommonClass* base = C->baseClass();
      uint32 dim = 1;
      while (base->isArray()) {
        base = base->asArrayClass()->baseClass();
        ++dim;
      }
     
      bool newSecondaryTypes = false;
      bool intf = base->isInterface();
      ClassArray* super = 0;
      
      if (base->isPrimitive()) {
        // If the base class is primitive, then the super is one
        // dimension below, e.g. the super of int[][] is Object[].
        --dim;
        const UTF8* superName = JCL->constructArrayName(dim, C->super->name);
        super = JCL->constructArray(superName);
      } else if (base == C->super) {
        // If the base class is java.lang.Object, then the super is one
        // dimension below, e.g. the super of Object[][] is Object[].
        // Also, the class is the first class in the dimension hierarchy,
        // so it must create a new secondary type list.
        --dim;
        newSecondaryTypes = true;
        super = C->baseClass()->asArrayClass();
      } else {
        // If the base class is any other class, interface or not,
        // the super is of the dimension of the current array class,
        // and whose base class is the super of this base class.
        const UTF8* superName = JCL->constructArrayName(dim, base->super->name);
        JnjvmClassLoader* superLoader = base->super->classLoader;
        super = superLoader->constructArray(superName);
      }
    
      assert(super && "No super found");
      JavaVirtualTable* superVT = super->virtualVT;
      depth = superVT->depth + 1;
      
      // Record if we need to add the super in the list of secondary types.
      uint32 addSuper = 0;

      uint32 length = getDisplayLength() < depth ? getDisplayLength() : depth;
      memcpy(display, superVT->display, length * sizeof(JavaVirtualTable*)); 
      if (depth < getDisplayLength() && !intf) {
        display[depth] = this;
        offset = getCacheIndex() + depth + 1;
      } else {
        offset = getCacheIndex();
        // We add the super if the current class is an interface or if the super
        // class is out of depth.
        if (intf || depth != getDisplayLength()) addSuper = 1;
      }
        
      vmkit::BumpPtrAllocator& allocator = JCL->allocator;

      if (!newSecondaryTypes) {
        if (base->nbInterfaces || addSuper) {
          // If the base class implements interfaces, we must also add the
          // arrays of these interfaces, of the same dimension than this array
          // class and add them to the secondary types list.
          // Finally, we must add the list of array of secondary classes from base
          nbSecondaryTypes = base->nbInterfaces + superVT->nbSecondaryTypes +
                                addSuper
//                                + base->virtualVT->nbSecondaryTypes
                                ;
          secondaryTypes = (JavaVirtualTable**)
            allocator.Allocate(sizeof(JavaVirtualTable*) * nbSecondaryTypes,
                               "Secondary types");
         
          // Put the super in the list of secondary types.
          if (addSuper) secondaryTypes[0] = superVT;

          // Copy the list of secondary types of the super.
          memcpy(secondaryTypes + addSuper, superVT->secondaryTypes,
                 superVT->nbSecondaryTypes * sizeof(JavaVirtualTable*));
        
          // Add our own secondary types: the interfaces of the base class put
          // in the dimension of the current array class.
          for (uint32 i = 0; i < base->nbInterfaces; ++i) {
            const UTF8* name = 
              JCL->constructArrayName(dim, base->interfaces[i]->name);
            ClassArray* interface = JCL->constructArray(name);
            JavaVirtualTable* CurVT = interface->virtualVT;
            secondaryTypes[i + superVT->nbSecondaryTypes + addSuper] = CurVT;
          }

//          int index = superVT->nbSecondaryTypes + addSuper + base->nbInterfaces;
//          for (uint32 i = 0; i < base->virtualVT->nbSecondaryTypes; ++i) {
//        	  if (base->virtualVT == base->virtualVT->secondaryTypes[i]) continue;
//
//			  const UTF8* name =
//				JCL->constructArrayName(dim, base->virtualVT->secondaryTypes[i]->cl->name);
//			  ClassArray* interface = JCL->constructArray(name);
//			  JavaVirtualTable* CurVT = interface->virtualVT;
//			  secondaryTypes[index++] = CurVT;
//           }
//
//          nbSecondaryTypes = index;
        } else {
          // If the super is not a secondary type and the base class does not
          // implement any interface, we can reuse the list of secondary types
          // of super. If the base class is a primitive, the array class shares
          // the same secondary types than the super of the current superVT
          // (eg Object[][] for int[][]).
          if (base->isPrimitive()) {
            const UTF8* superName = JCL->constructArrayName(dim + 1, C->super->name);
            super = JCL->constructArray(superName);
            superVT = super->virtualVT;
          }
          nbSecondaryTypes = superVT->nbSecondaryTypes;
          secondaryTypes = superVT->secondaryTypes;
        }
      } else {

        // This is an Object[....] array class. It will create the list of
        // secondary types and all array classes of the same dimension whose
        // base class does not have interfaces point to this array.

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
          allocator.Allocate(sizeof(JavaVirtualTable*) * nbSecondaryTypes,
                             "Secondary types");
        
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
      }
    } else {
      // This is java.lang.Object[].
      depth = 1;
      display[0] = C->super->virtualVT;
      display[1] = this;
      offset = getCacheIndex() + 2;
      nbSecondaryTypes = 2;
      
      vmkit::BumpPtrAllocator& allocator = JCL->allocator;
      secondaryTypes = (JavaVirtualTable**)
        allocator.Allocate(sizeof(JavaVirtualTable*) * nbSecondaryTypes,
                           "Secondary types");

      // The interfaces have already been resolved.
      secondaryTypes[0] = cl->interfaces[0]->virtualVT;
      secondaryTypes[1] = cl->interfaces[1]->virtualVT;
    }

  } else {
    // Set the tracer, destructor and delete
    tracer = (word_t)JavaObjectTracer;
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
  if (offset == getCacheIndex() && nbSecondaryTypes) {
	std::sort(secondaryTypes, &secondaryTypes[nbSecondaryTypes],LessThanPtrVirtualTable);
	JavaVirtualTable** it = std::unique (secondaryTypes, &secondaryTypes[nbSecondaryTypes], equalsPtrVirtualTable);   // (no changes)
	nbSecondaryTypes = std::distance(secondaryTypes,it);
	//isSortedSecondaryTypes = true;
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

void AnnotationReader::readAnnotation() {
  uint16 typeIndex = reader.readU2();
  uint16 numPairs = reader.readU2();

  for (uint16 j = 0; j < numPairs; ++j) {
    /* uint16 nameIndex = */ reader.readU2();
    readElementValue();
  }
  AnnotationNameIndex = typeIndex;
}

void AnnotationReader::readAnnotationElementValues() {
  uint16 numPairs = reader.readU2();

  for (uint16 j = 0; j < numPairs; ++j) {
    reader.readU2();
    readElementValue();
  }
}

void AnnotationReader::readElementValue() {
  uint8 tag = reader.readU1();
  if ((tag == 'B') || (tag == 'C') || (tag == 'D') || (tag == 'F') ||
      (tag == 'J') || (tag == 'S') || (tag == 'I') || (tag == 'Z') || 
      (tag == 's')) {
    /* uint16 constValue = */ reader.readU2();
  } else if (tag == 'e') {
    /* uint16 typeName = */ reader.readU2();
    /* uint16 constName = */ reader.readU2();
  } else if (tag == 'c') {
    /* uint16 classInfoIndex = */ reader.readU2();
  } else if (tag == '@') {
    readAnnotation();
  } else if (tag == '[') {
    uint16 numValues = reader.readU2();
    for (uint32 i = 0; i < numValues; ++i) {
      readElementValue();
    }
  }
}

JavaObject* AnnotationReader::createElementValue(bool nextParameterIsTypeOfMethod, JavaObject* type, const UTF8* lastKey) {
  JavaObject* res = 0;
  JavaObject* tmp = 0;
  JavaString* str = 0;
  JavaObject* field = 0;
  JavaObject* clazzLoaded = 0;
  JavaObject* classOfCurrentAnnotationProperty = 0;
  JavaObject* newHashMap = 0;
  JavaObject* annotationClass = 0;
  llvm_gcroot(res, 0);
  llvm_gcroot(tmp, 0);
  llvm_gcroot(str, 0);
  llvm_gcroot(clazzLoaded, 0);
  llvm_gcroot(field, 0);
  llvm_gcroot(type, 0);
  llvm_gcroot(classOfCurrentAnnotationProperty, 0);
  llvm_gcroot(newHashMap, 0);
  llvm_gcroot(annotationClass, 0);

  uint8 tag = reader.readU1();
	
  Jnjvm* vm = JavaThread::get()->getJVM();
  Classpath* upcalls = vm->upcalls;
  ddprintf("value:");

  if (tag == 'B') {
    sint32 val = cl->ctpInfo->IntegerAt(reader.readU2());
    ddprintf("B=%d", val);
    res = upcalls->byteClass->doNew(vm);
    upcalls->byteValue->setInstanceInt8Field(res, val);

  } else if (tag == 'C') {
    uint32 val = cl->ctpInfo->IntegerAt(reader.readU2());
    ddprintf("C=%c", val);
    res = upcalls->charClass->doNew(vm);
    upcalls->charValue->setInstanceInt16Field(res, val);

  } else if (tag == 'D') {
    double val = cl->ctpInfo->DoubleAt(reader.readU2());
    ddprintf("D=%f", val);
    res = upcalls->doubleClass->doNew(vm);
    upcalls->doubleValue->setInstanceDoubleField(res, val);

  } else if (tag == 'F') {
    float val = cl->ctpInfo->FloatAt(reader.readU2());
    ddprintf("F=%f", val);
    res = upcalls->floatClass->doNew(vm);
    upcalls->floatValue->setInstanceFloatField(res, val);

  } else if (tag == 'J') {
    sint64 val = cl->ctpInfo->LongAt(reader.readU2());
    ddprintf("J=%lld", val);
    res = upcalls->longClass->doNew(vm);
    upcalls->longValue->setInstanceLongField(res, val);

  } else if (tag == 'S') {
    uint32 val = cl->ctpInfo->IntegerAt(reader.readU2());
    ddprintf("S=%d", val);
    res = upcalls->shortClass->doNew(vm);
    upcalls->shortValue->setInstanceInt16Field(res, val);
    
  } else if (tag == 'I') {
    sint32 val = cl->ctpInfo->IntegerAt(reader.readU2());
    ddprintf("I=%d", val);
    res = upcalls->intClass->doNew(vm);
    upcalls->intValue->setInstanceInt32Field(res, val);

  } else if (tag == 'Z') {
    bool val = cl->ctpInfo->IntegerAt(reader.readU2());
    ddprintf("Z=%d", val);
    res = upcalls->boolClass->doNew(vm);
    upcalls->boolValue->setInstanceInt8Field(res, val);

  } else if (tag == 's') {
    const UTF8* s = cl->ctpInfo->UTF8At(reader.readU2());
    ddprintf("s=%s", PrintBuffer(s).cString());
    res = JavaString::internalToJava(s, vm);

  } else if (tag == 'e') {
    const UTF8* n = cl->ctpInfo->UTF8At(reader.readU2());
    const UTF8* m = cl->ctpInfo->UTF8At(reader.readU2());
    JnjvmClassLoader* JCL = this->cl->classLoader;
    n = n->extract(JCL->hashUTF8, 1,n->size-1);
    UserCommonClass* cl = JCL->loadClassFromUserUTF8(n,true,false, NULL);
    if (cl != NULL && !cl->isPrimitive()) {
    	if (cl->asClass())
    		cl->asClass()->initialiseClass(vm);
    	clazzLoaded = cl->getClassDelegatee(vm);
    	str = JavaString::internalToJava(m, vm);
    	assert(clazzLoaded && "Class not found");
    	assert(str && "Null string");
    	field = upcalls->getFieldInClass->invokeJavaObjectVirtual(vm, upcalls->newClass, clazzLoaded, &str);
    	assert(field && "Field not found");
    	JavaObject* obj = 0;
    	res = upcalls->getInField->invokeJavaObjectVirtual(vm, upcalls->newField, field, &obj);
    } else {
    	str = JavaString::internalToJava(n, vm);
    	vm->classNotFoundException(str);
    }
  } else if (tag == 'c') {
    ddprintf("class=");
    const UTF8* m = cl->ctpInfo->UTF8At(reader.readU2());

    JnjvmClassLoader* JCL = this->cl->classLoader;

    m = m->extract(JCL->hashUTF8, 1,m->size-1);

    UserCommonClass* cl = JCL->loadClassFromUserUTF8(m,true,false, NULL);

	if (cl != NULL && !cl->isPrimitive()) {
		if (cl->asClass())
		  cl->asClass()->initialiseClass(vm);
		res = cl->getClassDelegatee(vm);
	} else {
	  str = JavaString::internalToJava(m, vm);
	  vm->classNotFoundException(str);
	}

  } else if (tag == '[') {
    uint16 numValues = reader.readU2();
    classOfCurrentAnnotationProperty = type;
    if (!nextParameterIsTypeOfMethod) {
		UserCommonClass* uss =  UserCommonClass::resolvedImplClass(vm, type, false);
		//printf("Class name : %s\n", UTF8Buffer(uss->name).cString());
		UserClass* clazzOfAnnotation = uss->asClass();
		uint16 i = 0;
		while ( i < clazzOfAnnotation->nbVirtualMethods && !(lastKey->equals(clazzOfAnnotation->virtualMethods[i].name))) i++;
		assert((i < clazzOfAnnotation->nbVirtualMethods) && "Incorrect property for annotation");
		classOfCurrentAnnotationProperty = clazzOfAnnotation->virtualMethods[i].getReturnType(this->cl->classLoader);
    }
	UserClassArray* clazzOfProperty = UserCommonClass::resolvedImplClass(vm, classOfCurrentAnnotationProperty, false)->asArrayClass();
	//printf("Class name is : %s\n", UTF8Buffer(clazzOfProperty->name).cString());
	res = clazzOfProperty->doNew(numValues, vm);
	assert(res && "Error creating array of that type");
    fillArray(res, numValues, clazzOfProperty);

  } else if (tag == '@') {
	  uint16 typeIndex = reader.readU2();
	  annotationClass = type;
	  if (!nextParameterIsTypeOfMethod) {
	  		UserCommonClass* uss =  UserCommonClass::resolvedImplClass(vm, type, false);
	  		//printf("Class name : %s\n", UTF8Buffer(uss->name).cString());
	  		UserClass* clazzOfAnnotation = uss->asClass();
	  		uint16 i = 0;
	  		while ( i < clazzOfAnnotation->nbVirtualMethods && !(lastKey->equals(clazzOfAnnotation->virtualMethods[i].name))) i++;
	  		assert((i < clazzOfAnnotation->nbVirtualMethods) && "Incorrect property for annotation");
	  		annotationClass = clazzOfAnnotation->virtualMethods[i].getReturnType(this->cl->classLoader);
	 }

	  newHashMap = createAnnotationMapValues(annotationClass);
	  res = upcalls->createAnnotation->invokeJavaObjectStatic(vm, upcalls->newAnnotationHandler, &annotationClass, &newHashMap);
  } else {
    fprintf(stderr, "Wrong classfile format\n");
    abort();
  }
  ddprintf("\n");

  return res;
}

void AnnotationReader::fillArray(JavaObject* res, int numValues, UserClassArray* classArray) {
	llvm_gcroot(res, 0);
	JavaString* str = 0;
	JavaObject* clazzObject = 0;
	JavaObject* field = 0;
	JavaObject* clazzLoaded = 0;
	JavaObject* myEnum = 0;
	JavaObject* annotationClass = 0;
	JavaObject* newHashMap = 0;

	// for cast
	ArraySInt8* aSInt8 = 0;
	ArrayUInt8* aUInt8 = 0;
	ArraySInt16* aSInt16 = 0;
	ArraySInt32* aSInt32 = 0;
	ArrayLong* aSLong = 0;
	ArrayDouble* aDouble = 0;
	ArrayFloat* aFloat = 0;
	ArrayObject* aObject = 0;

	llvm_gcroot(str, 0);
	llvm_gcroot(clazzObject, 0);
	llvm_gcroot(clazzLoaded, 0);
	llvm_gcroot(field, 0);
	llvm_gcroot(myEnum, 0);
	llvm_gcroot(annotationClass, 0);
	llvm_gcroot(newHashMap, 0);
	llvm_gcroot(aSInt8, 0);
	llvm_gcroot(aUInt8, 0);
	llvm_gcroot(aSInt16, 0);
	llvm_gcroot(aSInt32, 0);
	llvm_gcroot(aSLong, 0);
	llvm_gcroot(aDouble, 0);
	llvm_gcroot(aFloat, 0);
	llvm_gcroot(aObject, 0);

	sint32 valS;
	uint32 valU;
	sint64 val64S;
	double valD;
	float valF;
	const UTF8* s = 0, *n = 0, * m = 0;
	JnjvmClassLoader* JCL;
	UserCommonClass* clLoaded;
	UserCommonClass* aaa;

	Jnjvm* vm = JavaThread::get()->getJVM();
	Classpath* upcalls = vm->upcalls;
	for (int i = 0 ; i < numValues ; i++) {
		uint8 tag = reader.readU1();
		switch (tag) {
			case 'B':
				valS = cl->ctpInfo->IntegerAt(reader.readU2());
				aSInt8 = (ArraySInt8*)res;
				ArraySInt8::setElement(aSInt8, valS, i);
				break;
			case 'C':
				valS = cl->ctpInfo->IntegerAt(reader.readU2());
				aSInt16 = (ArraySInt16*)res;
				ArraySInt16::setElement(aSInt16, valS, i);
				break;
			case 'D':
				valD = cl->ctpInfo->DoubleAt(reader.readU2());
				aDouble = (ArrayDouble*)res;
				ArrayDouble::setElement(aDouble, valD, i);
				break;
			case 'F':
				valF = cl->ctpInfo->FloatAt(reader.readU2());
				aFloat = (ArrayFloat*)res;
				ArrayFloat::setElement(aFloat, valF, i);
				break;
			case 'J':
				val64S = cl->ctpInfo->LongAt(reader.readU2());
				aSLong = (ArrayLong*)res;
				ArrayLong::setElement(aSLong, val64S, i);
				break;
			case 'S':
				valS = cl->ctpInfo->IntegerAt(reader.readU2());
				aSInt16 = (ArraySInt16*)res;
				ArraySInt16::setElement(aSInt16, valS, i);
				break;
			case 'I':
				valS = cl->ctpInfo->IntegerAt(reader.readU2());
				aSInt32 = (ArraySInt32*)res;
				ArraySInt32::setElement(aSInt32, valS,i);
				break;
			case 'Z':
				valU = cl->ctpInfo->IntegerAt(reader.readU2());
				aUInt8 = (ArrayUInt8*)res;
				ArrayUInt8::setElement(aUInt8, valU, i);
				break;
			case 's':
				s = cl->ctpInfo->UTF8At(reader.readU2());
				str = JavaString::internalToJava(s, JavaThread::get()->getJVM());
				aObject = (ArrayObject *)res;
				ArrayObject::setElement(aObject, str, i);
				break;
			case 'e':
				n = cl->ctpInfo->UTF8At(reader.readU2());
				m = cl->ctpInfo->UTF8At(reader.readU2());
				JCL = this->cl->classLoader;
				n = n->extract(JCL->hashUTF8, 1,n->size-1);
				clLoaded = JCL->loadClassFromUserUTF8(n,true,false, NULL);
				if (clLoaded != NULL && !clLoaded->isPrimitive()) {
					if (clLoaded->asClass())
						clLoaded->asClass()->initialiseClass(vm);
					clazzLoaded = clLoaded->getClassDelegatee(vm);
					str = JavaString::internalToJava(m, vm);
					assert(clazzLoaded && "Class not found");
					assert(str && "Null string");
					field = upcalls->getFieldInClass->invokeJavaObjectVirtual(vm, upcalls->newClass, clazzLoaded, &str);
					assert(field && "Field not found");
					JavaObject* obj = 0;
					myEnum = upcalls->getInField->invokeJavaObjectVirtual(vm, upcalls->newField, field, &obj);
					aObject = (ArrayObject *)res;
					ArrayObject::setElement(aObject, str, i);
				} else {
					str = JavaString::internalToJava(n, vm);
					vm->classNotFoundException(str);
				}
				break;
			case 'c':
				s = cl->ctpInfo->UTF8At(reader.readU2());
				JCL = this->cl->classLoader;
				s = s->extract(JCL->hashUTF8, 1,s->size-1);
				clLoaded = JCL->loadClassFromUserUTF8(s,true,false, NULL);
				if (clLoaded != NULL && !clLoaded->isPrimitive()) {
					if (clLoaded->asClass())
						clLoaded->asClass()->initialiseClass(vm);
					clazzObject = clLoaded->getClassDelegatee(vm);
				} else {
				  str = JavaString::internalToJava(s, vm);
				  vm->classNotFoundException(str);
				}
				aObject = (ArrayObject *)res;
				ArrayObject::setElement(aObject, clazzObject, i);
				break;
			case '@':
				reader.readU2();
				aaa = classArray->_baseClass;
				annotationClass = aaa->getDelegatee();
				newHashMap = createAnnotationMapValues(annotationClass);
				clazzObject = upcalls->createAnnotation->invokeJavaObjectStatic(vm, upcalls->newAnnotationHandler, &annotationClass, &newHashMap);
				aObject = (ArrayObject *)res;
				ArrayObject::setElement(aObject, clazzObject, i);
				break;
			default:
				fprintf(stderr, "Wrong class file\n");
				abort();
				break;
			}
	}
}

JavaObject* AnnotationReader::createAnnotationMapValues(JavaObject* type) {
  std::pair<JavaObject*, JavaObject*> pair;
  JavaObject* tmp = 0;
  JavaString* str = 0;
  JavaObject* newHashMap = 0;
  llvm_gcroot(type, 0);
  llvm_gcroot(tmp, 0);
  llvm_gcroot(str, 0);
  llvm_gcroot(newHashMap, 0);


  const UTF8* key = 0;

  Jnjvm * vm = JavaThread::get()->getJVM();
  Classpath* upcalls = vm->upcalls;
  UserClass* HashMap = upcalls->newHashMap;
  newHashMap = HashMap->doNew(vm);
  upcalls->initHashMap->invokeIntSpecial(vm, HashMap, newHashMap);

  uint16 numPairs = reader.readU2();
  dprintf("numPairs:%d\n", numPairs);
  for (uint16 j = 0; j < numPairs; ++j) {
    uint16 nameIndex = reader.readU2();
    key = cl->ctpInfo->UTF8At(nameIndex);
    dprintf("keyAn:%s|", PrintBuffer(key).cString());

    tmp = createElementValue(false, type, key);
    str = JavaString::internalToJava(key, vm);
    upcalls->putHashMap->invokeJavaObjectVirtual(vm, HashMap, newHashMap, &str, &tmp);
  }

  // annotationType
  str = vm->asciizToStr("annotationType");
  upcalls->putHashMap->invokeJavaObjectVirtual(vm, HashMap, newHashMap, &str, &type);

  return newHashMap;
}

uint16 JavaMethod::lookupLineNumber(vmkit::FrameInfo* info) {
  JavaAttribute* codeAtt = lookupAttribute(JavaAttribute::codeAttribute);      
  if (codeAtt == NULL) return 0;
  Reader reader(codeAtt, classDef->bytes);
  reader.readU2(); // max_stack
  reader.readU2(); // max_locals;
  uint32_t codeLength = reader.readU4();
  reader.seek(codeLength, Reader::SeekCur);
  uint16_t exceptionTableLength = reader.readU2();
  reader.seek(8 * exceptionTableLength, Reader::SeekCur);
  uint16_t nba = reader.readU2();
  for (uint16 att = 0; att < nba; ++att) {
    const UTF8* attName = classDef->ctpInfo->UTF8At(reader.readU2());
    uint32 attLen = reader.readU4();
    if (attName->equals(JavaAttribute::lineNumberTableAttribute)) {
      uint16_t lineLength = reader.readU2();
      uint16_t currentLine = 0;
      for (uint16 j = 0; j < lineLength; ++j) {
        uint16 pc = reader.readU2();
        if (pc > info->SourceIndex) return currentLine;
        currentLine = reader.readU2();
      }
      return currentLine;
    } else {
      reader.seek(attLen, Reader::SeekCur);      
    }
  }
  return 0;
}

uint16 JavaMethod::lookupCtpIndex(vmkit::FrameInfo* FI) {
  JavaAttribute* codeAtt = lookupAttribute(JavaAttribute::codeAttribute);
  Reader reader(codeAtt, classDef->bytes);
  reader.cursor = reader.cursor + 2 + 2 + 4 + FI->SourceIndex + 1;
  return reader.readU2();
}


void Class::acquire() {
  JavaObject* delegatee = NULL;
  llvm_gcroot(delegatee, 0);
  delegatee = getClassDelegatee(JavaThread::get()->getJVM());
  JavaObject::acquire(delegatee);
}
  
void Class::release() {
  JavaObject* delegatee = NULL;
  llvm_gcroot(delegatee, 0);
  delegatee = getClassDelegatee(JavaThread::get()->getJVM());
  JavaObject::release(delegatee);
}

void Class::waitClass() {
  JavaObject* delegatee = NULL;
  llvm_gcroot(delegatee, 0);
  delegatee = getClassDelegatee(JavaThread::get()->getJVM());
  JavaObject::wait(delegatee);
}
  
void Class::broadcastClass() {
  JavaObject* delegatee = NULL;
  llvm_gcroot(delegatee, 0);
  delegatee = getClassDelegatee(JavaThread::get()->getJVM());
  JavaObject::notifyAll(delegatee);
}

std::string& CommonClass::getName(std::string& nameBuffer, bool linkageName) const
{
	name->toString(nameBuffer);

	for (int32_t i=0; i < name->size; ++i) {
		if (name->elements[i] == '/')
			nameBuffer[i] = linkageName ? '_' : '.';
	}

	return nameBuffer;
}

std::string& JavaMethod::getName(std::string& nameBuffer, bool linkageName) const
{
	classDef->getName(nameBuffer, linkageName);
	nameBuffer += linkageName ? '_' : '.';

	string methName;
	return nameBuffer += name->toString(methName);
}

std::ostream& j3::operator << (std::ostream& os, const JavaMethod& m)
{
	return os << *m.classDef->name << '.' << *m.name << " (" << *m.type << ')';
}

void JavaMethod::dump() const
{
	cerr << *this << endl;
}

JavaField_IMPL_ASSESSORS(float, Float)
JavaField_IMPL_ASSESSORS(double, Double)
JavaField_IMPL_ASSESSORS(uint8, Int8)
JavaField_IMPL_ASSESSORS(uint16, Int16)
JavaField_IMPL_ASSESSORS(uint32, Int32)
JavaField_IMPL_ASSESSORS(sint64, Long)
// JavaField_IMPL_ASSESSORS(JavaObject*, Object)

JavaObject* JavaField::getStaticObjectField()
{
	return getStaticField<JavaObject*>();
}

void JavaField::setStaticObjectField(JavaObject* val)
{
	llvm_gcroot(val, 0);
	return setStaticField<JavaObject*>(val);
}

JavaObject* JavaField::getInstanceObjectField(JavaObject* obj)
{
	llvm_gcroot(obj, 0);
	return this->getInstanceField<JavaObject*>(obj);
}

void JavaField::setInstanceObjectField(JavaObject* obj, JavaObject* val)
{
	llvm_gcroot(obj, 0);
	llvm_gcroot(val, 0);
	return this->setInstanceField<JavaObject*>(obj, val);
}

template<>
void JavaField::setInstanceField(JavaObject* obj, JavaObject* val)
{
	llvm_gcroot(obj, 0);
	llvm_gcroot(val, 0);
	FieldSetter<JavaObject*>::setInstanceField(this, obj, val);
}

template <>
void JavaField::setStaticField(JavaObject* val)
{
	llvm_gcroot(val, 0);
	FieldSetter<JavaObject*>::setStaticField(this, val);
}

std::ostream& j3::operator << (std::ostream& os, const CommonClass& ccl)
{
	os << *ccl.name;
	return (!ccl.super) ? (os << ';') : (os << ':' << *ccl.super);
}

void CommonClass::dump() const
{
	cerr << *this << endl;
}
