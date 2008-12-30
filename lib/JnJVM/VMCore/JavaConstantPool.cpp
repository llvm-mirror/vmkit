//===--- JavaConstantPool.cpp - Java constant pool definition ---------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#define JNJVM_LOAD 0

#include <alloca.h>
#include <cstdio>
#include <cstdlib>

#include "debug.h"

#include "JavaAccess.h"
#include "JavaArray.h"
#include "JavaClass.h"
#include "JavaConstantPool.h"
#include "Jnjvm.h"
#include "JnjvmModule.h"
#include "JnjvmModuleProvider.h"
#include "JavaThread.h"
#include "JavaTypes.h"
#include "LockedMap.h"
#include "Reader.h"
 
using namespace jnjvm;

const uint32 JavaConstantPool::ConstantUTF8 = 1;
const uint32 JavaConstantPool::ConstantInteger = 3;
const uint32 JavaConstantPool::ConstantFloat = 4;
const uint32 JavaConstantPool::ConstantLong = 5;
const uint32 JavaConstantPool::ConstantDouble = 6;
const uint32 JavaConstantPool::ConstantClass = 7;
const uint32 JavaConstantPool::ConstantString = 8;
const uint32 JavaConstantPool::ConstantFieldref = 9;
const uint32 JavaConstantPool::ConstantMethodref = 10;
const uint32 JavaConstantPool::ConstantInterfaceMethodref = 11;
const uint32 JavaConstantPool::ConstantNameAndType = 12;


static uint32 unimplemented(JavaConstantPool* ctp, Reader& reader,
                            uint32 index) {
  JavaThread::get()->getJVM()->classFormatError(
                                    "unknown constant pool at index %d", 
                                    index);
  return 1;
}


uint32 JavaConstantPool::CtpReaderClass(JavaConstantPool* ctp, Reader& reader,
                                   uint32 index) {
  uint16 entry = reader.readU2();
  ctp->ctpDef[index] = entry;
  PRINT_DEBUG(JNJVM_LOAD, 3, COLOR_NORMAL, "; [%5d] <class>\t\tutf8 is at %d\n", e,
              entry);
  return 1;
}
  
uint32 JavaConstantPool::CtpReaderInteger(JavaConstantPool* ctp, Reader& reader,
                                     uint32 index) {
  uint32 val = reader.readU4();
  ctp->ctpDef[index] = val;
  PRINT_DEBUG(JNJVM_LOAD, 3, COLOR_NORMAL, "; [%5d] <class>\tinteger: %d\n", e,
              val);
  return 1;
}
  
uint32 JavaConstantPool::CtpReaderFloat(JavaConstantPool* ctp, Reader& reader,
                                   uint32 index) { 
  uint32 val = reader.readU4();
  ctp->ctpDef[index] = val;
  PRINT_DEBUG(JNJVM_LOAD, 3, COLOR_NORMAL, "; [%5d] <class>\tfloat: %d\n", e,
              val);
  return 1;
}
  
uint32 JavaConstantPool::CtpReaderUTF8(JavaConstantPool* ctp, Reader& reader,
                                  uint32 index) { 
  ctp->ctpDef[index] = reader.cursor;
  uint16 len = reader.readU2();
  reader.cursor += len;
  return 1;
}
  
uint32 JavaConstantPool::CtpReaderNameAndType(JavaConstantPool* ctp, Reader& reader,
                                         uint32 index) {
  uint32 entry = reader.readU4();
  ctp->ctpDef[index] = entry;
  PRINT_DEBUG(JNJVM_LOAD, 3, COLOR_NORMAL, 
              "; [%5d] <name/type>\tname is at %d, type is at %d\n", index,
              (entry >> 16), (entry & 0xffff));
  return 1;
}
  
uint32 JavaConstantPool::CtpReaderFieldref(JavaConstantPool* ctp, Reader& reader,
                                      uint32 index) {
  uint32 entry = reader.readU4();
  ctp->ctpDef[index] = entry;
  PRINT_DEBUG(JNJVM_LOAD, 3, COLOR_NORMAL, 
              "; [%5d] <fieldref>\tclass is at %d, name/type is at %d\n", index,
              (entry >> 16), (entry & 0xffff));
  return 1;
}

