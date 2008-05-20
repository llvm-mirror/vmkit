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
