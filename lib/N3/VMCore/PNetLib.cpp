//===--------------- PNetLib.cpp - PNetLib interface ----------------------===//
//
//                              N3
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <math.h>

#include <dlfcn.h>
#include <stdio.h>

#include "mvm/JIT.h"

#include "Assembly.h"
#include "CLIAccess.h"
#include "CLIJit.h"
#include "CLIString.h"
#include "NativeUtil.h"
#include "N3.h"
#include "PNetLib.h"
#include "Reader.h"
#include "VMArray.h"
#include "VMClass.h"
#include "VMObject.h"
#include "VMThread.h"

using namespace n3;

#define IL_CONSOLE_NORMAL 0


#define MEMBER_TYPES_CONSTRUCTOR 0x1
#define MEMBER_TYPES_EVENT 0x2
#define MEMBER_TYPES_FIELD 0x4
#define MEMBER_TYPES_METHOD 0x8
#define MEMBER_TYPES_PROPERTY 0x10
#define MEMBER_TYPES_TYPEINFO 0x20
#define MEMBER_TYPES_CUSTOM 0x40
#define MEMBER_TYPES_NESTEDTYPE 0x80
#define MEMBER_TYPES_ALL 0xBF

#define METHOD_SEMANTIC_ATTRIBUTES_SETTER   0x1
#define METHOD_SEMANTIC_ATTRIBUTES_GETTER   0x2
#define METHOD_SEMANTIC_ATTRIBUTES_OTHER    0x4
#define METHOD_SEMANTIC_ATTRIBUTES_ADDON    0x8
#define METHOD_SEMANTIC_ATTRIBUTES_REMOVEON 0x10
#define METHOD_SEMANTIC_ATTRIBUTES_FIRE     0x20


extern "C" {
extern uint32 ILGetCodePage(void);
extern uint32 ILGetCultureID(void);
extern char* ILGetCultureName(void);
extern sint32 ILAnsiGetMaxByteCount(sint32);
extern sint32  ILConsoleWriteChar(sint32);
extern uint32 ILConsoleGetMode(void);
extern sint32 ILAnsiGetBytes(uint16*, sint32, uint8*, sint32);
extern void _IL_Stdio_StdFlush(void*, sint32);
extern char ILGetUnicodeCategory(sint32);
extern sint64 _IL_TimeMethods_GetCurrentTime(void*);
extern uint32 ILUnicodeStringToLower(void*, void*, uint32);
extern sint32 ILUnicodeStringCompareIgnoreCase(void*, void*, sint32);
extern sint32 ILUnicodeStringCompareNoIgnoreCase(void*, void*, sint32);

// PNET wants this
void *GC_run_thread(void *(*thread_func)(void *), void *arg){ return 0; }
#ifndef USE_GC_BOEHM
int GC_invoke_finalizers (void) { return 0; }
int GC_should_invoke_finalizers (void) { return 0; }
void GC_register_finalizer_no_order(void*, void (*)(void*, void*), void*, void (**)(void*, void*), void**) { return; }
void GC_gcollect(void) {}
void* GC_malloc_uncollectable(size_t) { return 0; }
void GC_exclude_static_roots(void*, void*) {}
void GC_free(void*) {}
void GC_malloc_explicitly_typed(void) {}
size_t GC_get_heap_size(void) {return 0;}
int GC_register_disappearing_link(void**) { return 0; }
int GC_general_register_disappearing_link(void**, void*) { return 0; }
int GC_pthread_sigmask(int, const sigset_t*, sigset_t*) { return 0; }
int GC_pthread_detach(pthread_t) { return 0; }
int GC_pthread_create(pthread_t*, const pthread_attr_t*, void* (*)(void*), void*) { return 0; }
void* GC_malloc(size_t) { return 0; }
void GC_make_descriptor(void) {}
int GC_unregister_disappearing_link(void**) { return 0; }
void (*GC_finalizer_notifier)(void);
int GC_java_finalization;
int GC_finalize_on_demand;
void GC_set_max_heap_size(GC_word) {}
void* GC_malloc_atomic(size_t) { return 0; }
#endif

// Fake termcap symbols
void tigetstr(void) {
  abort();
}
void tgetstr(void) {
  abort();
}
void setupterm(void) {
  abort();
}
void tigetnum(void) {
  abort();
}
void tgetnum(void) {
  abort();
}
void tigetflag(void) {
  abort();
}
void tparm(void) {
  abort();
}
void tgetent(void) {
  abort();
}
void tputs(void) {
  abort();
}
void tgoto(void) {
  abort();
}
void tgetflag(void) {
  abort();
}
}



extern "C" void System_Runtime_CompilerServices_RuntimeHelpers_InitializeArray(
                                               VMArray* array, VMField* field) {
  if (!array || !field) return;

  VMClass* type = field->classDef;
  VMClassArray* ts = (VMClassArray*)array->classOf;
  VMCommonClass* bs = ts->baseClass;
  Assembly* ass = type->assembly;

  uint32 rva = ass->getRVAFromField(field->token);
  Section* rsrcSection = ass->rsrcSection;

  uint32 size = array->size;
  uint32 offset = rsrcSection->rawAddress + (rva - rsrcSection->virtualAddress);
  ArrayUInt8* bytes = ass->bytes;

  if (bs == N3::pChar) {
    for (uint32 i = 0; i < size; ++i) {
      ((uint16*)(void*)(array->elements))[i] = READ_U2(bytes, offset);
    }
  } else if (bs == N3::pSInt32) {
    for (uint32 i = 0; i < size; ++i) {
      ((sint32*)(void*)(array->elements))[i] = READ_U4(bytes, offset);
    }
  } else if (bs == N3::pDouble) {
    for (uint32 i = 0; i < size; ++i) {
      ((double*)(void*)(array->elements))[i] = READ_U8(bytes, offset);
    }
  } else {
    VMThread::get()->vm->error("implement me");
  }
}

