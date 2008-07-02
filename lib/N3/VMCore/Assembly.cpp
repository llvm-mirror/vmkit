//===--------- Assembly.cpp - Definition of an assembly -------------------===//
//
//                              N3
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <vector>

#include "debug.h"
#include "types.h"

#include "Assembly.h"
#include "CLIAccess.h"
#include "LockedMap.h"
#include "MSCorlib.h"
#include "N3.h"
#include "Reader.h"
#include "VirtualMachine.h"
#include "VMClass.h"
#include "VMObject.h"
#include "VMThread.h"

using namespace n3;


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
  

DEF_TABLE_MASK(METHOD_Module, 5,
  INT16(CONSTANT_MODULE_GENERATION)
  STRING(CONSTANT_MODULE_NAME)
  GUID(CONSTANT_MODULE_MVID)
  GUID(CONSTANT_MODULE_ENCID)
  GUID(CONSTANT_MODULE_ENCBASEID))
  
DEF_TABLE_MASK(METHOD_TypeRef, 3,
  RESOLUTION_SCOPE(CONSTANT_TYPEREF_RESOLUTION_SCOPE)
  STRING(CONSTANT_TYPEREF_NAME)
  STRING(CONSTANT_TYPEREF_NAMESPACE))
  
DEF_TABLE_MASK(METHOD_TypeDef, 6,
  INT32(CONSTANT_TYPEDEF_FLAGS)
  STRING(CONSTANT_TYPEDEF_NAME)
  STRING(CONSTANT_TYPEDEF_NAMESPACE)
  TYPEDEF_OR_REF(CONSTANT_TYPEDEF_EXTENDS)
  TABLE(CONSTANT_Field, CONSTANT_TYPEDEF_FIELDLIST)
  TABLE(CONSTANT_MethodDef, CONSTANT_TYPEDEF_METHODLIST))

DEF_TABLE_MASK(METHOD_MethodDef, 6,
  INT32(CONSTANT_METHODDEF_RVA)
  INT16(CONSTANT_METHODDEF_IMPLFLAGS)
  INT16(CONSTANT_METHODDEF_FLAGS)
  STRING(CONSTANT_METHODDEF_NAME)
  BLOB(CONSTANT_METHODDEF_SIGNATURE)
  TABLE(CONSTANT_Param, CONSTANT_METHODDEF_PARAMLIST))
  
DEF_TABLE_MASK(METHOD_Param, 3,
  INT16(CONSTANT_PARAM_FLAGS)
  INT16(CONSTANT_PARAM_SEQUENCE)
  STRING(CONSTANT_PARAM_NAME))
  
DEF_TABLE_MASK(METHOD_MemberRef, 3,
  MEMBER_REF_PARENT(CONSTANT_MEMBERREF_CLASS)
  STRING(CONSTANT_MEMBERREF_NAME)
  BLOB(CONSTANT_MEMBERREF_SIGNATURE))
  
DEF_TABLE_MASK(METHOD_CustomAttribute, 3,
  HAS_CUSTOM_ATTRIBUTE(CONSTANT_CUSTOM_ATTRIBUTE_PARENT)
  CUSTOM_ATTRIBUTE_TYPE(CONSTANT_CUSTOM_ATTRIBUTE_TYPE)
  BLOB(CONSTANT_CUSTOM_ATTRIBUTE_VALUE))
  
DEF_TABLE_MASK(METHOD_StandaloneSig, 1,
  BLOB(CONSTANT_STANDALONE_SIG_SIGNATURE))
  
DEF_TABLE_MASK(METHOD_TypeSpec, 1,
  BLOB(CONSTANT_TYPESPEC_SIGNATURE))
  
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
  
DEF_TABLE_MASK(METHOD_Field, 3,
  INT16(CONSTANT_FIELD_FLAGS)
  STRING(CONSTANT_FIELD_NAME)
  BLOB(CONSTANT_FIELD_SIGNATURE))
  
DEF_TABLE_MASK(METHOD_InterfaceImpl, 2,
  TABLE(CONSTANT_TypeDef, CONSTANT_INTERFACE_IMPL_CLASS)
  TYPEDEF_OR_REF(CONSTANT_INTERFACE_IMPL_INTERFACE))
  
DEF_TABLE_MASK(METHOD_NestedClass, 2,
  TABLE(CONSTANT_TypeDef, CONSTANT_NESTED_CLASS_NESTED_CLASS)
  TABLE(CONSTANT_TypeDef, CONSTANT_NESTED_CLASS_ENCLOSING_CLASS))

DEF_TABLE_MASK(METHOD_MethodSpec, 2,
  METHODDEF_OR_REF(CONSTANT_METHOD_SPEC_METHOD)
  BLOB(CONSTANT_METHOD_SPEC_INSTANTIATION))

DEF_TABLE_MASK(METHOD_GenericParamConstraint, 2,
  TABLE(CONSTANT_GenericParam, CONSTANT_GENERIC_PARAM_CONSTRAINT_OWNER)
  TYPEDEF_OR_REF(CONSTANT_GENERIC_PARAM_CONSTRAINT_CONSTRAINT))
  
DEF_TABLE_MASK(METHOD_Constant, 3,
  INT16(CONSTANT_CONSTANT_TYPE)
  HAS_CONSTANT(CONSTANT_CONSTANT_PARENT)
  BLOB(CONSTANT_CONSTANT_VALUE))
  
DEF_TABLE_MASK(METHOD_FieldMarshal, 2,
  HAS_FIELD_MARSHAL(CONSTANT_FIELD_MARSHAL_PARENT)
  BLOB(CONSTANT_FIELD_MARSHAL_NATIVE_TYPE))
  
DEF_TABLE_MASK(METHOD_DeclSecurity, 3,
  INT16(CONSTANT_DECL_SECURITY_ACTION)
  HAS_DECL_SECURITY(CONSTANT_DECL_SECURITY_PARENT)
  BLOB(CONSTANT_DECL_SECURITY_PERMISSION_SET))

DEF_TABLE_MASK(METHOD_ClassLayout, 3,
  INT16(CONSTANT_CLASS_LAYOUT_PACKING_SIZE)
  INT32(CONSTANT_CLASS_LAYOUT_CLASS_SIZE)
  TABLE(CONSTANT_TypeDef, CONSTANT_CLASS_LAYOUT_PARENT))
  
DEF_TABLE_MASK(METHOD_FieldLayout, 2,
  INT32(CONSTANT_FIELD_LAYOUT_OFFSET)
  TABLE(CONSTANT_Field, CONSTANT_FIELD_LAYOUT_FIELD))

DEF_TABLE_MASK(METHOD_EventMap, 2,
  TABLE(CONSTANT_TypeDef, CONSTANT_EVENT_MAP_PARENT)
  TABLE(CONSTANT_Event, CONSTANT_EVEN_MAP_EVENT_LIST))

DEF_TABLE_MASK(METHOD_Event, 3,
  INT16(CONSTANT_EVENT_EVENT_FLAGS)
  STRING(CONSTANT_EVENT_NAME)
  TYPEDEF_OR_REF(CONSTANT_EVENT_TYPE))
  
DEF_TABLE_MASK(METHOD_PropertyMap, 2,
  TABLE(CONSTANT_TypeDef, CONSTANT_PROPERTY_MAP_PARENT)
  TABLE(CONSTANT_Property, CONSTANT_PROPERTY_MAP_PROPERTY_LIST))
  
DEF_TABLE_MASK(METHOD_Property, 3,
  INT16(CONSTANT_PROPERTY_FLAGS)
  STRING(CONSTANT_PROPERTY_NAME)
  BLOB(CONSTANT_PROPERTY_TYPE))
  
DEF_TABLE_MASK(METHOD_MethodSemantics, 3,
  INT16(CONSTANT_METHOD_SEMANTICS_SEMANTICS)
  TABLE(CONSTANT_MethodDef, CONSTANT_METHOD_SEMANTICS_METHOD)
  HAS_SEMANTICS(CONSTANT_METHOD_SEMANTICS_ASSOCIATION))

DEF_TABLE_MASK(METHOD_MethodImpl, 3,
  TABLE(CONSTANT_TypeDef, CONSTANT_METHOD_IMPL_CLASS)
  METHODDEF_OR_REF(CONSTANT_METHOD_IMPL_METHOD_BODY)
  METHODDEF_OR_REF(CONSTANT_METHOD_IMPL_METHOD_DECLARATION))
  
