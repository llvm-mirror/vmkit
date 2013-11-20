#ifndef _UTF8_INTERNAL_H_
#define _UTF8_INTERNAL_H_

#include <map>
#include <iostream>
#include <string>
#include "vmkit/Allocator.h"
#include "vmkit/VmkitDenseMap.h"
#include "vmkit/VmkitDenseSet.h"

namespace vmkit {

class UTF8Map;

template<class T1, class T2>
int compare_null_terminated_arrays(const T1* array1, size_t size1, const T2* array2, size_t size2)
{
	// NULL array is treated as empty
	if (!array1) size1 = 0;
	if (!array2) size2 = 0;

	// Compute real sizes, excluding null terminators
	for (; (size1 != 0) && (array1[size1 - 1] == 0); --size1);
	for (; (size2 != 0) && (array2[size2 - 1] == 0); --size2);

	int diff = size1 - size2;	// Compare sizes
	if (diff == 0) {	// Equal sizes, compare contents
		for (; (size1 != 0) && (diff == 0); --size1, ++array1, ++array2)
			diff = *array1 - *array2;
	}
	return diff;
}

class UTF8 {
  friend class UTF8Map;
private:
  
  /// operator new - Redefines the new operator of this class to allocate
  /// its objects in permanent memory, not with the garbage collector.
  void* operator new(size_t sz, vmkit::BumpPtrAllocator& allocator, sint32 n) {
    return allocator.Allocate(sizeof(UTF8) + (n - 1) * sizeof(uint16), "UTF8");
  }
  
public:
  /// size - The (constant) size of the UTF8.
  int32_t size;

  /// elements - Elements of this UTF8.
  /// The size should be set to zero, but this is invalid C99.
  uint16 elements[1];
  
  /// extract - Extracts an UTF8 from the current UTF8
  const UTF8* extract(UTF8Map* map, uint32 start, uint32 len) const;
 
  /// equals - Are the two UTF8s equal?
  bool equals(const UTF8* other) const {
    if (other == this) return true;
    return (*this) == (*other);
  }
  
  /// equals - Does the UTF8 equal to the buffer? 
  bool equals(const uint16* buf, sint32 len) const {
    if (size != len) return false;
    else return !memcmp(elements, buf, size * sizeof(uint16));
  }

  /// lessThan - strcmp-like function for UTF8s, used by hash tables.
  bool lessThan(const UTF8* other) const
    { return (*this) < (*other); }

	static uint32_t readerHasher(const uint16* buf, sint32 size);
	
	uint32_t hash() const {
		return readerHasher(elements, size);
	}
  
  UTF8(sint32 n) {
    size = n;
  }

  friend bool operator < (const UTF8& str1, const UTF8& str2) {
    return UTF8::compare(&str1, &str2) < 0;
  }
  friend bool operator == (const UTF8& str1, const UTF8& str2) {
    return UTF8::compare(&str1, &str2) == 0;
  }
  friend std::ostream& operator << (std::ostream&, const UTF8&);

  void dump() const __attribute__((noinline));
  int compare(const char *str, int length = -1) const {
    return compare_null_terminated_arrays(
      elements, size, str, (length == -1) ? strlen(str) : length);
  }
  int compare(const UTF8& str) const {return UTF8::compare(this, &str);}
  static int compare(const UTF8* str1, const UTF8* str2) {
    if (!str1 && !str2) return 0;
    return compare_null_terminated_arrays(
    	(!str1 ? NULL : str1->elements), (!str1 ? 0 : str1->size),
    	(!str2 ? NULL : str2->elements), (!str2 ? 0 : str2->size));
  }
  std::string& toString(std::string& buffer) const;
};

extern "C" const UTF8 TombstoneKey;
extern "C" const UTF8 EmptyKey;

struct UTF8MapKey {
  ssize_t length;
  const uint16_t* data;

  UTF8MapKey(const uint16_t* d, ssize_t l) {
    data = d;
    length = l;
  }
};

class UTF8_Comparator {
public:
	bool operator() (const UTF8* str1, const UTF8* str2) const {
		return (*str1) < (*str2);
	}
};

// Provide VmkitDenseMapInfo for UTF8.
template<>
struct VmkitDenseMapInfo<const UTF8*> {
  static inline const UTF8* getEmptyKey() {
    return &EmptyKey;
  }
  static inline const UTF8* getTombstoneKey() {
    return &TombstoneKey;
  }
  static unsigned getHashValue(const UTF8* PtrVal) {
    return PtrVal->hash();
  }
  static bool vmkIsEqual(const UTF8* LHS, const UTF8* RHS) { return LHS->equals(RHS); }
  static bool vmkIsEqualKey(const UTF8* LHS, const UTF8MapKey& Key) {
    return LHS->equals(Key.data, Key.length);
  }
  static UTF8MapKey toKey(const UTF8* utf8) {
    return UTF8MapKey(utf8->elements, utf8->size);
  }
};


// Provide VmkitDenseMapInfo for UTF8MapKey.
template<>
struct VmkitDenseMapInfo<UTF8MapKey> {
  static inline const UTF8MapKey getEmptyKey() {
    static UTF8MapKey EmptyKey(NULL, -1);
    return EmptyKey;
  }
  static inline const UTF8MapKey getTombstoneKey() {
    static UTF8MapKey TombstoneKey(NULL, -2);
    return TombstoneKey;
  }
  static unsigned getHashValue(const UTF8MapKey& key) {
    return UTF8::readerHasher(key.data, key.length);
  }
  static bool vmkIsEqual(const UTF8MapKey& LHS, const UTF8MapKey& RHS) {
    if (LHS.data == RHS.data) return true;
    if (LHS.length != RHS.length) return false;
    return !memcmp(LHS.data, RHS.data, RHS.length * sizeof(uint16));
  }
};

class UTF8Map : public vmkit::PermanentObject {
public:
  typedef VmkitDenseSet<UTF8MapKey, const UTF8*>::iterator iterator;
  
  LockNormal lock;
  BumpPtrAllocator& allocator;
  VmkitDenseSet<UTF8MapKey, const UTF8*> map;

  const UTF8* lookupOrCreateAsciiz(const char* asciiz); 
  const UTF8* lookupOrCreateReader(const uint16* buf, uint32 size);
  const UTF8* lookupAsciiz(const char* asciiz); 
  const UTF8* lookupReader(const uint16* buf, uint32 size);
  
  UTF8Map(BumpPtrAllocator& A) : allocator(A) {}
  UTF8Map(BumpPtrAllocator& A, VmkitDenseSet<UTF8MapKey, const UTF8*>* m)
      : allocator(A), map(*m) {}

  ~UTF8Map() {
    for (iterator i = map.begin(), e = map.end(); i!= e; ++i) {
      allocator.Deallocate((void*)*i);
    }
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

} // end namespace vmkit

#endif
