//===--------------- P3Reader.cpp - Open and read files ---------------------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <cstdio>
#include <cstring>

#include "types.h"

#include "P3Reader.h"

using namespace p3;

const int P3Reader::SeekSet = SEEK_SET;
const int P3Reader::SeekCur = SEEK_CUR;
const int P3Reader::SeekEnd = SEEK_END;

P3ByteCode* P3Reader::openFile(mvm::BumpPtrAllocator& allocator, const char* path) {
  P3ByteCode* res = NULL;
  FILE* fp = fopen(path, "r");
  if (fp != 0) {
    fseek(fp, 0, SeekEnd);
    size_t nbb = ftell(fp);
    fseek(fp, 0, SeekSet);
    res = new(allocator, nbb) P3ByteCode(nbb);
    if (fread(res->elements, nbb, 1, fp) == 0) {
      fprintf(stderr, "fread error\n");
      abort();  
    }
    fclose(fp);
  }
  return res;
}

void P3Reader::seek(uint32 pos, int from) {
  uint32 n = 0;
  uint32 start = min;
  uint32 end = max;
  
  if (from == SeekCur) n = cursor + pos;
  else if (from == SeekSet) n = start + pos;
  else if (from == SeekEnd) n = end + pos;
  

  assert(n >= start && n <= end && "out of range");

  cursor = n;
}


