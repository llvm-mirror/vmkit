//===--------------- PrintBuffer.h - Printing objects ----------------------===//
//
//                     The Micro Virtual Machine
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef MVM_PRINTBUFFER_H
#define MVM_PRINTBUFFER_H

#include <cstdio> // sprintf
#include <cstring> // memcpy

#include "types.h"
#include "mvm/Object.h"

namespace mvm {


/// NativeString - This class is the equivalent of a char*, but allocated
/// by the GC, hence has a virtual table.
///
class NativeString : public gc {
public:
  
  /// VT - The virtual table of this class.
  ///
  static VirtualTable VT;

  /// cString - Returns the C equivalent of the NativeString.
  ///
  inline char *cString() { return (char *)(this + 1); }

  /// readString - Copies the C string to a newly allocated NativeString.
  inline static NativeString *readString(char *cStr) {
    size_t nbb = strlen(cStr);
    NativeString * res = alloc(nbb + 1);
    memcpy(res->cString(), cStr, nbb + 1);
    return res;
  }

  /// alloc - Allocates a NativeString of size len.
  ///
  static inline NativeString *alloc(size_t len) {
    return (NativeString *)gc::operator new(len + sizeof(VirtualTable*), &VT);
  }

  /// realloc - Reallocate a native string of size len.
  ///
  inline NativeString *realloc(size_t len) {
    return (NativeString *)gc::realloc(len + sizeof(VirtualTable*));
  }

  /// setAt - Sets the char c at position pos in the NativeString.
  ///
  inline void setAt(int pos, char c) {
    cString()[pos] = c;
  }

public:
  
  /// print - Just prints the NativeString.
  ///
  virtual void print(PrintBuffer *buf) const;

};

/// PrintBuffer - This class is a buffered string.
///
class PrintBuffer : public gc {
private:
 
  /// _contents - The buffer.
  ///
  NativeString* _contents;

  /// capacity - The capacity of the current buffer.
  ///
  uint32  capacity;

  /// writePosition - The position in the buffer where the next write will
  /// happen.
  ///
  uint32  writePosition;


public:
  
  /// VT - The virtual table of this class.
  ///
  static VirtualTable VT;
  
  
  /// contents - Returns the buffer.
  ///
  NativeString* contents() {
    return _contents;
  }

  /// setContents - Sets the buffer.
  ///
  void setContents(NativeString* n) {
    _contents = n;
  }

  /// write - Writes to this PrintBuffer.
  ///
  inline PrintBuffer *write(const char *string) {
    size_t len= strlen(string);
    if ((writePosition + len + 1) >= capacity) {
      while ((writePosition + len + 1) >= capacity)
        capacity*= 4;
      setContents(contents()->realloc(capacity));
    }
    strcpy(contents()->cString() + writePosition, string);
    writePosition+= len;
    return this;
  }
  

  /// writeChar - Writes a char.
  inline PrintBuffer *writeChar(char v) {
    char buf[32];
    sprintf(buf, "%c", v);
    return write(buf);
  }
  

  /// writeChar - Writes a int32.
  inline PrintBuffer *writeS4(int v) {
    char buf[32];
    sprintf(buf, "%d", v);
    return write(buf);
  }
  
  /// writes8 - Writes a int64.
  inline PrintBuffer *writeS8(sint64 v) {
    char buf[32];
    sprintf(buf, "%lld", (long long int)v);
    return write(buf);
  }
  
  /// writeFP - Writes a double.
  inline PrintBuffer *writeFP(double v) {
    char buf[32];
    sprintf(buf, "%f", v);
    return write(buf);
  }

  /// writePtr - Writes a pointer.
  inline PrintBuffer *writePtr(void *p) {
    char buf[32];
    sprintf(buf, "%p", p);
    return write(buf);
  }

  /// writeBytes - Writes len bytes in the buffer bytes.
  inline PrintBuffer *writeBytes(unsigned char *bytes, size_t len) {
    write("[");
    for (size_t idx= 0; idx < len; ++idx) {
      if (idx > 0)
        write(" ");
      char buf[32];
      sprintf(buf, "%d", bytes[idx]);
      write(buf);
    }
    write("]");
    return this;
  }

  /// writeObj - Writes an Object to the buffer.
  ///
  PrintBuffer *writeObj(const Object *);

public:
  
  /// alloc - Allocates a default PrintBuffer.
  ///
  static PrintBuffer *alloc(void) {
    PrintBuffer* pbf = (PrintBuffer*)gc::operator new(sizeof(PrintBuffer), &VT);
    pbf->capacity= 32;
    pbf->writePosition= 0;
    pbf->setContents(NativeString::alloc(pbf->capacity));
    return pbf;
  }

  /// tracer - Traces this PrintBuffer.
  ///
  static void staticTracer(PrintBuffer* obj) {
    obj->contents()->markAndTrace();
  }
};

} // end namespace mvm

#endif // MVM_PRINTBUFFER_H
