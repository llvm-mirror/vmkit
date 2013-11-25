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

#include <stdint.h>

#include "vmkit/allocator.h"

namespace j3 {

class JnjvmBootstrapLoader;
class JnjvmClassLoader;
class J3ZipArchive;

class J3ClassBytes : vmkit::PermanentObject {
 public:
  uint32_t size;
  uint8_t  elements[1];

  J3ClassBytes(int l) {
    size = l;
  }

  void* operator new(size_t sz, vmkit::BumpAllocator* allocator, int n) {
    return vmkit::PermanentObject::operator new(sizeof(J3ClassBytes) + (n - 1) * sizeof(uint8_t), allocator);
  }
};

class J3Reader {
	friend class J3ZipArchive;

  J3ClassBytes* bytes;
  uint32_t      min;
  uint32_t      cursor;
  uint32_t      max;

public:
  J3Reader(J3ClassBytes* bytes, uint32_t s=0, uint32_t nbb=0) {
    if (!nbb) nbb = bytes->size;
    this->bytes = bytes;
    this->cursor = s;
    this->min = s;
    this->max = s + nbb;
  }

  J3Reader(J3Reader& r, uint32_t nbb) {
    bytes = r.bytes;
    cursor = r.cursor;
    min = r.min;
    max = min + nbb;
  }

  static const int SeekSet;
  static const int SeekCur;
  static const int SeekEnd;

  static J3ClassBytes* openFile(vmkit::BumpAllocator* allocator, const char* path);
  static J3ClassBytes* openZip(vmkit::BumpAllocator* allocator, J3ZipArchive* archive, const char* filename);

	uint32_t remaining() {
		return max - cursor;
	}

  unsigned int tell() {
    return cursor - min;
  }
  
  void seek(uint32_t pos, int from);

	bool adjustSize(uint32_t nbb);

	const uint8_t* pointer() {
		return bytes->elements + cursor; 
	}
  
  uint8_t readU1() {
    ++cursor;
    return bytes->elements[cursor - 1];
  }
  
  int8_t readS1() {
    ++cursor;
    return bytes->elements[cursor - 1];
  }
  
  uint16_t readU2() {
    uint16_t tmp = ((uint16_t)(readU1())) << 8;
    return tmp | ((uint16_t)(readU1()));
  }
  
  int16_t readS2() {
    int16_t tmp = ((int16_t)(readS1())) << 8;
    return tmp | ((int16_t)(readU1()));
  }
  
  uint32_t readU4() {
    uint32_t tmp = ((uint32_t)(readU2())) << 16;
    return tmp | ((uint32_t)(readU2()));
  }
  
  int32_t readS4() {
    int32_t tmp = ((int32_t)(readS2())) << 16;
    return tmp | ((int32_t)(readU2()));
  }

  uint64_t readU8() {
    uint64_t tmp = ((uint64_t)(readU4())) << 32;
    return tmp | ((uint64_t)(readU4()));
  }
  
  int64_t readS8() {
    int64_t tmp = ((int64_t)(readS4())) << 32;
    return tmp | ((int64_t)(readU4()));
  }
};

} // end namespace j3

#endif