uint32 JavaConstantPool::CtpReaderString(JavaConstantPool* ctp, Reader& reader,
                                    uint32 index) {
  uint16 entry = reader.readU2();
  ctp->ctpDef[index] = entry;
  PRINT_DEBUG(JNJVM_LOAD, 3, COLOR_NORMAL, "; [%5d] <string>\tutf8 is at %d\n",
              index, entry);
  return 1;
}
  
uint32 JavaConstantPool::CtpReaderMethodref(JavaConstantPool* ctp, Reader& reader,
                                       uint32 index) {
  uint32 entry = reader.readU4();
  ctp->ctpDef[index] = entry;
  PRINT_DEBUG(JNJVM_LOAD, 3, COLOR_NORMAL, 
              "; [%5d] <methodref>\tclass is at %d, name/type is at %d\n",
              index, (entry >> 16), (entry & 0xffff));
  return 1;
}

uint32 JavaConstantPool::CtpReaderInterfaceMethodref(JavaConstantPool* ctp,
                                                Reader& reader,
                                                uint32 index) {
  uint32 entry = reader.readU4();
  ctp->ctpDef[index] = entry;
  PRINT_DEBUG(JNJVM_LOAD, 3, COLOR_NORMAL, 
        "; [%5d] <Interface xmethodref>\tclass is at %d, name/type is at %d\n",
        index, (entry >> 16), (entry & 0xffff));
  return 1;
}
  
uint32 JavaConstantPool::CtpReaderLong(JavaConstantPool* ctp, Reader& reader,
                                  uint32 index) {
  ctp->ctpDef[index + 1] = reader.readU4();
  ctp->ctpDef[index] = reader.readU4();
  PRINT_DEBUG(JNJVM_LOAD, 3, COLOR_NORMAL, "; [%5d] <long>%d %d\n", index,
              ctpDef[e], ctpDef[e + 1]);
  return 2;
}

uint32 JavaConstantPool::CtpReaderDouble(JavaConstantPool* ctp, Reader& reader,
                                    uint32 index) {
  ctp->ctpDef[index + 1] = reader.readU4();
  ctp->ctpDef[index] = reader.readU4();
  PRINT_DEBUG(JNJVM_LOAD, 3, COLOR_NORMAL, "; [%5d] <double>%d %d\n", index,
              ctp->ctpDef[index], ctp->ctpDef[index + 1]);
  return 2;
}


void*
JavaConstantPool::operator new(size_t sz, mvm::BumpPtrAllocator& allocator,
                               uint32 ctpSize) {
  uint32 size = sz + ctpSize * (sizeof(void*) + sizeof(sint32) + sizeof(uint8));
  return allocator.Allocate(size);
}

JavaConstantPool::JavaConstantPool(Class* cl, Reader& reader, uint32 size) {
  ctpSize = size;
  classDef = cl;
  
  ctpType  = (uint8*)((uint64)this + sizeof(JavaConstantPool));
  ctpDef   = (sint32*)((uint64)ctpType + ctpSize * sizeof(uint8));
  ctpRes   = (void**)((uint64)ctpDef + ctpSize * sizeof(sint32));

  memset(ctpType, 0, 
         ctpSize * (sizeof(uint8) + sizeof(sint32) + sizeof(void*)));

  uint32 cur = 1;
  while (cur < ctpSize) {
    uint8 curType = reader.readU1();
    ctpType[cur] = curType;
    cur += ((funcsReader[curType])(this, reader, cur));
  }
}

const UTF8* JavaConstantPool::UTF8At(uint32 entry) {
  if (! ((entry > 0) && (entry < ctpSize) && 
        typeAt(entry) == ConstantUTF8)) {
    JavaThread::get()->getJVM()->classFormatError(
              "bad constant pool number for utf8 at entry %d", entry);
  }
  
  if (!ctpRes[entry]) {
    Reader reader(classDef->bytes, ctpDef[entry]);
    uint32 len = reader.readU2();
    uint16* buf = (uint16*)alloca(len * sizeof(uint16));
    uint32 n = 0;
    uint32 i = 0;
  
    while (i < len) {
      uint32 cur = reader.readU1();
      if (cur & 0x80) {
        uint32 y = reader.readU1();
        if (cur & 0x20) {
          uint32 z = reader.readU1();
          cur = ((cur & 0x0F) << 12) +
                ((y & 0x3F) << 6) +
                (z & 0x3F);
          i += 3;
        } else {
          cur = ((cur & 0x1F) << 6) +
                (y & 0x3F);
          i += 2;
        }
      } else {
        ++i;
      }
      buf[n] = ((uint16)cur);
      ++n;
    }
  
    JnjvmClassLoader* loader = classDef->classLoader;
    const UTF8* utf8 = loader->hashUTF8->lookupOrCreateReader(buf, n);
    ctpRes[entry] = (UTF8*)utf8;
  
    PRINT_DEBUG(JNJVM_LOAD, 3, COLOR_NORMAL, "; [%5d] <utf8>\t\t\"%s\"\n", entry,
                utf8->printString());

  }
  return (const UTF8*)ctpRes[entry];
}

