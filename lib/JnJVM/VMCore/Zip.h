//===----------------- Zip.h - Interface with zlib ------------------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef JNJVM_ZIP_H
#define JNJVM_ZIP_H

#include "mvm/Object.h"
#include "mvm/PrintBuffer.h"


namespace jnjvm {

class ArrayUInt8;
class Reader;
class ZipFileMap;


class ZipFile : public mvm::Object {
public:
  static VirtualTable* VT;
  char* filename;
  int ucsize;
  int csize;
  uint32 filenameLength;
  uint32 extraFieldLength;
  uint32 fileCommentLength;
  int rolh;
  int compressionMethod;


  virtual void print(mvm::PrintBuffer* buf) const {
    buf->write("ZipFile<");
    buf->write(filename);
    buf->write(">");
  }
  virtual void tracer(size_t sz);
};

class ZipArchive : public mvm::Object {
public:
  static VirtualTable* VT;
  int ofscd;
  ZipFileMap* filetable;
  char* name;
  Reader* reader;

  virtual void print(mvm::PrintBuffer* buf) const {
    buf->write("ZipArchive<");
    buf->write(name);
    buf->write(">");
  }
  virtual void tracer(size_t sz);

  static ZipArchive* hashedArchive(char* archname);
  static ZipArchive* singleArchive(char* archname);
  ZipFile* getFile(char* filename);
  int readFile(ArrayUInt8* array, const ZipFile* file);
  void remove();

private:
  void findOfscd();
  void addFiles();
};

} // end namespace jnjvm

#endif
