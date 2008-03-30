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

#include <stdio.h>
#include <string.h>

#include "types.h"
#include "mvm/Object.h"

namespace mvm {

class NativeString : public Object {
public:
  
  static VirtualTable* VT;

	inline char *cString() { return (char *)(this + 1); }

  inline static NativeString *readString(char *cStr) {
		size_t nbb = strlen(cStr);
		NativeString * res = alloc(nbb + 1);
		memcpy(res->cString(), cStr, nbb + 1);
		return res;
	}

	static inline NativeString *alloc(size_t len) {
		return (NativeString *)gc::operator new(len, VT);
	}

	inline NativeString *realloc(size_t len) {
		return (NativeString *)gc::realloc(len);
	}

  inline void setAt(int pos, char c) {
    cString()[pos] = c;
  }

public:
  virtual void print(PrintBuffer *buf) const;

  inline bool compare(char *str) {
    return !strcmp(cString(), str);
  }

  inline bool compare(char *str, int len) {
    return ((int)strlen(cString()) == len) && !strncmp(cString(), str, len);
  }
};


class PrintBuffer : public Object {
public:
  static VirtualTable* VT;
  size_t  capacity;
  size_t  writePosition;
  GC_defass(NativeString, contents);

public:

  static inline PrintBuffer* allocPrintBuffer(void) {
    PrintBuffer* pbf = gc_new(PrintBuffer)();
    pbf->capacity= 32;
    pbf->writePosition= 0;
		pbf->contents(NativeString::alloc(pbf->capacity));
    return pbf;
  }

  inline PrintBuffer *write(const char *string) {
    size_t len= strlen(string);
    if ((writePosition + len + 1) >= capacity) {
		  while ((writePosition + len + 1) >= capacity)
				capacity*= 4;
			contents(contents()->realloc(capacity));
    }
    strcpy(contents()->cString() + writePosition, string);
    writePosition+= len;
    return this;
  }
  
  inline PrintBuffer *writeChar(char v) {
    char buf[32];
    sprintf(buf, "%c", v);
    return write(buf);
  }
  

  inline PrintBuffer *writeS4(int v) {
    char buf[32];
    sprintf(buf, "%d", v);
    return write(buf);
  }
  
  inline PrintBuffer *writeS8(sint64 v) {
    char buf[32];
    sprintf(buf, "%lld", v);
    return write(buf);
  }
  
  inline PrintBuffer *writeFP(double v) {
    char buf[32];
    sprintf(buf, "%f", v);
    return write(buf);
  }

  inline PrintBuffer *writePtr(void *p) {
    char buf[32];
    sprintf(buf, "%p", p);
    return write(buf);
  }

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

	PrintBuffer *writeObj(const Object *);

	NativeString      *getContents();
  
  static PrintBuffer *write_static(PrintBuffer*, char *);

public:
  static PrintBuffer *alloc(void);

  virtual void tracer(size_t sz);
};

} // end namespace mvm

#endif // MVM_PRINTBUFFER_H
