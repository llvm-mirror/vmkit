//===--------------- J3Reader.cpp - Open and read files ---------------------===//
//
//                            The VMKit project
//
//===----------------------------------------------------------------------===//

#include <cstdio>
#include <cstring>
#include <assert.h>

#include "j3/j3reader.h"
#include "j3/j3zip.h"

using namespace j3;

const int J3Reader::SeekSet = SEEK_SET;
const int J3Reader::SeekCur = SEEK_CUR;
const int J3Reader::SeekEnd = SEEK_END;

J3ClassBytes* J3Reader::openFile(vmkit::BumpAllocator* a, const char* path) {
  J3ClassBytes* res = NULL;
  FILE* fp = fopen(path, "r");
  if (fp != 0) {
    fseek(fp, 0, SeekEnd);
    long nbb = ftell(fp);
    fseek(fp, 0, SeekSet);
    res = new (a, nbb) J3ClassBytes(nbb);
    if (fread(res->elements, nbb, 1, fp) == 0) {
      fprintf(stderr, "fread error\n");
      abort();  
    }
    fclose(fp);
  }
  return res;
}

J3ClassBytes* J3Reader::openZip(vmkit::BumpAllocator* allocator, J3ZipArchive* archive,
                            const char* filename) {
  J3ClassBytes* res = 0;
  J3ZipFile* file = archive->getFile(filename);
  if (file != 0) {
    res = new (allocator, file->ucsize) J3ClassBytes(file->ucsize);
    if (archive->readFile(res, file) != 0) {
      return res;
    }
  }
  return NULL;
}

void J3Reader::seek(uint32_t pos, int from) {
  uint32_t n = 0;
  uint32_t start = min;
  uint32_t end = max;
  
  if (from == SeekCur) n = cursor + pos;
  else if (from == SeekSet) n = start + pos;
  else if (from == SeekEnd) n = end + pos;
  

  assert(n >= start && n <= end && "out of range");

  cursor = n;
}

bool J3Reader::adjustSize(uint32_t length) {
	if(cursor + length >= max)
		return 0;

	max = cursor + length;

	return 1;
}
