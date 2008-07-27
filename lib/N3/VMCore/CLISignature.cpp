//===--------- CLISignature.cpp - Reads CLI signatures --------------------===//
//
//                              N3
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//


#include "Assembly.h"
#include "MSCorlib.h"
#include "N3.h"
#include "Reader.h"
#include "VMClass.h"
#include "VMThread.h"

#include "SignatureNames.def"

using namespace n3;

// ECMA 335: page 150 23.1.16 Element types used in signatures 


static VMCommonClass* METHOD_ElementTypeEnd(uint32 op, Assembly* ass, uint32& offset) {
  VMThread::get()->vm->error("implement me");
  return 0;
}

static VMCommonClass* METHOD_ElementTypeVoid(uint32 op, Assembly* ass, uint32& offset) {
  return MSCorlib::pVoid;
}

static VMCommonClass* METHOD_ElementTypeBoolean(uint32 op, Assembly* ass, uint32& offset) {
  return MSCorlib::pBoolean;
}

static VMCommonClass* METHOD_ElementTypeChar(uint32 op, Assembly* ass, uint32& offset) {
  return MSCorlib::pChar;
}

static VMCommonClass* METHOD_ElementTypeI1(uint32 op, Assembly* ass, uint32& offset) {
  return MSCorlib::pSInt8;
}

static VMCommonClass* METHOD_ElementTypeU1(uint32 op, Assembly* ass, uint32& offset) {
  return MSCorlib::pUInt8;
}

static VMCommonClass* METHOD_ElementTypeI2(uint32 op, Assembly* ass, uint32& offset) {
  return MSCorlib::pSInt16;
}

static VMCommonClass* METHOD_ElementTypeU2(uint32 op, Assembly* ass, uint32& offset) {
  return MSCorlib::pUInt16;
}

static VMCommonClass* METHOD_ElementTypeI4(uint32 op, Assembly* ass, uint32& offset) {
  return MSCorlib::pSInt32;
}

static VMCommonClass* METHOD_ElementTypeU4(uint32 op, Assembly* ass, uint32& offset) {
  return MSCorlib::pUInt32;
}

static VMCommonClass* METHOD_ElementTypeI8(uint32 op, Assembly* ass, uint32& offset) {
  return MSCorlib::pSInt64;
}

static VMCommonClass* METHOD_ElementTypeU8(uint32 op, Assembly* ass, uint32& offset) {
  return MSCorlib::pUInt64;
}

static VMCommonClass* METHOD_ElementTypeR4(uint32 op, Assembly* ass, uint32& offset) {
  return MSCorlib::pFloat;
}

static VMCommonClass* METHOD_ElementTypeR8(uint32 op, Assembly* ass, uint32& offset) {
  return MSCorlib::pDouble;
}

static VMCommonClass* METHOD_ElementTypeString(uint32 op, Assembly* ass, uint32& offset) {
  return MSCorlib::pString;
}

static VMCommonClass* METHOD_ElementTypePtr(uint32 op, Assembly* ass, uint32& offset) {
  VMCommonClass* contains = ass->exploreType(offset);
  return ass->constructPointer(contains, 1);
}

static VMCommonClass* METHOD_ElementTypeByRef(uint32 op, Assembly* ass, uint32& offset) {
  VMCommonClass* contains = ass->exploreType(offset);
  return ass->constructPointer(contains, 1);
}

static VMCommonClass* METHOD_ElementTypeValueType(uint32 op, Assembly* ass,
                                                     uint32& offset) {
  uint32 value = ass->uncompressSignature(offset);
  uint32 table = value & 3;
  uint32 index = value >> 2;
  uint32 token = 0;


  switch (table) {
    case 0: table = CONSTANT_TypeDef; break;
    case 1: table = CONSTANT_TypeRef; break;
    case 2: table = CONSTANT_TypeSpec; break;
    default:
      VMThread::get()->vm->error("unknown TypeDefOrRefEncoded %d", index);
      break;
  }

  token = (table << 24) + index;
  VMCommonClass* cl = ass->loadType((N3*)(VMThread::get()->vm), token, false,
                                    false, false, true);
  return cl;
}