float JavaConstantPool::FloatAt(uint32 entry) {
  if (! ((entry > 0) && (entry < ctpSize) && 
        typeAt(entry) == ConstantFloat)) {
    JavaThread::get()->getJVM()->classFormatError(
              "bad constant pool number for float at entry %d", entry);
  }
  return ((float*)ctpDef)[entry];
}

sint32 JavaConstantPool::IntegerAt(uint32 entry) {
  if (! ((entry > 0) && (entry < ctpSize) && 
        typeAt(entry) == ConstantInteger)) {
    JavaThread::get()->getJVM()->classFormatError(
              "bad constant pool number for integer at entry %d", entry);
  }
  return ((sint32*)ctpDef)[entry];
}

sint64 JavaConstantPool::LongAt(uint32 entry) {
  if (! ((entry > 0) && (entry < ctpSize) && 
        typeAt(entry) == ConstantLong)) {
    JavaThread::get()->getJVM()->classFormatError(
              "bad constant pool number for long at entry %d", entry);
  }
  return Reader::readLong(ctpDef[entry], ctpDef[entry + 1]);
}

double JavaConstantPool::DoubleAt(uint32 entry) {
  if (! ((entry > 0) && (entry < ctpSize) && 
        typeAt(entry) == ConstantDouble)) {
    JavaThread::get()->getJVM()->classFormatError(
              "bad constant pool number for double at entry %d", entry);
  }
  return Reader::readDouble(ctpDef[entry], ctpDef[entry + 1]);
}

CommonClass* JavaConstantPool::isClassLoaded(uint32 entry) {
  if (! ((entry > 0) && (entry < ctpSize) && 
        typeAt(entry) == ConstantClass)) {
    JavaThread::get()->getJVM()->classFormatError(
              "bad constant pool number for class at entry %d", entry);
  }
  return (CommonClass*)ctpRes[entry];
}

const UTF8* JavaConstantPool::resolveClassName(uint32 index) {
  CommonClass* cl = isClassLoaded(index);
  if (cl) return cl->name;
  else return UTF8At(ctpDef[index]);
}

CommonClass* JavaConstantPool::loadClass(uint32 index) {
  CommonClass* temp = isClassLoaded(index);
#ifndef ISOLATE_SHARING
  if (!temp) {
    JnjvmClassLoader* loader = classDef->classLoader;
    const UTF8* name = UTF8At(ctpDef[index]);
    if (name->elements[0] == I_TAB) {
      temp = loader->constructArray(name);
    } else {
      // Put into ctpRes because there is only one representation of the class
      temp = loader->loadName(name, true, false);
    }
    ctpRes[index] = temp;
  }
#endif
  return temp;
}

CommonClass* JavaConstantPool::getMethodClassIfLoaded(uint32 index) {
  CommonClass* temp = isClassLoaded(index);
#ifndef ISOLATE_SHARING
  if (!temp) {
    JnjvmClassLoader* loader = classDef->classLoader;
    assert(loader && "Class has no loader?");
    const UTF8* name = UTF8At(ctpDef[index]);
    temp = loader->lookupClassOrArray(name);
  }
#endif

  if (!temp && classDef->classLoader->getModule()->isStaticCompiling()) {
    temp = loadClass(index);
  }
  return temp;
}

Typedef* JavaConstantPool::resolveNameAndType(uint32 index) {
  void* res = ctpRes[index];
  if (!res) {
    if (typeAt(index) != ConstantNameAndType) {
      JavaThread::get()->getJVM()->classFormatError(
                "bad constant pool number for name/type at entry %d", index);
    }
    sint32 entry = ctpDef[index];
    const UTF8* type = UTF8At(entry & 0xFFFF);
    Typedef* sign = classDef->classLoader->constructType(type);
    ctpRes[index] = sign;
    return sign;
  }
  return (Typedef*)res;
}

