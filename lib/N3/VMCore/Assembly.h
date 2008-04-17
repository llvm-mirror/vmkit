//===---------- Assembly.h - Definition of an assembly --------------------===//
//
//                              N3
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//


#ifndef N3_ASSEMBLY_H
#define N3_ASSEMBLY_H

#include <vector>

#include "types.h"

#include "mvm/Object.h"
#include "mvm/Threads/Locks.h"
#include "mvm/Threads/Cond.h"

#include "llvm/DerivedTypes.h"
#include "llvm/Type.h"

namespace llvm {
  class GenericValue;
}

namespace n3 {

class ArrayUInt8;
class ArrayObject;
class Assembly;
class ClassNameMap;
class ClassTokenMap;
class FieldTokenMap;
class MethodTokenMap;
class N3;
class Param;
class Property;
class Reader;
class UTF8;
class VirtualMachine;
class VMClass;
class VMClassArray;
class VMClassPointer;
class VMCommonClass;
class VMField;
class VMMethod;
class VMObject;

class Section : public mvm::Object {
public:
  static VirtualTable* VT;
  virtual void print(mvm::PrintBuffer* buf) const;
  virtual void TRACER;
  
  char* name;
  uint32 virtualSize;
  uint32 virtualAddress;
  uint32 rawSize;
  uint32 rawAddress;
  uint32 relocAddress;
  uint32 lineNumbers;
  uint32 relocationsNumber;
  uint32 lineNumbersNumber;
  uint32 characteristics;

  void read(Reader* reader, N3* vm);
};

class Stream : public mvm::Object {
public:
  static VirtualTable* VT;
  virtual void print(mvm::PrintBuffer* buf) const;
  virtual void TRACER;

  char* name;
  uint32 realOffset;
  uint32 size;
};

class Table : public mvm::Object {
public:
  static VirtualTable* VT;
  virtual void print(mvm::PrintBuffer* buf) const;
  virtual void TRACER;

  uint32 offset;
  uint32 rowsNumber;
  uint32 rowSize;
  uint32 count;
  uint32 sizeMask;

  void readRow(uint32* result, uint32 row, ArrayUInt8* array);
  uint32 readIndexInRow(uint32 row, uint32 index, ArrayUInt8* array);

};


class Header : public mvm::Object {
public:
  static VirtualTable* VT;
  virtual void print(mvm::PrintBuffer* buf) const;
  virtual void TRACER;

  uint32 signature;
  uint32 major;
  uint32 minor;
  uint32 reserved;
  uint32 versionLength;
  uint32 flags;
  uint32 nbStreams;

  const UTF8* versionName;
  Stream* tildStream;
  Stream* stringStream;
  Stream* usStream;
  Stream* blobStream;
  Stream* guidStream;
  std::vector<Table*, gc_allocator<Table*> > tables;
  
  void read(Reader* reader, N3* vm);
};

typedef void (*maskVector_t)(uint32 index,
                             std::vector<Table*, gc_allocator<Table*> >& tables,
                             uint32 heapSizes);

typedef VMCommonClass* (*signatureVector_t)(uint32 op, Assembly* ass,
                                            uint32& offset);

class Assembly : public mvm::Object {
public:
  static VirtualTable* VT;
  virtual void print(mvm::PrintBuffer* buf) const;
  virtual void TRACER;

  VMClassPointer* constructPointer(VMCommonClass* base, uint32 dims);
  VMClassArray* constructArray(VMCommonClass* base, uint32 dims);
  VMClassArray* constructArray(const UTF8* name, const UTF8* nameSpace,
                               uint32 dims);
  VMClass*      constructClass(const UTF8* name,
                               const UTF8* nameSpace, uint32 token);
  VMField*      constructField(VMClass* cl, const UTF8* name,
                               VMCommonClass* signature, uint32 token);
  VMMethod*     constructMethod(VMClass* cl, const UTF8* name,
                                uint32 token);
  VMCommonClass* lookupClassFromName(const UTF8* name, const UTF8* nameSpace);
  VMCommonClass* lookupClassFromToken(uint32 token);
  VMMethod* lookupMethodFromToken(uint32 token);
  VMField* lookupFieldFromToken(uint32 token);
  
  ClassNameMap* loadedNameClasses;
  ClassTokenMap* loadedTokenClasses;
  MethodTokenMap* loadedTokenMethods;
  FieldTokenMap* loadedTokenFields;

  mvm::Lock* lockVar;
  mvm::Cond* condVar;
  const UTF8* name;
  ArrayUInt8* bytes;
  Section* textSection;
  Section* rsrcSection;
  Section* relocSection;
  Header* CLIHeader;
  N3* vm;
  VMObject* delegatee;
  Assembly** assemblyRefs;
  VMObject* getAssemblyDelegatee();

  uint32 CLIHeaderLocation;
  bool isRead;
  uint32 cb;
  uint32 major;
  uint32 minor;
  uint32 mdRva;
  uint32 mdSize;
  uint32 flags;
  uint32 entryPoint;
  uint32 resRva;
  uint32 resSize;

