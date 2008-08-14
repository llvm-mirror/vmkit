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

#include "mvm/PrintBuffer.h"

#include "types.h"

#include "JavaObject.h"

namespace jnjvm {

class ClassArray;
class CommonClass;
class JavaAllocator;
class JavaObject;
class Jnjvm;

/// TJavaArray - Template class to be instantiated by real arrays. All arrays
///  have a constant size and an array of element. When JnJVM allocates an
///  instantiation of this class, it allocates with the actual size of this
///  array. Hence instantiation of TJavaArrays have a layout of 
///  {JavaObject, size, [0 * T]}.
template <class T>
class TJavaArray : public JavaObject {
public:
  /// size - The (constant) size of the array.
  sint32 size;

  /// elements - Elements of this array. The size here is different than the
  /// actual size of the Java array. This is to facilitate Java array accesses
  /// in JnJVM code. The size should be set to zero, but this is invalid C99.
  T elements[1];
};

/// JavaArray - This class is just a placeholder for constants and for the
///  virtual table of arrays of primitive types (e.g. double, int).
class JavaArray : public TJavaArray<void*> {
public:

  /// VT - The virtual table of Java arrays of primitive types (e.g. int,
  /// double).
  static VirtualTable *VT;
  
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

  /// The Java class of Java arrays used in JnJVM.
  static ClassArray* ofByte;
  static ClassArray* ofChar;
  static ClassArray* ofString;
  static ClassArray* ofInt;
  static ClassArray* ofShort;
  static ClassArray* ofBool;
  static ClassArray* ofLong;
  static ClassArray* ofFloat;
  static ClassArray* ofDouble;
  static ClassArray* ofObject;
  
  /// tracer - The trace method of Java arrays of primitive types. Since their
  /// class lives throughout the lifetime of the application, there is no need
  /// to trace them. Therefore this trace function does nothing.
  virtual void TRACER;
  
};

/// Instantiation of the TJavaArray class for Java arrays of primitive types.
#define ARRAYCLASS(name, elmt)                                \
  class name : public TJavaArray<elmt> {                      \
  public:                                                     \
    static name* acons(sint32 n, ClassArray* cl, JavaAllocator* allocator);  \
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
  /// VT - The virtual table of arrays of objects.
  static VirtualTable *VT;

  /// acons - Allocates a Java array of objects. The class given as argument is
  /// the class of the array, not the class of its elements.
  static ArrayObject* acons(sint32 n, ClassArray* cl, JavaAllocator* allocator);

  /// tracer - The tracer method of Java arrays of objects. This method will
  /// trace all objects in the array.
  virtual void TRACER;
};


/// UTF8 - The UTF8 class is basically the ArrayUInt16 class (arrays of elements
/// of type uint16) with helper functions for manipulating UTF8. Each JVM
/// instance hashes UTF8. UTF8 are not allocated by the application's garbage
/// collector, but resides in permanent memory (e.g malloc).
class UTF8 : public ArrayUInt16 {
public:
  
  /// acons - Allocates an UTF8 in permanent memory. The class argument must be
  /// JavaArray::ofChar.
  static const UTF8* acons(sint32 n, ClassArray* cl, JavaAllocator* allocator);

  /// internalToJava - Creates a copy of the UTF8 at its given offset and size
  /// woth all its '.' replaced by '/'. The JVM bytecode reference classes in
  /// packages with the '.' as the separating character. The JVM language uses
  /// the '/' character.
  const UTF8* internalToJava(UTF8Map* map, unsigned int start,
                             unsigned int len) const;
  
  /// javaToInternal - Replaces all '/' into '.'.
  const UTF8* javaToInternal(UTF8Map* map, unsigned int start,
                             unsigned int len) const;
  
  /// UTF8ToAsciiz - Allocates a C string with the contents of this UTF8.
  char* UTF8ToAsciiz() const;

  /// extract - Creates an UTF8 by extracting the contents at the given size
  /// of this.
  const UTF8* extract(UTF8Map* map, uint32 start, uint32 len) const;

  /// equals - Returns whether two UTF8s are equals. When the JnJVM executes
  /// in single mode, equality is just a pointer comparison. When executing
  /// in multiple mode, we compare the contents f the UTF8s.
#ifndef MULTIPLE_VM
  bool equals(const UTF8* other) const {
    return this == other;
  }

  bool lessThan(const UTF8* other) const {
    return this < other;
  }
#else
  bool equals(const UTF8* other) const {
    if (size != other->size) return false;
    else return !memcmp(elements, other->elements, size * sizeof(uint16));
  }

  bool lessThan(const UTF8* other) const {
    if (size < other->size) return true;
    else if (size > other->size) return false;
    else return memcmp((const char*)elements, (const char*)other->elements, 
                       size * sizeof(uint16)) < 0;
  }
#endif
  
  /// print - Prints the UTF8 for debugging purposes.
  virtual void print(mvm::PrintBuffer* buf) const;

  /// operator new - Redefines the new operator of this class to allocate
  /// its objects in permanent memory, not with the garbage collector.
  void* operator new(size_t sz, sint32 size);

  /// operator delete - Redefines the delete operator to remove the object
  /// from permanent memory.
  void operator delete(void* obj);
};

} // end namespace jnjvm

#endif
