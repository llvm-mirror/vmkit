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
#include "JavaUpcalls.h"
#include "Reader.h"
#include "Zip.h"

using namespace jnjvm;

const int Reader::SeekSet = SEEK_SET;
const int Reader::SeekCur = SEEK_CUR;
const int Reader::SeekEnd = SEEK_END;

ArrayUInt8* Reader::openFile(JnjvmClassLoader* loader, char* path) {
  FILE* fp = fopen(path, "r");
  ArrayUInt8* res = 0;
  if (fp != 0) {
    fseek(fp, 0, SeekEnd);
    long nbb = ftell(fp);
    fseek(fp, 0, SeekSet);
    UserClassArray* array = loader->bootstrapLoader->upcalls->ArrayOfByte;
    res = ArrayUInt8::acons(nbb, array, loader->allocator);
    fread(res->elements, nbb, 1, fp);
    fclose(fp);
  }
  return res;
}

ArrayUInt8* Reader::openZip(JnjvmClassLoader* loader, ZipArchive* archive,
                            char* filename) {
  ArrayUInt8* ret = 0;
  ZipFile* file = archive->getFile(filename);
  if (file != 0) {
    UserClassArray* array = loader->bootstrapLoader->upcalls->ArrayOfByte;
    ArrayUInt8* res = ArrayUInt8::acons(file->ucsize, array, loader->allocator);
    if (archive->readFile(res, file) != 0) {
      ret = res;
    }
  }
  return ret;
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
