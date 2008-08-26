//===---- IsolateSharedLoader.h - Shared loader for isolates --------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef ISOLATE_SHARED_LOADER_H
#define ISOLATE_SHARED_LOADER_H

#include "JnjvmClassLoader.h"
#include "SharedMaps.h"

namespace jnjvm {

class JnjvmSharedLoader : public JnjvmClassLoader {
private:
  
  /// internalLoad - Load the class with the given name.
  ///
  virtual UserClass* internalLoad(const UTF8* utf8) {
    fprintf(stderr, "Don't use me");
    exit(1);
  }
  
  SharedClassByteMap* byteClasses;
  SharedClassNameMap* nameClasses;

public:
  
  /// VT - The virtual table of this class.
  ///
  static VirtualTable* VT;


  /// constructSharedClass - Create a shared representation of the class.
  /// If two classes have the same name but not the same array of bytes, 
  /// raise an exception.
  ///
  Class* constructSharedClass(const UTF8* name, ArrayUInt8* bytes);
  ClassArray* constructSharedClassArray(const UTF8* name);
  ClassPrimitive* constructSharedClassPrimitive(const UTF8* name);

  static JnjvmSharedLoader* createSharedLoader();

  static JnjvmSharedLoader* sharedLoader;
};

} // end namespace jnjvm

#endif //ISOLATE_SHARED_LOADER_H