static VMCommonClass* METHOD_ElementTypeClass(uint32 op, 
                                              Assembly* ass, uint32& offset) {
  uint32 value = ass->uncompressSignature(offset);
  uint32 table = value & 3;
  uint32 index = value >> 2;
  uint32 token = 0;


  switch (table) {
    case 0: table = CONSTANT_TypeDef; break;
    case 1: table = CONSTANT_TypeRef; break;
    case 2: table = CONSTANT_TypeSpec; break;
    default:
      VMThread::get()->vm->error("unknown TypeDefOrRefEncoded %d", index);
      break;
  }

  token = (table << 24) + index;
  VMCommonClass* cl = ass->loadType((N3*)(VMThread::get()->vm), token, false, 
                                    false, false, true);
  return cl;
}

static VMCommonClass* METHOD_ElementTypeVar(uint32 op, Assembly* ass, uint32& offset) {
  uint32 number = ass->uncompressSignature(offset);
  
  return VMThread::get()->currGenericClass->genericParams[number];
}

static VMCommonClass* METHOD_ElementTypeArray(uint32 op, Assembly* ass, uint32& offset) {
  VMCommonClass* cl = ass->exploreType(offset);
  uint32 rank = ass->uncompressSignature(offset);
  uint32 numSizes = ass->uncompressSignature(offset);

  if (numSizes != 0) {
    printf("type = %s\n", cl->printString());
    VMThread::get()->vm->error("implement me");
  }

  for (uint32 i = 0; i < numSizes; ++i) {
    ass->uncompressSignature(offset);
  }

  uint32 numObounds = ass->uncompressSignature(offset);
  if (numObounds != 0) VMThread::get()->vm->error("implement me");

  for (uint32 i = 0; i < numObounds; ++i) {
    ass->uncompressSignature(offset);
  }
  
  VMClassArray* array = ass->constructArray(cl, rank);
  return array;
}

static VMCommonClass* METHOD_ElementTypeGenericInst(uint32 op, Assembly* ass, uint32& offset) {
  // offset points to (CLASS | VALUETYPE) TypeDefOrRefEncoded

  // skip generic type definition
  offset++; // this is (CLASS | VALUETYPE)

  // save starting offset for later use
  uint32 genericTypeOffset = offset;
  
  ass->uncompressSignature(offset); // TypeDefOrRefEncoded

  //VMCommonClass* cl = ass->exploreType(offset);
  
  uint32 argCount = ass->uncompressSignature(offset);
  
  std::vector<VMCommonClass*> args;
  
  // Get generic arguments.
  for (uint32 i = 0; i < argCount; ++i) {
	  args.push_back(ass->exploreType(offset));
  }

  // save offset
  uint32 endOffset = offset;
  // restore starting offset
  offset = genericTypeOffset;
  
  // TypeDefOrRefEncoded
  uint32 value = ass->uncompressSignature(offset);
  uint32 table = value & 3;
  uint32 index = value >> 2;
  uint32 token = 0;

  switch (table) {
    case 0: table = CONSTANT_TypeDef; break;
    case 1: table = CONSTANT_TypeRef; break;
    case 2: table = CONSTANT_TypeSpec; break;
    default:
      VMThread::get()->vm->error("unknown TypeDefOrRefEncoded %d", index);
      break;
  }

  token = (table << 24) + index;
  VMCommonClass* cl = ass->loadType((N3*)(VMThread::get()->vm), token, false, 
                                    false, false, true, args);
  // restore endOffset
  offset = endOffset;
  
  return cl;
}

static VMCommonClass* METHOD_ElementTypeTypedByRef(uint32 op, Assembly* ass, uint32& offset) {
  return MSCorlib::typedReference;
}

static VMCommonClass* METHOD_ElementTypeI(uint32 op, Assembly* ass, uint32& offset) {
  return MSCorlib::pIntPtr;
}

static VMCommonClass* METHOD_ElementTypeU(uint32 op, Assembly* ass, uint32& offset) {
  return MSCorlib::pUIntPtr;
}

static VMCommonClass* METHOD_ElementTypeFnptr(uint32 op, Assembly* ass, uint32& offset) {
  VMThread::get()->vm->error("implement me");
  return 0;
}

static VMCommonClass* METHOD_ElementTypeObject(uint32 op, Assembly* ass, uint32& offset) {
  return MSCorlib::pObject;
}

static VMCommonClass* METHOD_ElementTypeSzarray(uint32 op, Assembly* ass, uint32& offset) {
  VMCommonClass* contains = ass->exploreType(offset);
  VMClassArray* res = ass->constructArray(contains, 1);
  return res;
}

