//===---------------- Key.h - Private thread keys -------------------------===//
//
//                     The Micro Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef MVM_KEY_H
#define MVM_KEY_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mvm/GC/GC.h"

namespace mvm {

class ThreadKey {
public:
  void * val;
  
  ThreadKey(void (*_destr)(void *));
  ThreadKey();
  void* get();
  void set(void*);
  void initialise();

};

template <class T>
class Key {
public:
  ThreadKey key;
  
  Key() {
    initialise();
  }

  void initialise() {
    key.initialise();
  }

  T*   get() { return (T*)key.get(); }
  void set(T *v) { key.set(v); }
  
};


} // end namespace mvm

#endif // MVM_KEY_H