extern "C" VMObject* System_Type_GetTypeFromHandle(VMCommonClass* cl) {
  return cl->getClassDelegatee();
}

extern "C" void System_Threading_Monitor_Enter(VMObject* obj) {
  obj->aquire();
}

extern "C" void System_Threading_Monitor_Exit(VMObject* obj) {
  obj->unlock();
}

extern "C" uint32 System_Text_DefaultEncoding_InternalCodePage() {
  return ILGetCodePage();
}

extern "C" VMObject* System_Reflection_Assembly_GetCallingAssembly() {
  Assembly* ass = Assembly::getCallingAssembly();
  assert(ass);
  return ass->getAssemblyDelegatee();
}

extern "C" VMObject* System_Reflection_Assembly_GetExecutingAssembly() {
  Assembly* ass = Assembly::getExecutingAssembly();
  assert(ass);
  return ass->getAssemblyDelegatee();
}

extern "C" uint32 System_Globalization_CultureInfo_InternalCultureID() {
  return ILGetCultureID();
}

extern "C" VMObject* System_Globalization_CultureInfo_InternalCultureName() {
  char* val = ILGetCultureName();
  N3* vm = (N3*)(VMThread::get()->vm);
  if (val) {
    VMObject* ret = vm->asciizToStr(val);
    free(val);
    return ret;
  } else {
    VMObject* ret = vm->asciizToStr("iv");
    return ret;
  }
}

extern "C" void System_Reflection_Assembly_LoadFromFile() {
  VMThread::get()->vm->error("implement me");
}

static const UTF8* newBuilder(N3* vm, CLIString* value, uint32 length) {
  uint32 valueLength = value ? value->length : 0;
  const UTF8* utf8 = value ? value->value : 0;
  uint32 roundLength = (7 + length) & 0xfffffff8;
  uint16* buf = (uint16*)alloca(roundLength * sizeof(uint16));
  uint32 strLength = 0;

  if (value != 0) {
    if (valueLength <= roundLength) {
      memcpy(buf, utf8->elements, valueLength * sizeof(uint16));
      strLength = valueLength;
    } else {
      memcpy(buf, utf8->elements, roundLength * sizeof(uint16));
      strLength = roundLength;
    }
  }

  return vm->readerConstructUTF8(buf, strLength);

}

extern "C" VMObject* System_String_NewBuilder(CLIString* value, 
                                               uint32 length) {
  N3* vm = (N3*)(VMThread::get()->vm);
  CLIString* str = (CLIString*)vm->UTF8ToStr(newBuilder(vm, value, length));
  return str;
}

extern "C" VMObject* Platform_SysCharInfo_GetNewLine() {
  N3* vm = (N3*)(VMThread::get()->vm);
  return vm->asciizToStr("\n");
}

extern "C" void System_String_CopyToChecked(CLIString* str, sint32 sstart, 
                                            ArrayUInt16* dest, sint32 dstart,
                                            sint32 count) {
  const UTF8* value = str->value;
  memcpy(&dest->elements[dstart], &value->elements[sstart], count << 1);
}

extern "C" sint32 System_Text_DefaultEncoding_InternalGetMaxByteCount(sint32 val) {
  return ILAnsiGetMaxByteCount(val);
}

extern "C" void Platform_Stdio_StdWrite(sint32 fd, ArrayUInt8* value, 
                                        sint32 index, sint32 count) {
  if (fd == 1) {
    if (ILConsoleGetMode() == IL_CONSOLE_NORMAL) {
      fwrite(&value->elements[index], 1, count, stdout);
    } else {
      char* buf = (char*)(&value->elements[index]);
      while (count > 0) {
        ILConsoleWriteChar(*buf);
        ++buf;
        --count;
      }
      fflush(stdout);
    }
  } else {
    fwrite(&value->elements[index], 1, count, stderr);
  }
}

extern "C" sint32 System_Text_DefaultEncoding_InternalGetBytes(ArrayUInt16* chars,
            sint32 charIndex, sint32 charCount, ArrayUInt8* bytes, sint32 byteIndex) {
  
  return ILAnsiGetBytes(&chars->elements[charIndex], charCount, &bytes->elements[byteIndex], bytes->size - byteIndex);
}

extern "C" void Platform_Stdio_StdFlush(sint32 fd) {
  _IL_Stdio_StdFlush(0, fd);
}

