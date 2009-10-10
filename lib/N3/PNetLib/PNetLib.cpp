//===--------------- PNetLib.cpp - PNetLib interface ----------------------===//
//
//                              N3
//
// This file is distributed under the University of Illinois Open Source
// License with the following additions:
//
// Copyright (C) 2001, 2002, 2003  Southern Storm Software, Pty Ltd.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <math.h>

#include <dlfcn.h>
#include <stdio.h>

#include "mvm/JIT.h"

#include "Assembly.h"
#include "CLIAccess.h"
#include "CLIJit.h"
#include "PNetString.h"
#include "NativeUtil.h"
#include "MSCorlib.h"
#include "N3.h"
#include "PNetLib.h"
#include "Reader.h"
#include "VMArray.h"
#include "VMClass.h"
#include "VMObject.h"
#include "VMThread.h"
#include "CLIString.h"

#include "PNetPath.inc"

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

#include "mvm/Config/config.h"
// PNET wants this
void *GC_run_thread(void *(*thread_func)(void *), void *arg){ return 0; }
#if not(USE_GC_BOEHM)
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
void GC_set_max_heap_size(size_t) {}
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



extern "C" uint32 System_Text_DefaultEncoding_InternalCodePage() {
  return ILGetCodePage();
}

extern "C" uint32 System_Globalization_CultureInfo_InternalCultureID() {
  return ILGetCultureID();
}

extern "C" VMObject* System_Globalization_CultureInfo_InternalCultureName() {
  char* val = ILGetCultureName();
  N3* vm = (N3*)(VMThread::get()->vm);
  if (val) {
    VMObject* ret = vm->arrayToString(vm->asciizToArray(val));
    free(val);
    return ret;
  } else {
    VMObject* ret = vm->arrayToString(vm->asciizToArray("iv"));
    return ret;
  }
}

static const ArrayUInt16* newBuilder(N3* vm, PNetString* value, uint32 length) {
  uint32 valueLength = value ? value->length : 0;
  const ArrayUInt16* array = value ? value->value : 0;
  uint32 roundLength = (7 + length) & 0xfffffff8;
  uint16* buf = (uint16*)alloca(roundLength * sizeof(uint16));
  uint32 strLength = 0;

  if (value != 0) {
    if (valueLength <= roundLength) {
      memcpy(buf, array->elements, valueLength * sizeof(uint16));
      strLength = valueLength;
    } else {
      memcpy(buf, array->elements, roundLength * sizeof(uint16));
      strLength = roundLength;
    }
  }

  return vm->bufToArray(buf, strLength);
}

extern "C" VMObject* System_String_NewBuilder(PNetString* value, 
                                               uint32 length) {
  N3* vm = (N3*)(VMThread::get()->vm);
  PNetString* str = (PNetString*)vm->arrayToString(newBuilder(vm, value, length));
  return str;
}

extern "C" VMObject* Platform_SysCharInfo_GetNewLine() {
  N3* vm = (N3*)(VMThread::get()->vm);
  return vm->arrayToString(vm->asciizToArray("\n"));
}

