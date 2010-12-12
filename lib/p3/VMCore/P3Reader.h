#ifndef _P3_READER_H_
#define _P3_READER_H_

#include "mvm/Allocator.h"

namespace p3 {

class P3ByteCode {
 public:
  P3ByteCode(int l) {
    size = l;
  }

  void* operator new(size_t sz, mvm::BumpPtrAllocator& allocator, int n) {
    return allocator.Allocate(sizeof(uint32_t) + n * sizeof(uint8_t),
                              "Class bytes");
  }

  uint32_t size;
  uint8_t elements[1];
};

class P3Reader {
public:
  // bytes - Pointer to a reference array. The array is not manipulated directly
  // in order to support copying GC.
  P3ByteCode* bytes;
  uint32 min;
  uint32 cursor;
  uint32 max;

  P3Reader(P3ByteCode* array, uint32 start = 0, uint32 end = 0) {
    if (!end) end = array->size;
    this->bytes = array;
    this->cursor = start;
    this->min = start;
    this->max = start + end;
  }

  P3Reader(P3Reader& r, uint32 nbb) {
    bytes = r.bytes;
    cursor = r.cursor;
    min = r.min;
    max = min + nbb;
  }

  static const int SeekSet;
  static const int SeekCur;
  static const int SeekEnd;

  static P3ByteCode* openFile(mvm::BumpPtrAllocator& allocator, const char* path);
  
  uint8 readU1() {
    ++cursor;
    return bytes->elements[cursor - 1];
  }
  
  sint8 readS1() {
    ++cursor;
    return bytes->elements[cursor - 1];
  }
  
  uint16 readU2() {
    uint16 tmp = ((uint16)(readU1()));
    return tmp | ((uint16)(readU1())) << 8;
  }
  
  sint16 readS2() {
    sint16 tmp = ((sint16)(readS1()));
    return tmp | ((sint16)(readU1())) << 8;
  }
  
  uint32 readU4() {
    uint32 tmp = ((uint32)(readU2()));
    return tmp | ((uint32)(readU2())) << 16;
  }
  
  sint32 readS4() {
    sint32 tmp = ((sint32)(readS2()));
    return tmp | ((sint32)(readU2())) << 16;
  }

  uint64 readU8() {
    uint64 tmp = ((uint64)(readU4()));
    return tmp | ((uint64)(readU4())) << 32;
  }
  
  sint64 readS8() {
    sint64 tmp = ((sint64)(readS4()));
    return tmp | ((sint64)(readU4())) << 32;
  }

  static double readF8(int first, int second) {
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

//   static sint64 readS8(int first, int second) {
//     int values[2];
//     sint64 res[1];
// #if defined(__PPC__)
//     values[0] = second;
//     values[1] = first;
// #else
//     values[0] = first;
//     values[1] = second;
// #endif
//     memcpy(res, values, 8); 
//     return res[0];
//   }
  
  unsigned int tell() {
    return cursor - min;
  }
  
  void seek(uint32 pos, int from);
};

} // namespace p3

#endif