extern "C" void System_Array_InternalCopy(VMArray* src, sint32 sstart, 
                                          VMArray* dst, sint32 dstart, 
                                          sint32 len) {
  N3* vm = (N3*)(VMThread::get()->vm);
  verifyNull(src);
  verifyNull(dst);
  
  if (!(src->classOf->isArray && dst->classOf->isArray)) {
    vm->arrayStoreException();
  }
  
  VMClassArray* ts = (VMClassArray*)src->classOf;
  VMClassArray* td = (VMClassArray*)dst->classOf;
  VMCommonClass* dstType = td->baseClass;
  VMCommonClass* srcType = ts->baseClass;

  if (len > src->size) {
    vm->indexOutOfBounds(src, len);
  } else if (len > dst->size) {
    vm->indexOutOfBounds(dst, len);
  } else if (len + sstart > src->size) {
    vm->indexOutOfBounds(src, len + sstart);
  } else if (len + dstart > dst->size) {
    vm->indexOutOfBounds(dst, len + dstart);
  } else if (dstart < 0) {
    vm->indexOutOfBounds(dst, dstart);
  } else if (sstart < 0) {
    vm->indexOutOfBounds(src, sstart);
  } else if (len < 0) {
    vm->indexOutOfBounds(src, len);
  } 
  
  bool doThrow = false;
  
  if (srcType->super == N3::pValue && srcType != dstType) {
    vm->arrayStoreException();
  } else if (srcType->super != N3::pValue && srcType->super != N3::pEnum) {
    sint32 i = sstart;
    while (i < sstart + len && !doThrow) {
      VMObject* cur = ((ArrayObject*)src)->at(i);
      if (cur) {
        if (!(cur->classOf->isAssignableFrom(dstType))) {
          doThrow = true;
          len = i;
        }
      }
      ++i;
    }
  }
  
  
  uint32 size = srcType->naturalType->getPrimitiveSizeInBits() / 8;
  if (size == 0) size = sizeof(void*);
  void* ptrDst = (void*)((int64_t)(dst->elements) + size * dstart);
  void* ptrSrc = (void*)((int64_t)(src->elements) + size * sstart);
  memmove(ptrDst, ptrSrc, size * len);

  if (doThrow)
    vm->arrayStoreException();
  
}

extern "C" sint32 System_Array_GetRank(VMObject* arr) {
  verifyNull(arr);
  if (arr->classOf->isArray) {
    return ((VMClassArray*)(arr->classOf))->dims;
  } else {
    VMThread::get()->vm->error("implement me");
    return 0;
  }
}

extern "C" sint32 System_Array_GetLength(VMObject* arr) {
  verifyNull(arr);
  if (arr->classOf->isArray) {
    return ((VMArray*)arr)->size;
  } else {
    VMThread::get()->vm->error("implement me");
    return 0;
  }
}

extern "C" sint32 System_Array_GetLowerBound(VMObject* arr, sint32 dim) {
  return 0;
}

extern "C" VMObject* System_Object_GetType(VMObject* obj) {
  verifyNull(obj);
  return obj->classOf->getClassDelegatee();
}

extern "C" VMObject* System_Reflection_ClrType_GetElementType(VMObject* Klass) {
  VMCommonClass* cl = (VMCommonClass*)((*Klass)(N3::typeClrType).PointerVal);
  if (!cl->isArray) {
    VMThread::get()->vm->error("implement me");
    return 0;
  } else {
    return ((VMClassArray*)cl)->baseClass->getClassDelegatee();
  }
}

extern "C" CLIString* System_String_NewString(uint32 size) {
  CLIString* str = (CLIString*)(N3::pString->doNew());
  str->length = size;
  return str;
}

extern "C" void System_String_Copy_3(CLIString* dest, sint32 pos, 
                                     CLIString* src) {
  ArrayUInt16* arr = ArrayUInt16::acons(pos + src->value->size, 
                                        (VMClassArray*)N3::pChar);
  
  for (sint32 i = 0; i < pos; ++i) {
    arr->setAt(i, dest->value->at(i));
  }

  for (sint32 i = 0; i < src->length; ++i) {
    arr->setAt(pos + i, src->value->at(i));
  }

  dest->value = ((UTF8*)arr)->extract(VMThread::get()->vm, 0, pos + src->value->size);
  dest->length = dest->value->size;
}

extern "C" void System_String_Copy_5(CLIString* dest, sint32 destPos, 
                                     CLIString* src, sint32 srcPos, 
                                     sint32 length) {
  const UTF8* utf8Src = src->value->extract(VMThread::get()->vm, srcPos, 
                                            srcPos + length);
  if (destPos == 0) {
    dest->value = utf8Src;
    dest->length = dest->value->size;
  } else {
    const UTF8* utf8Dest = dest->value->extract(VMThread::get()->vm, 0, 
                                                destPos);
    sint32 len1 = utf8Dest->size;
    sint32 len2 = utf8Src->size;
    uint16* buf = (uint16*)alloca((len1 + len2) * sizeof(uint16));

    memcpy(buf, utf8Dest->elements, len1 * sizeof(uint16));
    memcpy(buf + len1, utf8Dest->elements, len2 * sizeof(uint16));

    const UTF8* utf8 = VMThread::get()->vm->readerConstructUTF8(buf, 
                                                                len1 + len2);
    dest->value = utf8;
    dest->length = dest->value->size;
  }
}

extern "C" bool System_String_Equals(CLIString* str1, CLIString* str2) {
  return str1->value == str2->value;
}

extern "C" VMObject* System_Threading_Thread_InternalCurrentThread() {
  return VMThread::get()->vmThread;
}

extern "C" sint32 Platform_SysCharInfo_GetUnicodeCategory(char c) {
  return ILGetUnicodeCategory(c);
}

extern "C" uint16 System_String_GetChar(CLIString* str, sint32 index) {
  return str->value->at(index);
}

extern "C" sint32 System_String_IndexOf(CLIString* str, uint16 value, 
                                        sint32 startIndex, sint32 count) {
  if (startIndex < 0) {
    VMThread::get()->vm->error("shoud throw arg range");
  }

  if ((count < 0) || (str->length - startIndex < count)) {
    VMThread::get()->vm->error("shoud throw arg range");
  }

  sint32 i = startIndex;
  const UTF8* utf8 = str->value;
  while (i < startIndex + count) {
    if (utf8->at(i) == value) return i;
    else ++i;
  }

  return -1;
}

extern "C" sint32 System_String_GetHashCode(CLIString* str) {
  sint32 hash = 0;
  const UTF8* utf8 = str->value;
  for (sint32 i = 0; i < utf8->size; ++i) {
    hash += ((hash << 5) + utf8->elements[i]);
  }
  return hash;
}

