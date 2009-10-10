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
#include "mvm/UTF8.h"

namespace mvm {

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

	template <class T>
	PrintBuffer(const T *obj) {
		init();
		writeObj(obj);
	}

	~PrintBuffer() {
		delete contents;
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
	template <class T>
  inline PrintBuffer *writeObj(const T *obj) {
		obj->print(this);
		return this;
	}
};

} // end namespace mvm

#endif // MVM_PRINTBUFFER_H