  static Assembly* allocate(const UTF8* name);
  static const UTF8* readUTF8(VirtualMachine* vm, uint32 len, Reader* reader);
  static const UTF8* readUTF16(VirtualMachine* vm, uint32 len, Reader* reader);
  static const UTF8* readUTF8(VirtualMachine* vm, uint32 len, ArrayUInt8* bytes,
                              uint32& offset);
  static const UTF8* readUTF16(VirtualMachine* vm, uint32 len, 
                               ArrayUInt8* bytes, uint32& offset);
  const UTF8* readString(VirtualMachine* vm, uint32 offset);
  void read();
  void readTables(Reader* reader);

  static maskVector_t maskVector[64];
  static const char* maskVectorName[64];
  static signatureVector_t signatureVector[0x46];
  static const char* signatureNames[0x46];
  

  uint32 uncompressSignature(uint32& offset);
  uint32 getTypeDefTokenFromMethod(uint32 token);
  VMCommonClass* loadType(N3* vm, uint32 token, bool resolveFunc, bool resolve,
                          bool clinit, bool dothrow);
  
  VMCommonClass* loadTypeFromName(const UTF8* name, const UTF8* nameSpace, 
                                  bool resolveFunc, bool resolve,
                                  bool clinit, bool dothrow);
  void readClass(VMCommonClass* cl);
  void getProperties(VMCommonClass* cl);
  Property* readProperty(uint32 index, VMCommonClass* cl);
  VMMethod* readMethodDef(uint32 index, VMCommonClass* cl);
  VMField* readField(uint32 index, VMCommonClass* cl);
  Param* readParam(uint32 index, VMMethod* meth);
  VMClass* readTypeDef(N3* vm, uint32 index);
  VMCommonClass* readTypeSpec(N3* vm, uint32 index);
  Assembly* readAssemblyRef(N3* vm, uint32 index);
  VMCommonClass* readTypeRef(N3* vm, uint32 index);
  void readSignature(uint32 offset, std::vector<VMCommonClass*>& locals);
  
  void getInterfacesFromTokenType(std::vector<uint32>& tokens, uint32 token);
  
  bool extractMethodSignature(uint32& offset, VMCommonClass* cl,
                              std::vector<VMCommonClass*> &params);
  void localVarSignature(uint32& offset,
                         std::vector<VMCommonClass*>& locals);
  VMCommonClass* extractFieldSignature(uint32& offset);
  VMCommonClass* extractTypeInSignature(uint32& offset);
  VMCommonClass* exploreType(uint32& offset); 
  
  VMCommonClass* getClassFromName(N3* vm, const UTF8* name, const UTF8* nameSpace);
  
  VMField* getFieldFromToken(uint32 token, bool stat); 
  uint32 getTypedefTokenFromField(uint32 token);
  VMField* readMemberRefAsField(uint32 token, bool stat);
  
  VMMethod* getMethodFromToken(uint32 token); 
  uint32 getTypedefTokenFromMethod(uint32 token);
  VMMethod* readMemberRefAsMethod(uint32 token);

  const UTF8* readUserString(uint32 token); 
  uint32 getExplicitLayout(uint32 token);
  uint32 getRVAFromField(uint32 token);
  
  static Assembly* getCallingAssembly();
  static Assembly* getExecutingAssembly();

  void readCustomAttributes(uint32 offset, std::vector<llvm::GenericValue>& args, VMMethod* meth);
  ArrayObject* getCustomAttributes(uint32 token, VMCommonClass* cl);


};


#define CONSTANT_Assembly 0x20
#define CONSTANT_AssemblyOS 0x22
#define CONSTANT_AssemblyProcessor 0x21
#define CONSTANT_AssemblyRef 0x23
#define CONSTANT_AssemblyRefOS 0x25
#define CONSTANT_AssemblyRefProcessor 0x24
#define CONSTANT_ClassLayout 0x0F
#define CONSTANT_Constant 0x0B
#define CONSTANT_CustomAttribute 0x0C
#define CONSTANT_DeclSecurity 0x0E
#define CONSTANT_EventMap 0x12
#define CONSTANT_Event 0x14
#define CONSTANT_ExportedType 0x27
#define CONSTANT_Field 0x04
#define CONSTANT_FieldLayout 0x10
#define CONSTANT_FieldMarshal 0x0D
#define CONSTANT_FieldRVA 0x1D
#define CONSTANT_File 0x26
#define CONSTANT_GenericParam 0x2A
#define CONSTANT_GenericParamConstraint 0x2C
#define CONSTANT_ImplMap 0x1C
#define CONSTANT_InterfaceImpl 0x09
#define CONSTANT_ManifestResource 0x28
#define CONSTANT_MemberRef 0x0A
#define CONSTANT_MethodDef 0x06
#define CONSTANT_MethodImpl 0x19
#define CONSTANT_MethodSemantics 0x18
#define CONSTANT_MethodSpec 0x2B
#define CONSTANT_Module 0x00
#define CONSTANT_ModuleRef 0x1A
#define CONSTANT_NestedClass 0x29
#define CONSTANT_Param 0x08
#define CONSTANT_Property 0x17
#define CONSTANT_PropertyMap 0x15
#define CONSTANT_StandaloneSig 0x11
#define CONSTANT_TypeDef 0x02
#define CONSTANT_TypeRef 0x01
#define CONSTANT_TypeSpec 0x1B


#define CONSTANT_HasThis 0x20
#define CONSTANT_ExplicitThis 0x40
#define CONSTANT_Default 0x0
#define CONSTANT_Vararg 0x5
#define CONSTANT_C 0x1
#define CONSTANT_StdCall 0x2
#define CONSTANT_ThisCall 0x3
#define CONSTANT_FastCall 0x4
#define CONSTANT_Generic 0x10
#define CONSTANT_Sentinel 0x41


#define CONSTANT_CorILMethod_TinyFormat 0x2
#define CONSTANT_CorILMethod_FatFormat 0x3
#define CONSTANT_CorILMethod_MoreSects 0x8
#define CONSTANT_CorILMethod_InitLocals 0x10
#define CONSTANT_CorILMethod_Sect_FatFormat 0x40
#define CONSTANT_COR_ILEXCEPTION_CLAUSE_EXCEPTION  0x0000
#define CONSTANT_COR_ILEXCEPTION_CLAUSE_FILTER     0x0001
#define CONSTANT_COR_ILEXCEPTION_CLAUSE_FINALLY    0x0002
#define CONSTANT_COR_ILEXCEPTION_CLAUSE_FAULT      0x0004


#define CLI_HEADER 0x168
#define TEXT_SECTION_HEADER 0x178
#define RSRC_SECTION_HEADER 0x178
#define RELOC_SECTION_HEADER 0x178
#define SECTION_NAME_LENGTH 8
#define TABLE_MAX 64
#define NB_TABLES 41

#define INT16(offset)                       \
  rowSize += 2;                             \
  bitmask = bitmask | (1 << (offset << 1));

#define INT32(offset)                       \
  rowSize += 4;                             \
  bitmask = bitmask | (3 << (offset << 1));


#define STRING(offset) {                    \
  uint32 fieldSize = 0;                     \
  if (heapSizes & 0x01) {                   \
    fieldSize = 4;                          \
  } else {                                  \
    fieldSize = 2;                          \
  }                                         \
  rowSize += fieldSize;                     \
  bitmask = bitmask | ((fieldSize - 1) << (offset << 1)); \
}

