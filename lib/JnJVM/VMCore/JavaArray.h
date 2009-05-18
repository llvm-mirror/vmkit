//===----------------- JavaArray.h - Java arrays --------------------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the internal representation of Java arrays.
//
//===----------------------------------------------------------------------===//

#ifndef JNJVM_JAVA_ARRAY_H
#define JNJVM_JAVA_ARRAY_H

#include "mvm/Allocator.h"

#include "types.h"

#include "JavaObject.h"

namespace jnjvm {

class ClassArray;
class CommonClass;
class JavaObject;
class Jnjvm;
class UTF8Map;

/// TJavaArray - Template class to be instantiated by real arrays. All arrays
///  have a constant size and an array of element. When JnJVM allocates an
///  instantiation of this class, it allocates with the actual size of this
///  array. Hence instantiation of TJavaArrays have a layout of 
///  {JavaObject, size, [0 * T]}.
template <class T>
class TJavaArray : public JavaObject {
public:
  /// size - The (constant) size of the array.
  ssize_t size;

  /// elements - Elements of this array. The size here is different than the
  /// actual size of the Java array. This is to facilitate Java array accesses
  /// in JnJVM code. The size should be set to zero, but this is invalid C99.
  T elements[1];
};

/// JavaArray - This class is just a placeholder for constants and for the
///  virtual table of arrays of primitive types (e.g. double, int).
class JavaArray : public TJavaArray<void*> {
public:

  /// MaxArraySize - The maximum size a Java array can have. Allocating an
  /// array with a bigger size than MaxArraySize raises an out of memory
  /// error.
  static const sint32 MaxArraySize;
  
  /// JVM representation of Java arrays of primitive types.
  static const unsigned int T_BOOLEAN;
  static const unsigned int T_CHAR;
  static const unsigned int T_FLOAT;
  static const unsigned int T_DOUBLE;
  static const unsigned int T_BYTE;
  static const unsigned int T_SHORT;
  static const unsigned int T_INT;
  static const unsigned int T_LONG;
  
  /// tracer - The trace method of Java arrays of primitive types. Since their
  /// class lives throughout the lifetime of the application, there is no need
  /// to trace them. Therefore this trace function does nothing.
  virtual void TRACER;
  
};

/// Instantiation of the TJavaArray class for Java arrays of primitive types.
#define ARRAYCLASS(name, elmt)                                \
  class name : public TJavaArray<elmt> {                      \
  }

ARRAYCLASS(ArrayUInt8,  uint8);
ARRAYCLASS(ArraySInt8,  sint8);
ARRAYCLASS(ArrayUInt16, uint16);
ARRAYCLASS(ArraySInt16, sint16);
ARRAYCLASS(ArrayUInt32, uint32);
ARRAYCLASS(ArraySInt32, sint32);
ARRAYCLASS(ArrayLong,   sint64);
ARRAYCLASS(ArrayFloat,  float);
ARRAYCLASS(ArrayDouble, double);

#undef ARRAYCLASS

/// ArrayObject - Instantiation of the TJavaArray class for arrays of objects.
/// Arrays of objects are different than arrays of primitive types because
/// they have to trace all objects in the array.
class ArrayObject : public TJavaArray<JavaObject*> {
public:
  /// tracer - The tracer method of Java arrays of objects. This method will
  /// trace all objects in the array.
  virtual void TRACER;
};


/// UTF8 - The UTF8 class is basically the ArrayUInt16 class (arrays of elements
/// of type uint16) with helper functions for manipulating UTF8. Each JVM
/// instance hashes UTF8. UTF8 are not allocated by the application's garbage
/// collector, but resides in permanent memory (e.g malloc).
class UTF8 {
  friend class UTF8Map;
private:
  
  /// operator new - Redefines the new operator of this class to allocate
  /// its objects in permanent memory, not with the garbage collector.
  void* operator new(size_t sz, mvm::BumpPtrAllocator& allocator,
                     sint32 size) {
    return allocator.Allocate(sizeof(ssize_t) + size * sizeof(uint16));
  }
  
  UTF8(sint32 n) {
    size = n;
  }

public:
  /// size - The (constant) size of the array.
  ssize_t size;

  /// elements - Elements of this array. The size here is different than the
  /// actual size of the Java array. This is to facilitate Java array accesses
  /// in JnJVM code. The size should be set to zero, but this is invalid C99.
  uint16 elements[1];
  
  /// extract - Similar, but creates it in the map.
  const UTF8* extract(UTF8Map* map, uint32 start, uint32 len) const;
 
  /// equals - Are the two UTF8s equal?
  bool equals(const UTF8* other) const {
    if (other == this) return true;
    else if (size != other->size) return false;
    else return !memcmp(elements, other->elements, size * sizeof(uint16));
  }
  
  /// equals - Does the UTF8 equal to the buffer? 
  bool equals(const uint16* buf, sint32 len) const {
    if (size != len) return false;
    else return !memcmp(elements, buf, size * sizeof(uint16));
  }

  /// lessThan - strcmp-like function for UTF8s, used by hash tables.
  bool lessThan(const UTF8* other) const {
    if (size < other->size) return true;
    else if (size > other->size) return false;
    else return memcmp((const char*)elements, (const char*)other->elements, 
                       size * sizeof(uint16)) < 0;
  }
  
};


/// UTF8Buffer - Helper class to create char* buffers suitable for
/// printf.
///
class UTF8Buffer {

  /// buffer - The buffer that holds a string representation.
  ///
  char* buffer;
public:

  /// UTF8Buffer - Create a buffer with the following UTF8.
  ///
  UTF8Buffer(const UTF8* val) {
    buffer = new char[val->size];
    for (sint32 i = 0; i < val->size; ++i)
      buffer[i] = val->elements[i];
  }

  /// ~UTF8Buffer - Delete the buffer, as well as all dynamically
  /// allocated memory.
  ///
  ~UTF8Buffer() {
    delete[] buffer;
  }

  /// toClassName - Change '/' into '.' in the buffer.
  ///
  UTF8Buffer* toClassName() {
    uint32 len = strlen(buffer);
    for (uint32 i = 0; i < len; ++i)
      if (buffer[i] == '/') buffer[i] = '.';
    return this;
  }

  /// cString - Return a C string representation of the buffer, suitable
  /// for printf.
  ///
  const char* cString() {
    return buffer;
  }

};

} // end namespace jnjvm

#endif
