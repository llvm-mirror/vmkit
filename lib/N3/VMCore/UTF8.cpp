#include "UTF8.h"
#include "VMThread.h"
#include "VMClass.h"
#include "VMArray.h"
#include "N3.h"
#include "MSCorlib.h"

using namespace n3;

#define AT(name, elmt)                                                      \
  elmt name::at(sint32 offset) const {                                      \
    if (offset >= size)                                                     \
      VMThread::get()->vm->indexOutOfBounds(this, offset);                  \
    return elements[offset];                                                \
  }                                                                         \
  void name::setAt(sint32 offset, elmt value) {                             \
    if (offset >= size)                                                     \
      VMThread::get()->vm->indexOutOfBounds(this, offset);                  \
    elements[offset] = value;                                               \
  }

#define INITIALISE(name)                                                    \
  void name::initialise(VMCommonClass* atype, sint32 n) {                   \
    VMObject::initialise(atype);                                            \
    this->size = n;                                                         \
    for (int i = 0; i < n; i++)                                             \
      elements[i] = 0;                                                      \
  }                                                                         \

AT(UTF8, uint16)
INITIALISE(UTF8)

#undef AT
#undef INITIALISE


UTF8* UTF8::acons(sint32 n, VMClassArray* atype) {
  if (n < 0)
    VMThread::get()->vm->negativeArraySizeException(n);
  else if (n > VMArray::MaxArraySize)
    VMThread::get()->vm->outOfMemoryError(n);
  uint32 size = sizeof(VMObject) + sizeof(sint32) + n * sizeof(uint16);
  UTF8* res = (UTF8*)gc::operator new(size, UTF8::VT);
  res->initialise(atype, n);
  return res;
}

void UTF8::print(mvm::PrintBuffer* buf) const {
  for (int i = 0; i < size; i++)
    buf->writeChar((char)elements[i]);
}

const UTF8* UTF8::extract(N3 *vm, uint32 start, uint32 end) const {
  uint32 len = end - start;
  uint16* buf = (uint16*)alloca(sizeof(uint16) * len);

  for (uint32 i = 0; i < len; i++) {
    buf[i] = at(i + start);
  }

  return readerConstruct(vm, buf, len);
}

const UTF8* UTF8::asciizConstruct(N3* vm, const char* asciiz) {
  return vm->asciizToUTF8(asciiz);
}

const UTF8* UTF8::readerConstruct(N3* vm, uint16* buf, uint32 n) {
  return vm->bufToUTF8(buf, n);
}

char* UTF8::UTF8ToAsciiz() const {
  mvm::NativeString* buf = mvm::NativeString::alloc(size + 1);
  for (sint32 i = 0; i < size; ++i) {
    buf->setAt(i, elements[i]);
  }
  buf->setAt(size, 0);
  return buf->cString();
}



static uint32 asciizHasher(const char* asciiz, sint32 size) {
  uint32 r0 = 0, r1 = 0;
  for (sint32 i = 0; i < size; i++) {
    char c = asciiz[i];
    r0 += c;
    r1 ^= c;
  }
  return (r1 & 255) + ((r0 & 255) << 8);
}

static uint32 readerHasher(const uint16* buf, sint32 size) {
  uint32 r0 = 0, r1 = 0;
  for (sint32 i = 0; i < size; i++) {
    uint16 c = buf[i];
    r0 += c;
    r1 ^= c;
  }
  return (r1 & 255) + ((r0 & 255) << 8);
}

static bool asciizEqual(const UTF8* val, const char* asciiz, sint32 size) {
  sint32 len = val->size;
  if (len != size) return false;
  else {
    for (sint32 i = 0; i < len; i++) {
      if (asciiz[i] != val->at(i)) return false;
    }
    return true;
  }
}

static bool readerEqual(const UTF8* val, const uint16* buf, sint32 size) {
  sint32 len = val->size;
  if (len != size) return false;
  else return !(memcmp(val->elements, buf, len * sizeof(uint16)));
}


const UTF8* UTF8Map::lookupOrCreateAsciiz(const char* asciiz) {
  sint32 size = strlen(asciiz);
  uint32 key = asciizHasher(asciiz, size);
  const UTF8* res = 0;
  lock->lock();
  
  std::pair<UTF8Map::iterator, UTF8Map::iterator> p = map.equal_range(key);
  
  for (UTF8Map::iterator i = p.first; i != p.second; i++) {
    if (asciizEqual(i->second, asciiz, size)) {
      res = i->second;
      break;
    }
  }

  if (res == 0) {
    UTF8* tmp = (UTF8 *)UTF8::acons(size, MSCorlib::arrayChar);
    for (sint32 i = 0; i < size; i++) {
      tmp->setAt(i, asciiz[i]);
    }
    res = (const UTF8*)tmp;
    map.insert(std::make_pair(key, res));
  }
  
  lock->unlock();
  return res;
}

const UTF8* UTF8Map::lookupOrCreateReader(const uint16* buf, uint32 len) {
  sint32 size = (sint32)len;
  uint32 key = readerHasher(buf, size);
  const UTF8* res = 0;
  lock->lock();
  
  std::pair<UTF8Map::iterator, UTF8Map::iterator> p = map.equal_range(key);

  for (UTF8Map::iterator i = p.first; i != p.second; i++) {
    if (readerEqual(i->second, buf, size)) {
      res = i->second;
      break;
    }
  }

  if (res == 0) {
    UTF8* tmp = UTF8::acons(size, MSCorlib::arrayChar);
    memcpy(tmp->elements, buf, len * sizeof(uint16));
    res = (const UTF8*)tmp;
    map.insert(std::make_pair(key, res));
  }
  
  lock->unlock();
  return res;
}