static VMCommonClass* METHOD_ElementTypeMvar(uint32 op, Assembly* ass, uint32& offset) {
  VMThread::get()->vm->error("implement me");
  return 0;
}

static VMCommonClass* METHOD_ElementTypeCmodReqd(uint32 op, Assembly* ass, uint32& offset) {
  VMThread::get()->vm->error("implement me");
  return 0;
}

static VMCommonClass* METHOD_ElementTypeCmodOpt(uint32 op, Assembly* ass, uint32& offset) {
  VMThread::get()->vm->error("implement me");
  return 0;
}

static VMCommonClass* METHOD_ElementTypeInternal(uint32 op, Assembly* ass, uint32& offset) {
  VMThread::get()->vm->error("implement me");
  return 0;
}

static VMCommonClass* METHOD_ElementTypeModifier(uint32 op, Assembly* ass, uint32& offset) {
  VMThread::get()->vm->error("implement me");
  return 0;
}

static VMCommonClass* METHOD_ElementTypeSentinel(uint32 op, Assembly* ass, uint32& offset) {
  VMThread::get()->vm->error("implement me");
  return 0;
}

static VMCommonClass* METHOD_ElementTypePinned(uint32 op, Assembly* ass, uint32& offset) {
  return 0;
}

static VMCommonClass* unimplemented(uint32 op, Assembly* ass, uint32& offset) {
  VMThread::get()->vm->error("unknown signature");
  return 0;
}

signatureVector_t Assembly::signatureVector[0x46] = {
  METHOD_ElementTypeEnd,           // 0x00
  METHOD_ElementTypeVoid,          // 0x01
  METHOD_ElementTypeBoolean,       // 0x02
  METHOD_ElementTypeChar,          // 0x03
  METHOD_ElementTypeI1,            // 0x04
  METHOD_ElementTypeU1,            // 0x05
  METHOD_ElementTypeI2,            // 0x06
  METHOD_ElementTypeU2,            // 0x07
  METHOD_ElementTypeI4,            // 0x08
  METHOD_ElementTypeU4,            // 0x09
  METHOD_ElementTypeI8,            // 0x0A
  METHOD_ElementTypeU8,            // 0x0B
  METHOD_ElementTypeR4,            // 0x0C
  METHOD_ElementTypeR8,            // 0x0D
  METHOD_ElementTypeString,        // 0x0E
  METHOD_ElementTypePtr,           // 0x1F
  METHOD_ElementTypeByRef,         // 0x10
  METHOD_ElementTypeValueType,     // 0x11
  METHOD_ElementTypeClass,         // 0x12
  METHOD_ElementTypeVar,           // 0x13
  METHOD_ElementTypeArray,         // 0x14
  METHOD_ElementTypeGenericInst,   // 0x15
  METHOD_ElementTypeTypedByRef,    // 0x16
  unimplemented,                   // 0x17
  METHOD_ElementTypeI,             // 0x18
  METHOD_ElementTypeU,             // 0x19
  unimplemented,                   // 0x1A
  METHOD_ElementTypeFnptr,         // 0x1B
  METHOD_ElementTypeObject,        // 0x1C
  METHOD_ElementTypeSzarray,       // 0x1D
  METHOD_ElementTypeMvar,          // 0x1E
  METHOD_ElementTypeCmodReqd,      // 0x1F
  METHOD_ElementTypeCmodOpt,       // 0x20
  METHOD_ElementTypeInternal,      // 0x21
  METHOD_ElementTypeModifier,      // 0x22
  unimplemented,                   // 0x23
  unimplemented,                   // 0x24
  unimplemented,                   // 0x25
  unimplemented,                   // 0x26
  unimplemented,                   // 0x27
  unimplemented,                   // 0x28
  unimplemented,                   // 0x29
  unimplemented,                   // 0x2A
  unimplemented,                   // 0x2B
  unimplemented,                   // 0x2C
  unimplemented,                   // 0x2D
  unimplemented,                   // 0x2E
  unimplemented,                   // 0x2F
  unimplemented,                   // 0x30
  unimplemented,                   // 0x31
  unimplemented,                   // 0x32
  unimplemented,                   // 0x33
  unimplemented,                   // 0x34
  unimplemented,                   // 0x35
  unimplemented,                   // 0x36
  unimplemented,                   // 0x37
  unimplemented,                   // 0x38
  unimplemented,                   // 0x39
  unimplemented,                   // 0x3A
  unimplemented,                   // 0x3B
  unimplemented,                   // 0x3C
  unimplemented,                   // 0x3D
  unimplemented,                   // 0x3E
  unimplemented,                   // 0x3F
  unimplemented,                   // 0x40
  METHOD_ElementTypeSentinel,      // 0x41
  unimplemented,                   // 0x42
  unimplemented,                   // 0x43
  unimplemented,                   // 0x44
  METHOD_ElementTypePinned         // 0x45
};