#define GUID(offset) {                      \
  uint32 fieldSize = 0;                     \
  if (heapSizes & 0x02) {                   \
    fieldSize = 4;                          \
  } else {                                  \
    fieldSize = 2;                          \
  }                                         \
  rowSize += fieldSize;                     \
  bitmask = bitmask | ((fieldSize - 1) << (offset << 1)); \
}

#define BLOB(offset) {                      \
  uint32 fieldSize = 0;                     \
  if (heapSizes & 0x04) {                   \
    fieldSize = 4;                          \
  } else {                                  \
    fieldSize = 2;                          \
  }                                         \
  rowSize += fieldSize;                     \
  bitmask = bitmask | ((fieldSize - 1) << (offset << 1)); \
}

#define TABLE(table, offset) {                  \
  uint32 fieldSize = 0;                         \
  if (tables[table]->rowsNumber < 0x10000) {    \
    fieldSize = 2;                              \
  } else {                                      \
    fieldSize = 4;                              \
  }                                             \
  rowSize += fieldSize;                         \
  bitmask = bitmask | ((fieldSize - 1) << (offset << 1)); \
}


#define TYPEDEF_OR_REF(offset) { \
  uint32 fieldSize = 0; \
  if (tables[CONSTANT_TypeDef]->rowsNumber < 0x4000 &&      \
      tables[CONSTANT_TypeRef]->rowsNumber < 0x4000 &&      \
      tables[CONSTANT_TypeSpec]->rowsNumber < 0x4000) {     \
    fieldSize = 2;                                        \
  } else {                                                \
    fieldSize = 4;                                        \
  }                                                       \
  rowSize += fieldSize;                                   \
  bitmask = bitmask | ((fieldSize - 1) << (offset << 1)); \
}


#define HAS_CONSTANT(offset) { \
  uint32 fieldSize = 0; \
  if (tables[CONSTANT_Field]->rowsNumber < 0x4000 &&      \
      tables[CONSTANT_Param]->rowsNumber < 0x4000 &&      \
      tables[CONSTANT_Property]->rowsNumber < 0x4000) {   \
    fieldSize = 2;                                        \
  } else {                                                \
    fieldSize = 4;                                        \
  }                                                       \
  rowSize += fieldSize;                                   \
  bitmask = bitmask | ((fieldSize - 1) << (offset << 1)); \
}

