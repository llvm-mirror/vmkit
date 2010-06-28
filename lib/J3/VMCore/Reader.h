//===----------------- Reader.h - Open and read files ---------------------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef JNJVM_READER_H
#define JNJVM_READER_H

#include "mvm/Object.h"

#include "types.h"

#include "JavaArray.h"
#include "JavaClass.h"

namespace j3 {

class JnjvmBootstrapLoader;
class ZipArchive;

class Reader {
public:
  // bytes - Pointer to a reference array. The array is not manipulated directly
  // in order to support copying GC.
  ArrayUInt8** bytes;
  uint32 min;
  uint32 cursor;
  uint32 max;

  Reader(Attribut* attr, ArrayUInt8** bytes) {
    this->bytes = bytes;
    this->cursor = attr->start;
    this->min = attr->start;
    this->max = attr->start + attr->nbb;
  }

  Reader(Reader& r, uint32 nbb) {
    bytes = r.bytes;
    cursor = r.cursor;
    min = r.min;
    max = min + nbb;
  }

  static double readDouble(int first, int second) {
    int values[2];
    double res[1];
#if defined(__PPC__)
    values[0] = second;
    values[1] = first;
#else
    values[0] = first;
    values[1] = second;
#endif
    memcpy(res, values, 8); 
    return res[0];
  }


  static sint64 readLong(int first, int second) {
    int values[2];
    sint64 res[1];
#if defined(__PPC__)
    values[0] = second;
    values[1] = first;
#else
    values[0] = first;
    values[1] = second;
#endif
    memcpy(res, values, 8); 
    return res[0];
  }

  static const int SeekSet;
  static const int SeekCur;
  static const int SeekEnd;

  static ArrayUInt8* openFile(JnjvmBootstrapLoader* loader, const char* path);
  static ArrayUInt8* openZip(JnjvmBootstrapLoader* loader, ZipArchive* archive,
                             const char* filename);
  
  uint8 readU1() {
    ++cursor;
    return ArrayUInt8::getElement(*bytes, cursor - 1);
  }
  
  sint8 readS1() {
    ++cursor;
    return ArrayUInt8::getElement(*bytes, cursor - 1);
  }
  
  uint16 readU2() {
    uint16 tmp = ((uint16)(readU1())) << 8;
    return tmp | ((uint16)(readU1()));
  }
  
  sint16 readS2() {
    sint16 tmp = ((sint16)(readS1())) << 8;
    return tmp | ((sint16)(readS1()));
  }
  
  uint32 readU4() {
    uint32 tmp = ((uint32)(readU2())) << 16;
    return tmp | ((uint32)(readU2()));
  }
  
  sint32 readS4() {
    sint32 tmp = ((sint32)(readS2())) << 16;
    return tmp | ((sint32)(readS2()));
  }

  uint64 readU8() {
    uint64 tmp = ((uint64)(readU4())) << 32;
    return tmp | ((uint64)(readU4()));
  }
  
  sint64 readS8() {
    sint64 tmp = ((sint64)(readS8())) << 32;
    return tmp | ((sint64)(readS8()));
  }

  Reader(ArrayUInt8** array, uint32 start = 0, uint32 end = 0) {
    if (!end) end = ArrayUInt8::getSize(*array);
    this->bytes = array;
    this->cursor = start;
    this->min = start;
    this->max = start + end;
  }

  
  unsigned int tell() {
    return cursor - min;
  }
  
  void seek(uint32 pos, int from);

};

} // end namespace j3

#endif
