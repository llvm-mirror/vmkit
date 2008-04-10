//===--------------- Reader.cpp - Open and read files ---------------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <stdio.h>
#include <string.h>

#include "types.h"

#include "Jnjvm.h"
#include "JavaArray.h"
#include "JavaThread.h"
#include "Reader.h"
#include "Zip.h"

using namespace jnjvm;

double Reader::readDouble(int first, int second) {
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

sint64 Reader::readLong(int first, int second) {
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

const int Reader::SeekSet = SEEK_SET;
const int Reader::SeekCur = SEEK_CUR;
const int Reader::SeekEnd = SEEK_END;

ArrayUInt8* Reader::openFile(Jnjvm* vm, char* path) {
  FILE* fp = fopen(path, "r");
  ArrayUInt8* res = 0;
  if (fp != 0) {
    fseek(fp, 0, SeekEnd);
    long nbb = ftell(fp);
    fseek(fp, 0, SeekSet);
    res = ArrayUInt8::acons(nbb, JavaArray::ofByte, vm);
    fread(res->elements, nbb, 1, fp);
    fclose(fp);
  }
  return res;
}

ArrayUInt8* Reader::openZip(Jnjvm* vm, char* zipname, char* filename) {
  ZipArchive* archive = ZipArchive::hashedArchive(vm, zipname);
  ArrayUInt8* ret = 0;
  if (archive != 0) {
    ZipFile* file = archive->getFile(filename);
    if (file != 0) {
      ArrayUInt8* res = ArrayUInt8::acons(file->ucsize, JavaArray::ofByte, vm);
      if (archive->readFile(res, file) != 0) {
        ret = res;
      }
    }
  }
  return ret;
}

uint8 Reader::readU1() {
  return bytes->at(cursor++);
}

sint8 Reader::readS1() {
  return readU1();
}

uint16 Reader::readU2() {
  uint16 tmp = ((uint16)(readU1())) << 8;
  return tmp | ((uint16)(readU1()));
}

sint16 Reader::readS2() {
  sint16 tmp = ((sint16)(readS1())) << 8;
  return tmp | ((sint16)(readS1()));
}

uint32 Reader::readU4() {
  uint32 tmp = ((uint32)(readU2())) << 16;
  return tmp | ((uint32)(readU2()));
}

sint32 Reader::readS4() {
  sint32 tmp = ((sint32)(readS2())) << 16;
  return tmp | ((sint32)(readS2()));
}

uint64 Reader::readU8() {
  uint64 tmp = ((uint64)(readU4())) << 32;
  return tmp | ((uint64)(readU4()));
}

sint64 Reader::readS8() {
  sint64 tmp = ((sint64)(readS8())) << 32;
  return tmp | ((sint64)(readS8()));
}

Reader::Reader(ArrayUInt8* array, uint32 start, uint32 end) {
  if (!end) end = array->size;
  this->bytes = array;
  this->cursor = start;
  this->min = start;
  this->max = start + end;
}

unsigned int Reader::tell() {
  return cursor - min;
}

Reader* Reader::derive(Jnjvm* vm, uint32 nbb) {
  return vm_new(vm, Reader)(bytes, cursor, nbb);
}

void Reader::seek(uint32 pos, int from) {
  uint32 n = 0;
  uint32 start = min;
  uint32 end = max;
  
  if (from == SeekCur) n = cursor + pos;
  else if (from == SeekSet) n = start + pos;
  else if (from == SeekEnd) n = end + pos;
  

  if ((n < start) || (n > end))
    JavaThread::get()->isolate->unknownError("out of range %d %d", n, end);

  cursor = n;
}

void Reader::print(mvm::PrintBuffer* buf) const {
  buf->write("Reader<>");
}