#define HAS_CUSTOM_ATTRIBUTE(offset) {                          \
  uint32 fieldSize = 0;                                         \
  if (tables[CONSTANT_MethodDef]->rowsNumber < 0x800 &&         \
      tables[CONSTANT_Field]->rowsNumber < 0x800 &&             \
      tables[CONSTANT_TypeRef]->rowsNumber < 0x800 &&           \
      tables[CONSTANT_TypeDef]->rowsNumber < 0x800 &&           \
      tables[CONSTANT_Param]->rowsNumber < 0x800 &&             \
      tables[CONSTANT_InterfaceImpl]->rowsNumber < 0x800 &&     \
      tables[CONSTANT_MemberRef]->rowsNumber < 0x800 &&         \
      tables[CONSTANT_Module]->rowsNumber < 0x800 &&            \
      tables[CONSTANT_DeclSecurity]->rowsNumber < 0x800 &&      \
      tables[CONSTANT_Property]->rowsNumber < 0x800 &&          \
      tables[CONSTANT_Event]->rowsNumber < 0x800 &&             \
      tables[CONSTANT_StandaloneSig]->rowsNumber < 0x800 &&     \
      tables[CONSTANT_ModuleRef]->rowsNumber < 0x800 &&         \
      tables[CONSTANT_TypeSpec]->rowsNumber < 0x800 &&          \
      tables[CONSTANT_Assembly]->rowsNumber < 0x800 &&          \
      tables[CONSTANT_AssemblyRef]->rowsNumber < 0x800 &&       \
      tables[CONSTANT_File]->rowsNumber < 0x800 &&              \
      tables[CONSTANT_ExportedType]->rowsNumber < 0x800 &&      \
      tables[CONSTANT_ManifestResource]->rowsNumber < 0x800) {  \
    fieldSize = 2;                                              \
  } else {                                                      \
    fieldSize = 4;                                              \
  }                                                             \
  rowSize += fieldSize;                                         \
  bitmask = bitmask | ((fieldSize - 1) << (offset << 1));       \
}

#define HAS_FIELD_MARSHAL(offset) {                       \
  uint32 fieldSize = 0;                                   \
  if (tables[CONSTANT_Field]->rowsNumber < 0x8000 &&      \
      tables[CONSTANT_Param]->rowsNumber < 0x8000) {      \
    fieldSize = 2;                                        \
  } else {                                                \
    fieldSize = 4;                                        \
  }                                                       \
  rowSize += fieldSize;                                   \
  bitmask = bitmask | ((fieldSize - 1) << (offset << 1)); \
}

#define HAS_DECL_SECURITY(offset) {                       \
  uint32 fieldSize = 0;                                   \
  if (tables[CONSTANT_TypeDef]->rowsNumber < 0x4000 &&    \
      tables[CONSTANT_MethodDef]->rowsNumber < 0x4000 &&  \
      tables[CONSTANT_Assembly]->rowsNumber < 0x4000) {   \
    fieldSize = 2;                                        \
  } else {                                                \
    fieldSize = 4;                                        \
  }                                                       \
  rowSize += fieldSize;                                   \
  bitmask = bitmask | ((fieldSize - 1) << (offset << 1)); \
}

#define MEMBER_REF_PARENT(offset) {                       \
  uint32 fieldSize = 0;                                   \
  if (tables[CONSTANT_TypeDef]->rowsNumber < 0x2000 &&    \
      tables[CONSTANT_TypeRef]->rowsNumber < 0x2000 &&    \
      tables[CONSTANT_ModuleRef]->rowsNumber < 0x2000 &&  \
      tables[CONSTANT_MethodDef]->rowsNumber < 0x2000 &&  \
      tables[CONSTANT_TypeSpec]->rowsNumber < 0x2000) {   \
    fieldSize = 2;                                        \
  } else {                                                \
    fieldSize = 4;                                        \
  }                                                       \
  rowSize += fieldSize;                                   \
  bitmask = bitmask | ((fieldSize - 1) << (offset << 1)); \
}

#define HAS_SEMANTICS(offset) {                           \
  uint32 fieldSize = 0;                                   \
  if (tables[CONSTANT_Event]->rowsNumber < 0x8000 &&      \
      tables[CONSTANT_Property]->rowsNumber < 0x8000) {   \
    fieldSize = 2;                                        \
  } else {                                                \
    fieldSize = 4;                                        \
  }                                                       \
  rowSize += fieldSize;                                   \
  bitmask = bitmask | ((fieldSize - 1) << (offset << 1)); \
}

#define METHODDEF_OR_REF(offset) {                        \
  uint32 fieldSize = 0;                                   \
  if (tables[CONSTANT_MethodDef]->rowsNumber < 0x8000 &&  \
      tables[CONSTANT_MemberRef]->rowsNumber < 0x8000) {  \
    fieldSize = 2;                                        \
  } else {                                                \
    fieldSize = 4;                                        \
  }                                                       \
  rowSize += fieldSize;                                   \
  bitmask = bitmask | ((fieldSize - 1) << (offset << 1)); \
}

#define MEMBER_FORWARDED(offset) {                        \
  uint32 fieldSize = 0;                                   \
  if (tables[CONSTANT_Field]->rowsNumber < 0x8000 &&      \
      tables[CONSTANT_MethodDef]->rowsNumber < 0x8000) {  \
    fieldSize = 2;                                        \
  } else {                                                \
    fieldSize = 4;                                        \
  }                                                       \
  rowSize += fieldSize;                                   \
  bitmask = bitmask | ((fieldSize - 1) << (offset << 1)); \
}

#define IMPLEMENTATION(offset) {                              \
  uint32 fieldSize = 0;                                       \
  if (tables[CONSTANT_File]->rowsNumber < 0x4000 &&           \
      tables[CONSTANT_AssemblyRef]->rowsNumber < 0x4000 &&    \
      tables[CONSTANT_ExportedType]->rowsNumber < 0x4000) {   \
    fieldSize = 2;                                            \
  } else {                                                    \
    fieldSize = 4;                                            \
  }                                                           \
  rowSize += fieldSize;                                       \
  bitmask = bitmask | ((fieldSize - 1) << (offset << 1));     \
}