extern "C" void System_String_CopyToChecked(PNetString* str, sint32 sstart, 
                                            ArrayUInt16* dest, sint32 dstart,
                                            sint32 count) {
  const ArrayUInt16* value = str->value;
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

extern "C" VMObject* System_Reflection_ClrType_GetElementType(VMObject* Klass) {
  VMCommonClass* cl = (VMCommonClass*)((*Klass)(MSCorlib::typeClrType).PointerVal);
  if (!cl->isArray) {
    VMThread::get()->vm->error("implement me");
    return 0;
  } else {
    return ((VMClassArray*)cl)->baseClass->getClassDelegatee();
  }
}

extern "C" PNetString* System_String_NewString(uint32 size) {
  PNetString* str = (PNetString*)(MSCorlib::pString->doNew());
  str->length = size;
  return str;
}

extern "C" void System_String_Copy_3(PNetString* dest, sint32 pos, 
                                     PNetString* src) {
  ArrayUInt16* arr = (ArrayUInt16*)MSCorlib::arrayChar->doNew(pos + src->value->size);
  
  for (sint32 i = 0; i < pos; ++i) {
    arr->setAt(i, dest->value->at(i));
  }

  for (sint32 i = 0; i < src->length; ++i) {
    arr->setAt(pos + i, src->value->at(i));
  }

  dest->value = arr;
  dest->length = dest->value->size;
}

extern "C" void System_String_Copy_5(PNetString* dest, sint32 destPos, 
                                     PNetString* src, sint32 srcPos, 
                                     sint32 length) {
	const ArrayUInt16 *arraySrc = src->value;

	if(destPos == 0 && srcPos == 0 && length == src->length) {
		dest->value = arraySrc;
		dest->length = length;
	} else {
		sint32 newLength = destPos + length;

		if(newLength <= dest->length) {
			memcpy((uint16*)dest->value->elements + destPos, (uint16*)src->value->elements + srcPos, length<<1);
		} else {
			uint16 *buf = (uint16*)alloca(newLength<<1);
			memcpy(buf,           (uint16*)dest->value->elements, destPos<<1);
			memcpy(buf + destPos, (uint16*)src->value->elements + srcPos, length<<1);
			dest->length = newLength;
			dest->value  = VMThread::get()->vm->bufToArray(buf, newLength);
		}
	}
}
//   const UTF8* utf8Src = src->value->extract(VMThread::get()->vm, srcPos, 
//                                             srcPos + length);
//   if (destPos == 0) {
//     dest->value = utf8Src;
//     dest->length = dest->value->size;
//   } else {
//     const UTF8* utf8Dest = dest->value->extract(VMThread::get()->vm, 0, 
//                                                 destPos);
//     sint32 len1 = utf8Dest->size;
//     sint32 len2 = utf8Src->size;
//     uint16* buf = (uint16*)alloca((len1 + len2) * sizeof(uint16));

//     memcpy(buf, utf8Dest->elements, len1 * sizeof(uint16));
//     memcpy(buf + len1, utf8Dest->elements, len2 * sizeof(uint16));

//     const UTF8* utf8 = VMThread::get()->vm->bufToUTF8(buf, 
//                                                                 len1 + len2);
//     dest->value = utf8;
//     dest->length = dest->value->size;
//   }

extern "C" void System_Threading_Monitor_Enter(VMObject* obj) {
  obj->aquire();
}

extern "C" void System_Threading_Monitor_Exit(VMObject* obj) {
  obj->unlock();
}


extern "C" bool System_String_Equals(PNetString* str1, PNetString* str2) {
  return str1->value == str2->value;
}

extern "C" sint32 Platform_SysCharInfo_GetUnicodeCategory(char c) {
  return ILGetUnicodeCategory(c);
}

extern "C" uint16 System_String_GetChar(PNetString* str, sint32 index) {
  return str->value->at(index);
}

extern "C" sint32 System_String_IndexOf(PNetString* str, uint16 value, 
                                        sint32 startIndex, sint32 count) {
  if (startIndex < 0) {
    VMThread::get()->vm->error("shoud throw arg range");
  }

  if ((count < 0) || (str->length - startIndex < count)) {
    VMThread::get()->vm->error("shoud throw arg range");
  }

  sint32 i = startIndex;
  const ArrayUInt16* array = str->value;
  while (i < startIndex + count) {
    if (array->at(i) == value) return i;
    else ++i;
  }

  return -1;
}

extern "C" sint32 System_String_GetHashCode(PNetString* str) {
  sint32 hash = 0;
  const ArrayUInt16* array = str->value;
  for (sint32 i = 0; i < array->size; ++i) {
    hash += ((hash << 5) + array->elements[i]);
  }
  return hash;
}

extern "C" VMObject* System_Text_StringBuilder_Insert_System_Text_StringBuilder_System_Int32_System_Char(
                                                      StringBuilder* obj,
                                                      sint32 index, 
                                                      uint16 value) {
  N3* vm = (N3*)(VMThread::get()->vm);
  PNetString* buildString = obj->buildString;
  const ArrayUInt16* array = buildString->value;
  sint32 strLength = buildString->length;
  sint32 length = (index + 1) > strLength ? index + 1 : strLength + 1;
  uint16* buf = (uint16*)alloca(length * sizeof(uint16));

  if (index != 0) {
    memcpy(buf, array->elements, index * sizeof(uint16));
  }

  if (strLength > index) {
    memcpy(&(buf[index + 1]), &(array->elements[index]), 
               (strLength - index) * sizeof(uint16));
  }

  buf[index] = value;
  PNetString* str = (PNetString*)vm->arrayToString(vm->bufToArray(buf, length));
  obj->buildString = str;
  
  return obj;
}

extern "C" VMObject* System_Text_StringBuilder_Insert_System_Text_StringBuilder_System_Int32_System_String(
                                                      StringBuilder* obj,
                                                      sint32 index, 
                                                      PNetString* str) {
  N3* vm = (N3*)(VMThread::get()->vm);
  PNetString* buildString = obj->buildString;
  const ArrayUInt16* strArray = str->value;
  const ArrayUInt16* buildArray = buildString->value;
  sint32 strLength = str->length;
  sint32 buildLength = buildString->length;
  sint32 length = strLength + buildLength;
  uint16* buf = (uint16*)alloca(length * sizeof(uint16));

  if (index != 0) {
    memcpy(buf, buildArray->elements, index * sizeof(uint16));
  }

  if (strLength != 0) {
    memcpy(&(buf[index]), strArray->elements, strLength * sizeof(uint16));
  }
    
  if (buildLength - index > 0) {
    memcpy(&(buf[strLength + index]), &(buildArray->elements[index]), 
               (buildLength - index) * sizeof(uint16));
  }

  PNetString* val = (PNetString*)vm->arrayToString(vm->bufToArray(buf, length));
  obj->buildString = val;

  return obj;
}

extern "C" VMObject* System_Text_StringBuilder_Append_System_Text_StringBuilder_System_Char(
                                                StringBuilder* obj,
                                                uint16 value) {
  N3* vm = (N3*)(VMThread::get()->vm);
  PNetString* buildString = obj->buildString;
  const ArrayUInt16* array = buildString->value;
  sint32 length = buildString->length;
  uint16* buf = (uint16*)alloca((length + 1) * sizeof(uint16));

  memcpy(buf, array->elements, length * sizeof(uint16));

  buf[length] = value;
  PNetString* val = (PNetString*)vm->arrayToString(vm->bufToArray(buf, length + 1));
  obj->buildString = val;
  return obj;
}


extern "C" VMObject* System_Text_StringBuilder_Append_System_Text_StringBuilder_System_String(
                                                StringBuilder* obj,
                                                PNetString* str) {
  N3* vm = (N3*)(VMThread::get()->vm);
  PNetString* buildString = obj->buildString;
  const ArrayUInt16* buildArray = buildString->value;
  const ArrayUInt16* strArray = str->value;
  sint32 buildLength = buildString->length;
  sint32 strLength = str->length;
  sint32 length = buildLength + strLength;
  uint16* buf = (uint16*)alloca(length * sizeof(uint16));

  memcpy(buf, buildArray->elements, buildLength * sizeof(uint16));
  memcpy(&(buf[buildLength]), strArray->elements, strLength * sizeof(uint16));

  PNetString* val = (PNetString*)vm->arrayToString(vm->bufToArray(buf, length));
  obj->buildString = val;
  return obj;
}

extern "C" sint32 System_String_FindInRange(PNetString* obj, sint32 srcFirst, 
                                            sint32 srcLast, sint32 step,
                                            PNetString* dest) {
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

extern "C" VMObject* System_Reflection_Assembly_LoadFromName(PNetString* str, sint32 & error, VMObject* parent) {
  N3* vm = (N3*)(VMThread::get()->vm);
  Assembly* ass = vm->loadAssembly(vm->arrayToUTF8(str->value), "dll");
  if (!ass) vm->error("unfound assembly %s\n", mvm::PrintBuffer(str->value).cString());
  error = 0;
  return ass->getAssemblyDelegatee();
}

extern "C" PNetString* System_String_Concat_2(PNetString* str1, PNetString* str2) {
  N3* vm = (N3*)(VMThread::get()->vm);
  const ArrayUInt16* a1 = str1->value;
  const ArrayUInt16* a2 = str2->value;
  sint32 len1 = str1->length;
  sint32 len2 = str2->length;
  uint16* buf = (uint16*)alloca((len1 + len2) * sizeof(uint16));

  memcpy(buf, a1->elements, len1 * sizeof(uint16));
  memcpy(&(buf[len1]), a2->elements, len2 * sizeof(uint16));
  
  PNetString* val = (PNetString*)vm->arrayToString(vm->bufToArray(buf, len1 + len2));
  
  return val;
}

extern "C" PNetString* System_String_Concat_3(PNetString* str1, PNetString* str2, PNetString* str3) {
  N3* vm = (N3*)(VMThread::get()->vm);
  const ArrayUInt16* a1 = str1->value;
  const ArrayUInt16* a2 = str2->value;
  const ArrayUInt16* a3 = str3->value;
  sint32 len1 = str1->length;
  sint32 len2 = str2->length;
  sint32 len3 = str3->length;
  uint16* buf = (uint16*)alloca((len1 + len2 + len3) * sizeof(uint16));

  memcpy(buf, a1->elements, len1 * sizeof(uint16));
  memcpy(&(buf[len1]), a2->elements, len2 * sizeof(uint16));
  memcpy(&(buf[len1 + len2]), a3->elements, len3 * sizeof(uint16));
  
  PNetString* val = (PNetString*)vm->arrayToString(vm->bufToArray(buf, len1 + len2 + len3));
  
  return val;
}

extern "C" void System_String_RemoveSpace(PNetString* str, sint32 index, sint32 length) {
  const ArrayUInt16* array = str->value;
  sint32 strLength = str->length;
  uint16* buf = (uint16*)alloca(strLength * sizeof(uint16));
  sint32 j = index;

  if (index != 0) {
    memcpy(buf, array->elements, index * sizeof(uint16));
  }
  
  // 32 is space
  for (sint32 i = 0; i < length; ++i) {
    uint16 cur = array->elements[index + i];
    if (cur != 32) {
      buf[j] = cur;
    } else {
      ++j;
    }
  }

  if (strLength > (index + length)) {
    memcpy(&(buf[j]), &(array->elements[index + length]), (strLength - (index + length)) * sizeof(uint16));
  }

  const ArrayUInt16* res = VMThread::get()->vm->bufToArray(buf, j);
  str->value = res;
  str->length = j;
}

extern "C" void System_String__ctor_3(PNetString* str, uint16 ch, sint32 count) {
  ArrayUInt16* array = (ArrayUInt16*)MSCorlib::arrayChar->doNew(count);
  for (sint32 i = 0; i < count; ++i) {
    array->elements[i] = ch;
  }

  str->value = array;
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

extern "C" VMObject* System_Reflection_Assembly_GetType(VMObject* obj, PNetString* str, bool onError, bool ignoreCase) {
  Assembly* ass = ASSEMBLY_VALUE(obj);
  const ArrayUInt16* array = str->value;
	mvm::PrintBuffer pb(array);
  char* asciiz = pb.cString();
  char* index = (char*)sys_memrchr(asciiz, '.', strlen(asciiz));
  N3* vm = ass->vm;
  
  index[0] = 0;
  ++index;
  VMCommonClass* cl = ass->loadTypeFromName(vm->asciizToUTF8(index), vm->asciizToUTF8(asciiz), true, true, true, onError);
  if (!cl) VMThread::get()->vm->error("implement me");
  return cl->getClassDelegatee();
}

static bool parameterMatch(std::vector<VMCommonClass*> params, ArrayObject* types, bool virt) {
  uint32 v = virt ? 1 : 0;
  if (types->size + v + 1 != params.size()) return false;
  for (sint32 i = 0; i < types->size; ++i) {
    VMCommonClass* cur = (VMCommonClass*)(*MSCorlib::typeClrType)(types->elements[i]).PointerVal;
    if (cur != params[i + 1 + v]) return false;
  }
  return true;
}

extern "C" VMObject* System_Reflection_ClrType_GetMemberImpl(VMObject* Type, PNetString* str, sint32 memberTypes, sint32 bindingFlags, VMObject* binder, 
                                                   sint32 callingConventions, ArrayObject* types, VMObject* modifiers) {
  VMCommonClass* type = (VMCommonClass*)((*MSCorlib::typeClrType)(Type).PointerVal);
  N3* vm = (N3*)(VMThread::get()->vm);
  const UTF8* name = vm->arrayToUTF8(str->value);
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
      const char* asciiz = utf8ToAsciiz(prop->name);
      char* buf = (char*)alloca(strlen(asciiz) + 5);
      sprintf(buf, "get_%s", asciiz);
      N3* vm = VMThread::get()->vm;
      VMMethod* meth = prop->type->lookupMethod(vm->asciizToUTF8(buf), prop->parameters, true, false);
      assert(meth);
      return meth->getMethodDelegatee();
    }
  } else {
    VMThread::get()->vm->error("implement me");
  }
  return 0;
}

static void decapsulePrimitive(VMObject* arg, const llvm::Type* type, std::vector<llvm::GenericValue>& args) {
  if (type == llvm::Type::getInt1Ty(llvm::getGlobalContext())) {
    llvm::GenericValue gv;
    gv.IntVal = llvm::APInt(1, (bool)((uint32*)arg)[VALUE_OFFSET]);
    args.push_back(gv);
  } else if (type == llvm::Type::getInt8Ty(llvm::getGlobalContext())) {
    llvm::GenericValue gv;
    gv.IntVal = llvm::APInt(8, (uint8)((uint32*)arg)[VALUE_OFFSET]);
    args.push_back(gv);
  } else if (type == llvm::Type::getInt16Ty(llvm::getGlobalContext())) {
    llvm::GenericValue gv;
    gv.IntVal = llvm::APInt(16, (uint16)((uint32*)arg)[VALUE_OFFSET]);
    args.push_back(gv);
  } else if (type == llvm::Type::getInt32Ty(llvm::getGlobalContext())) {
    llvm::GenericValue gv;
    gv.IntVal = llvm::APInt(32, (uint32)((uint32*)arg)[VALUE_OFFSET]);
    args.push_back(gv);
  } else if (type == llvm::Type::getInt64Ty(llvm::getGlobalContext())) {
    llvm::GenericValue gv;
    uint32* ptr = &((uint32*)arg)[VALUE_OFFSET];
    gv.IntVal = llvm::APInt(64, ((uint64*)ptr)[0]);
    args.push_back(gv);
  } else if (type == llvm::Type::getFloatTy(llvm::getGlobalContext())) {
    llvm::GenericValue gv;
    float* ptr = &((float*)arg)[VALUE_OFFSET];
    gv.FloatVal = ((float*)ptr)[0];
    args.push_back(gv);
  } else if (type == llvm::Type::getDoubleTy(llvm::getGlobalContext())) {
    llvm::GenericValue gv;
    uint32* ptr = &((uint32*)arg)[VALUE_OFFSET];
    gv.DoubleVal = ((double*)ptr)[0];
    args.push_back(gv);
  } else if (type == llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(llvm::getGlobalContext()))) {
    llvm::GenericValue gv(((void**)arg)[VALUE_OFFSET]);
    args.push_back(gv);
  } else {
    VMThread::get()->vm->error("implement me");
  }
}

