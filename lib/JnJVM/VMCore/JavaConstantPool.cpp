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
#include "Reader.h"
 
using namespace jnjvm;

const uint32 JavaCtpInfo::ConstantUTF8 = 1;
const uint32 JavaCtpInfo::ConstantInteger = 3;
const uint32 JavaCtpInfo::ConstantFloat = 4;
const uint32 JavaCtpInfo::ConstantLong = 5;
const uint32 JavaCtpInfo::ConstantDouble = 6;
const uint32 JavaCtpInfo::ConstantClass = 7;
const uint32 JavaCtpInfo::ConstantString = 8;
const uint32 JavaCtpInfo::ConstantFieldref = 9;
const uint32 JavaCtpInfo::ConstantMethodref = 10;
const uint32 JavaCtpInfo::ConstantInterfaceMethodref = 11;
const uint32 JavaCtpInfo::ConstantNameAndType = 12;


static uint32 unimplemented(Jnjvm* vm, uint32 type, uint32 e, Reader& reader,
                            sint32* ctpDef, void** ctpRes, uint8* ctpType) {
  JavaThread::get()->isolate->error(Jnjvm::ClassFormatError, 
                                    "unknown constant pool type %d", 
                                    type);
  return 1;
}


uint32 JavaCtpInfo::CtpReaderClass(Jnjvm* vm, uint32 type, uint32 e,
                                   Reader& reader, sint32* ctpDef,
                                   void** ctpRes, uint8* ctpType) {
  uint16 entry = reader.readU2();
  ctpDef[e] = entry;
  PRINT_DEBUG(JNJVM_LOAD, 3, COLOR_NORMAL, "; [%5d] <class>\t\tutf8 is at %d\n", e,
              entry);
  return 1;
}
  
uint32 JavaCtpInfo::CtpReaderInteger(Jnjvm* vm, uint32 type, uint32 e,
                                     Reader& reader, sint32* ctpDef,
                                     void** ctpRes, uint8* ctpType) {
  uint32 val = reader.readU4();
  ctpDef[e] = val;
  PRINT_DEBUG(JNJVM_LOAD, 3, COLOR_NORMAL, "; [%5d] <class>\tinteger: %d\n", e,
              val);
  return 1;
}
  
uint32 JavaCtpInfo::CtpReaderFloat(Jnjvm* vm, uint32 type, uint32 e,
                                   Reader& reader, sint32* ctpDef,
                                   void** ctpRes, uint8* ctpType) {
  uint32 val = reader.readU4();
  ctpDef[e] = val;
  PRINT_DEBUG(JNJVM_LOAD, 3, COLOR_NORMAL, "; [%5d] <class>\tfloat: %d\n", e,
              val);
  return 1;
}
  
uint32 JavaCtpInfo::CtpReaderUTF8(Jnjvm* vm, uint32 type, uint32 e,
                                  Reader& reader, sint32* ctpDef, void** ctpRes,
                                  uint8* ctpType) {
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

  const UTF8* utf8 = UTF8::readerConstruct(vm, buf, n);
  ctpRes[e] = (UTF8*)utf8;
  
  PRINT_DEBUG(JNJVM_LOAD, 3, COLOR_NORMAL, "; [%5d] <utf8>\t\t\"%s\"\n", e,
              utf8->printString());

  return 1;
}
  
uint32 JavaCtpInfo::CtpReaderNameAndType(Jnjvm* vm, uint32 type, uint32 e,
                                         Reader& reader, sint32* ctpDef,
                                         void** ctpRes, uint8* ctpType) {
  uint32 entry = reader.readU4();
  ctpDef[e] = entry;
  PRINT_DEBUG(JNJVM_LOAD, 3, COLOR_NORMAL, 
              "; [%5d] <name/type>\tname is at %d, type is at %d\n", e,
              (entry >> 16), (entry & 0xffff));
  return 1;
}
  
uint32 JavaCtpInfo::CtpReaderFieldref(Jnjvm* vm, uint32 type, uint32 e,
                                      Reader& reader, sint32* ctpDef,
                                      void** ctpRes, uint8* ctpType) {
  uint32 entry = reader.readU4();
  ctpDef[e] = entry;
  PRINT_DEBUG(JNJVM_LOAD, 3, COLOR_NORMAL, 
              "; [%5d] <fieldref>\tclass is at %d, name/type is at %d\n", e,
              (entry >> 16), (entry & 0xffff));
  return 1;
}

