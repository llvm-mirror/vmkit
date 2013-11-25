//===----------------- Zip.h - Interface with zlib ------------------------===//
//
//                          The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef JNJVM_ZIP_H
#define JNJVM_ZIP_H

#include <map>

#include "vmkit/allocator.h"
#include "vmkit/util.h"

namespace j3 {

class J3ClassBytes;

class J3ZipFile : public vmkit::PermanentObject {
public:
  char*    filename;
  int      ucsize;
  int      csize;
  uint32_t filenameLength;
  uint32_t extraFieldLength;
  uint32_t fileCommentLength;
  int      rolh;
  int      compressionMethod;
};


class J3ZipArchive : public vmkit::PermanentObject {
  vmkit::BumpAllocator* allocator;

	typedef std::map<const char*, J3ZipFile*, vmkit::Util::char_less_t, vmkit::StdAllocator<std::pair<const char*, J3ZipFile*> > > FileMap;

  int           ofscd;
  FileMap       filetable;
  J3ClassBytes* bytes;
  
  void findOfscd();
  void addFiles();
  
  void remove();

public:
  J3ZipArchive(J3ClassBytes* bytes, vmkit::BumpAllocator* allocator);

  int getOfscd() { return ofscd; }
  J3ZipFile* getFile(const char* filename);
  int readFile(J3ClassBytes* array, const J3ZipFile* file);

  typedef FileMap::iterator table_iterator;
};

} // end namespace j3

#endif