extern "C" VMObject* System_Reflection_ClrMethod_Invoke(VMObject* Method, VMObject* obj, sint32 invokeAttr, VMObject* binder, ArrayObject* args, VMObject* culture) {
  VMMethod* meth = (VMMethod*)(*MSCorlib::methodMethodType)(Method).PointerVal;
  meth->getSignature(NULL);
  meth->compiledPtr(NULL);
  llvm::Function* func = CLIJit::compile(meth->classDef, meth);
  VMClass* type = meth->classDef;
  type->resolveStatic(true, NULL);
  uint32 virt = meth->virt;

  if ((obj != 0) && virt) {
    if (!(obj->classOf->isAssignableFrom(type))) {
      VMThread::get()->vm->illegalArgumentException(mvm::PrintBuffer(meth->name).cString());
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
    if (llvm::isa<llvm::PointerType>(type) && type != llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(llvm::getGlobalContext()))) {
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
  if (retType == MSCorlib::pVoid) {
    res = (*MSCorlib::pVoid)();
  } else if (retType == MSCorlib::pBoolean) {
    res = (*MSCorlib::pBoolean)();
    (*MSCorlib::ctorBoolean)(res, gv.IntVal.getBoolValue());
  } else if (retType == MSCorlib::pUInt8) {
    res = (*MSCorlib::pUInt8)();
    (*MSCorlib::ctorUInt8)(res, (uint8)gv.IntVal.getZExtValue());
  } else if (retType == MSCorlib::pSInt8) {
    res = (*MSCorlib::pSInt8)();
    (*MSCorlib::ctorSInt8)(res, (uint8)gv.IntVal.getSExtValue());
  } else if (retType == MSCorlib::pChar) {
    res = (*MSCorlib::pChar)();
    (*MSCorlib::ctorChar)(res, (uint16)gv.IntVal.getZExtValue());
  } else if (retType == MSCorlib::pSInt16) {
    res = (*MSCorlib::pSInt16)();
    (*MSCorlib::ctorSInt16)(res, (sint16)gv.IntVal.getSExtValue());
  } else if (retType == MSCorlib::pUInt16) {
    res = (*MSCorlib::pUInt16)();
    (*MSCorlib::ctorUInt16)(res, (uint16)gv.IntVal.getZExtValue());
  } else if (retType == MSCorlib::pSInt32) {
    res = (*MSCorlib::pSInt32)();
    (*MSCorlib::ctorSInt32)(res, (sint32)gv.IntVal.getSExtValue());
  } else if (retType == MSCorlib::pUInt32) {
    res = (*MSCorlib::pUInt32)();
    (*MSCorlib::ctorUInt32)(res, (sint32)gv.IntVal.getZExtValue());
  } else if (retType == MSCorlib::pSInt64) {
    res = (*MSCorlib::pSInt64)();
    (*MSCorlib::ctorSInt64)(res, (sint64)gv.IntVal.getSExtValue());
  } else if (retType == MSCorlib::pUInt64) {
    res = (*MSCorlib::pUInt64)();
    (*MSCorlib::ctorUInt64)(res, (sint64)gv.IntVal.getZExtValue());
  } else if (retType == MSCorlib::pIntPtr) {
    res = (*MSCorlib::pIntPtr)();
    (*MSCorlib::ctorIntPtr)(res, (void*)gv.IntVal.getSExtValue());
  } else if (retType == MSCorlib::pUIntPtr) {
    res = (*MSCorlib::pUIntPtr)();
    (*MSCorlib::ctorUIntPtr)(res, (void*)gv.IntVal.getZExtValue());
  } else if (retType == MSCorlib::pFloat) {
    res = (*MSCorlib::pFloat)();
    (*MSCorlib::ctorFloat)(res, gv.FloatVal);
  } else if (retType == MSCorlib::pDouble) {
    res = (*MSCorlib::pDouble)();
    (*MSCorlib::ctorDouble)(res, gv.DoubleVal);
  } else {
    if (retType->super == MSCorlib::pValue || retType->super == MSCorlib::pEnum)
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
  
  Reader* reader = ass->newReader(ass->bytes);
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

  VMObject* res = (*MSCorlib::resourceStreamType)();
  (*MSCorlib::ctorResourceStreamType)(res, ass, (uint64)start, (uint64)length);

  return res;
}
      
extern "C" VMObject* System_Reflection_Assembly_GetManifestResourceStream(VMObject* Ass, PNetString* str) {
  Assembly* ass = (Assembly*)(*MSCorlib::assemblyAssemblyReflection)(Ass).PointerVal;
  N3* vm = (N3*)(VMThread::get()->vm);
  const UTF8* id = vm->arrayToUTF8(str->value);
  Header* header = ass->CLIHeader;
  uint32 stringOffset = header->stringStream->realOffset;
  Table* manTable  = header->tables[CONSTANT_ManifestResource];
  uint32 manRows   = manTable->rowsNumber;
  sint32 pos = -1;
  uint32 i = 0;
  
  while ((pos == -1) && (i < manRows)) {
    uint32 nameOffset = manTable->readIndexInRow(i + 1, CONSTANT_MANIFEST_RESOURCE_NAME, ass->bytes);
    const UTF8* name = ass->readString(vm, stringOffset + nameOffset);

    if (name == id) {
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

extern "C" VMObject* System_Globalization_TextInfo_ToLower(VMObject* obj, PNetString* str) {
  verifyNull(str);
  const ArrayUInt16* array = str->value;
  uint32 length = str->length;

  uint16* buf = (uint16*)alloca(length * sizeof(uint16));

  N3* vm = VMThread::get()->vm;

  memcpy(buf, array->elements, length * sizeof(uint16));
  ILUnicodeStringToLower((void*)buf, (void*)array->elements, length);
  const ArrayUInt16* res = vm->bufToArray(buf, length);
  return ((N3*)vm)->arrayToString(res);
}

extern "C" VMObject* System_String_Replace(PNetString* str, uint16 c1, uint16 c2) {
  const ArrayUInt16* array = str->value;
  uint32 length = str->length;
  if ((c1 == c2) || length == 0) return str;

  uint16* buf = (uint16*)alloca(length * sizeof(uint16));
  memcpy(buf, array->elements, length * sizeof(uint16));
  for (uint32 i = 0; i < length; ++i) {
    if (buf[i] == c1) buf[i] = c2;
  }
  
  N3* vm = (N3*)VMThread::get()->vm;
  const ArrayUInt16* res = vm->bufToArray(buf, length);
  return vm->arrayToString(res);
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

extern "C" sint32 System_String_CompareInternal(PNetString* strA, sint32 indexA, sint32 lengthA, PNetString* strB, sint32 indexB, sint32 lengthB, bool ignoreCase) {
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

extern "C" void System_String_CharFill(PNetString* str, sint32 start, sint32 count, char ch) {
  const ArrayUInt16* array = str->value;
  sint32 length = start + count;
  uint16* buf = (uint16*)alloca(length * sizeof(uint16));

  memcpy(buf, array->elements, start * sizeof(uint16));
  for (sint32 i = 0; i < count; ++i) {
    buf[i + start] = ch;
  }
  
  N3* vm = VMThread::get()->vm;
  const ArrayUInt16* val = vm->bufToArray(buf, length);
  str->value = val;
  str->length = length;
}


extern "C" sint32 System_String_InternalOrdinal(PNetString *strA, sint32 indexA, sint32 lengthA,
						PNetString *strB, sint32 indexB, sint32 lengthB) {
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

extern "C" VMObject* System_Threading_Thread_InternalCurrentThread() {
  return VMThread::get()->vmThread;
}