Signdef* JavaConstantPool::resolveNameAndSign(uint32 index) {
  void* res = ctpRes[index];
  if (!res) {
    if (typeAt(index) != ConstantNameAndType) {
      JavaThread::get()->getJVM()->classFormatError(
                "bad constant pool number for name/type at entry %d", index);
    }
    sint32 entry = ctpDef[index];
    const UTF8* type = UTF8At(entry & 0xFFFF);
    Signdef* sign = classDef->classLoader->constructSign(type);
    ctpRes[index] = sign;
    return sign;
  }
  return (Signdef*)res;
}

Typedef* JavaConstantPool::infoOfField(uint32 index) {
  if (typeAt(index) != ConstantFieldref)
    JavaThread::get()->getJVM()->classFormatError(
              "bad constant pool number for field at entry %d", index);
  return resolveNameAndType(ctpDef[index] & 0xFFFF);
}

void JavaConstantPool::infoOfMethod(uint32 index, uint32 access, 
                                    CommonClass*& cl, JavaMethod*& meth) {
  uint8 id = typeAt(index);
  if (id != ConstantMethodref && id != ConstantInterfaceMethodref)
    JavaThread::get()->getJVM()->classFormatError(
              "bad constant pool number for method at entry %d", index);
  
  Signdef* sign = resolveNameAndSign(ctpDef[index] & 0xFFFF);
  sint32 entry = ctpDef[index];
  sint32 ntIndex = entry & 0xFFFF;
  const UTF8* utf8 = UTF8At(ctpDef[ntIndex] >> 16);
  cl = getMethodClassIfLoaded(entry >> 16);
  if (cl) {
    Class* lookup = cl->isArray() ? cl->super : cl->asClass();
    if (lookup->isResolved()) {
    
      // lookup the method
      meth = lookup->lookupMethodDontThrow(utf8, sign->keyName, isStatic(access),
                                         true, 0);
      // OK, this is rare, but the Java bytecode may do an invokevirtual on an
      // interface method. Lookup the method as if it was static.
      // The caller is responsible for taking any action if the method is
      // an interface method.
      if (!meth) {
        meth = lookup->lookupInterfaceMethodDontThrow(utf8, sign->keyName);
      }
    }
  }
}

uint32 JavaConstantPool::getClassIndexFromMethod(uint32 index) {
  sint32 entry = ctpDef[index];
  return (uint32)(entry >> 16);
}


void JavaConstantPool::nameOfStaticOrSpecialMethod(uint32 index, 
                                              const UTF8*& cl,
                                              const UTF8*& name,
                                              Signdef*& sign) {
  uint8 id = typeAt(index);
  if (id != ConstantMethodref && id != ConstantInterfaceMethodref)
    JavaThread::get()->getJVM()->classFormatError(
              "bad constant pool number for method at entry %d", index);
  
  sign = resolveNameAndSign(ctpDef[index] & 0xFFFF);
  sint32 entry = ctpDef[index];
  sint32 ntIndex = entry & 0xFFFF;
  name = UTF8At(ctpDef[ntIndex] >> 16);
  cl = resolveClassName(entry >> 16);
}

void* JavaConstantPool::infoOfStaticOrSpecialMethod(uint32 index, 
                                                    uint32 access,
                                                    Signdef*& sign,
                                                    JavaMethod*& meth) {
  uint8 id = typeAt(index);
  if (id != ConstantMethodref && id != ConstantInterfaceMethodref)
    JavaThread::get()->getJVM()->classFormatError(
              "bad constant pool number for method at entry %d", index);
  
  sign = resolveNameAndSign(ctpDef[index] & 0xFFFF);
  sint32 entry = ctpDef[index];
  sint32 ntIndex = entry & 0xFFFF;
  const UTF8* utf8 = UTF8At(ctpDef[ntIndex] >> 16);
  CommonClass* cl = getMethodClassIfLoaded(entry >> 16);
  if (cl) {
    Class* lookup = cl->isArray() ? cl->super : cl->asClass();
    if (lookup->isResolved()) {
      // lookup the method
      meth = lookup->lookupMethodDontThrow(utf8, sign->keyName,
                                           isStatic(access), true, 0);
      if (meth) { 
        // don't throw if no meth, the exception will be thrown just in time
        JnjvmModule* M = classDef->classLoader->getModule();
        void* F = M->getMethod(meth);
        return F;
      }
    }
  }
 
  // If it's static we're not using ctpRes for anything. We can store
  // the callback. If it's special, the ctpRes contains the offset in
  // the virtual table, so we can't put the callback and must rely on
  // the module provider to hash callbacks.
  if (isStatic(access) && ctpRes[index]) return ctpRes[index];

  void* val =
    classDef->classLoader->getModuleProvider()->addCallback(classDef, index,
                                                            sign,
                                                            isStatic(access));
        
  if (isStatic(access)) ctpRes[index] = val;
  
  return val;
}