bool Assembly::extractMethodSignature(uint32& offset, VMCommonClass* cl,
                                      std::vector<VMCommonClass*>& types) {
  //uint32 count      = 
  uncompressSignature(offset);
  uint32 call       = uncompressSignature(offset);
  uint32 paramCount = uncompressSignature(offset);

  uint32 hasThis = call & CONSTANT_HasThis ? 1 : 0;
  uint32 realCount = paramCount + hasThis;
  //uint32 generic = call & CONSTANT_Generic ? 1 : 0;

  VMCommonClass* ret = exploreType(offset);
  types.push_back(ret);
  
  if (hasThis) {
    types.push_back(cl);
  }
  for (uint32 i = hasThis; i < realCount; ++i) {
    VMCommonClass* cur = exploreType(offset);
    types.push_back(cur);
  }
  
  return hasThis != 0;
}

// checks whether the MethodDefSig at offset contains generic parameters
bool Assembly::isGenericMethod(uint32& offset) {
  uncompressSignature(offset); // count
  
  uint32 callingConvention = READ_U1(bytes, offset);
  
  return callingConvention & CONSTANT_Generic ? true : false;
}

void Assembly::methodSpecSignature(uint32& offset,
                                   std::vector<VMCommonClass*>& genArgs) {
  uncompressSignature(offset); // count
  uint32 genericSig = uncompressSignature(offset);

  if (genericSig != 0x0a) {
    VMThread::get()->vm->error("unknown methodSpec sig %x", genericSig);
  }

  uint32 genArgCount = uncompressSignature(offset);
  
  for (uint32 i = 0; i < genArgCount; i++) {
    genArgs.push_back(exploreType(offset));
  }
}

void Assembly::localVarSignature(uint32& offset,
                                 std::vector<VMCommonClass*>& locals) {
  //uint32 count      = 
  uncompressSignature(offset);
  uint32 localSig   = uncompressSignature(offset);
  uint32 nbLocals   = uncompressSignature(offset);

  if (localSig != 0x7) {
    VMThread::get()->vm->error("unknown local sig %x", localSig);
  }
  
  for (uint32 i = 0; i < nbLocals; ++i) {
    VMCommonClass* cl = exploreType(offset);
    if (!cl) --i; // PINNED
    else locals.push_back(cl);
  }
}

VMCommonClass* Assembly::extractFieldSignature(uint32& offset) {
  //uint32 count      = 
  uncompressSignature(offset);
  uint32 fieldSig   = uncompressSignature(offset);
  
  if (fieldSig != 0x6) {
    VMThread::get()->vm->error("unknown field sig %x", fieldSig);
  }
  
  // TODO implement support for custom modifiers
  //      see ECMA 335 23.2.4, 23.2.7 

  return exploreType(offset);

}

VMCommonClass* Assembly::extractTypeInSignature(uint32& offset) {
  //uint32 count      = 
  uncompressSignature(offset);
  return exploreType(offset); 
}

VMCommonClass* Assembly::exploreType(uint32& offset) {
  uint32 op = READ_U1(bytes, offset);
  assert(op < 0x46 && "unknown signature type");
  return (signatureVector[op])(op, this, offset);
}

uint32 Assembly::uncompressSignature(uint32& offset) {
  uint32 value = READ_U1(bytes, offset);

  if ((value & 0x80) == 0) {
    return value;
  } else if ((value & 0x40) == 0) {
    uint32 val2 = READ_U1(bytes, offset);
    return (((value & 0x3f) << 8) | val2);
  } else {
    uint32 val2 = READ_U1(bytes, offset);
    uint32 val3 = READ_U1(bytes, offset);
    uint32 val4 = READ_U1(bytes, offset);
    return ((value & 0x1f) << 24) | (val2 << 16) | (val3 << 8) | val4;
  }
}