DEF_TABLE_MASK(METHOD_ModuleRef, 1,
  STRING(CONSTANT_MODULE_REF_NAME))
  
DEF_TABLE_MASK(METHOD_ImplMap, 4,
  INT16(CONSTANT_IMPL_MAP_MAPPING_FLAGS)
  MEMBER_FORWARDED(CONSTANT_IMPL_MAP_MEMBER_FORWARDED)
  STRING(CONSTANT_IMPL_MAP_IMPORT_NAME)
  TABLE(CONSTANT_ModuleRef, CONSTANT_IMPL_MAP_IMPORT_SCOPE))
  
DEF_TABLE_MASK(METHOD_FieldRva, 2,
  INT32(CONSTANT_FIELD_RVA_RVA)
  TABLE(CONSTANT_Field, CONSTANT_FIELD_RVA_FIELD))

DEF_TABLE_MASK(METHOD_ManifestResource, 4,
  INT32(CONSTANT_MANIFEST_RESOURCE_OFFSET)
  INT32(CONSTANT_MANIFEST_RESOURCE_FLAGS)
  STRING(CONSTANT_MANIFEST_RESOURCE_NAME)
  IMPLEMENTATION(CONSTANT_MANIFEST_RESOURCE_IMPLEMENTATION))
  
DEF_TABLE_MASK(METHOD_AssemblyProcessor, 1,
  INT32(CONSTANT_ASSEMBLY_PROCESSOR_PROCESSOR))
  
DEF_TABLE_MASK(METHOD_AssemblyOS, 3,
  INT32(CONSTANT_ASSEMBLY_OS_PLATFORM_ID)
  INT32(CONSTANT_ASSEMBLY_OS_MAJOR_VERSION)
  INT32(CONSTANT_ASSEMBLY_OS_MINOR_VERSION))
  
DEF_TABLE_MASK(METHOD_File, 3,
  INT32(CONSTANT_FILE_FLAGS)
  STRING(CONSTANT_FILE_NAME)
  BLOB(CONSTANT_FILE_HASH_VALUE))

DEF_TABLE_MASK(METHOD_GenericParam, 4,
  INT16(CONSTANT_GENERIC_PARAM_NUMBER)
  INT16(CONSTANT_GENERIC_PARAM_FLAGS)
  TYPE_OR_METHODDEF(CONSTANT_GENERIC_PARAM_OWNER)
  STRING(CONSTANT_GENERIC_PARAM_NAME))

void Header::print(mvm::PrintBuffer* buf) const {
  buf->write("Header<>");
}

void Section::print(mvm::PrintBuffer* buf) const {
  buf->write("Section<>");
}

void Table::print(mvm::PrintBuffer* buf) const {
  buf->write("Table<>");
}

void Stream::print(mvm::PrintBuffer* buf) const {
  buf->write("Table<>");
}

void Assembly::print(mvm::PrintBuffer* buf) const {
  buf->write("Assembly<");
  name->print(buf);
  buf->write(">");
}

static VMCommonClass* arrayDup(ClassNameCmp &cmp, Assembly* ass) {
  VMClassArray* cl = gc_new(VMClassArray)();
  cl->initialise(ass->vm, true);
  cl->name = cmp.name;
  cl->nameSpace = cmp.nameSpace;
  cl->super = VMClassArray::SuperArray;
  cl->interfaces = VMClassArray::InterfacesArray;
  cl->virtualMethods = VMClassArray::VirtualMethodsArray;
  cl->staticMethods = VMClassArray::StaticMethodsArray;
  cl->virtualFields = VMClassArray::VirtualFieldsArray;
  cl->staticFields = VMClassArray::StaticFieldsArray;
  cl->depth = 1;
  cl->display.push_back(VMClassArray::SuperArray);
  cl->display.push_back(cl);
  cl->status = loaded;
  cl->assembly = ass;
  return cl; 
}

VMClassArray* Assembly::constructArray(VMCommonClass* base, uint32 dims) {
  if (this != base->assembly) 
    return base->assembly->constructArray(base, dims);

  ClassNameCmp CC(VMClassArray::constructArrayName(base->name, dims),
                  base->nameSpace);
  VMClassArray* cl = (VMClassArray*)(loadedNameClasses->lookupOrCreate(CC, this, arrayDup));
  if (!cl->baseClass) {
    cl->dims = dims;
    if (dims > 1)
      cl->baseClass = constructArray(base, dims - 1);
    else
      cl->baseClass = base;
  }
  return cl;
}

static VMCommonClass* pointerDup(ClassNameCmp &cmp, Assembly* ass) {
  VMClassPointer* cl = gc_new(VMClassPointer)();
  cl->initialise(ass->vm, false);
  cl->isPointer = true;
  cl->name = cmp.name;
  cl->nameSpace = cmp.nameSpace;
  cl->depth = 0;
  cl->display.push_back(cl);
  cl->status = loaded;
  cl->assembly = ass;
  return cl; 
}

VMClassPointer* Assembly::constructPointer(VMCommonClass* base, uint32 dims) {
  if (this != base->assembly) 
    return base->assembly->constructPointer(base, dims);
  
  ClassNameCmp CC(VMClassPointer::constructPointerName(base->name, dims),
                  base->nameSpace);
  VMClassPointer* cl = (VMClassPointer*)(loadedNameClasses->lookupOrCreate(CC, this, pointerDup));
  if (!cl->baseClass) {
    cl->dims = dims;
    cl->baseClass = base;
  }
  return cl;
}

VMClassArray* Assembly::constructArray(const UTF8* name, const UTF8* nameSpace,
                                       uint32 dims) {
  assert(this == ((N3*)VMThread::get()->vm)->coreAssembly);
  ClassNameCmp CC(VMClassArray::constructArrayName(name, dims),
                  nameSpace);
  VMClassArray* cl = (VMClassArray*)(loadedNameClasses->lookupOrCreate(CC, this, arrayDup));
  cl->dims = dims;
  return cl;
}

static VMCommonClass* classDup(ClassNameCmp &cmp, Assembly* ass) {
  VMClass* cl = gc_new(VMClass)();
  cl->initialise(ass->vm, false);
  cl->name = cmp.name;
  cl->nameSpace = cmp.nameSpace;
  cl->virtualTracer = 0;
  cl->staticInstance = 0;
  cl->virtualInstance = 0;
  cl->virtualType = 0;
  cl->super = 0;
  cl->status = hashed;
  cl->assembly = ass;
  return cl;
}

VMClass* Assembly::constructClass(const UTF8* name,
                                  const UTF8* nameSpace, uint32 token) {
  ClassNameCmp CC(name, nameSpace);
  VMClass* cl = (VMClass*)loadedNameClasses->lookupOrCreate(CC, this, classDup);
  loadedTokenClasses->lookupOrCreate(token, cl);
  cl->token = token;
  return cl;
}

static VMCommonClass* genClassDup(ClassNameCmp &cmp, Assembly* ass) {
  VMClass* cl = gc_new(VMGenericClass)();
  cl->initialise(ass->vm, false);
  cl->name = cmp.name;
  cl->nameSpace = cmp.nameSpace;
  cl->virtualTracer = 0;
  cl->staticInstance = 0;
  cl->virtualInstance = 0;
  cl->virtualType = 0;
  cl->super = 0;
  cl->status = hashed;
  cl->assembly = ass;
  return cl;
}

VMGenericClass* Assembly::constructGenericClass(const UTF8* name,
                                         const UTF8* nameSpace, std::vector<VMCommonClass*> genArgs, uint32 token) {
  uint32 size = name->size + 2;
  sint32 i = 0;
  for (std::vector<VMCommonClass*>::iterator it = genArgs.begin(), e = genArgs.end(); it!= e; ++it) {
    size += (*it)->name->size + 1;
  }
  uint16* buf = (uint16*) alloca(sizeof(uint16) * size);
  for (i = 0; i < name->size; i++) {
    buf[i] = name->at(i);
  }
  buf[i++] = '<';
  for (std::vector<VMCommonClass*>::iterator it = genArgs.begin(), e = genArgs.end(); it!= e; ++it) {
    for (int j = 0; j < (*it)->name->size; i++, j++) {
      buf[i] = (*it)->name->at(j);
    }
    buf[i++] = ',';
  }
  buf[i] = '>';
  const UTF8* genName = UTF8::readerConstruct(VMThread::get()->vm, buf, size);
  printf("%s\n", genName->printString());
  
  ClassNameCmp CC(genName, nameSpace);
  VMGenericClass* cl = (VMGenericClass*) loadedNameClasses->lookupOrCreate(CC, this, genClassDup);
  
  cl->genericParams = genArgs; // TODO GC safe?
  cl->token = token;
  
  return cl;
}