extern "C" double System_Decimal_ToDouble(void* ptr) {
  VMThread::get()->vm->error("implement me");
  return 0.0;
}

extern "C" double System_Math_Log10(double val) {
  return log10(val);
}

extern "C" double System_Math_Floor(double val) {
  return floor(val);
}

extern "C" VMObject* System_Text_StringBuilder_Insert_System_Text_StringBuilder_System_Int32_System_Char(
                                                      StringBuilder* obj,
                                                      sint32 index, 
                                                      uint16 value) {
  N3* vm = (N3*)(VMThread::get()->vm);
  CLIString* buildString = obj->buildString;
  const UTF8* utf8 = buildString->value;
  sint32 strLength = buildString->length;
  sint32 length = (index + 1) > strLength ? index + 1 : strLength + 1;
  uint16* buf = (uint16*)alloca(length * sizeof(uint16));

  if (index != 0) {
    memcpy(buf, utf8->elements, index * sizeof(uint16));
  }

  if (strLength > index) {
    memcpy(&(buf[index + 1]), &(utf8->elements[index]), 
               (strLength - index) * sizeof(uint16));
  }

  buf[index] = value;
  CLIString* str = (CLIString*)vm->UTF8ToStr(vm->readerConstructUTF8(buf, length));
  obj->buildString = str;
  
  return obj;
}

extern "C" VMObject* System_Text_StringBuilder_Insert_System_Text_StringBuilder_System_Int32_System_String(
                                                      StringBuilder* obj,
                                                      sint32 index, 
                                                      CLIString* str) {
  N3* vm = (N3*)(VMThread::get()->vm);
  CLIString* buildString = obj->buildString;
  const UTF8* strUtf8 = str->value;
  const UTF8* buildUtf8 = buildString->value;
  sint32 strLength = str->length;
  sint32 buildLength = buildString->length;
  sint32 length = strLength + buildLength;
  uint16* buf = (uint16*)alloca(length * sizeof(uint16));

  if (index != 0) {
    memcpy(buf, buildUtf8->elements, index * sizeof(uint16));
  }

  if (strLength != 0) {
    memcpy(&(buf[index]), strUtf8->elements, strLength * sizeof(uint16));
  }
    
  if (buildLength - index > 0) {
    memcpy(&(buf[strLength + index]), &(buildUtf8->elements[index]), 
               (buildLength - index) * sizeof(uint16));
  }

  CLIString* val = (CLIString*)vm->UTF8ToStr(vm->readerConstructUTF8(buf, length));
  obj->buildString = val;

  return obj;
}

extern "C" VMObject* System_Text_StringBuilder_Append_System_Text_StringBuilder_System_Char(
                                                StringBuilder* obj,
                                                uint16 value) {
  N3* vm = (N3*)(VMThread::get()->vm);
  CLIString* buildString = obj->buildString;
  const UTF8* utf8 = buildString->value;
  sint32 length = buildString->length;
  uint16* buf = (uint16*)alloca((length + 1) * sizeof(uint16));

  memcpy(buf, utf8->elements, length * sizeof(uint16));

  buf[length] = value;
  CLIString* val = (CLIString*)vm->UTF8ToStr(vm->readerConstructUTF8(buf, length + 1));
  obj->buildString = val;
  return obj;
}


extern "C" VMObject* System_Text_StringBuilder_Append_System_Text_StringBuilder_System_String(
                                                StringBuilder* obj,
                                                CLIString* str) {
  N3* vm = (N3*)(VMThread::get()->vm);
  CLIString* buildString = obj->buildString;
  const UTF8* buildUtf8 = buildString->value;
  const UTF8* strUtf8 = str->value;
  sint32 buildLength = buildString->length;
  sint32 strLength = str->length;
  sint32 length = buildLength + strLength;
  uint16* buf = (uint16*)alloca(length * sizeof(uint16));

  memcpy(buf, buildUtf8->elements, buildLength * sizeof(uint16));
  memcpy(&(buf[buildLength]), strUtf8->elements, strLength * sizeof(uint16));

  CLIString* val = (CLIString*)vm->UTF8ToStr(vm->readerConstructUTF8(buf, length));
  obj->buildString = val;
  return obj;
}

extern "C" sint32 System_String_FindInRange(CLIString* obj, sint32 srcFirst, 
                                            sint32 srcLast, sint32 step,
                                            CLIString* dest) {
  uint16* buf1 = (uint16*)&(obj->value->elements[srcFirst]);
  uint16* buf2 = (uint16*)(dest->value->elements);
  sint32 destLength = dest->length;
  sint32 size = destLength * sizeof(uint16);
  
  if (step > 0) {
    if (destLength == 1) {
      while (srcFirst <= srcLast) {
        if (buf1[0]  == buf2[0]) {
          return srcFirst;
        } else {
          buf1 = &(buf1[1]);
          ++srcFirst; 
        }
      } 
    } else {
      while (srcFirst <= srcLast) {
        if ((buf1[0] == buf2[0]) && !memcmp(buf1, buf2, size)) {
          return srcFirst;
        } else {
          buf1 = &(buf1[1]);
          ++srcFirst;
        }
      }
    }
  } else {
    if (destLength == 1) {
      while (srcFirst >= srcLast) {
        if (buf1[0] == buf2[0]) {
          return srcFirst;
        } else {
          buf1 = buf1 - 1;
          --srcFirst;
        }
      }
    } else {
      while (srcFirst >= srcLast) {
        if ((buf1[0] == buf2[0]) && !memcmp(buf1, buf2, size)) {
          return srcFirst;
        } else {
          buf1 = buf1 - 1;
          --srcFirst;
        }
      }
    }
  }
  return -1;
}

