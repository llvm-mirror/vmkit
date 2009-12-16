//===--- JavaString.h - Internal correspondance with Java Strings ---------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef JNJVM_JAVA_STRING_H
#define JNJVM_JAVA_STRING_H

#include "types.h"

#include "JavaObject.h"
#include "UTF8.h"

namespace j3 {

class ArrayUInt16;
class Jnjvm;

class JavaString : public JavaObject {
public:

  // CLASSPATH FIELDS!!
  const ArrayUInt16* value;
  sint32 count;
  sint32 cachedHashCode;
  sint32 offset;
  
  static JavaString* stringDup(const ArrayUInt16*& array, Jnjvm* vm);

  /// internalToJava - Creates a copy of the UTF8 at its given offset and size
  /// with all its '.' replaced by '/'. The JVM bytecode reference classes in
  /// packages with the '.' as the separating character. The JVM language uses
  /// the '/' character. Returns a Java String.
  ///
  static JavaString* internalToJava(const UTF8* utf8, Jnjvm* vm);

  static void stringDestructor(JavaString*);
  char* strToAsciiz();
  const ArrayUInt16* strToArray(Jnjvm* vm);

    /// javaToInternal - Replaces all '/' into '.'.
  const UTF8* javaToInternal(UTF8Map* map) const;

  static JavaVirtualTable* internStringVT;
};

} // end namespace j3

#endif
