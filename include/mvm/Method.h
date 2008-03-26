//===-------------- Method.h - Collectable methods ------------------------===//
//
//                     The Micro Virtual Machine
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef MVM_METHOD_H
#define MVM_METHOD_H

#include "mvm/Object.h"

namespace mvm {

class Code;
class ExceptionTable;

class Method : public Object {
public:
  inline Method() {}
  static VirtualTable* VT;
  GC_defass(Object, name);
  GC_defass(Code, code);
  GC_defass(Object, definition);
  GC_defass(Object, literals);
  GC_defass(ExceptionTable, exceptionTable);
  size_t  codeSize;

  /* use this constructor to map a function which is compiled by llvm  */
  inline Method(Code *c, size_t sz) {
    code(c);
    codeSize = sz;
  }

  virtual void print(PrintBuffer *buf) const;
  virtual void tracer(size_t sz);
};


class Code : public Object {
public:
  inline Code() {}
  static VirtualTable* VT;
  static void *allocate(size_t _nbb, void* metaInfo);

  inline Method *method(Method *m, size_t nbb) {
    return 
      (Method *)gcset((gc **)((uintptr_t)this + nbb), m);
  }

  inline Method *method(size_t nbb) {
    return ((Method **)((uintptr_t)this + nbb))[0];
  }
    
  inline Method *method(Method *m) { return method(m, objectSize()); }
  inline Method *method() { return method(objectSize()); }

  virtual void print(PrintBuffer *buf) const;
  virtual void tracer(size_t sz);
};

class ExceptionTable : public Object {
public:
  inline ExceptionTable() {}
  static VirtualTable* VT;

  inline void *frameRegister(void *m, size_t nbb) {
    return 
      (void *)gcset((gc **)((uintptr_t)this + nbb), 
                            (Object*)m);
  }

  inline void *frameRegister(size_t nbb) {
    return ((void **)((uintptr_t)this + nbb))[0];
  }
    
  inline void *frameRegister(void *m) { return frameRegister(m, objectSize()); }
  inline void *frameRegister() { return frameRegister(objectSize()); }
  
  virtual void destroyer(size_t sz);
};

} // end namespace mvm

#endif // MVM_METHOD_H
