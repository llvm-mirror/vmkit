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

#include <stdio.h>
#include <stdlib.h>

#include "debug.h"

#include "JavaAccess.h"
#include "JavaArray.h"
#include "JavaClass.h"
#include "JavaConstantPool.h"
#include "JavaJIT.h"
#include "Jnjvm.h"
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


static uint32 unimplemented(JavaConstantPool* ctp, Reader& reader, uint32 index) {
  JavaThread::get()->isolate->classFormatError(
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
  uint16 len = reader.readU2();
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
  
  Class* cl = ctp->classDef;
  const UTF8* utf8 = cl->classLoader->hashUTF8->lookupOrCreateReader(buf, n);
  ctp->ctpRes[index] = (UTF8*)utf8;
  
  PRINT_DEBUG(JNJVM_LOAD, 3, COLOR_NORMAL, "; [%5d] <utf8>\t\t\"%s\"\n", index,
              utf8->printString());

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

JavaConstantPool::JavaConstantPool(Class* cl, Reader& reader) {
  ctpSize = reader.readU2();
  classDef = cl;
  JInfo = 0;
  
  ctpRes   = new void*[ctpSize];
  ctpDef   = new sint32[ctpSize];
  ctpType  = new uint8[ctpSize];
  memset(ctpRes, 0, sizeof(void**) * ctpSize);
  memset(ctpDef, 0, sizeof(sint32) * ctpSize);
  memset(ctpType, 0, sizeof(uint8) * ctpSize);

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
    JavaThread::get()->isolate->classFormatError(
              "bad constant pool number for utf8 at entry %d", entry);
  }
  return (const UTF8*)ctpRes[entry];
}

float JavaConstantPool::FloatAt(uint32 entry) {
  if (! ((entry > 0) && (entry < ctpSize) && 
        typeAt(entry) == ConstantFloat)) {
    JavaThread::get()->isolate->classFormatError(
              "bad constant pool number for float at entry %d", entry);
  }
  return ((float*)ctpDef)[entry];
}

sint32 JavaConstantPool::IntegerAt(uint32 entry) {
  if (! ((entry > 0) && (entry < ctpSize) && 
        typeAt(entry) == ConstantInteger)) {
    JavaThread::get()->isolate->classFormatError(
              "bad constant pool number for integer at entry %d", entry);
  }
  return ((sint32*)ctpDef)[entry];
}

sint64 JavaConstantPool::LongAt(uint32 entry) {
  if (! ((entry > 0) && (entry < ctpSize) && 
        typeAt(entry) == ConstantLong)) {
    JavaThread::get()->isolate->classFormatError(
              "bad constant pool number for long at entry %d", entry);
  }
  return Reader::readLong(ctpDef[entry], ctpDef[entry + 1]);
}

double JavaConstantPool::DoubleAt(uint32 entry) {
  if (! ((entry > 0) && (entry < ctpSize) && 
        typeAt(entry) == ConstantDouble)) {
    JavaThread::get()->isolate->classFormatError(
              "bad constant pool number for double at entry %d", entry);
  }
  return Reader::readDouble(ctpDef[entry], ctpDef[entry + 1]);
}

CommonClass* JavaConstantPool::isClassLoaded(uint32 entry) {
  if (! ((entry > 0) && (entry < ctpSize) && 
        typeAt(entry) == ConstantClass)) {
    JavaThread::get()->isolate->classFormatError(
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
  if (!temp) {
    JnjvmClassLoader* loader = classDef->classLoader;
    const UTF8* name = UTF8At(ctpDef[index]);
    if (name->elements[0] == AssessorDesc::I_TAB) {
      temp = loader->constructArray(name);
      loader->resolveClass(temp, false);
    } else {
      // Put into ctpRes because there is only one representation of the class
      temp = loader->loadName(name, true, false, false);
    }
    ctpRes[index] = temp;
  }
  return temp;
}

CommonClass* JavaConstantPool::getMethodClassIfLoaded(uint32 index) {
  CommonClass* temp = isClassLoaded(index);
  if (!temp) {
    JnjvmClassLoader* loader = classDef->classLoader;
    const UTF8* name = UTF8At(ctpDef[index]);
    temp = loader->lookupClass(name);
    if (!temp) 
      temp = JnjvmClassLoader::bootstrapLoader->lookupClass(name);
  }
  return temp;
}

Typedef* JavaConstantPool::resolveNameAndType(uint32 index) {
  void* res = ctpRes[index];
  if (!res) {
    if (typeAt(index) != ConstantNameAndType) {
      JavaThread::get()->isolate->classFormatError(
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
      JavaThread::get()->isolate->classFormatError(
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
    JavaThread::get()->isolate->classFormatError(
              "bad constant pool number for field at entry %d", index);
  return resolveNameAndType(ctpDef[index] & 0xFFFF);
}

void JavaConstantPool::infoOfMethod(uint32 index, uint32 access, 
                                    CommonClass*& cl, JavaMethod*& meth) {
  uint8 id = typeAt(index);
  if (id != ConstantMethodref && id != ConstantInterfaceMethodref)
    JavaThread::get()->isolate->classFormatError(
              "bad constant pool number for method at entry %d", index);
  
  Signdef* sign = resolveNameAndSign(ctpDef[index] & 0xFFFF);
  sint32 entry = ctpDef[index];
  sint32 ntIndex = entry & 0xFFFF;
  const UTF8* utf8 = UTF8At(ctpDef[ntIndex] >> 16);
  cl = getMethodClassIfLoaded(entry >> 16);
  if (cl && cl->status >= classRead) {
    // lookup the method
    meth = 
      cl->lookupMethodDontThrow(utf8, sign->keyName, isStatic(access), false);
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
    JavaThread::get()->isolate->classFormatError(
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
    JavaThread::get()->isolate->classFormatError(
              "bad constant pool number for method at entry %d", index);
  
  sign = resolveNameAndSign(ctpDef[index] & 0xFFFF);
  sint32 entry = ctpDef[index];
  sint32 ntIndex = entry & 0xFFFF;
  const UTF8* utf8 = UTF8At(ctpDef[ntIndex] >> 16);
  CommonClass* cl = getMethodClassIfLoaded(entry >> 16);
  if (cl && cl->status >= classRead) {
    // lookup the method
    meth =
      cl->lookupMethodDontThrow(utf8, sign->keyName, isStatic(access), false);
    if (meth) { 
      // don't throw if no meth, the exception will be thrown just in time
      JnjvmModule* M = classDef->classLoader->TheModule;
      void* F = M->getMethod(meth);
      return F;
    }
  }
  
  // Return the callback.
  void* val =
    classDef->classLoader->TheModuleProvider->addCallback(classDef, index, sign,
                                                          isStatic(access));
        
  return val;
}


Signdef* JavaConstantPool::infoOfInterfaceOrVirtualMethod(uint32 index) {

  uint8 id = typeAt(index);
  if (id != ConstantMethodref && id != ConstantInterfaceMethodref)
    JavaThread::get()->isolate->classFormatError(
              "bad constant pool number for method at entry %d", index);
  
  Signdef* sign = resolveNameAndSign(ctpDef[index] & 0xFFFF);
  
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
  cl->resolveClass(true);
}
  
void JavaConstantPool::resolveField(uint32 index, CommonClass*& cl,
                                    const UTF8*& utf8, Typedef*& sign) {
  sint32 entry = ctpDef[index];
  sint32 ntIndex = entry & 0xFFFF;
  sign = (Typedef*)ctpRes[ntIndex];
  assert(sign && "No cached Typedef after JITting");
  utf8 = UTF8At(ctpDef[ntIndex] >> 16);
  cl = loadClass(entry >> 16);
  cl->resolveClass(true);
}

JavaField* JavaConstantPool::lookupField(uint32 index, bool stat) {
  sint32 entry = ctpDef[index];
  sint32 ntIndex = entry & 0xFFFF;
  Typedef* sign = (Typedef*)ctpRes[ntIndex];
  const UTF8* utf8 = UTF8At(ctpDef[ntIndex] >> 16);
  CommonClass* cl = getMethodClassIfLoaded(entry >> 16);
  if (cl && cl->status >= resolved) {
    JavaField* field = cl->lookupFieldDontThrow(utf8, sign->keyName, stat, 
                                                true);
    // don't throw if no field, the exception will be thrown just in time  
    if (field) {
      if (!stat) {
        ctpRes[index] = (void*)field->ptrOffset;
      } 
#ifndef MULTIPLE_VM
      else if (cl->isReady()) {
        JavaObject* S = field->classDef->staticInstance();
        ctpRes[index] = (void*)((uint64)S + field->ptrOffset);
      }
#endif
    }
    return field;
  } 
  return 0;
}

JavaString* JavaConstantPool::resolveString(const UTF8* utf8, uint16 index) {
  JavaString* str = JavaThread::get()->isolate->UTF8ToStr(utf8);
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
