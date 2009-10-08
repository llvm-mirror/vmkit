#ifndef _N3_UTF8_
#define _N3_UTF8_

#include "VMObject.h"
#include "mvm/PrintBuffer.h"

namespace mvm {
	class VirtualTable;
}

namespace n3 {
	class VMClassArray;
	class UTF8Map;
	class N3;

class UTF8 : public VMObject {
public:
  static VirtualTable* VT;
  sint32 size;
  uint16 elements[1];
  
  /// operator new - Redefines the new operator of this class to allocate
  /// its objects in permanent memory, not with the garbage collector.
  void* operator new(size_t sz, sint32 n) {
    return gc::operator new(sizeof(VMObject) + sizeof(sint32) + n * sizeof(uint16), UTF8::VT);
  }
  
  UTF8(sint32 n) {
    size = n;
  }

  virtual void print(mvm::PrintBuffer* buf) const;

  static const UTF8* asciizConstruct(N3 *vm, const char* asciiz);
  static const UTF8* readerConstruct(N3 *vm, uint16* buf, uint32 n);

  const UTF8* extract(UTF8Map *vm, uint32 start, uint32 len) const;
};

class UTF8Map : public mvm::PermanentObject {
public:
  typedef std::multimap<const uint32, const UTF8*>::iterator iterator;
  
  mvm::Lock* lock;
  std::multimap<uint32, const UTF8*, std::less<uint32>,
                gc_allocator<std::pair<const uint32, const UTF8*> > > map;

  const UTF8* lookupOrCreateAsciiz(const char* asciiz); 
  const UTF8* lookupOrCreateReader(const uint16* buf, uint32 size);
  
  UTF8Map() {
    lock = new mvm::LockNormal();
  }
  
  virtual void TRACER {
    //lock->MARK_AND_TRACE;
    for (iterator i = map.begin(), e = map.end(); i!= e; ++i) {
      i->second->MARK_AND_TRACE;
    }
  }

  virtual void print(mvm::PrintBuffer* buf) const {
    buf->write("UTF8 Hashtable<>");
  }
};


class UTF8Builder {
	uint16 *buf;
	uint32  cur;
	uint32  size;

public:
	UTF8Builder(size_t size) {
		size = (size < 4) ? 4 : size;
		this->buf = new uint16[size];
		this->size = size;
	}

	UTF8Builder *append(const UTF8 *utf8, uint32 start=0, uint32 length=0xffffffff) {
		length = length == 0xffffffff ? utf8->size : length;
		uint32 req = cur + length;

		if(req > size) {
			uint32 newSize = size<<1;
			while(req < newSize)
				newSize <<= 1;
			uint16 *newBuf = new uint16[newSize];
			memcpy(newBuf, buf, cur<<1);
			delete []buf;
			buf = newBuf;
			size = newSize;
		}

		memcpy(buf + cur, &utf8->elements + start, length<<1);
		cur = req;

		return this;
	}

	const UTF8 *toUTF8(UTF8Map *map) {
		return map->lookupOrCreateReader(buf, size);
	}

	~UTF8Builder() {
		delete [] buf;
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
    buffer = new char[val->size + 1];
    for (sint32 i = 0; i < val->size; ++i)
      buffer[i] = val->elements[i];
    buffer[val->size] = 0;
		printf("buffer: %s (%d)\n", buffer, val->size);
  }

  /// ~UTF8Buffer - Delete the buffer, as well as all dynamically
  /// allocated memory.
  ///
  ~UTF8Buffer() {
		printf("Destructor :(\n");
    delete[] buffer;
  }

  /// replaceWith - replace the content of the buffer and free the old buffer
  ///
	void replaceWith(char *buffer) {
		delete[] this->buffer;
		this->buffer = buffer;
	}


  /// cString - Return a C string representation of the buffer, suitable
  /// for printf.
  ///
  const char* cString() {
		printf("cString: %s\n", buffer);
    return buffer;
  }
};

inline char *_utf8ToAsciiz(const UTF8 *val, char *buffer) {
	for (sint32 i = 0; i < val->size; ++i)
		buffer[i] = val->elements[i];
	buffer[val->size] = 0;
	return buffer;
}

#define utf8ToAsciiz(utf8) _utf8ToAsciiz(utf8, (char*)alloca(utf8->size + 1))

}

#endif