static VMField* fieldDup(uint32& key, Assembly* ass) {
  VMField* field = gc_new(VMField)();
  field->token = key;
  return field;
}

VMField*  Assembly::constructField(VMClass* cl, const UTF8* name,
                                   VMCommonClass* signature,
                                   uint32 token) {
  VMField* field = loadedTokenFields->lookupOrCreate(token, this, fieldDup);
  field->classDef = cl;
  field->signature = signature;
  field->name = name;
  return field;
}

static VMMethod* methodDup(uint32& key, Assembly* ass) {
  VMMethod* meth = gc_new(VMMethod)();
  meth->token = key;
  meth->canBeInlined = false;
  return meth;
}

VMMethod* Assembly::constructMethod(VMClass* cl, const UTF8* name, 
                                    uint32 token) {
  VMMethod* meth = loadedTokenMethods->lookupOrCreate(token, this, methodDup);
  meth->classDef = cl;
  meth->_signature = 0;
  meth->name = name;
  return meth;
}

Assembly* Assembly::allocate(const UTF8* name) {
  Assembly* ass = gc_new(Assembly)();
  ass->loadedNameClasses = ClassNameMap::allocate();
  ass->loadedTokenClasses = ClassTokenMap::allocate();
  ass->loadedTokenMethods = MethodTokenMap::allocate();
  ass->loadedTokenFields = FieldTokenMap::allocate();
  ass->assemblyRefs = 0;
  ass->isRead = false;
  ass->name = name;
  ass->currGenericClass = 0;
  return ass;
}

static void unimplemented(uint32 index,
                          std::vector<Table*, gc_allocator<Table*> >& tables,
                          uint32 heapSizes) {
  VMThread::get()->vm->error("Unknown table %x", index);
}

maskVector_t Assembly::maskVector[64] = {
  METHOD_Module,                // 0x00
  METHOD_TypeRef,               // 0x01
  METHOD_TypeDef,               // 0x02
  unimplemented,                // 0x03
  METHOD_Field,                 // 0x04
  unimplemented,                // 0x05
  METHOD_MethodDef,             // 0x06
  unimplemented,                // 0x07
  METHOD_Param,                 // 0x08
  METHOD_InterfaceImpl,         // 0x09
  METHOD_MemberRef,             // 0x0A
  METHOD_Constant,              // 0x0B
  METHOD_CustomAttribute,       // 0x0C
  METHOD_FieldMarshal,          // 0x0D
  METHOD_DeclSecurity,          // 0x0E
  METHOD_ClassLayout,           // 0x1F
  METHOD_FieldLayout,           // 0x10
  METHOD_StandaloneSig,         // 0x11
  METHOD_EventMap,              // 0x12
  unimplemented,                // 0x13
  METHOD_Event,                 // 0x14
  METHOD_PropertyMap,           // 0x15
  unimplemented,                // 0x16
  METHOD_Property,              // 0x17
  METHOD_MethodSemantics,       // 0x18
  METHOD_MethodImpl,            // 0x19
  METHOD_ModuleRef,             // 0x1A
  METHOD_TypeSpec,              // 0x1B
  METHOD_ImplMap,               // 0x1C
  METHOD_FieldRva,              // 0x1D
  unimplemented,                // 0x1E
  unimplemented,                // 0x1F
  METHOD_Assembly,              // 0x20
  METHOD_AssemblyProcessor,     // 0x21
  METHOD_AssemblyOS,            // 0x22
  METHOD_AssemblyRef,           // 0x23
  unimplemented,                // 0x24
  unimplemented,                // 0x25
  METHOD_File,                  // 0x26
  unimplemented,                // 0x27
  METHOD_ManifestResource,      // 0x28
  METHOD_NestedClass,           // 0x29
  METHOD_GenericParam,          // 0x2A
  METHOD_MethodSpec,            // 0x2B
  METHOD_GenericParamConstraint,// 0x2C
  unimplemented,
  unimplemented,
  unimplemented,
  unimplemented, 
  unimplemented,
  unimplemented,
  unimplemented,
  unimplemented,
  unimplemented,
  unimplemented,
  unimplemented,
  unimplemented,
  unimplemented,
  unimplemented,
  unimplemented,
  unimplemented,
  unimplemented,
  unimplemented,
  unimplemented
};

const char* Assembly::maskVectorName[64] = {
  "METHOD_Module",                // 0x00
  "METHOD_TypeRef",               // 0x01
  "METHOD_TypeDef",               // 0x02
  "unimplemented",                // 0x03
  "METHOD_Field",                 // 0x04
  "unimplemented",                // 0x05
  "METHOD_MethodDef",             // 0x06
  "unimplemented",                // 0x07
  "METHOD_Param",                 // 0x08
  "METHOD_InterfaceImpl",         // 0x09
  "METHOD_MemberRef",             // 0x0A
  "METHOD_Constant",              // 0x0B
  "METHOD_CustomAttribute",       // 0x0C
  "METHOD_FieldMarshal",          // 0x0D
  "METHOD_DeclSecurity",          // 0x0E
  "METHOD_ClassLayout",           // 0x1F
  "METHOD_FieldLayout",           // 0x10
  "METHOD_StandaloneSig",         // 0x11
  "METHOD_EventMap",              // 0x12
  "unimplemented",                // 0x13
  "METHOD_Event",                 // 0x14
  "METHOD_PropertyMap",           // 0x15
  "unimplemented",                // 0x16
  "METHOD_Property",              // 0x17
  "METHOD_MethodSemantics",       // 0x18
  "METHOD_MethodImpl",            // 0x19
  "METHOD_ModuleRef",             // 0x1A
  "METHOD_TypeSpec",              // 0x1B
  "METHOD_ImplMap",               // 0x1C
  "METHOD_FieldRva",              // 0x1D
  "unimplemented",                // 0x1E
  "unimplemented",                // 0x1F
  "METHOD_Assembly",              // 0x20
  "METHOD_AssemblyProcessor",     // 0x21
  "METHOD_AssemblyOS",            // 0x22
  "METHOD_AssemblyRef",           // 0x23
  "unimplemented",                // 0x24
  "unimplemented",                // 0x25
  "METHOD_File",                  // 0x26
  "unimplemented",                // 0x27
  "METHOD_ManifestResource",      // 0x28
  "METHOD_NestedClass",           // 0x29
  "METHOD_GenericParam",          // 0x2A
  "METHOD_MethodSpec",            // 0x2B
  "METHOD_GenericParamConstraint", // 0x2C
  "unimplemented",
  "unimplemented",
  "unimplemented",
  "unimplemented", 
  "unimplemented",
  "unimplemented",
  "unimplemented",
  "unimplemented",
  "unimplemented",
  "unimplemented",
  "unimplemented",
  "unimplemented",
  "unimplemented",
  "unimplemented",
  "unimplemented",
  "unimplemented",
  "unimplemented",
  "unimplemented",
  "unimplemented"
};

#define EXTRACT_SIZE(bitmask, index) \
  (1 + (3 & (bitmask >> (index << 1))))

void Table::readRow(uint32* result, uint32 row, ArrayUInt8* array) {
  uint32 rowOffset = offset + ((row - 1) * rowSize);
  for (uint32 i = 0; i < count; ++i) {
    uint32 size = EXTRACT_SIZE(sizeMask, i);
    switch(size) {
      case 1: VMThread::get()->vm->error("implement me"); break;
      case 2: result[i] = READ_U2(array, rowOffset); break;
      case 4: result[i] = READ_U4(array, rowOffset); break;
      default: VMThread::get()->vm->error("unknown size %d", size); break;
    }
  }
}

