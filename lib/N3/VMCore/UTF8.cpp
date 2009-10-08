#include "UTF8.h"
#include "VMThread.h"
#include "VMClass.h"
#include "VMArray.h"
#include "N3.h"
#include "MSCorlib.h"

using namespace n3;

void UTF8::print(mvm::PrintBuffer* buf) const {
  for (int i = 0; i < size; i++)
    buf->writeChar((char)elements[i]);
}

const UTF8* UTF8::extract(UTF8Map *map, uint32 start, uint32 end) const {
  uint32 len = end - start;
  uint16* buf = (uint16*)alloca(sizeof(uint16) * len);

  for (uint32 i = 0; i < len; i++) {
    buf[i] = elements[i + start];
  }

  return map->lookupOrCreateReader(buf, len);
}

const UTF8* UTF8::asciizConstruct(N3* vm, const char* asciiz) {
  return vm->asciizToUTF8(asciiz);
}

const UTF8* UTF8::readerConstruct(N3* vm, uint16* buf, uint32 n) {
  return vm->bufToUTF8(buf, n);
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
      if (asciiz[i] != val->elements[i]) return false;
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
    UTF8* tmp = (UTF8 *)new(size) UTF8(size);
    for (sint32 i = 0; i < size; i++) {
      tmp->elements[i] = asciiz[i];
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
    UTF8* tmp = new(size) UTF8(size);
    memcpy(tmp->elements, buf, len * sizeof(uint16));
    res = (const UTF8*)tmp;
    map.insert(std::make_pair(key, res));
  }
  
  lock->unlock();
  return res;
}