// Some encondigs are not used here
#define CUSTOM_ATTRIBUTE_TYPE(offset) {                   \
  uint32 fieldSize = 0;                                   \
  if (tables[CONSTANT_MethodDef]->rowsNumber < 0x2000 &&  \
      tables[CONSTANT_MemberRef]->rowsNumber < 0x2000) {  \
    fieldSize = 2;                                        \
  } else {                                                \
    fieldSize = 4;                                        \
  }                                                       \
  rowSize += fieldSize;                                   \
  bitmask = bitmask | ((fieldSize - 1) << (offset << 1)); \
}

#define RESOLUTION_SCOPE(offset) {                            \
  uint32 fieldSize = 0;                                       \
  if (tables[CONSTANT_Module]->rowsNumber < 0x4000 &&         \
      tables[CONSTANT_ModuleRef]->rowsNumber < 0x4000 &&      \
      tables[CONSTANT_AssemblyRef]->rowsNumber < 0x4000 &&    \
      tables[CONSTANT_TypeRef]->rowsNumber < 0x4000) {        \
    fieldSize = 2;                                            \
  } else {                                                    \
    fieldSize = 4;                                            \
  }                                                           \
  rowSize += fieldSize;                                       \
  bitmask = bitmask | ((fieldSize - 1) << (offset << 1));     \
}

#define TYPE_OR_METHODDEF(offset) {                       \
  uint32 fieldSize = 0;                                   \
  if (tables[CONSTANT_TypeDef]->rowsNumber < 0x8000 &&    \
      tables[CONSTANT_MethodDef]->rowsNumber < 0x8000) {  \
    fieldSize = 2;                                        \
  } else {                                                \
    fieldSize = 4;                                        \
  }                                                       \
  rowSize += fieldSize;                                   \
  bitmask = bitmask | ((fieldSize - 1) << (offset << 1)); \
}


#define DEF_TABLE_MASK(name, nb, ...) \
  static void name(uint32 index, \
                   std::vector<Table*, gc_allocator<Table*> >& tables, \
                   uint32 heapSizes) { \
    Table* table = tables[index]; \
    uint32 rowSize = 0; \
    uint32 bitmask = 0; \
    __VA_ARGS__; \
    table->count = nb; \
    table->sizeMask = bitmask; \
    table->rowSize = rowSize; \
  }
  
#define CONSTANT_MODULE_GENERATION 0
#define CONSTANT_MODULE_NAME 1
#define CONSTANT_MODULE_MVID 2
#define CONSTANT_MODULE_ENCID 3
#define CONSTANT_MODULE_ENCBASEID 4


DEF_TABLE_MASK(METHOD_Module, 5,
  INT16(CONSTANT_MODULE_GENERATION)
  STRING(CONSTANT_MODULE_NAME)
  GUID(CONSTANT_MODULE_MVID)
  GUID(CONSTANT_MODULE_ENCID)
  GUID(CONSTANT_MODULE_ENCBASEID))
  
#define CONSTANT_TYPEREF_RESOLUTION_SCOPE 0
#define CONSTANT_TYPEREF_NAME 1
#define CONSTANT_TYPEREF_NAMESPACE 2

DEF_TABLE_MASK(METHOD_TypeRef, 3,
  RESOLUTION_SCOPE(CONSTANT_TYPEREF_RESOLUTION_SCOPE)
  STRING(CONSTANT_TYPEREF_NAME)
  STRING(CONSTANT_TYPEREF_NAMESPACE))
  
#define CONSTANT_TYPEDEF_FLAGS 0
#define CONSTANT_TYPEDEF_NAME 1
#define CONSTANT_TYPEDEF_NAMESPACE 2
#define CONSTANT_TYPEDEF_EXTENDS 3
#define CONSTANT_TYPEDEF_FIELDLIST 4
#define CONSTANT_TYPEDEF_METHODLIST 5

DEF_TABLE_MASK(METHOD_TypeDef, 6,
  INT32(CONSTANT_TYPEDEF_FLAGS)
  STRING(CONSTANT_TYPEDEF_NAME)
  STRING(CONSTANT_TYPEDEF_NAMESPACE)
  TYPEDEF_OR_REF(CONSTANT_TYPEDEF_EXTENDS)
  TABLE(CONSTANT_Field, CONSTANT_TYPEDEF_FIELDLIST)
  TABLE(CONSTANT_MethodDef, CONSTANT_TYPEDEF_METHODLIST))

#define CONSTANT_METHODDEF_RVA 0
#define CONSTANT_METHODDEF_IMPLFLAGS 1
#define CONSTANT_METHODDEF_FLAGS 2
#define CONSTANT_METHODDEF_NAME 3
#define CONSTANT_METHODDEF_SIGNATURE 4
#define CONSTANT_METHODDEF_PARAMLIST 5

DEF_TABLE_MASK(METHOD_MethodDef, 6,
  INT32(CONSTANT_METHODDEF_RVA)
  INT16(CONSTANT_METHODDEF_IMPLFLAGS)
  INT16(CONSTANT_METHODDEF_FLAGS)
  STRING(CONSTANT_METHODDEF_NAME)
  BLOB(CONSTANT_METHODDEF_SIGNATURE)
  TABLE(CONSTANT_Param, CONSTANT_METHODDEF_PARAMLIST))
  