uint32 Table::readIndexInRow(uint32 row, uint32 index, ArrayUInt8* array) {
  uint32 indexOffset = offset + ((row - 1) * rowSize);
  for (uint32 i = 0; i < index; ++i) {
    indexOffset += EXTRACT_SIZE(sizeMask, i);
  }

  uint32 size = EXTRACT_SIZE(sizeMask, index);

  switch(size) {
    case 1: VMThread::get()->vm->error("implement me"); break;
    case 2: return READ_U2(array, indexOffset);
    case 4: return READ_U4(array, indexOffset);
    default: VMThread::get()->vm->error("unknown size %d", size); break;
  }

  // unreachable
  return 0;
}

void Section::read(Reader* reader, N3* vm) {
  name = (char*) malloc(SECTION_NAME_LENGTH);
  memcpy(name, &(reader->bytes->elements[reader->cursor]),
             SECTION_NAME_LENGTH);
  reader->cursor += SECTION_NAME_LENGTH;

  virtualSize       = reader->readU4();
  virtualAddress    = reader->readU4();
  rawSize           = reader->readU4();
  rawAddress        = reader->readU4();
  relocAddress      = reader->readU4();
  lineNumbers       = reader->readU4();
  relocationsNumber = reader->readU2();
  lineNumbersNumber = reader->readU2();
  characteristics   = reader->readU4();
}

void Header::read(Reader* reader, N3* vm) {
  uint32 start = reader->cursor;
  signature = reader->readU4();
  major = reader->readU2();
  minor = reader->readU2();
  reserved = reader->readU4();
  versionLength = reader->readU4();
  versionName = Assembly::readUTF8(vm, versionLength, reader);
  flags = reader->readU2();
  nbStreams = reader->readU2();

  for (uint32 i = 0; i < nbStreams; ++i) {
    uint32 offset = reader->readU4();
    uint32 size = reader->readU4();
    uint32 len = 
      strlen((char*)(&(reader->bytes->elements[reader->cursor])));

    Stream* stream = gc_new(Stream)();
    char* str = (char*)malloc(len + 1);
    memcpy(str, &(reader->bytes->elements[reader->cursor]), len + 1);
    reader->cursor += (len + (4 - (len % 4)));

    stream->realOffset = start + offset;
    stream->size = size;
    stream->name = str;

    if (!(strcmp(str, "#~"))) tildStream = stream;
    else if (!(strcmp(str, "#Strings"))) stringStream = stream;
    else if (!(strcmp(str, "#US"))) usStream = stream;
    else if (!(strcmp(str, "#Blob"))) blobStream = stream;
    else if (!(strcmp(str, "#GUID"))) guidStream = stream;
    else VMThread::get()->vm->error("invalid stream %s", str);
  }
}

const UTF8* Assembly::readUTF16(VirtualMachine* vm, uint32 len, 
                                Reader* reader) {
  return readUTF16(vm, len, reader->bytes, reader->cursor);
}

const UTF8* Assembly::readUTF16(VirtualMachine* vm, uint32 len, 
                                ArrayUInt8* bytes, uint32 &offset) {
  uint32 realLen = len >> 1;
  uint16* buf = (uint16*)alloca(len);
  uint32 i = 0;
  while (i < realLen) {
    uint16 cur = READ_U2(bytes, offset);
    buf[i] = cur;
    ++i;
  }
  const UTF8* utf8 = UTF8::readerConstruct(vm, buf, realLen);

  return utf8;
}

const UTF8* Assembly::readUTF8(VirtualMachine* vm, uint32 len, Reader* reader) {
  return readUTF8(vm, len, reader->bytes, reader->cursor);
}