extern "C" VMObject* System_Reflection_Assembly_LoadFromName(CLIString* str, sint32 & error, VMObject* parent) {
  N3* vm = (N3*)(VMThread::get()->vm);
  Assembly* ass = vm->loadAssembly(str->value, "dll");
  if (!ass) vm->error("unfound assembly %s\n", str->value->printString());
  error = 0;
  return ass->getAssemblyDelegatee();
}

extern "C" CLIString* System_String_Concat_2(CLIString* str1, CLIString* str2) {
  N3* vm = (N3*)(VMThread::get()->vm);
  const UTF8* u1 = str1->value;
  const UTF8* u2 = str2->value;
  sint32 len1 = str1->length;
  sint32 len2 = str2->length;
  uint16* buf = (uint16*)alloca((len1 + len2) * sizeof(uint16));

  memcpy(buf, u1->elements, len1 * sizeof(uint16));
  memcpy(&(buf[len1]), u2->elements, len2 * sizeof(uint16));
  
  CLIString* val = (CLIString*)vm->UTF8ToStr(vm->readerConstructUTF8(buf, len1 + len2));
  
  return val;
}

extern "C" CLIString* System_String_Concat_3(CLIString* str1, CLIString* str2, CLIString* str3) {
  N3* vm = (N3*)(VMThread::get()->vm);
  const UTF8* u1 = str1->value;
  const UTF8* u2 = str2->value;
  const UTF8* u3 = str3->value;
  sint32 len1 = str1->length;
  sint32 len2 = str2->length;
  sint32 len3 = str3->length;
  uint16* buf = (uint16*)alloca((len1 + len2 + len3) * sizeof(uint16));

  memcpy(buf, u1->elements, len1 * sizeof(uint16));
  memcpy(&(buf[len1]), u2->elements, len2 * sizeof(uint16));
  memcpy(&(buf[len1 + len2]), u3->elements, len3 * sizeof(uint16));
  
  CLIString* val = (CLIString*)vm->UTF8ToStr(vm->readerConstructUTF8(buf, len1 + len2 + len3));
  
  return val;
}

extern "C" void System_String_RemoveSpace(CLIString* str, sint32 index, sint32 length) {
  const UTF8* utf8 = str->value;
  sint32 strLength = str->length;
  uint16* buf = (uint16*)alloca(strLength * sizeof(uint16));
  sint32 j = index;

  if (index != 0) {
    memcpy(buf, utf8->elements, index * sizeof(uint16));
  }
  
  // 32 is space
  for (sint32 i = 0; i < length; ++i) {
    uint16 cur = utf8->elements[index + i];
    if (cur != 32) {
      buf[j] = cur;
    } else {
      ++j;
    }
  }

  if (strLength > (index + length)) {
    memcpy(&(buf[j]), &(utf8->elements[index + length]), (strLength - (index + length)) * sizeof(uint16));
  }

  const UTF8* res = VMThread::get()->vm->readerConstructUTF8(buf, j);
  str->value = res;
  str->length = j;
}

extern "C" void System_String__ctor_3(CLIString* str, uint16 ch, sint32 count) {
  ArrayUInt16* array = ArrayUInt16::acons(count, N3::arrayChar);
  for (sint32 i = 0; i < count; ++i) {
    array->elements[i] = ch;
  }

  const UTF8* utf8 = VMThread::get()->vm->readerConstructUTF8(array->elements, array->size);
  str->value = utf8;
  str->length = array->size;
  str->capacity = array->size;
}

extern "C" int64_t Platform_TimeMethods_GetCurrentTime() {
  return _IL_TimeMethods_GetCurrentTime(0);
}

#define ASSEMBLY_VALUE(obj) ((Assembly**)obj)[3]

void* sys_memrchr(const void* s, int c, size_t n) {
  unsigned char* m = (unsigned char*) s + n;
  for (;;) {
    if (!(n--)) return NULL;
    else if (*m-- == (unsigned char)c) return (void*)(m+1);
  }
}

extern "C" VMObject* System_Reflection_Assembly_GetType(VMObject* obj, CLIString* str, bool onError, bool ignoreCase) {
  Assembly* ass = ASSEMBLY_VALUE(obj);
  const UTF8* utf8 = str->value;
  char* asciiz = utf8->UTF8ToAsciiz();
  char* index = (char*)sys_memrchr(asciiz, '.', strlen(asciiz));
  N3* vm = ass->vm;
  
  index[0] = 0;
  ++index;
  VMCommonClass* cl = ass->loadTypeFromName(vm->asciizConstructUTF8(index), vm->asciizConstructUTF8(asciiz), true, true, true, onError);
  if (!cl) VMThread::get()->vm->error("implement me");
  return cl->getClassDelegatee();
}

static bool parameterMatch(std::vector<VMCommonClass*> params, ArrayObject* types, bool virt) {
  uint32 v = virt ? 1 : 0;
  if (types->size + v + 1 != params.size()) return false;
  for (sint32 i = 0; i < types->size; ++i) {
    VMCommonClass* cur = (VMCommonClass*)(*N3::typeClrType)(types->elements[i]).PointerVal;
    if (cur != params[i + 1 + v]) return false;
  }
  return true;
}