#define CONSTANT_PARAM_FLAGS 0
#define CONSTANT_PARAM_SEQUENCE 1
#define CONSTANT_PARAM_NAME 2

DEF_TABLE_MASK(METHOD_Param, 3,
  INT16(CONSTANT_PARAM_FLAGS)
  INT16(CONSTANT_PARAM_SEQUENCE)
  STRING(CONSTANT_PARAM_NAME))
  
#define CONSTANT_MEMBERREF_CLASS 0
#define CONSTANT_MEMBERREF_NAME 1
#define CONSTANT_MEMBERREF_SIGNATURE 2

DEF_TABLE_MASK(METHOD_MemberRef, 3,
  MEMBER_REF_PARENT(CONSTANT_MEMBERREF_CLASS)
  STRING(CONSTANT_MEMBERREF_NAME)
  BLOB(CONSTANT_MEMBERREF_SIGNATURE))
  
#define CONSTANT_CUSTOM_ATTRIBUTE_PARENT 0
#define CONSTANT_CUSTOM_ATTRIBUTE_TYPE 1
#define CONSTANT_CUSTOM_ATTRIBUTE_VALUE 2
  
DEF_TABLE_MASK(METHOD_CustomAttribute, 3,
  HAS_CUSTOM_ATTRIBUTE(CONSTANT_CUSTOM_ATTRIBUTE_PARENT)
  CUSTOM_ATTRIBUTE_TYPE(CONSTANT_CUSTOM_ATTRIBUTE_TYPE)
  BLOB(CONSTANT_CUSTOM_ATTRIBUTE_VALUE))
  
#define CONSTANT_STANDALONE_SIG_SIGNATURE 0

DEF_TABLE_MASK(METHOD_StandaloneSig, 1,
  BLOB(CONSTANT_STANDALONE_SIG_SIGNATURE))
  
#define CONSTANT_TYPESPEC_SIGNATURE 0

DEF_TABLE_MASK(METHOD_TypeSpec, 1,
  BLOB(CONSTANT_TYPESPEC_SIGNATURE))
  
#define CONSTANT_ASSEMBLY_HASH_ALG_ID 0
#define CONSTANT_ASSEMBLY_MAJOR 1
#define CONSTANT_ASSEMBLY_MINOR 2
#define CONSTANT_ASSEMBLY_BUILD 3
#define CONSTANT_ASSEMBLY_REVISION 4
#define CONSTANT_ASSEMBLY_FLAGS 5
#define CONSTANT_ASSEMBLY_PUBLIC_KEY 6
#define CONSTANT_ASSEMBLY_NAME 7
#define CONSTANT_ASSEMBLY_CULTURE 8

DEF_TABLE_MASK(METHOD_Assembly, 9,
  INT32(CONSTANT_ASSEMBLY_HASH_ALG_ID)
  INT16(CONSTANT_ASSEMBLY_MAJOR)
  INT16(CONSTANT_ASSEMBLY_MINOR)
  INT16(CONSTANT_ASSEMBLY_BUILD)
  INT16(CONSTANT_ASSEMBLY_REVISION)
  INT32(CONSTANT_ASSEMBLY_FLAGS)
  BLOB(CONSTANT_ASSEMBLY_PUBLIC_KEY)
  STRING(CONSTANT_ASSEMBLY_NAME)
  STRING(CONSTANT_ASSEMBLY_CULTURE))
  
#define CONSTANT_ASSEMBLY_REF_MAJOR 0
#define CONSTANT_ASSEMBLY_REF_MINOR 1
#define CONSTANT_ASSEMBLY_REF_BUILD 2
#define CONSTANT_ASSEMBLY_REF_REVISION 3
#define CONSTANT_ASSEMBLY_REF_FLAGS 4
#define CONSTANT_ASSEMBLY_REF_PUBLIC_KEY 5
#define CONSTANT_ASSEMBLY_REF_NAME 6
#define CONSTANT_ASSEMBLY_REF_CULTURE 7
#define CONSTANT_ASSEMBLY_REF_HASH_VALUE 8

DEF_TABLE_MASK(METHOD_AssemblyRef, 9,
  INT16(CONSTANT_ASSEMBLY_REF_MAJOR)
  INT16(CONSTANT_ASSEMBLY_REF_MINOR)
  INT16(CONSTANT_ASSEMBLY_REF_BUILD)
  INT16(CONSTANT_ASSEMBLY_REF_REVISION)
  INT32(CONSTANT_ASSEMBLY_REF_FLAGS)
  BLOB(CONSTANT_ASSEMBLY_REF_PUBLIC_KEY)
  STRING(CONSTANT_ASSEMBLY_REF_NAME)
  STRING(CONSTANT_ASSEMBLY_REF_CULTURE)
  BLOB(CONSTANT_ASSEMBLY_REF_HASH_VALUE))
  
#define CONSTANT_FIELD_FLAGS 0
#define CONSTANT_FIELD_NAME 1
#define CONSTANT_FIELD_SIGNATURE 2

DEF_TABLE_MASK(METHOD_Field, 3,
  INT16(CONSTANT_FIELD_FLAGS)
  STRING(CONSTANT_FIELD_NAME)
  BLOB(CONSTANT_FIELD_SIGNATURE))
  
#define CONSTANT_INTERFACE_IMPL_CLASS 0
#define CONSTANT_INTERFACE_IMPL_INTERFACE 1

