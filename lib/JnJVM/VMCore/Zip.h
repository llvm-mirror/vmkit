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

#include <map>

#include "mvm/Allocator.h"

namespace jnjvm {

class ArrayUInt8;
class JnjvmBootstrapLoader;

struct ZipFile : public mvm::PermanentObject {
  char* filename;
  int ucsize;
  int csize;
  uint32 filenameLength;
  uint32 extraFieldLength;
  uint32 fileCommentLength;
  int rolh;
  int compressionMethod;
};



class ZipArchive : public mvm::PermanentObject {
  friend class JnjvmBootstrapLoader;
private:
  
  mvm::Allocator* allocator;

  struct ltstr
  {
    bool operator()(const char* s1, const char* s2) const
    {
      return strcmp(s1, s2) < 0;
    }
  };
  
  int ofscd;
  std::map<const char*, ZipFile*, ltstr> filetable;
  typedef std::map<const char*, ZipFile*, ltstr>::iterator table_iterator;
  ArrayUInt8* bytes;
  
  void findOfscd();
  void addFiles();
  
  void remove();

public:
  
  ~ZipArchive() {
    for (table_iterator I = filetable.begin(), E = filetable.end(); I != E; 
         ++I) {
      allocator->freePermanentMemory((void*)I->first);
      I->second->~ZipFile();
      allocator->freePermanentMemory((void*)I->second);
    }
  }

  int getOfscd() { return ofscd; }
  ZipArchive(ArrayUInt8* bytes, mvm::Allocator* allocator);
  ZipFile* getFile(const char* filename);
  int readFile(ArrayUInt8* array, const ZipFile* file);

};

} // end namespace jnjvm

#endif