extern "C" VMObject* System_Reflection_ClrType_GetMemberImpl(VMObject* Type, CLIString* str, sint32 memberTypes, sint32 bindingFlags, VMObject* binder, 
                                                   sint32 callingConventions, ArrayObject* types, VMObject* modifiers) {
  VMCommonClass* type = (VMCommonClass*)((*N3::typeClrType)(Type).PointerVal);
  const UTF8* name = str->value;
  if (memberTypes == MEMBER_TYPES_PROPERTY) {
    std::vector<Property*, gc_allocator<Property*> > properties = 
                                                    type->properties;
    Property *res = 0;
    for (std::vector<Property*, gc_allocator<Property*> >::iterator i = properties.begin(), 
            e = properties.end(); i!= e; ++i) {
      if ((*i)->name == name) {
        res = *i; 
        break;
      }
    }
    if (res == 0) VMThread::get()->vm->error("implement me");
    return res->getPropertyDelegatee();
  } else if (memberTypes == MEMBER_TYPES_METHOD) {
    std::vector<VMMethod*> virtualMethods = type->virtualMethods;
    std::vector<VMMethod*> staticMethods = type->staticMethods;
    
    for (std::vector<VMMethod*>::iterator i = virtualMethods.begin(), 
            e = virtualMethods.end(); i!= e; ++i) {
      VMMethod* meth = *i;
      if (meth->name == name) {
        if (parameterMatch(meth->parameters, types, true)) {
          return meth->getMethodDelegatee();
        }
      }
    }
    
    for (std::vector<VMMethod*>::iterator i = staticMethods.begin(), 
            e = staticMethods.end(); i!= e; ++i) {
      VMMethod* meth = *i;
      if (meth->name == name) {
        if (parameterMatch(meth->parameters, types, false)) {
          return meth->getMethodDelegatee();
        }
      }
    }

  } else {
    VMThread::get()->vm->error("implement me");
  }
  return 0;
}

extern "C" VMObject* System_Reflection_ClrHelpers_GetSemantics(mvm::Object* item, uint32 attributes, bool nonPublic) {
  if (item->getVirtualTable() == Property::VT) {
    Property* prop = (Property*)item;
    if (attributes == METHOD_SEMANTIC_ATTRIBUTES_GETTER) {
      char* asciiz = prop->name->UTF8ToAsciiz();
      char* buf = (char*)alloca(strlen(asciiz) + 5);
      sprintf(buf, "get_%s", asciiz);
      VirtualMachine* vm = VMThread::get()->vm;
      VMMethod* meth = prop->type->lookupMethod(vm->asciizConstructUTF8(buf), prop->parameters, true, false);
      assert(meth);
      return meth->getMethodDelegatee();
    }
  } else {
    VMThread::get()->vm->error("implement me");
  }
  return 0;
}

static void decapsulePrimitive(VMObject* arg, const llvm::Type* type, std::vector<llvm::GenericValue>& args) {
  if (type == llvm::Type::Int1Ty) {
    llvm::GenericValue gv;
    gv.IntVal = llvm::APInt(1, (bool)((uint32*)arg)[VALUE_OFFSET]);
    args.push_back(gv);
  } else if (type == llvm::Type::Int8Ty) {
    llvm::GenericValue gv;
    gv.IntVal = llvm::APInt(8, (uint8)((uint32*)arg)[VALUE_OFFSET]);
    args.push_back(gv);
  } else if (type == llvm::Type::Int16Ty) {
    llvm::GenericValue gv;
    gv.IntVal = llvm::APInt(16, (uint16)((uint32*)arg)[VALUE_OFFSET]);
    args.push_back(gv);
  } else if (type == llvm::Type::Int32Ty) {
    llvm::GenericValue gv;
    gv.IntVal = llvm::APInt(32, (uint32)((uint32*)arg)[VALUE_OFFSET]);
    args.push_back(gv);
  } else if (type == llvm::Type::Int64Ty) {
    llvm::GenericValue gv;
    uint32* ptr = &((uint32*)arg)[VALUE_OFFSET];
    gv.IntVal = llvm::APInt(64, ((uint64*)ptr)[0]);
    args.push_back(gv);
  } else if (type == llvm::Type::FloatTy) {
    llvm::GenericValue gv;
    float* ptr = &((float*)arg)[VALUE_OFFSET];
    gv.FloatVal = ((float*)ptr)[0];
    args.push_back(gv);
  } else if (type == llvm::Type::DoubleTy) {
    llvm::GenericValue gv;
    uint32* ptr = &((uint32*)arg)[VALUE_OFFSET];
    gv.DoubleVal = ((double*)ptr)[0];
    args.push_back(gv);
  } else if (type == llvm::PointerType::getUnqual(llvm::Type::Int8Ty)) {
    llvm::GenericValue gv(((void**)arg)[VALUE_OFFSET]);
    args.push_back(gv);
  } else {
    VMThread::get()->vm->error("implement me");
  }
}

