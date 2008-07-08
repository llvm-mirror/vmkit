//===-------------- Mono.cpp - The Mono interface -------------------------===//
//
//                              N3
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "mvm/JIT.h"

#include "Assembly.h"
#include "MonoString.h"
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
  Section* rsrcSection = ass->rsrcSection;

  uint32 size = array->size;
  uint32 offset = rsrcSection->rawAddress + (rva - rsrcSection->virtualAddress);
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

extern "C" int System_Environment_get_Platform (void) {
#if defined (PLATFORM_WIN32)
  /* Win32NT */
  return 2;
#else
  /* Unix */
  return 128; 
#endif
}
