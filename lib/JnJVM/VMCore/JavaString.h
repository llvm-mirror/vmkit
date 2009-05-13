//===--- JavaString.h - Internal correspondance with Java Strings ---------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef JNJVM_JAVA_STRING_H
#define JNJVM_JAVA_STRING_H

#include "types.h"

#include "JavaObject.h"

namespace jnjvm {

class Jnjvm;

class JavaString : public JavaObject {
public:

  // CLASSPATH FIELDS!!
  const UTF8* value;
  sint32 count;
  sint32 cachedHashCode;
  sint32 offset;
  
  static JavaString* stringDup(const UTF8*& utf8, Jnjvm* vm);

  /// internalToJava - Creates a copy of the UTF8 at its given offset and size
  /// with all its '.' replaced by '/'. The JVM bytecode reference classes in
  /// packages with the '.' as the separating character. The JVM language uses
  /// the '/' character. Returns a Java String.
  ///
  static JavaString* internalToJava(const UTF8* utf8, Jnjvm* vm);

  static void stringDestructor(JavaString*);
  char* strToAsciiz();
  const UTF8* strToUTF8(Jnjvm* vm);

  static JavaVirtualTable* internStringVT;
};

} // end namespace jnjvm

#endif
