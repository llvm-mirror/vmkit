//===--------- Strings.cpp - Implementation of the Strings class  ---------===//
//
//                              The VMKit project
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "JavaArray.h"
#include "JavaObject.h"
#include "JavaString.h"
#include "JavaThread.h"

using namespace j3;

extern "C" void Java_org_j3_mmtk_Strings_write___3CI(JavaObject* str,
                                                     ArrayUInt16* msg,
                                                     sint32 len) {
  for (sint32 i = 0; i < len; ++i) {
    fprintf(stderr, "%c", msg->elements[i]);
  }
}

extern "C" void Java_org_j3_mmtk_Strings_writeThreadId___3CI(JavaObject* str,
                                                             ArrayUInt16* msg,
                                                             sint32 len) {
  fprintf(stderr, "[%p] ", (void*)JavaThread::get());
  
  for (sint32 i = 0; i < len; ++i) {
    fprintf(stderr, "%c", msg->elements[i]);
  }
}


extern "C" sint32
Java_org_j3_mmtk_Strings_copyStringToChars__Ljava_lang_String_2_3CII(
    JavaObject* obj, JavaString* str, ArrayUInt16* dst, uint32 dstBegin,
    uint32 dstEnd) {

  sint32 len = str->count;
  sint32 n = (dstBegin + len <= dstEnd) ? len : (dstEnd - dstBegin);

  for (sint32 i = 0; i < n; i++) {
    dst->elements[dstBegin + i] = str->value->elements[str->offset + i];
  }
  
  return n;
 
}

