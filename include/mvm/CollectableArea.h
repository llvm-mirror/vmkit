//===----------- CollectableArea.h - Collectable memory -------------------===//
//
//                     The Micro Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef MVM_COLLECTABLE_AREA_H
#define MVM_COLLECTABLE_AREA_H

#include "mvm/Object.h"
#include "mvm/PrintBuffer.h"

namespace mvm {

template <class T>
class CollectableArea : public Object {
public:
  
  static VirtualTable* VT;
  unsigned int firstIndex;
  unsigned int lastIndex;
  unsigned int capacity;

  inline T **elements() { return (T **)(this + 1); }

  inline T *setAt(T *o, unsigned int idx) { 
    return (T *)gcset(elements() + idx, o); 
  }
  
  inline T *getAt(unsigned int idx) { return elements()[idx]; }

  static inline CollectableArea *alloc(size_t cap, size_t st, size_t e) {
    CollectableArea *res = 
      (CollectableArea *)gc::operator new(sizeof(CollectableArea) + cap<<2, VT);
    res->firstIndex = st;
    res->lastIndex = e;
    res->capacity = cap;
    return res;
  }

  inline CollectableArea *realloc(size_t new_cap) {
    CollectableArea *res = 
      (CollectableArea *)gc::realloc(sizeof(CollectableArea) + new_cap<<2);
    res->capacity = new_cap;
    return res;
  }

  virtual void print(PrintBuffer *buf) {
     CollectableArea *const self= (CollectableArea *)this;
     buf->write("");
    for (size_t idx= self->firstIndex; idx < self->lastIndex; ++idx) {   
      if (idx > self->firstIndex)
        buf->write(" ");
      buf->writeObj(self->getAt(idx));
    }   
  }


  virtual void TRACER {
    CollectableArea *const self= (CollectableArea *)this;
    register T **e = self->elements();
    size_t lim= self->lastIndex;
    size_t idx= self->firstIndex;
    lim = ((lim << 2) < sz) ? lim : 0;
    idx = ((idx << 2) < sz) ? idx : (sz >> 2); 
    for (; idx < lim; ++idx)
      e[idx]->MARK_AND_TRACE;
  }
};

} // end namespace mvm
#endif // MVM_COLLECTABLE_AREA_H