extern "C" VMObject* System_Reflection_ClrMethod_Invoke(VMObject* Method, VMObject* obj, sint32 invokeAttr, VMObject* binder, ArrayObject* args, VMObject* culture) {
  VMMethod* meth = (VMMethod*)(*N3::methodMethodType)(Method).PointerVal;
  meth->getSignature();
  meth->compiledPtr();
  llvm::Function* func = CLIJit::compile(meth->classDef, meth);
  VMClass* type = meth->classDef;
  type->resolveStatic(true);
  uint32 virt = meth->virt;

  if ((obj != 0) && virt) {
    if (!(obj->classOf->isAssignableFrom(type))) {
      VMThread::get()->vm->illegalArgumentException(meth->name->printString());
    }
    verifyNull(obj);
  }
    
  std::vector<llvm::GenericValue> gvargs;
  uint32 index = 0;
  
  llvm::Function::arg_iterator i = func->arg_begin();
  llvm::Function::arg_iterator e = func->arg_end();
  if (virt) {
    llvm::GenericValue gv(obj);
    gvargs.push_back(gv);
    ++i;
  }
  
  for ( ;i != e; ++i, ++index) {
    const llvm::Type* type = i->getType();
    if (llvm::isa<llvm::PointerType>(type) && type != llvm::PointerType::getUnqual(llvm::Type::Int8Ty)) {
      llvm::GenericValue gv(args->elements[index]);
      gvargs.push_back(gv);
    } else {
      decapsulePrimitive(args->elements[index], type, gvargs);
    }
  }
  
  llvm::GenericValue gv;
  try{
    gv = (*meth)(gvargs);
  }catch(...) {
    assert(0);
  }
  
  VMObject* res = 0;
  VMCommonClass* retType = meth->parameters[0];
  if (retType == N3::pVoid) {
    res = (*N3::pVoid)();
  } else if (retType == N3::pBoolean) {
    res = (*N3::pBoolean)();
    (*N3::ctorBoolean)(res, gv.IntVal.getBoolValue());
  } else if (retType == N3::pUInt8) {
    res = (*N3::pUInt8)();
    (*N3::ctorUInt8)(res, (uint8)gv.IntVal.getZExtValue());
  } else if (retType == N3::pSInt8) {
    res = (*N3::pSInt8)();
    (*N3::ctorSInt8)(res, (uint8)gv.IntVal.getSExtValue());
  } else if (retType == N3::pChar) {
    res = (*N3::pChar)();
    (*N3::ctorChar)(res, (uint16)gv.IntVal.getZExtValue());
  } else if (retType == N3::pSInt16) {
    res = (*N3::pSInt16)();
    (*N3::ctorSInt16)(res, (sint16)gv.IntVal.getSExtValue());
  } else if (retType == N3::pUInt16) {
    res = (*N3::pUInt16)();
    (*N3::ctorUInt16)(res, (uint16)gv.IntVal.getZExtValue());
  } else if (retType == N3::pSInt32) {
    res = (*N3::pSInt32)();
    (*N3::ctorSInt32)(res, (sint32)gv.IntVal.getSExtValue());
  } else if (retType == N3::pUInt32) {
    res = (*N3::pUInt32)();
    (*N3::ctorUInt32)(res, (sint32)gv.IntVal.getZExtValue());
  } else if (retType == N3::pSInt64) {
    res = (*N3::pSInt64)();
    (*N3::ctorSInt64)(res, (sint64)gv.IntVal.getSExtValue());
  } else if (retType == N3::pUInt64) {
    res = (*N3::pUInt64)();
    (*N3::ctorUInt64)(res, (sint64)gv.IntVal.getZExtValue());
  } else if (retType == N3::pIntPtr) {
    res = (*N3::pIntPtr)();
    (*N3::ctorIntPtr)(res, (void*)gv.IntVal.getSExtValue());
  } else if (retType == N3::pUIntPtr) {
    res = (*N3::pUIntPtr)();
    (*N3::ctorUIntPtr)(res, (void*)gv.IntVal.getZExtValue());
  } else if (retType == N3::pFloat) {
    res = (*N3::pFloat)();
    (*N3::ctorFloat)(res, gv.FloatVal);
  } else if (retType == N3::pDouble) {
    res = (*N3::pDouble)();
    (*N3::ctorDouble)(res, gv.DoubleVal);
  } else {
    if (retType->super == N3::pValue || retType->super == N3::pEnum)
      VMThread::get()->vm->error("implement me");
    res = (VMObject*)gv.PointerVal;
  }
  
  return res;
}


static VMObject* createResourceStream(Assembly* ass, sint32 posn) {
  uint32 resSize = ass->resSize;
  uint32 resRva = ass->resRva;
  Section* textSection = ass->textSection;
  uint32 sectionLen = resSize;
  uint32 section = 0;
  uint32 start = 0;
  uint32 length = 0;
  uint32 pad = 0;
  
  Reader* reader = Reader::allocateReader(ass->bytes);
  section = textSection->rawAddress + (resRva - textSection->virtualAddress);

  reader->seek(section, Reader::SeekSet);
  while (posn > 0) {
    if (sectionLen < 4) return 0;
    length = reader->readU4();
    if (length > (sectionLen - 4)) return 0;
    if ((length % 4) != 0) {
      pad = 4 - (length % 4);
    } else {
      pad = 0;
    }
    start = start + length + pad + 4;
    section = section + length + pad + 4;
    reader->seek(section + length + pad + 4, Reader::SeekSet);
    sectionLen = sectionLen - (length + pad + 4);
    posn = posn - 1;
  }

  start = start + 4;
  if (sectionLen < 4) return 0;
  length = reader->readU4();
  if (length > (sectionLen - 4)) return 0;

  VMObject* res = (*N3::resourceStreamType)();
  (*N3::ctorResourceStreamType)(res, ass, (uint64)start, (uint64)length);

  return res;
}
      
extern "C" VMObject* System_Reflection_Assembly_GetManifestResourceStream(VMObject* Ass, CLIString* str) {
  Assembly* ass = (Assembly*)(*N3::assemblyAssemblyReflection)(Ass).PointerVal;
  const UTF8* utf8 = str->value;
  Header* header = ass->CLIHeader;
  uint32 stringOffset = header->stringStream->realOffset;
  Table* manTable  = header->tables[CONSTANT_ManifestResource];
  uint32 manRows   = manTable->rowsNumber;
  sint32 pos = -1;
  uint32 i = 0;
  VirtualMachine* vm = VMThread::get()->vm;
  
  while ((pos == -1) && (i < manRows)) {
    uint32 nameOffset = manTable->readIndexInRow(i + 1, CONSTANT_MANIFEST_RESOURCE_NAME, ass->bytes);
    const UTF8* name = ass->readString(vm, stringOffset + nameOffset);

    if (name == utf8) {
      pos = i;
    } else {
      ++i;
    }
  }

  if (pos != -1) {
    return createResourceStream(ass, pos);
  } else {
    return 0;
  }
}


