//===------- LockedMap.cpp - Implementation of the UTF8 map ---------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <map>

#include "JavaArray.h"
#include "JavaClass.h"
#include "JavaString.h"
#include "JavaTypes.h"
#include "LockedMap.h"
#include "Zip.h"

#include <cstring>

using namespace jnjvm;


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

void UTF8Map::replace(const UTF8* oldUTF8, const UTF8* newUTF8) {
  lock.lock();
  uint32 key = readerHasher(oldUTF8->elements, oldUTF8->size);
  std::pair<UTF8Map::iterator, UTF8Map::iterator> p = map.equal_range(key);
  
  for (UTF8Map::iterator i = p.first; i != p.second; i++) {
    if (i->second == oldUTF8) {
      map.erase(i);
      break;
    }
  }
  map.insert(std::make_pair(key, newUTF8));
  lock.unlock();

}

const UTF8* UTF8Map::lookupOrCreateAsciiz(const char* asciiz) {
  sint32 size = strlen(asciiz);
  uint32 key = asciizHasher(asciiz, size);
  const UTF8* res = 0;
  lock.lock();
  
  std::pair<UTF8Map::iterator, UTF8Map::iterator> p = map.equal_range(key);
  
  for (UTF8Map::iterator i = p.first; i != p.second; i++) {
    if (asciizEqual(i->second, asciiz, size)) {
      res = i->second;
      break;
    }
  }

  if (res == 0) {
    UTF8* tmp = (UTF8*)UTF8::acons(size, array, allocator);
    for (sint32 i = 0; i < size; i++) {
      tmp->elements[i] = asciiz[i];
    }
    res = (const UTF8*)tmp;
    map.insert(std::make_pair(key, res));
  }
  
  lock.unlock();
  return res;
}

const UTF8* UTF8Map::lookupOrCreateReader(const uint16* buf, uint32 len) {
  sint32 size = (sint32)len;
  uint32 key = readerHasher(buf, size);
  const UTF8* res = 0;
  lock.lock();
  
  std::pair<UTF8Map::iterator, UTF8Map::iterator> p = map.equal_range(key);

  for (UTF8Map::iterator i = p.first; i != p.second; i++) {
    if (readerEqual(i->second, buf, size)) {
      res = i->second;
      break;
    }
  }

  if (res == 0) {
    UTF8* tmp = (UTF8*)UTF8::acons(size, array, allocator);
    memcpy(tmp->elements, buf, len * sizeof(uint16));
    res = (const UTF8*)tmp;
    map.insert(std::make_pair(key, res));
  }
  
  lock.unlock();
  return res;
}

const UTF8* UTF8Map::lookupAsciiz(const char* asciiz) {
  sint32 size = strlen(asciiz);
  uint32 key = asciizHasher(asciiz, size);
  const UTF8* res = 0;
  lock.lock();
  
  std::pair<UTF8Map::iterator, UTF8Map::iterator> p = map.equal_range(key);
  
  for (UTF8Map::iterator i = p.first; i != p.second; i++) {
    if (asciizEqual(i->second, asciiz, size)) {
      res = i->second;
      break;
    }
  }

  lock.unlock();
  return res;
}

const UTF8* UTF8Map::lookupReader(const uint16* buf, uint32 len) {
  sint32 size = (sint32)len;
  uint32 key = readerHasher(buf, size);
  const UTF8* res = 0;
  lock.lock();
  
  std::pair<UTF8Map::iterator, UTF8Map::iterator> p = map.equal_range(key);

  for (UTF8Map::iterator i = p.first; i != p.second; i++) {
    if (readerEqual(i->second, buf, size)) {
      res = i->second;
      break;
    }
  }

  lock.unlock();
  return res;
}


void UTF8Map::insert(const UTF8* val) {
  map.insert(std::make_pair(readerHasher(val->elements, val->size), val));
}

void StringMap::insert(JavaString* str) {
  map.insert(std::make_pair(str->value, str));
}
