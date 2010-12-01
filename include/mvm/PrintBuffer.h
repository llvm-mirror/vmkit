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
#include "mvm/UTF8.h"

namespace mvm {

// #define VT_DESTRUCTOR_OFFSET 0
// #define VT_OPERATOR_DELETE_OFFSET 1
// #define VT_TRACER_OFFSET 2
// #define VT_PRINT_OFFSET 3
// #define VT_HASHCODE_OFFSET 4
// #define VT_NB_FUNCS 5
// #define VT_SIZE 5 * sizeof(void*)

class PrintBuffer;

/// OldObject - Root of all objects. Define default implementations of virtual
/// methods and some commodity functions.
/// ************           Technically this class is not used anywhere, to be removed?           ************
///
class OldObject : public gc {
public:
	static void     default_tracer(gc *o, uintptr_t closure) {}
	static intptr_t default_hashCode(const gc *o)            { return (intptr_t)o; }
	static void     default_print(const gc *o, PrintBuffer *buf);

  /// tracer - Default implementation of tracer. Does nothing.
  ///
  virtual void tracer(uintptr_t closure) { default_tracer(this, closure); }

  /// print - Default implementation of print.
  ///
  virtual void      print(PrintBuffer *buf) const { default_print(this, buf); }

  /// hashCode - Default implementation of hashCode. Returns the address
  /// of this object.
  ///
  virtual intptr_t  hashCode() const { return default_hashCode(this);}
  
};

/// PrintBuffer - This class is a buffered string.
///
class PrintBuffer {
public:
 
  /// contents - The buffer.
  ///
  char* contents;

  /// capacity - The capacity of the current buffer.
  ///
  uint32  capacity;

  /// writePosition - The position in the buffer where the next write will
  /// happen.
  ///
  uint32  writePosition;

	void init() {
		capacity = 256;
		contents = new char[capacity];
		writePosition = 0;
	}

public:
	PrintBuffer() {
		init();
	}

	PrintBuffer(const OldObject *obj) {
		llvm_gcroot(obj, 0);
		init();
		writeObj(obj);
	}

	PrintBuffer(const UTF8 *utf8) {
		init();
		writeUTF8(utf8);
	}

	virtual ~PrintBuffer() {
		delete[] contents;
	}
	
	char *cString() {
		return contents;
	}

	void ensureCapacity(uint32 len) {
		uint32 size = writePosition + len + 1;
    if (size >= capacity) {
      while (size >= capacity)
        capacity*= 4;
			char *newContents = new char[capacity];
			memcpy(newContents, contents, writePosition);
			delete[] contents;
			contents = newContents;
    }
	}

  /// write - Writes to this PrintBuffer.
  ///
  virtual PrintBuffer *write(const char *string) {
    uint32 len= strlen(string);
		ensureCapacity(len);
    strcpy(cString() + writePosition, string);
    writePosition+= len;
    return this;
  }

  /// writeChar - Writes a char.
  inline PrintBuffer *writeUTF8(const UTF8 *utf8) {
		uint32 len = utf8->size;
		ensureCapacity(len);
		for(uint32 i=0; i<len; i++) {
			//			printf("%d/%d: '%c (%d)'\n", i, len, utf8->elements[i], utf8->elements[i]);
			contents[writePosition + i] = utf8->elements[i];
		}
		contents[writePosition += len] = 0;
		return this;
  }

  /// writeChar - Writes a char.
  inline PrintBuffer *writeBool(bool v) {
		return write(v ? "true" : "false");
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

  /// writeObj - Writes a gc Object to the buffer.
  ///
  inline PrintBuffer *writeObj(const OldObject *obj) {
		llvm_gcroot(obj, 0);
		obj->print(this);
		return this;
	}


  /// replaceWith - replace the content of the buffer and free the old buffer
  ///
	void replaceWith(char *buffer) {
		delete[] this->contents;
		this->contents = buffer;
	}
};

} // end namespace mvm

#endif // MVM_PRINTBUFFER_H