const UTF8* Assembly::readUTF8(VirtualMachine* vm, uint32 len,
                               ArrayUInt8* bytes, uint32 &offset) {
  uint16* buf = (uint16*)alloca(len * sizeof(uint16));
  uint32 n = 0;
  uint32 i = 0;
  
  while (i < len) {
    uint32 cur = READ_U1(bytes, offset);
    if (cur & 0x80) {
      uint32 y = READ_U1(bytes, offset);
      if (cur & 0x20) {
        uint32 z = READ_U1(bytes, offset);
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

  return utf8;
}

const UTF8* Assembly::readString(VirtualMachine* vm, uint32 offset) {
  uint32 end = offset;
  uint32 cur = 0;
  while ((cur = READ_U1(bytes, end)) != 0) {}
  return readUTF8(vm, (end - 1 - offset), bytes, offset);
}

void Assembly::readTables(Reader* reader) {
  //uint32 reserved1 = 
  reader->readU4();
  //uint8 major = 
  reader->readU1();
  //uint8 minor = 
  reader->readU1();
  uint8 heapSizes = reader->readU1();
  //uint8 reserved2 = 
  reader->readU1();
  uint32 validLow = reader->readU4();
  uint32 validHigh = reader->readU4();
  //uint32 sortedLow = 
  reader->readU4();
  //uint32 sortedHigh = 
  reader->readU4();
  uint32 tableNumber = 0;
  uint32 offset = 0;

  for (uint32 i = 0; i < 32; ++i) {
    Table* table = gc_new(Table)();
    if ((1 << i) & validLow) {
      table->rowsNumber = reader->readU4();
      ++tableNumber;
    } else {
      table->rowsNumber = 0;
    }
    CLIHeader->tables.push_back(table);
  }

  for (uint32 i = 0; i < 32; ++i) {
    Table* table = gc_new(Table)();
    if ((1 << i) & validHigh) {
      table->rowsNumber = reader->readU4();
      ++tableNumber;
    } else {
      table->rowsNumber = 0;
    }
    CLIHeader->tables.push_back(table);
  }

  offset = reader->cursor;
  
  uint32 index = 0;
  for (std::vector<Table*, gc_allocator<Table*> >::iterator i = 
           CLIHeader->tables.begin(),
       e = CLIHeader->tables.end(); i != e; ++i, ++index) {
    Table* table = (*i);
    if (table->rowsNumber) {
      maskVector[index](index, CLIHeader->tables, heapSizes);
      table->offset = offset;
      offset += (table->rowsNumber * table->rowSize);
    }
  }
}

void Assembly::read() {
  Reader* reader = Reader::allocateReader(bytes);
  PRINT_DEBUG(DEBUG_LOAD, 1, LIGHT_GREEN, "Reading %s::%s", vm->printString(),
              this->printString());

  textSection = gc_new(Section)();
  rsrcSection = gc_new(Section)();
  relocSection = gc_new(Section)();

  reader->seek(TEXT_SECTION_HEADER, Reader::SeekSet);
  textSection->read(reader, vm);
  rsrcSection->read(reader, vm);
  relocSection->read(reader, vm);

  reader->seek(CLI_HEADER, Reader::SeekSet);
  CLIHeaderLocation = reader->readU4();
  reader->seek(textSection->rawAddress + 
                  (CLIHeaderLocation - textSection->virtualAddress),
               Reader::SeekSet);

  cb          = reader->readU4();
  major       = reader->readU2();
  minor       = reader->readU2();
  mdRva       = reader->readU4();
  mdSize      = reader->readU4();
  flags       = reader->readU4();
  entryPoint  = reader->readU4();
  resRva      = reader->readU4();
  resSize     = reader->readU4();
  
  reader->seek(textSection->rawAddress + (mdRva - textSection->virtualAddress),
               Reader::SeekSet);

  CLIHeader = gc_new(Header)();
  CLIHeader->read(reader, vm);

  reader->seek(CLIHeader->tildStream->realOffset, Reader::SeekSet);

  readTables(reader);
}

uint32 Assembly::getTypeDefTokenFromMethod(uint32 token) {
  uint32 index = token & 0xffff;
  Table* typeTable = CLIHeader->tables[CONSTANT_TypeDef];
  uint32 nbRows = typeTable->rowsNumber;
  bool found = false;
  uint32 i = 0;
  
  while (!found && (i < nbRows - 1)) {
    
    uint32 myId = typeTable->readIndexInRow(i + 1, CONSTANT_TYPEDEF_METHODLIST,
                                            bytes);
    uint32 nextId = typeTable->readIndexInRow(i + 2, CONSTANT_TYPEDEF_METHODLIST,
                                              bytes);

    if (index < nextId && index >= myId) {
      found = true;
    } else {
      ++i;
    }
  }
  
  return i + 1 + (CONSTANT_TypeDef << 24);
}

VMCommonClass* Assembly::readTypeSpec(N3* vm, uint32 index) {
  uint32 blobOffset = CLIHeader->blobStream->realOffset;
  Table* typeTable  = CLIHeader->tables[CONSTANT_TypeSpec];
  uint32* typeArray = (uint32*)alloca(sizeof(uint32) * typeTable->rowSize);
  typeTable->readRow(typeArray, index, bytes);
  
  uint32 signOffset = typeArray[CONSTANT_TYPESPEC_SIGNATURE];
  
  uint32 offset = blobOffset + signOffset;
  return extractTypeInSignature(offset);
  
}

VMCommonClass* Assembly::lookupClassFromName(const UTF8* name,
                                             const UTF8* nameSpace) {
  ClassNameCmp CC(name, nameSpace);
  return loadedNameClasses->lookup(CC);
}

VMCommonClass* Assembly::lookupClassFromToken(uint32 token) {
  return loadedTokenClasses->lookup(token);
}

VMMethod* Assembly::lookupMethodFromToken(uint32 token) {
  return loadedTokenMethods->lookup(token);
}
VMField* Assembly::lookupFieldFromToken(uint32 token) {
  return loadedTokenFields->lookup(token);
}



VMCommonClass* Assembly::getClassFromName(N3* vm, const UTF8* name,
                                          const UTF8* nameSpace) {
  VMCommonClass* type = lookupClassFromName(name, nameSpace);

  if (type == 0) {
    Table* typeTable  = CLIHeader->tables[CONSTANT_TypeDef];
    uint32 nb = typeTable->rowsNumber;
    uint32 stringOffset = CLIHeader->stringStream->realOffset;

    for (uint32 i = 0; i < nb; ++i) {
      uint32 value = typeTable->readIndexInRow(i + 1, CONSTANT_TYPEDEF_NAME, bytes);
      const UTF8* curName = readString(vm, stringOffset + value);
      if (name == curName) {
        uint32 value = typeTable->readIndexInRow(i + 1,
                                                 CONSTANT_TYPEDEF_NAMESPACE,
                                                 bytes);
        const UTF8* curNameSpace = readString(vm, stringOffset + value);
        if (curNameSpace == nameSpace) {
          return readTypeDef(vm, i + 1);
        }
      }
    }
  }

  return type;
}

Assembly* Assembly::readAssemblyRef(N3* vm, uint32 index) {
  if (assemblyRefs == 0) {
    uint32 size = sizeof(Assembly*)*
                      CLIHeader->tables[CONSTANT_AssemblyRef]->rowsNumber;
    assemblyRefs = (Assembly**)malloc(size);
    memset(assemblyRefs, 0, size);
  }
  Assembly* ref = assemblyRefs[index - 1];
  if (ref == 0) {
    uint32 stringOffset = CLIHeader->stringStream->realOffset;
    Table* assTable = CLIHeader->tables[CONSTANT_AssemblyRef];
    uint32* assArray = (uint32*)alloca(sizeof(uint32) * assTable->rowSize);
    assTable->readRow(assArray, index, bytes);
    const UTF8* name = 
      readString(vm, stringOffset + assArray[CONSTANT_ASSEMBLY_REF_NAME]);
    
    ref = vm->loadAssembly(name, "dll");
    if (ref == 0) VMThread::get()->vm->error("implement me");
    assemblyRefs[index - 1] = ref;
  }
  return ref;
}

VMCommonClass* Assembly::readTypeRef(N3* vm, uint32 index) {
  uint32 stringOffset = CLIHeader->stringStream->realOffset;
  
  Table* typeTable  = CLIHeader->tables[CONSTANT_TypeRef];
  uint32* typeArray = (uint32*)alloca(sizeof(uint32) * typeTable->rowSize);
  
  typeTable->readRow(typeArray, index, bytes);

  uint32 resScope   = typeArray[CONSTANT_TYPEREF_RESOLUTION_SCOPE];
  uint32 name       = typeArray[CONSTANT_TYPEREF_NAME];
  uint32 nameSpace  = typeArray[CONSTANT_TYPEREF_NAMESPACE];
  

  uint32 val = resScope & 3;
  uint32 entry = resScope >> 2;
  VMCommonClass* type = 0;
  
  switch (val) {
    case 0: VMThread::get()->vm->error("implement me %d %d", val, entry); break;
    case 1: VMThread::get()->vm->error("implement me %d, %d", val, entry); break;
    case 2: {
      Assembly* refAssembly = readAssemblyRef(vm, entry);
      type = refAssembly->getClassFromName(vm, readString(vm, stringOffset + name), 
                                           readString(vm, stringOffset + nameSpace));
      break;
    }
    case 3: VMThread::get()->vm->error("implement me %d %d",val, entry); break;
    default: 
      VMThread::get()->vm->error("unkknown resolution scope %x", val);
      break;
  }
  return type;
}

VMClass* Assembly::readTypeDef(N3* vm, uint32 index) {
  return readTypeDef(vm, index, (std::vector<VMCommonClass*>) 0);
}

VMClass* Assembly::readTypeDef(N3* vm, uint32 index, std::vector<VMCommonClass*> genArgs) {
  uint32 token = (CONSTANT_TypeDef << 24) + index;
  uint32 stringOffset = CLIHeader->stringStream->realOffset;

  Table* typeTable  = CLIHeader->tables[CONSTANT_TypeDef];
  uint32* typeArray = (uint32*)alloca(sizeof(uint32) * typeTable->rowSize);
  
  typeTable->readRow(typeArray, index, bytes);

  uint32 flags      = typeArray[CONSTANT_TYPEDEF_FLAGS];
  uint32 name       = typeArray[CONSTANT_TYPEDEF_NAME];
  uint32 nameSpace  = typeArray[CONSTANT_TYPEDEF_NAMESPACE];
  uint32 extends    = typeArray[CONSTANT_TYPEDEF_EXTENDS];
  //uint32 fieldList  = typeArray[CONSTANT_TYPEDEF_FIELDLIST];
  //uint32 methodList = typeArray[CONSTANT_TYPEDEF_METHODLIST];

  //Table* fieldTable   = CLIHeader->tables[CONSTANT_Field];
  //uint32 fieldSize    = fieldTable->rowsNumber;
  //Table* methodTable  = CLIHeader->tables[CONSTANT_MethodDef];
  //uint32 methodSize   = methodTable->rowsNumber;

  VMClass* type;
  
  if (genArgs == (std::vector<VMCommonClass*>) 0) {
    type = constructClass(readString(vm, name + stringOffset),
                          readString(vm, nameSpace + stringOffset),
                          token);
  } else {
	// generic type
	type = constructGenericClass(readString(vm, name + stringOffset),
                                 readString(vm, nameSpace + stringOffset),
                                 genArgs, token);
  }

  type->vm = vm;

  uint32 val = 3 & extends;
  uint32 entry = extends >> 2;

  switch (val) {
    case 0: {
      type->superToken = entry + (CONSTANT_TypeDef << 24);
      break;
    }
    case 1: {
      type->superToken = entry + (CONSTANT_TypeRef << 24);
      break;
    }
    case 2: {
      VMThread::get()->vm->error("implement me");
      break;
    }
    default: {
      VMThread::get()->vm->error("implement me");
      break;
    }
  }

  getInterfacesFromTokenType(type->interfacesToken, token);
  type->flags = flags;

  return type;
}

void Assembly::getInterfacesFromTokenType(std::vector<uint32>& tokens,
                                          uint32 token) {
  uint32 index = token & 0xffff;
  Table* interfacesTable = CLIHeader->tables[CONSTANT_InterfaceImpl];
  uint32 nbRows = interfacesTable->rowsNumber;

  for (uint32 i = 0; i < nbRows; ++i) {
    uint32 interfaceId = 
      interfacesTable->readIndexInRow(i + 1, CONSTANT_INTERFACE_IMPL_CLASS,
                                      bytes);
    if (interfaceId == index) {
      uint32 cl = interfacesTable->readIndexInRow(i + 1,
                                        CONSTANT_INTERFACE_IMPL_INTERFACE,
                                        bytes);
      uint32 table = cl & 3;
      uint32 interfaceToken = cl >> 2;

      switch (table) {
        case 0: interfaceToken += (CONSTANT_TypeDef << 24); break;
        case 1: interfaceToken += (CONSTANT_TypeRef << 24); break;
        case 2: interfaceToken += (CONSTANT_TypeSpec << 24); break;
        default: VMThread::get()->vm->error("unknown table %x", table); break;
      }
      tokens.push_back(interfaceToken);
    }
  }
}

VMCommonClass* Assembly::loadType(N3* vm, uint32 token, bool resolve,
                            bool resolveStatic, bool clinit, bool dothrow) {
	return loadType(vm, token, resolve, resolveStatic, clinit, dothrow, (std::vector<VMCommonClass*>) 0);
}


VMCommonClass* Assembly::loadType(N3* vm, uint32 token, bool resolve,
                            bool resolveStatic, bool clinit, bool dothrow,
                            std::vector<VMCommonClass*> genArgs) {
  
  VMCommonClass* type = lookupClassFromToken(token);
  if (!type || type->status == hashed) {
    uint32 table = token >> 24;
    uint32 index = token & 0xffff;

    if (table == CONSTANT_TypeDef) {
      type = readTypeDef(vm, index, genArgs);
    } else if (table == CONSTANT_TypeRef) {
      type = readTypeRef(vm, index);
    } else if (table == CONSTANT_TypeSpec) {
      type = readTypeSpec(vm, index);
    } else {
      VMThread::get()->vm->error("implement me %x", token);
    }
  }

  if (type == 0) VMThread::get()->vm->error("implement me");
  if (type->status == hashed) {
    type->aquire();
    if (type->status == hashed) {
      type->status = loaded;
    }
    type->release();
  }

  if (resolve) type->resolveType(resolveStatic, clinit);

  return type;
}

void Assembly::readClass(VMCommonClass* cl) {
  // temporarily store the class being read in case it is a generic class
  currGenericClass = dynamic_cast<VMGenericClass*>(cl);
	
  uint32 index = cl->token & 0xffff;
  Table* typeTable = CLIHeader->tables[CONSTANT_TypeDef];
  uint32 typeSize = typeTable->rowsNumber;
  uint32* typeArray = (uint32*)alloca(sizeof(uint32) * typeTable->rowSize);
  typeTable->readRow(typeArray, index, bytes);
  
  uint32 fieldList  = typeArray[CONSTANT_TYPEDEF_FIELDLIST];
  uint32 methodList = typeArray[CONSTANT_TYPEDEF_METHODLIST];

  Table* fieldTable   = CLIHeader->tables[CONSTANT_Field];
  uint32 fieldSize    = fieldTable->rowsNumber;
  Table* methodTable  = CLIHeader->tables[CONSTANT_MethodDef];
  uint32 methodSize   = methodTable->rowsNumber;
  
  getProperties(cl);

  if (methodList && methodTable != 0 && methodList <= methodSize) {
    uint32 endMethod = (index == typeSize) ? 
        methodSize + 1 : 
        typeTable->readIndexInRow(index + 1, CONSTANT_TYPEDEF_METHODLIST,
                                  bytes);

    uint32 nbMethods = endMethod - methodList;

    for (uint32 i = 0; i < nbMethods; ++i) {
      VMMethod* meth = readMethodDef(i + methodList, cl);
      if (isStatic(meth->flags)) {
        cl->staticMethods.push_back(meth);
      } else {
        cl->virtualMethods.push_back(meth);
      }
    }
  }
  
  if (fieldList && fieldTable != 0 && fieldList <= fieldSize) {
    uint32 endField = (index == typeSize) ? 
        fieldSize + 1 : 
        typeTable->readIndexInRow(index + 1, CONSTANT_TYPEDEF_FIELDLIST, bytes);

    uint32 nbFields = endField - fieldList;

    for (uint32 i = 0; i < nbFields; ++i) {
      VMField* field = readField(i + fieldList, cl);
      if (isStatic(field->flags)) {
        cl->staticFields.push_back(field);
      } else {
        cl->virtualFields.push_back(field);
      }
    }
  }
  
  // we have stopped reading a generic class
  currGenericClass = 0;
}

void Assembly::readCustomAttributes(uint32 offset, std::vector<llvm::GenericValue>& args, VMMethod* meth) {
  uncompressSignature(offset);
  uint16 prolog = READ_U2(bytes, offset);

  if (prolog != 0x1) VMThread::get()->vm->error("unknown prolog");

  uint32 start = meth->virt ? 1 : 0;

  for (uint32 i = start + 1; i < meth->parameters.size(); ++i) {
    if (meth->parameters[i] == MSCorlib::pSInt32) {
      llvm::GenericValue gv;
      gv.IntVal = llvm::APInt(32, READ_U4(bytes, offset));
      args.push_back(gv);
    } else {
      VMThread::get()->vm->error("implement me");
    }
  }

}

ArrayObject* Assembly::getCustomAttributes(uint32 token, VMCommonClass* cl) {
  Table* attrTable = CLIHeader->tables[CONSTANT_CustomAttribute];
  uint32 attrSize = attrTable->rowsNumber;
  uint32* attrArray = (uint32*)alloca(sizeof(uint32) * attrTable->rowSize);
  std::vector<VMObject*> vec;

  for (uint32 i = 0; i < attrSize; ++i) {
    attrTable->readRow(attrArray, i + 1, bytes);
    uint32 meth = attrArray[CONSTANT_CUSTOM_ATTRIBUTE_TYPE];
    uint32 table = meth & 7;
    uint32 index = meth >> 3;
    VMMethod* cons = 0;

    switch(table) {
      default: 
        VMThread::get()->vm->error("implement me"); 
        break;
      case 2: 
        cons = getMethodFromToken(index + (CONSTANT_MethodDef << 24));
        break;
      case 3:
        cons = getMethodFromToken(index + (CONSTANT_MemberRef << 24));
        break;
    }

    if (cl == cons->classDef) {
      uint32 blobOffset = CLIHeader->blobStream->realOffset;
      std::vector<llvm::GenericValue> args;
      VMObject* obj = (*cons->classDef)();
      args.push_back(llvm::GenericValue(obj));
      readCustomAttributes(blobOffset + attrArray[CONSTANT_CUSTOM_ATTRIBUTE_VALUE], args, cons);

      (*cons)(args);
      vec.push_back(obj);
    }
  }

  ArrayObject* res = ArrayObject::acons(vec.size(), MSCorlib::arrayObject);
  for (uint32 i = 0; i < vec.size(); ++i)
    res->elements[i] = vec[i];
  
  return res;
}

void Assembly::getProperties(VMCommonClass* cl) {
  uint32 index = cl->token & 0xffff;
  Table* mapTable = CLIHeader->tables[CONSTANT_PropertyMap];
  uint32 mapSize = mapTable->rowsNumber;
  
  Table* propertyTable = CLIHeader->tables[CONSTANT_Property];
  uint32 propertySize = propertyTable->rowsNumber;
  
  uint32 propertyList = 0;
  uint32 i = 0;

  while (!propertyList && i != mapSize) {
    uint32 parent = mapTable->readIndexInRow(i + 1,
                                        CONSTANT_PROPERTY_MAP_PARENT, bytes);
    if (parent == index) {
      propertyList = mapTable->readIndexInRow(i + 1,
                                  CONSTANT_PROPERTY_MAP_PROPERTY_LIST, bytes);
    } else {
      ++i;
    }
  }

  if (propertyList && propertyTable != 0 && propertyList <= propertySize) {
    uint32 endProperty = (i + 1 == mapSize) ? 
        propertySize + 1 : 
        mapTable->readIndexInRow(i + 2, CONSTANT_PROPERTY_MAP_PROPERTY_LIST,
                                 bytes);
    uint32 nbProperties = endProperty - propertyList;

    for (uint32 j = 0; j < nbProperties; ++j) {
      cl->properties.push_back(readProperty(j + propertyList, cl));
    }

  } 
}

Property* Assembly::readProperty(uint32 index, VMCommonClass* cl) {
  uint32 stringOffset = CLIHeader->stringStream->realOffset;
  uint32 blobOffset = CLIHeader->blobStream->realOffset;
  
  Table* propTable  = CLIHeader->tables[CONSTANT_Property];
  uint32* propArray = (uint32*)alloca(sizeof(uint32) * propTable->rowSize);
  
  propTable->readRow(propArray, index, bytes);

  uint32 flags      = propArray[CONSTANT_PROPERTY_FLAGS];
  uint32 nameIndex  = propArray[CONSTANT_PROPERTY_NAME];
  uint32 type       = propArray[CONSTANT_PROPERTY_TYPE];

  Property* prop = gc_new(Property)();
  prop->name = readString(VMThread::get()->vm, stringOffset + nameIndex);
  prop->flags = flags;
  prop->type = cl;
  uint32 offset = blobOffset + type;
  prop->virt = extractMethodSignature(offset, cl, prop->parameters);
  return prop;
}

VMMethod* Assembly::readMethodDef(uint32 index, VMCommonClass* cl) {
  uint32 token = index + (CONSTANT_MethodDef << 24);
  uint32 stringOffset = CLIHeader->stringStream->realOffset;
  uint32 blobOffset = CLIHeader->blobStream->realOffset;
  
  Table* paramTable  = CLIHeader->tables[CONSTANT_Param];
  uint32 paramSize    = paramTable->rowsNumber;
  
  Table* methTable  = CLIHeader->tables[CONSTANT_MethodDef];
  uint32 methodSize    = methTable->rowsNumber;
  uint32* methArray = (uint32*)alloca(sizeof(uint32) * methTable->rowSize);
  
  methTable->readRow(methArray, index, bytes);

  uint32 rva        = methArray[CONSTANT_METHODDEF_RVA];
  uint32 implFlags  = methArray[CONSTANT_METHODDEF_IMPLFLAGS];
  uint32 flags      = methArray[CONSTANT_METHODDEF_FLAGS];
  uint32 name       = methArray[CONSTANT_METHODDEF_NAME];
  uint32 signature  = methArray[CONSTANT_METHODDEF_SIGNATURE];
  uint32 paramList  = methArray[CONSTANT_METHODDEF_PARAMLIST];
  
  uint32 offset = blobOffset + signature;
  VMMethod* meth = 
    constructMethod((VMClass*)cl, readString(cl->vm, (name + stringOffset)),
                    token);
  meth->virt = extractMethodSignature(offset, cl, meth->parameters);
  meth->flags = flags;
  meth->implFlags = implFlags;
  
  if (rva) {
    meth->offset = textSection->rawAddress + 
                   (rva - textSection->virtualAddress);
  } else {
    meth->offset = 0;
  }
  
  if (paramList && paramTable != 0 && paramList <= paramSize) {
    uint32 endParam = (index == methodSize) ? 
        paramSize + 1 : 
        methTable->readIndexInRow(index + 1, CONSTANT_METHODDEF_PARAMLIST,
                                  bytes);

    uint32 nbParams = endParam - paramList;

    for (uint32 j = 0; j < nbParams; ++j) {
      meth->params.push_back(readParam(j + paramList, meth));
    }

  }

  return meth;
}

VMField* Assembly::readField(uint32 index, VMCommonClass* cl) {
  uint32 token = index + (CONSTANT_Field << 24);
  uint32 stringOffset = CLIHeader->stringStream->realOffset;
  uint32 blobOffset = CLIHeader->blobStream->realOffset;
  
  Table* fieldTable  = CLIHeader->tables[CONSTANT_Field];
  uint32* fieldArray = (uint32*)alloca(sizeof(uint32) * fieldTable->rowSize);
  
  fieldTable->readRow(fieldArray, index, bytes);

  uint32 flags      = fieldArray[CONSTANT_FIELD_FLAGS];
  uint32 name       = fieldArray[CONSTANT_FIELD_NAME];
  uint32 signature  = fieldArray[CONSTANT_FIELD_SIGNATURE];
  
  uint32 offset = blobOffset + signature;
  VMField* field = 
    constructField((VMClass*)cl, readString(cl->vm, (name + stringOffset)),
                   extractFieldSignature(offset), token);
  field->flags = flags;
  
  return field;
}

Param* Assembly::readParam(uint32 index, VMMethod* meth) {
  uint32 stringOffset = CLIHeader->stringStream->realOffset;
  
  Table* paramTable  = CLIHeader->tables[CONSTANT_Param];
  uint32* paramArray = (uint32*)alloca(sizeof(uint32) * paramTable->rowSize);
  
  paramTable->readRow(paramArray, index, bytes);

  uint32 flags      = paramArray[CONSTANT_PARAM_FLAGS];
  uint32 name       = paramArray[CONSTANT_PARAM_NAME];
  uint32 sequence   = paramArray[CONSTANT_PARAM_SEQUENCE];
  
  Param* param = gc_new(Param)();
  param->flags = flags;
  param->sequence = sequence;
  param->name = readString(meth->classDef->vm, stringOffset + name);
  param->method = meth;
  
  return param;
}

VMCommonClass* Assembly::loadTypeFromName(const UTF8* name, 
                                          const UTF8* nameSpace, 
                                          bool resolve, bool unify,
                                          bool clinit, bool dothrow) {
  VMCommonClass* cl = lookupClassFromName(name, nameSpace);
  if (cl == 0 || cl->status == hashed) {
    cl = getClassFromName(((N3*)VMThread::get()->vm), name, nameSpace);
    
    if (cl == 0) VMThread::get()->vm->error("implement me");

    if (cl->status == hashed) {
      cl->aquire();
      if (cl->status == hashed) {
        cl->status = loaded;
      }
      cl->release();
    }
  }

  if (resolve) cl->resolveType(unify, clinit);

  return cl;
}

void Assembly::readSignature(uint32 localVarSig, 
                             std::vector<VMCommonClass*>& locals) {
  uint32 table = localVarSig >> 24;
  uint32 index = localVarSig & 0xffff;
  if (table != CONSTANT_StandaloneSig) {
    VMThread::get()->vm->error("locals do not point to a StandAloneSig table");
  }
  Table* signatures = CLIHeader->tables[CONSTANT_StandaloneSig];
  uint32* array = (uint32*)alloca(sizeof(uint32) * signatures->rowSize);
  signatures->readRow(array, index, bytes);

  uint32 blobOffset = CLIHeader->blobStream->realOffset;
  uint32 blobEntry = blobOffset + array[CONSTANT_STANDALONE_SIG_SIGNATURE];
  

  localVarSignature(blobEntry, locals);
}

VMField* Assembly::getFieldFromToken(uint32 token, bool stat) {
  VMField* field = lookupFieldFromToken(token);
  if (!field) {
    uint32 table = token >> 24;
    switch (table) {
      case CONSTANT_Field : {
        uint32 typeToken = getTypedefTokenFromField(token);
        uint32 newTable = typeToken >> 24;
        switch (newTable) {
          case CONSTANT_TypeDef : {
            loadType((N3*)(VMThread::get()->vm), typeToken, true, true, false,
                     true);
            field = lookupFieldFromToken(token);
            if (!field) {
              VMThread::get()->vm->error("implement me");
            }
            break;
          }
          default : {
            VMThread::get()->vm->error("implement me");
          }
        }
        break;
      }

      case CONSTANT_MemberRef : {
        field = readMemberRefAsField(token, stat);
        break;
      }

      default : {
        VMThread::get()->vm->error("implement me");
      }
    }
  }
  field->classDef->resolveType(false, false);
  return field;
}

uint32 Assembly::getTypedefTokenFromField(uint32 token) {
  uint32 index = token & 0xffff;
  Table* typeTable = CLIHeader->tables[CONSTANT_TypeDef];
  uint32 nbRows = typeTable->rowsNumber;

  bool found = false;
  uint32 i = 0;

  while (!found && i < nbRows - 1) {
    uint32 myId = typeTable->readIndexInRow(i + 1, CONSTANT_TYPEDEF_FIELDLIST, bytes);
    uint32 nextId = typeTable->readIndexInRow(i + 2, CONSTANT_TYPEDEF_FIELDLIST, bytes);

    if ((index < nextId) && (index >= myId)) {
      found = true;
    } else {
      ++i;
    }
  }

  return i + 1 + (CONSTANT_TypeDef << 24);
}

uint32 Assembly::getExplicitLayout(uint32 token) {
  uint32 index = token & 0xffff;
  Table* layoutTable = CLIHeader->tables[CONSTANT_ClassLayout];
  uint32 tableSize = layoutTable->rowsNumber;

  bool found = false;
  uint32 i = 0;
  uint32 size = 0;

  while (!found && i != tableSize) {
    uint32 parent = layoutTable->readIndexInRow(i + 1,
                                        CONSTANT_CLASS_LAYOUT_PARENT, bytes);
    if (parent == index) {
      found = true;
      size = layoutTable->readIndexInRow(i + 1,
                                  CONSTANT_CLASS_LAYOUT_CLASS_SIZE, bytes);
    }
    ++i;
  }

  if (!found)
    VMThread::get()->vm->error("implement me");

  return size;
}

VMField* Assembly::readMemberRefAsField(uint32 token, bool stat) {
  uint32 index = token & 0xffff;
  Table* memberTable = CLIHeader->tables[CONSTANT_MemberRef];
  uint32* memberArray = (uint32*)alloca(sizeof(uint32) * memberTable->rowSize);
  
  memberTable->readRow(memberArray, index, bytes);

  uint32 stringOffset = CLIHeader->stringStream->realOffset;
  uint32 blobOffset   = CLIHeader->blobStream->realOffset;
  
  const UTF8* name = readString((N3*)(VMThread::get()->vm), stringOffset + 
                                          memberArray[CONSTANT_MEMBERREF_NAME]);
  
  uint32 offset = blobOffset + memberArray[CONSTANT_MEMBERREF_SIGNATURE];
  VMCommonClass* signature = extractFieldSignature(offset);
                                    

  uint32 value = memberArray[CONSTANT_MEMBERREF_CLASS];
  uint32 table = value & 7;
  index = value >> 3;

  switch (table) {
    case 0 : {
      uint32 typeToken = index + (CONSTANT_TypeDef << 24);
      VMCommonClass* type = loadType(((N3*)VMThread::get()->vm), typeToken,
                                     true, false, false, true);
      VMField* field = type->lookupField(name, signature, stat, true);
      return field;
    }

    case 1 : {
      uint32 typeToken = index + (CONSTANT_TypeRef << 24);
      VMCommonClass* type = loadType(((N3*)VMThread::get()->vm), typeToken,
                                     true, false, false, true);
      VMField* field = type->lookupField(name, signature, stat, true);
      return field;
    }

    case 2:
    case 3:
    case 4: VMThread::get()->vm->error("implement me"); break;
    default:
      VMThread::get()->vm->error("unknown MemberRefParent tag %d", table);
      
  }

  return 0;
}


VMMethod* Assembly::getMethodFromToken(uint32 token) {
  VMMethod* meth = lookupMethodFromToken(token);
  if (!meth) {
    uint32 table = token >> 24;
    switch (table) {
      case CONSTANT_MethodDef : {
        uint32 typeToken = getTypedefTokenFromMethod(token);
        uint32 newTable = typeToken >> 24;
        switch (newTable) {
          case CONSTANT_TypeDef : {
            loadType((N3*)(VMThread::get()->vm), typeToken, true, true, false,
                     true);
            meth = lookupMethodFromToken(token);
            if (!meth) {
              VMThread::get()->vm->error("implement me");
            }
            break;
          }
          default : {
            VMThread::get()->vm->error("implement me");
          }
        }
        break;
      }

      case CONSTANT_MemberRef : {
        meth = readMemberRefAsMethod(token);
        break;
      }

      default : {
        VMThread::get()->vm->error("implement me");
      }
    }
  }
  meth->getSignature();
  return meth;
}

uint32 Assembly::getTypedefTokenFromMethod(uint32 token) {
  uint32 index = token & 0xffff;
  Table* typeTable = CLIHeader->tables[CONSTANT_TypeDef];
  uint32 nbRows = typeTable->rowsNumber;

  bool found = false;
  uint32 i = 0;

  while (!found && i < nbRows - 1) {
    uint32 myId = typeTable->readIndexInRow(i + 1, CONSTANT_TYPEDEF_METHODLIST, bytes);
    uint32 nextId = typeTable->readIndexInRow(i + 2, CONSTANT_TYPEDEF_METHODLIST, bytes);

    if ((index < nextId) && (index >= myId)) {
      found = true;
    } else {
      ++i;
    }
  }

  return i + 1 + (CONSTANT_TypeDef << 24);
}

VMMethod* Assembly::readMemberRefAsMethod(uint32 token) {
  uint32 index = token & 0xffff;
  Table* memberTable = CLIHeader->tables[CONSTANT_MemberRef];
  uint32* memberArray = (uint32*)alloca(sizeof(uint32) * memberTable->rowSize);
  
  memberTable->readRow(memberArray, index, bytes);

  uint32 stringOffset = CLIHeader->stringStream->realOffset;
  uint32 blobOffset   = CLIHeader->blobStream->realOffset;
  
  const UTF8* name = readString((N3*)(VMThread::get()->vm), stringOffset + 
                                          memberArray[CONSTANT_MEMBERREF_NAME]);
  
  uint32 offset = blobOffset + memberArray[CONSTANT_MEMBERREF_SIGNATURE];
  std::vector<VMCommonClass*> args;
                                    

  uint32 value = memberArray[CONSTANT_MEMBERREF_CLASS];
  uint32 table = value & 7;
  index = value >> 3;

  switch (table) {
    case 0 : {
      uint32 typeToken = index + (CONSTANT_TypeDef << 24);
      VMCommonClass* type = loadType(((N3*)VMThread::get()->vm), typeToken,
                                     true, false, false, true);
      bool virt = extractMethodSignature(offset, type, args);
      VMMethod* meth = type->lookupMethod(name, args, !virt, true);
      return meth;
    }

    case 1 : {
      uint32 typeToken = index + (CONSTANT_TypeRef << 24);
      VMCommonClass* type = loadType(((N3*)VMThread::get()->vm), typeToken,
                                     true, false, false, true);
      bool virt = extractMethodSignature(offset, type, args);
      VMMethod* meth = type->lookupMethod(name, args, !virt, true);
      return meth;
    }

    case 2:
    case 3: VMThread::get()->vm->error("implement me %d", table); break;
    case 4: {
      VMClass* type = (VMClass*)readTypeSpec(vm, index);
      
      VMGenericClass* genClass = dynamic_cast<VMGenericClass*>(type);

      if (genClass) {
        bool virt = extractMethodSignature(offset, type, args);
        VMMethod* meth = type->lookupMethod(name, args, !virt, true);
        return meth;
      } else {
	    VMMethod* meth = gc_new(VMMethod)();
	    bool virt = extractMethodSignature(offset, type, args);
	    bool structReturn = false;
	    const llvm::FunctionType* signature = VMMethod::resolveSignature(args, virt, structReturn);
	    meth->_signature = signature;
	    meth->classDef = type;
	    meth->name = name;
	    meth->virt = virt;
	    meth->structReturn = structReturn;
	    meth->parameters = args; // TODO check whether this fix is correct
	    return meth;
      }
    }
    default:
      VMThread::get()->vm->error("unknown MemberRefParent tag %d", table);
      
  }

  return 0;
}

const UTF8* Assembly::readUserString(uint32 token) {
  uint32 offset = CLIHeader->usStream->realOffset + token;

  uint8 size = READ_U1(bytes, offset);
  if (size >> 7) {
    if ((size >> 6) == 3) {
      uint32 size1 = READ_U1(bytes, offset);
      uint32 size2 = READ_U1(bytes, offset);
      uint32 size3 = READ_U1(bytes, offset);
      size = ((size ^ 0xc0) << 24) + (size1 << 16) + (size2 << 8) + size3;
    } else {
      size = ((size ^ 0x80) << 8) + READ_U1(bytes, offset);
    }
  }

  const UTF8* res = readUTF16((N3*)(VMThread::get()->vm), size, bytes, offset);
  return res;
}

uint32 Assembly::getRVAFromField(uint32 token) {
  
  uint32 index = token & 0xffff;
  Table* rvaTable = CLIHeader->tables[CONSTANT_FieldRVA];
  uint32 rvaSize = rvaTable->rowsNumber;
  
  uint32 i = 0;
  bool found = false;

  while (!found && i != rvaSize) {
    uint32 fieldId = rvaTable->readIndexInRow(i + 1, CONSTANT_FIELD_RVA_FIELD,
                                              bytes);
    if (fieldId == index) {
      found = true;
    } else {
      ++i;
    }
  }
  if (!found) {
    return 0;
  } else {
    return rvaTable->readIndexInRow(i + 1, CONSTANT_FIELD_RVA_RVA, bytes);
  }
}