uint32 JavaCtpInfo::CtpReaderString(Jnjvm* vm, uint32 type, uint32 e,
                                    Reader& reader, sint32* ctpDef,
                                    void** ctpRes, uint8* ctpType) {
  uint16 entry = reader.readU2();
  ctpDef[e] = entry;
  PRINT_DEBUG(JNJVM_LOAD, 3, COLOR_NORMAL, "; [%5d] <string>\tutf8 is at %d\n",
              e, entry);
  return 1;
}
  
uint32 JavaCtpInfo::CtpReaderMethodref(Jnjvm* vm, uint32 type, uint32 e,
                                       Reader& reader, sint32* ctpDef,
                                       void** ctpRes, uint8* ctpType) {
  uint32 entry = reader.readU4();
  ctpDef[e] = entry;
  PRINT_DEBUG(JNJVM_LOAD, 3, COLOR_NORMAL, 
              "; [%5d] <methodref>\tclass is at %d, name/type is at %d\n", e,
              (entry >> 16), (entry & 0xffff));
  return 1;
}

uint32 JavaCtpInfo::CtpReaderInterfaceMethodref(Jnjvm* vm, uint32 type,
                                                uint32 e, Reader& reader,
                                                sint32* ctpDef, void** ctpRes,
                                                uint8* ctpType) {
  uint32 entry = reader.readU4();
  ctpDef[e] = entry;
  PRINT_DEBUG(JNJVM_LOAD, 3, COLOR_NORMAL, 
        "; [%5d] <Interface xmethodref>\tclass is at %d, name/type is at %d\n",
        e, (entry >> 16), (entry & 0xffff));
  return 1;
}
  
uint32 JavaCtpInfo::CtpReaderLong(Jnjvm* vm, uint32 type, uint32 e,
                                  Reader& reader, sint32* ctpDef, void** ctpRes,
                                  uint8* ctpType) {
  ctpDef[e + 1] = reader.readU4();
  ctpDef[e] = reader.readU4();
  ctpType[e] = ConstantLong;
  PRINT_DEBUG(JNJVM_LOAD, 3, COLOR_NORMAL, "; [%5d] <long>%d %d\n", e,
              ctpDef[e], ctpDef[e + 1]);
  return 2;
}

uint32 JavaCtpInfo::CtpReaderDouble(Jnjvm* vm, uint32 type, uint32 e,
                                    Reader& reader, sint32* ctpDef,
                                    void** ctpRes, uint8* ctpType) {
  ctpDef[e + 1] = reader.readU4();
  ctpDef[e] = reader.readU4();
  ctpType[e] = ConstantDouble;
  PRINT_DEBUG(JNJVM_LOAD, 3, COLOR_NORMAL, "; [%5d] <double>%d %d\n", e,
              ctpDef[e], ctpDef[e + 1]);
  return 2;
}

void JavaCtpInfo::read(Jnjvm *vm, Class* cl, Reader& reader) {
  uint32 nbCtp = reader.readU2();
  JavaCtpInfo* res = new JavaCtpInfo();
  
  res->ctpRes   = (void**)malloc(sizeof(void*)*nbCtp);
  res->ctpDef   = (sint32*)malloc(sizeof(sint32)*nbCtp);
  res->ctpType  = (uint8*)malloc(sizeof(uint8)*nbCtp);
  memset(res->ctpRes, 0, sizeof(void**)*nbCtp);
  memset(res->ctpDef, 0, sizeof(sint32)*nbCtp);
  memset(res->ctpType, 0, sizeof(uint8)*nbCtp);

  res->ctpSize = nbCtp;
  res->classDef = cl;
  cl->ctpInfo = res;

  uint32 cur = 1;
  while (cur < nbCtp) {
    uint8 curType = reader.readU1();
    res->ctpType[cur] = curType;
    cur += ((funcsReader[curType])(vm, curType, cur, reader, res->ctpDef,
                                   res->ctpRes, res->ctpType));
  }
}

const UTF8* JavaCtpInfo::UTF8At(uint32 entry) {
  if (! ((entry > 0) && (entry < ctpSize) && 
        typeAt(entry) == ConstantUTF8)) {
    JavaThread::get()->isolate->error(Jnjvm::ClassFormatError, 
              "bad constant pool number for utf8 at entry %d", entry);
  }
  return (const UTF8*)ctpRes[entry];
}

float JavaCtpInfo::FloatAt(uint32 entry) {
  if (! ((entry > 0) && (entry < ctpSize) && 
        typeAt(entry) == ConstantFloat)) {
    JavaThread::get()->isolate->error(Jnjvm::ClassFormatError, 
              "bad constant pool number for float at entry %d", entry);
  }
  return ((float*)ctpDef)[entry];
}

