#ifndef _UTF8_INTERNAL_H_
#define _UTF8_INTERNAL_H_

#include <map>
#include "mvm/Allocator.h"
#include "mvm/MvmDenseMap.h"

namespace mvm {

class PrintBuffer;
class UTF8Map;

class UTF8 {
  friend class UTF8Map;
private:
  
  /// operator new - Redefines the new operator of this class to allocate
  /// its objects in permanent memory, not with the garbage collector.
  void* operator new(size_t sz, mvm::BumpPtrAllocator& allocator, sint32 n) {
    return allocator.Allocate(sizeof(UTF8) + (n - 1) * sizeof(uint16), "UTF8");
  }
  
public:
  /// size - The (constant) size of the UTF8.
  ssize_t size;

  /// elements - Elements of this UTF8.
  /// The size should be set to zero, but this is invalid C99.
  uint16 elements[1];
  
  /// extract - Extracts an UTF8 from the current UTF8
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

	void print(PrintBuffer *pb) const;

	static uint32_t readerHasher(const uint16* buf, sint32 size);
	
	uint32_t hash() const {
		return readerHasher(elements, size);
	}
  
  UTF8(sint32 n) {
    size = n;
  }
};

extern "C" const UTF8 TombstoneKey;
extern "C" const UTF8 EmptyKey;

// Provide MvmDenseMapInfo for UTF8.
template<>
struct MvmDenseMapInfo<const UTF8*> {
  static inline const UTF8* getEmptyKey() {
    return &EmptyKey;
  }
  static inline const UTF8* getTombstoneKey() {
    return &TombstoneKey;
  }
  static unsigned getHashValue(const UTF8* PtrVal) {
    return PtrVal->hash();
  }
  static bool isEqual(const UTF8* LHS, const UTF8* RHS) { return LHS->equals(RHS); }
};


struct UTF8MapKey {
  uint32_t hash;
  const UTF8* data;
  UTF8MapKey(uint32_t h, const UTF8* d) : hash(h), data(d) {}
};

// Provide MvmDenseMapInfo for UTF8MapKey.
template<>
struct MvmDenseMapInfo<UTF8MapKey> {
  static inline const UTF8MapKey getEmptyKey() {
    static UTF8MapKey EmptyKey(0, NULL);
    return EmptyKey;
  }
  static inline const UTF8MapKey getTombstoneKey() {
    static UTF8MapKey TombstoneKey(-1, NULL);
    return TombstoneKey;
  }
  static unsigned getHashValue(const UTF8MapKey& key) {
    return key.data->hash();
  }
  static bool isEqual(const UTF8MapKey& LHS, const UTF8MapKey& RHS) { return LHS.data->equals(RHS.data); }
};

class UTF8Map : public mvm::PermanentObject {
private:
  typedef std::multimap<const uint32, const UTF8*>::iterator iterator;
  
  mvm::LockNormal lock;
  mvm::BumpPtrAllocator& allocator;
  std::multimap<const uint32, const UTF8*> map;
public:

  const UTF8* lookupOrCreateAsciiz(const char* asciiz); 
  const UTF8* lookupOrCreateReader(const uint16* buf, uint32 size);
  const UTF8* lookupAsciiz(const char* asciiz); 
  const UTF8* lookupReader(const uint16* buf, uint32 size);
  
  UTF8Map(mvm::BumpPtrAllocator& A) : allocator(A) {}

  ~UTF8Map() {
    for (iterator i = map.begin(), e = map.end(); i!= e; ++i) {
      allocator.Deallocate((void*)i->second);
    }
  }

  void insert(const UTF8* val);
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

	UTF8Builder *append(const UTF8 *utf8, uint32 start=0, ssize_t length=-1) {
		length = length == -1 ? utf8->size : length;
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

} // end namespace mvm

#endif
