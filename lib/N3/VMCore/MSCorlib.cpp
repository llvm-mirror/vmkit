//===------------- MSCorlib.cpp - mscorlib interface ----------------------===//
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
#include "NativeUtil.h"
#include "MSCorlib.h"
#include "N3.h"
#include "Reader.h"
#include "VMArray.h"
#include "VMClass.h"
#include "VMObject.h"
#include "VMThread.h"

using namespace n3;

extern "C" void System_Runtime_CompilerServices_RuntimeHelpers_InitializeArray(
                                               VMArray* array, VMField* field) {
  if (!array || !field) return;

  VMClass* type = field->classDef;
  VMClassArray* ts = (VMClassArray*)array->classOf;
  VMCommonClass* bs = ts->baseClass;
  Assembly* ass = type->assembly;

  uint32 rva = ass->getRVAFromField(field->token);
  Section* inSection = 0;
  
  if (rva >= ass->rsrcSection->virtualAddress && rva < ass->rsrcSection->virtualAddress + ass->rsrcSection->virtualSize)
    inSection = ass->rsrcSection;
  if (rva >= ass->textSection->virtualAddress && rva < ass->textSection->virtualAddress + ass->textSection->virtualSize)
    inSection = ass->textSection;
  if (rva >= ass->relocSection->virtualAddress && rva < ass->relocSection->virtualAddress + ass->relocSection->virtualSize)
    inSection = ass->relocSection;

  uint32 size = array->size;
  uint32 offset = inSection->rawAddress + (rva - inSection->virtualAddress);
  ArrayUInt8* bytes = ass->bytes;

  if (bs == MSCorlib::pChar) {
    for (uint32 i = 0; i < size; ++i) {
      ((ArrayUInt16*)array)->elements[i] = READ_U2(bytes, offset);
    }
  } else if (bs == MSCorlib::pSInt32) {
    for (uint32 i = 0; i < size; ++i) {
      ((ArraySInt32*)array)->elements[i] = READ_U4(bytes, offset);
    }
  } else if (bs == MSCorlib::pDouble) {
    for (uint32 i = 0; i < size; ++i) {
      ((ArrayDouble*)array)->elements[i] = READ_U8(bytes, offset);
    }
  } else {
    VMThread::get()->vm->error("implement me");
  }
}

extern "C" VMObject* System_Type_GetTypeFromHandle(VMCommonClass* cl) {
  return cl->getClassDelegatee();
}


extern "C" VMObject* System_Reflection_Assembly_GetCallingAssembly() {
  // Call to this function.
  void** cur = (void**)__builtin_frame_address(0);
  
  // Stub from CLI to native.
  cur = (void**)cur[0];

  // The CLI function.
  cur = (void**)cur[0];
  
  // The caller of the CLI function;
  cur = (void**)cur[0];
 
  VirtualMachine* vm = VMThread::get()->vm;

  VMMethod* meth = vm->IPToMethod<VMMethod>(FRAME_IP(cur));

  assert(meth && "Wrong stack");
  
  return meth->classDef->assembly->getAssemblyDelegatee();
}

extern "C" VMObject* System_Reflection_Assembly_GetExecutingAssembly() {
  // Call to this function.
  void** cur = (void**)__builtin_frame_address(0);
  
  // Stub from CLI to native.
  cur = (void**)cur[0];

  // The CLI function.
  cur = (void**)cur[0];
  
  VirtualMachine* vm = VMThread::get()->vm;

  VMMethod* meth = vm->IPToMethod<VMMethod>(FRAME_IP(cur));

  assert(meth && "Wrong stack");
  
  return meth->classDef->assembly->getAssemblyDelegatee();
}

extern "C" void System_Reflection_Assembly_LoadFromFile() {
  VMThread::get()->vm->error("implement me");
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
  
  if (srcType->super == MSCorlib::pValue && srcType != dstType) {
    vm->arrayStoreException();
  } else if (srcType->super != MSCorlib::pValue && srcType->super != MSCorlib::pEnum) {
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

extern "C" void System_GC_Collect() {
#ifdef MULTIPLE_GC
  VMThread::get()->GC->collect();
#else
  Collector::collect();
#endif
}

extern "C" void System_GC_SuppressFinalize(VMObject* obj) {
  // TODO: implement me
}