extern "C" ArrayObject* System_Reflection_ClrHelpers_GetCustomAttributes(Assembly* ass, VMCommonClass* clrTypePrivate, bool inherit) {
  return ass->getCustomAttributes(clrTypePrivate->token, clrTypePrivate);
}

extern "C" VMObject* System_Globalization_TextInfo_ToLower(VMObject* obj, CLIString* str) {
  verifyNull(str);
  const UTF8* utf8 = str->value;
  uint32 length = str->length;

  uint16* buf = (uint16*)alloca(length * sizeof(uint16));

  VirtualMachine* vm = VMThread::get()->vm;

  memcpy(buf, utf8->elements, length * sizeof(uint16));
  ILUnicodeStringToLower((void*)buf, (void*)utf8->elements, length);
  const UTF8* res = vm->readerConstructUTF8(buf, length);
  return ((N3*)vm)->UTF8ToStr(res);
}

extern "C" VMObject* System_String_Replace(CLIString* str, uint16 c1, uint16 c2) {
  const UTF8* utf8 = str->value;
  uint32 length = str->length;
  if ((c1 == c2) || length == 0) return str;

  uint16* buf = (uint16*)alloca(length * sizeof(uint16));
  memcpy(buf, utf8->elements, length * sizeof(uint16));
  for (uint32 i = 0; i < length; ++i) {
    if (buf[i] == c1) buf[i] = c2;
  }
  
  N3* vm = (N3*)VMThread::get()->vm;
  const UTF8* res = vm->readerConstructUTF8(buf, length);
  return vm->UTF8ToStr(res);
}

extern "C" uint32 System_Reflection_ClrResourceStream_ResourceRead(Assembly* assembly, uint64 position, ArrayUInt8* buffer, uint32 offset, uint32 count) {
  uint32 resRva = assembly->resRva;
  ArrayUInt8* bytes = assembly->bytes;
  Section* textSection = assembly->textSection;
  uint32 section = 0;

  section = textSection->rawAddress + (resRva - textSection->virtualAddress);
  memcpy(&(buffer->elements[offset]), &(bytes->elements[section + position]), count);

  return count;
}

extern "C" sint32 System_String_CompareInternal(CLIString* strA, sint32 indexA, sint32 lengthA, CLIString* strB, sint32 indexB, sint32 lengthB, bool ignoreCase) {
  if (strA == 0) {
    if (strB == 0) {
      return 0;
    }
    return -1;
  } else if (strB == 0) {
    return 1;
  } else {
    sint32 cmp = 0;
    if (lengthA >= lengthB) {
      if (ignoreCase) {
        cmp = ILUnicodeStringCompareIgnoreCase((void*)&(strA->value->elements[indexA]), (void*)&(strB->value->elements[indexB]), lengthB);
      } else {
        cmp = ILUnicodeStringCompareNoIgnoreCase((void*)&(strA->value->elements[indexA]), (void*)&(strB->value->elements[indexB]), lengthB);
      }

      if (cmp != 0) return cmp;
      else if (lengthA > lengthB) return 1;
      else return 0;
    } else {
      if (ignoreCase) {
        cmp = ILUnicodeStringCompareIgnoreCase((void*)&(strA->value->elements[indexA]), (void*)&(strB->value->elements[indexB]), lengthA);
      } else {
        cmp = ILUnicodeStringCompareNoIgnoreCase((void*)&(strA->value->elements[indexA]), (void*)&(strB->value->elements[indexB]), lengthA);
      }

      if (cmp != 0) return cmp;
      else return -1;
    }
  }
}

extern "C" void System_String_CharFill(CLIString* str, sint32 start, sint32 count, char ch) {
  const UTF8* utf8 = str->value;
  sint32 length = start + count;
  uint16* buf = (uint16*)alloca(length * sizeof(uint16));

  memcpy(buf, utf8->elements, start * sizeof(uint16));
  for (sint32 i = 0; i < count; ++i) {
    buf[i + start] = ch;
  }
  
  VirtualMachine* vm = VMThread::get()->vm;
  const UTF8* val = vm->readerConstructUTF8(buf, length);
  str->value = val;
  str->length = length;
}


extern "C" sint32 System_String_InternalOrdinal(CLIString *strA, sint32 indexA, sint32 lengthA,
						CLIString *strB, sint32 indexB, sint32 lengthB) {
	const uint16 *bufA;
	const uint16 *bufB;

	/* Handle the easy cases first */
	if(!strA)
	{
		if(!strB)
		{
			return 0;
		}
		else
		{
			return -1;
		}
	}
	else if(!strB)
	{
		return 1;
	}

	/* Compare the two strings */
	bufA = &(strA->value->elements[indexA]);
	bufB = &(strB->value->elements[indexB]);
	while(lengthA > 0 && lengthB > 0)
	{
		if(*bufA < *bufB)
		{
			return -1;
		}
		else if(*bufA > *bufB)
		{
			return 1;
		}
		++bufA;
		++bufB;
		--lengthA;
		--lengthB;
	}

	/* Determine the ordering based on the tail sections */
	if(lengthA > 0)
	{
		return 1;
	}
	else if(lengthB > 0)
	{
		return -1;
	}
	else
	{
		return 0;
	}
}

extern "C" void System_GC_Collect() {
#ifdef MULTIPLE_GC
  mvm::Thread::get()->GC->collect();
#else
  Collector::collect();
#endif
}



void NativeUtil::initialise() {
  void* p;
  p = (void*)&System_Runtime_CompilerServices_RuntimeHelpers_InitializeArray;
  p = (void*)&System_Type_GetTypeFromHandle;
}
