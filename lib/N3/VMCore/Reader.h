//===----------------- Reader.h - Open and read files ---------------------===//
//
//                                N3
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef N3_READER_H
#define N3_READER_H

#include "mvm/Object.h"

#include "types.h"

namespace n3 {

class ByteCode : public mvm::PermanentObject {
public:
	sint32 size;
	uint8  *elements;

	ByteCode(mvm::BumpPtrAllocator &allocator, int nbb);
};

class Reader : public mvm::PermanentObject {
public:
  ByteCode* bytes;
  uint32 min;
  uint32 cursor;
  uint32 max;

  Reader(ByteCode* array, uint32 start = 0, uint32 end = 0);

  static double readDouble(int first, int second);
  static sint64 readLong(int first, int second);

  static const int SeekSet;
  static const int SeekCur;
  static const int SeekEnd;

  static ByteCode* openFile(mvm::BumpPtrAllocator &allocator, char* path);
  uint8 readU1();
  sint8 readS1();
  uint16 readU2();
  sint16 readS2();
  uint32 readU4();
  sint32 readS4();
  uint64 readU8();
  sint64 readS8();
  unsigned int tell();
	//  Reader* derive(uint32 nbb);
  void seek(uint32 pos, int from);

  virtual void print(mvm::PrintBuffer* buf) const;
};

static sint8 inline READ_S1(ByteCode* bytes, uint32& offset) {
  return (sint8)(bytes->elements[offset++]);
}

static uint8 inline READ_U1(ByteCode* bytes, uint32& offset) {
  return(uint8)(bytes->elements[offset++]);
}

static sint16 inline READ_S2(ByteCode* bytes, uint32& offset) {
  sint16 val = READ_S1(bytes, offset);
  return val | (READ_U1(bytes, offset) << 8);
}

static uint16 inline READ_U2(ByteCode* bytes, uint32& offset) {
  uint16 val = READ_U1(bytes, offset);
  return val | (READ_U1(bytes, offset) << 8);
}

static sint32 inline READ_S4(ByteCode* bytes, uint32& offset) {
  sint32 val = READ_U2(bytes, offset);
  return val | (READ_U2(bytes, offset) << 16);
}

static uint32 inline READ_U3(ByteCode* bytes, uint32& offset) {
  uint32 val = READ_U2(bytes, offset);
  return val | (READ_U1(bytes, offset) << 16);
}

static uint32 inline READ_U4(ByteCode* bytes, uint32& offset) {
  return READ_S4(bytes, offset);
}

static uint32 inline READ_U8(ByteCode* bytes, uint32& offset) {
  uint64 val1 = READ_U4(bytes, offset);
  uint64 val2 = READ_U4(bytes, offset);
  return (val2 << 32) + val1;
}


} // end namespace jnjvm

#endif