Signdef* JavaConstantPool::infoOfInterfaceOrVirtualMethod(uint32 index,
                                                          const UTF8*& name) {

  uint8 id = typeAt(index);
  if (id != ConstantMethodref && id != ConstantInterfaceMethodref)
    JavaThread::get()->getJVM()->classFormatError(
              "bad constant pool number for method at entry %d", index);
  
  sint32 entry = ctpDef[index];
  sint32 ntIndex = entry & 0xFFFF;
  Signdef* sign = resolveNameAndSign(ntIndex); 
  name = UTF8At(ctpDef[ntIndex] >> 16);
  return sign;
}

void JavaConstantPool::resolveMethod(uint32 index, CommonClass*& cl,
                                     const UTF8*& utf8, Signdef*& sign) {
  sint32 entry = ctpDef[index];
  sint32 ntIndex = entry & 0xFFFF;
  sign = (Signdef*)ctpRes[ntIndex];
  assert(sign && "No cached signature after JITting");
  utf8 = UTF8At(ctpDef[ntIndex] >> 16);
  cl = loadClass(entry >> 16);
  assert(cl && "No class after loadClass");
  assert((cl->isClass() && cl->asClass()->isResolved()) && 
         "Class not resolved after loadClass");
}
  
void JavaConstantPool::resolveField(uint32 index, CommonClass*& cl,
                                    const UTF8*& utf8, Typedef*& sign) {
  sint32 entry = ctpDef[index];
  sint32 ntIndex = entry & 0xFFFF;
  sign = (Typedef*)ctpRes[ntIndex];
  assert(sign && "No cached Typedef after JITting");
  utf8 = UTF8At(ctpDef[ntIndex] >> 16);
  cl = loadClass(entry >> 16);
  assert(cl && "No class after loadClass");
  assert((cl->isClass() && cl->asClass()->isResolved()) && 
         "Class not resolved after loadClass");
}

JavaField* JavaConstantPool::lookupField(uint32 index, bool stat) {
  sint32 entry = ctpDef[index];
  sint32 ntIndex = entry & 0xFFFF;
  Typedef* sign = (Typedef*)ctpRes[ntIndex];
  const UTF8* utf8 = UTF8At(ctpDef[ntIndex] >> 16);
  CommonClass* cl = getMethodClassIfLoaded(entry >> 16);
  if (cl) {
    Class* lookup = cl->isArray() ? cl->super : cl->asClass();
    if (lookup->isResolved()) {
      JavaField* field = lookup->lookupFieldDontThrow(utf8, sign->keyName, stat,
                                                      true, 0);
      // don't throw if no field, the exception will be thrown just in time  
      if (field) {
        if (!stat) {
          ctpRes[index] = (void*)field->ptrOffset;
        } 
#ifndef ISOLATE_SHARING
        else if (lookup->isReady()) {
          void* S = field->classDef->getStaticInstance();
          ctpRes[index] = (void*)((uint64)S + field->ptrOffset);
        }
#endif
      }
      return field;
    }
  } 
  return 0;
}

JavaString* JavaConstantPool::resolveString(const UTF8* utf8, uint16 index) {
  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaString* str = vm->internalUTF8ToStr(utf8);
  return str;
}

JavaConstantPool::ctpReader JavaConstantPool::funcsReader[16] = {
  unimplemented,
  CtpReaderUTF8,
  unimplemented,
  CtpReaderInteger,
  CtpReaderFloat,
  CtpReaderLong,
  CtpReaderDouble,
  CtpReaderClass,
  CtpReaderString,
  CtpReaderFieldref,
  CtpReaderMethodref,
  CtpReaderInterfaceMethodref,
  CtpReaderNameAndType,
  unimplemented,
  unimplemented,
  unimplemented
};