DEF_TABLE_MASK(METHOD_InterfaceImpl, 2,
  TABLE(CONSTANT_TypeDef, CONSTANT_INTERFACE_IMPL_CLASS)
  TYPEDEF_OR_REF(CONSTANT_INTERFACE_IMPL_INTERFACE))
  
#define CONSTANT_NESTED_CLASS_NESTED_CLASS 0
#define CONSTANT_NESTED_CLASS_ENCLOSING_CLASS 1

DEF_TABLE_MASK(METHOD_NestedClass, 2,
  TABLE(CONSTANT_TypeDef, CONSTANT_NESTED_CLASS_NESTED_CLASS)
  TABLE(CONSTANT_TypeDef, CONSTANT_NESTED_CLASS_ENCLOSING_CLASS))

#define CONSTANT_METHOD_SPEC_METHOD 0
#define CONSTANT_METHOD_SPEC_INSTANTIATION 1

DEF_TABLE_MASK(METHOD_MethodSpec, 2,
  METHODDEF_OR_REF(CONSTANT_METHOD_SPEC_METHOD)
  BLOB(CONSTANT_METHOD_SPEC_INSTANTIATION))

#define CONSTANT_GENERIC_PARAM_CONSTRAINT_OWNER 0
#define CONSTANT_GENERIC_PARAM_CONSTRAINT_CONSTRAINT 1

DEF_TABLE_MASK(METHOD_GenericParamConstraint, 2,
  TABLE(CONSTANT_GenericParam, CONSTANT_GENERIC_PARAM_CONSTRAINT_OWNER)
  TYPEDEF_OR_REF(CONSTANT_GENERIC_PARAM_CONSTRAINT_CONSTRAINT))
  
#define CONSTANT_CONSTANT_TYPE 0
#define CONSTANT_CONSTANT_PARENT 1
#define CONSTANT_CONSTANT_VALUE 2

DEF_TABLE_MASK(METHOD_Constant, 3,
  INT16(CONSTANT_CONSTANT_TYPE)
  HAS_CONSTANT(CONSTANT_CONSTANT_PARENT)
  BLOB(CONSTANT_CONSTANT_VALUE))
  
#define CONSTANT_FIELD_MARSHAL_PARENT 0
#define CONSTANT_FIELD_MARSHAL_NATIVE_TYPE 1

DEF_TABLE_MASK(METHOD_FieldMarshal, 2,
  HAS_FIELD_MARSHAL(CONSTANT_FIELD_MARSHAL_PARENT)
  BLOB(CONSTANT_FIELD_MARSHAL_NATIVE_TYPE))
  
#define CONSTANT_DECL_SECURITY_ACTION 0
#define CONSTANT_DECL_SECURITY_PARENT 1
#define CONSTANT_DECL_SECURITY_PERMISSION_SET 2

DEF_TABLE_MASK(METHOD_DeclSecurity, 3,
  INT16(CONSTANT_DECL_SECURITY_ACTION)
  HAS_DECL_SECURITY(CONSTANT_DECL_SECURITY_PARENT)
  BLOB(CONSTANT_DECL_SECURITY_PERMISSION_SET))

#define CONSTANT_CLASS_LAYOUT_PACKING_SIZE 0
#define CONSTANT_CLASS_LAYOUT_CLASS_SIZE 1
#define CONSTANT_CLASS_LAYOUT_PARENT 2

DEF_TABLE_MASK(METHOD_ClassLayout, 3,
  INT16(CONSTANT_CLASS_LAYOUT_PACKING_SIZE)
  INT32(CONSTANT_CLASS_LAYOUT_CLASS_SIZE)
  TABLE(CONSTANT_TypeDef, CONSTANT_CLASS_LAYOUT_PARENT))
  
#define CONSTANT_FIELD_LAYOUT_OFFSET 0
#define CONSTANT_FIELD_LAYOUT_FIELD 1

DEF_TABLE_MASK(METHOD_FieldLayout, 2,
  INT32(CONSTANT_FIELD_LAYOUT_OFFSET)
  TABLE(CONSTANT_Field, CONSTANT_FIELD_LAYOUT_FIELD))

#define CONSTANT_EVENT_MAP_PARENT 0
#define CONSTANT_EVEN_MAP_EVENT_LIST 1

DEF_TABLE_MASK(METHOD_EventMap, 2,
  TABLE(CONSTANT_TypeDef, CONSTANT_EVENT_MAP_PARENT)
  TABLE(CONSTANT_Event, CONSTANT_EVEN_MAP_EVENT_LIST))

#define CONSTANT_EVENT_EVENT_FLAGS 0
#define CONSTANT_EVENT_NAME 1
#define CONSTANT_EVENT_TYPE 2

DEF_TABLE_MASK(METHOD_Event, 3,
  INT16(CONSTANT_EVENT_EVENT_FLAGS)
  STRING(CONSTANT_EVENT_NAME)
  TYPEDEF_OR_REF(CONSTANT_EVENT_TYPE))
  
#define CONSTANT_PROPERTY_MAP_PARENT 0
#define CONSTANT_PROPERTY_MAP_PROPERTY_LIST 1