sint32 JavaCtpInfo::IntegerAt(uint32 entry) {
  if (! ((entry > 0) && (entry < ctpSize) && 
        typeAt(entry) == ConstantInteger)) {
    JavaThread::get()->isolate->error(Jnjvm::ClassFormatError, 
              "bad constant pool number for integer at entry %d", entry);
  }
  return ((sint32*)ctpDef)[entry];
}

sint64 JavaCtpInfo::LongAt(uint32 entry) {
  if (! ((entry > 0) && (entry < ctpSize) && 
        typeAt(entry) == ConstantLong)) {
    JavaThread::get()->isolate->error(Jnjvm::ClassFormatError, 
              "bad constant pool number for long at entry %d", entry);
  }
  return Reader::readLong(ctpDef[entry], ctpDef[entry + 1]);
}

double JavaCtpInfo::DoubleAt(uint32 entry) {
  if (! ((entry > 0) && (entry < ctpSize) && 
        typeAt(entry) == ConstantDouble)) {
    JavaThread::get()->isolate->error(Jnjvm::ClassFormatError, 
              "bad constant pool number for double at entry %d", entry);
  }
  return Reader::readDouble(ctpDef[entry], ctpDef[entry + 1]);
}

CommonClass* JavaCtpInfo::isLoadedClassOrClassName(uint32 entry) {
  if (! ((entry > 0) && (entry < ctpSize) && 
        typeAt(entry) == ConstantClass)) {
    JavaThread::get()->isolate->error(Jnjvm::ClassFormatError, 
              "bad constant pool number for class at entry %d", entry);
  }
  return (CommonClass*)ctpRes[entry];
}

const UTF8* JavaCtpInfo::resolveClassName(uint32 index) {
  CommonClass* cl = isLoadedClassOrClassName(index);
  if (cl) return cl->name;
  else return UTF8At(ctpDef[index]);
}

CommonClass* JavaCtpInfo::loadClass(uint32 index) {
  CommonClass* temp = isLoadedClassOrClassName(index);
  if (!temp) {
    JavaObject* loader = classDef->classLoader;
    const UTF8* name = UTF8At(ctpDef[index]);
    if (name->elements[0] == AssessorDesc::I_TAB) {
      // Don't put into ctpRes because the class can be isolate specific
      temp = JavaThread::get()->isolate->constructArray(name, loader);
    } else {
      // Put into ctpRes because there is only one representation of the class
      ctpRes[index] = temp = classDef->isolate->loadName(name, loader, false,
                                                         false, false);
    }
  }
  return temp;
}

CommonClass* JavaCtpInfo::getMethodClassIfLoaded(uint32 index) {
  CommonClass* temp = isLoadedClassOrClassName(index);
  if (!temp) {
    JavaObject* loader = classDef->classLoader;
    const UTF8* name = UTF8At(ctpDef[index]);
    temp = JavaThread::get()->isolate->lookupClass(name, loader);
    if (!temp) 
      temp = Jnjvm::bootstrapVM->lookupClass(name,
                                             CommonClass::jnjvmClassLoader);
  }
  return temp;
}

void JavaCtpInfo::checkInfoOfClass(uint32 index) {
  if (typeAt(index) != ConstantClass)
    JavaThread::get()->isolate->error(Jnjvm::ClassFormatError,
              "bad constant pool number for class at entry %d", index);
  /*if (!(ctpRes[index]))
    ctpRes[index] = JavaJIT::newLookupLLVM;*/
}

Typedef* JavaCtpInfo::resolveNameAndType(uint32 index) {
  void* res = ctpRes[index];
  if (!res) {
    if (typeAt(index) != ConstantNameAndType) {
      JavaThread::get()->isolate->error(Jnjvm::ClassFormatError,
                "bad constant pool number for name/type at entry %d", index);
    }
    sint32 entry = ctpDef[index];
    const UTF8* type = UTF8At(entry & 0xFFFF);
    Typedef* sign = classDef->isolate->constructType(type);
    ctpRes[index] = sign;
    return sign;
  }
  return (Typedef*)res;
}

Signdef* JavaCtpInfo::resolveNameAndSign(uint32 index) {
  void* res = ctpRes[index];
  if (!res) {
    if (typeAt(index) != ConstantNameAndType) {
      JavaThread::get()->isolate->error(Jnjvm::ClassFormatError,
                "bad constant pool number for name/type at entry %d", index);
    }
    sint32 entry = ctpDef[index];
    const UTF8* type = UTF8At(entry & 0xFFFF);
    Signdef* sign = classDef->isolate->constructSign(type);
    ctpRes[index] = sign;
    return sign;
  }
  return (Signdef*)res;
}

