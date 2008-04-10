//===----------------- Reader.h - Open and read files ---------------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef JNJVM_READER_H
#define JNJVM_READER_H

#include "mvm/Object.h"

#include "types.h"

namespace jnjvm {

class ArrayUInt8;
class Jnjvm;

class Reader : public mvm::Object {
public:
  static VirtualTable* VT;
  ArrayUInt8* bytes;
  uint32 min;
  uint32 cursor;
  uint32 max;

  static double readDouble(int first, int second);
  static sint64 readLong(int first, int second);

  static const int SeekSet;
  static const int SeekCur;
  static const int SeekEnd;

  static ArrayUInt8* openFile(Jnjvm* vm, char* path);
  static ArrayUInt8* openZip(Jnjvm* vm, char* zipname, char* filename);
  uint8 readU1();
  sint8 readS1();
  uint16 readU2();
  sint16 readS2();
  uint32 readU4();
  sint32 readS4();
  uint64 readU8();
  sint64 readS8();
  Reader(ArrayUInt8* array, uint32 start = 0, uint32 end = 0);
  Reader() {}
  unsigned int tell();
  Reader* derive(Jnjvm* vm, uint32 nbb);
  void seek(uint32 pos, int from);

  virtual void print(mvm::PrintBuffer* buf) const;
  virtual void tracer(size_t sz);
};

} // end namespace jnjvm

#endif