DEF_TABLE_MASK(METHOD_PropertyMap, 2,
  TABLE(CONSTANT_TypeDef, CONSTANT_PROPERTY_MAP_PARENT)
  TABLE(CONSTANT_Property, CONSTANT_PROPERTY_MAP_PROPERTY_LIST))
  
#define CONSTANT_PROPERTY_FLAGS 0
#define CONSTANT_PROPERTY_NAME 1
#define CONSTANT_PROPERTY_TYPE 2

DEF_TABLE_MASK(METHOD_Property, 3,
  INT16(CONSTANT_PROPERTY_FLAGS)
  STRING(CONSTANT_PROPERTY_NAME)
  BLOB(CONSTANT_PROPERTY_TYPE))
  
#define CONSTANT_METHOD_SEMANTICS_SEMANTICS 0
#define CONSTANT_METHOD_SEMANTICS_METHOD 1
#define CONSTANT_METHOD_SEMANTICS_ASSOCIATION 2

DEF_TABLE_MASK(METHOD_MethodSemantics, 3,
  INT16(CONSTANT_METHOD_SEMANTICS_SEMANTICS)
  TABLE(CONSTANT_MethodDef, CONSTANT_METHOD_SEMANTICS_METHOD)
  HAS_SEMANTICS(CONSTANT_METHOD_SEMANTICS_ASSOCIATION))

#define CONSTANT_METHOD_IMPL_CLASS 0
#define CONSTANT_METHOD_IMPL_METHOD_BODY 1
#define CONSTANT_METHOD_IMPL_METHOD_DECLARATION 2

DEF_TABLE_MASK(METHOD_MethodImpl, 3,
  TABLE(CONSTANT_TypeDef, CONSTANT_METHOD_IMPL_CLASS)
  METHODDEF_OR_REF(CONSTANT_METHOD_IMPL_METHOD_BODY)
  METHODDEF_OR_REF(CONSTANT_METHOD_IMPL_METHOD_DECLARATION))
  
#define CONSTANT_MODULE_REF_NAME 0

DEF_TABLE_MASK(METHOD_ModuleRef, 1,
  STRING(CONSTANT_MODULE_REF_NAME))
  
#define CONSTANT_IMPL_MAP_MAPPING_FLAGS 0
#define CONSTANT_IMPL_MAP_MEMBER_FORWARDED 1
#define CONSTANT_IMPL_MAP_IMPORT_NAME 2
#define CONSTANT_IMPL_MAP_IMPORT_SCOPE 3

DEF_TABLE_MASK(METHOD_ImplMap, 4,
  INT16(CONSTANT_IMPL_MAP_MAPPING_FLAGS)
  MEMBER_FORWARDED(CONSTANT_IMPL_MAP_MEMBER_FORWARDED)
  STRING(CONSTANT_IMPL_MAP_IMPORT_NAME)
  TABLE(CONSTANT_ModuleRef, CONSTANT_IMPL_MAP_IMPORT_SCOPE))
  
#define CONSTANT_FIELD_RVA_RVA 0
#define CONSTANT_FIELD_RVA_FIELD 1

DEF_TABLE_MASK(METHOD_FieldRva, 2,
  INT32(CONSTANT_FIELD_RVA_RVA)
  TABLE(CONSTANT_Field, CONSTANT_FIELD_RVA_FIELD))

#define CONSTANT_MANIFEST_RESOURCE_OFFSET 0
#define CONSTANT_MANIFEST_RESOURCE_FLAGS 1
#define CONSTANT_MANIFEST_RESOURCE_NAME 2
#define CONSTANT_MANIFEST_RESOURCE_IMPLEMENTATION 3

DEF_TABLE_MASK(METHOD_ManifestResource, 4,
  INT32(CONSTANT_MANIFEST_RESOURCE_OFFSET)
  INT32(CONSTANT_MANIFEST_RESOURCE_FLAGS)
  STRING(CONSTANT_MANIFEST_RESOURCE_NAME)
  IMPLEMENTATION(CONSTANT_MANIFEST_RESOURCE_IMPLEMENTATION))
  
#define CONSTANT_ASSEMBLY_PROCESSOR_PROCESSOR 0

DEF_TABLE_MASK(METHOD_AssemblyProcessor, 1,
  INT32(CONSTANT_ASSEMBLY_PROCESSOR_PROCESSOR))
  
#define CONSTANT_ASSEMBLY_OS_PLATFORM_ID 0
#define CONSTANT_ASSEMBLY_OS_MAJOR_VERSION 1
#define CONSTANT_ASSEMBLY_OS_MINOR_VERSION 2

DEF_TABLE_MASK(METHOD_AssemblyOS, 3,
  INT32(CONSTANT_ASSEMBLY_OS_PLATFORM_ID)
  INT32(CONSTANT_ASSEMBLY_OS_MAJOR_VERSION)
  INT32(CONSTANT_ASSEMBLY_OS_MINOR_VERSION))
  
#define CONSTANT_FILE_FLAGS 0
#define CONSTANT_FILE_NAME 1
#define CONSTANT_FILE_HASH_VALUE 2

DEF_TABLE_MASK(METHOD_File, 3,
  INT32(CONSTANT_FILE_FLAGS)
  STRING(CONSTANT_FILE_NAME)
  BLOB(CONSTANT_FILE_HASH_VALUE))


} // end namespace n3

#endif