Typedef* JavaCtpInfo::infoOfField(uint32 index) {
  if (typeAt(index) != ConstantFieldref)
    JavaThread::get()->isolate->error(Jnjvm::ClassFormatError,
              "bad constant pool number for field at entry %d", index);
  return resolveNameAndType(ctpDef[index] & 0xFFFF);
}

void JavaCtpInfo::infoOfMethod(uint32 index, uint32 access, 
                               CommonClass*& cl, JavaMethod*& meth) {
  uint8 id = typeAt(index);
  if (id != ConstantMethodref && id != ConstantInterfaceMethodref)
    JavaThread::get()->isolate->error(Jnjvm::ClassFormatError,
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

uint32 JavaCtpInfo::getClassIndexFromMethod(uint32 index) {
  sint32 entry = ctpDef[index];
  return (uint32)(entry >> 16);
}


void JavaCtpInfo::nameOfStaticOrSpecialMethod(uint32 index, 
                                              const UTF8*& cl,
                                              const UTF8*& name,
                                              Signdef*& sign) {
  uint8 id = typeAt(index);
  if (id != ConstantMethodref && id != ConstantInterfaceMethodref)
    JavaThread::get()->isolate->error(Jnjvm::ClassFormatError,
              "bad constant pool number for method at entry %d", index);
  
  sign = resolveNameAndSign(ctpDef[index] & 0xFFFF);
  sint32 entry = ctpDef[index];
  sint32 ntIndex = entry & 0xFFFF;
  name = UTF8At(ctpDef[ntIndex] >> 16);
  cl = resolveClassName(entry >> 16);
}

void* JavaCtpInfo::infoOfStaticOrSpecialMethod(uint32 index, 
                                               uint32 access,
                                               Signdef*& sign,
                                               JavaMethod*& meth) {
  uint8 id = typeAt(index);
  if (id != ConstantMethodref && id != ConstantInterfaceMethodref)
    JavaThread::get()->isolate->error(Jnjvm::ClassFormatError,
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
      void* F = JavaRuntime::getMethod(meth);
      ctpRes[index] = (void*)F;
      return F;
    }
  }
  
  // Must be a callback
  if (ctpRes[index]) {
    return ctpRes[index];
  } else {
    void* val =
      classDef->isolate->TheModuleProvider->addCallback(classDef, index, sign,
                                                        isStatic(access));
        
    ctpRes[index] = val;
    return val;
  }
}


Signdef* JavaCtpInfo::infoOfInterfaceOrVirtualMethod(uint32 index) {

  uint8 id = typeAt(index);
  if (id != ConstantMethodref && id != ConstantInterfaceMethodref)
    JavaThread::get()->isolate->error(Jnjvm::ClassFormatError,
              "bad constant pool number for method at entry %d", index);
  
  Signdef* sign = resolveNameAndSign(ctpDef[index] & 0xFFFF);
  
  return sign;
}

void JavaCtpInfo::resolveInterfaceOrMethod(uint32 index,
                                           CommonClass*& cl, const UTF8*& utf8,
                                           Signdef*& sign) {
  sint32 entry = ctpDef[index];
  sint32 ntIndex = entry & 0xFFFF;
  sign = (Signdef*)ctpRes[ntIndex];
  utf8 = UTF8At(ctpDef[ntIndex] >> 16);
  cl = loadClass(entry >> 16);
  cl->resolveClass(true);
}
  
void JavaCtpInfo::resolveField(uint32 index, CommonClass*& cl,
                               const UTF8*& utf8, Typedef*& sign) {
  sint32 entry = ctpDef[index];
  sint32 ntIndex = entry & 0xFFFF;
  sign = (Typedef*)ctpRes[ntIndex];
  utf8 = UTF8At(ctpDef[ntIndex] >> 16);
  cl = loadClass(entry >> 16);
  cl->resolveClass(true);
}

JavaField* JavaCtpInfo::lookupField(uint32 index, bool stat) {
  if (!(ctpRes[index])) {
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
        ctpRes[index] = field;
        return field;
      }
    } else {
      return 0;
    }
  }
  return (JavaField*)ctpRes[index];
}

JavaString* JavaCtpInfo::resolveString(const UTF8* utf8, uint16 index) {
  JavaString* str = JavaThread::get()->isolate->UTF8ToStr(utf8);
  return str;
}

ctpReader JavaCtpInfo::funcsReader[16] = {
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
