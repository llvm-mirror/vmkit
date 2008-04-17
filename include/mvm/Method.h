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
#include "mvm/Threads/Thread.h"

namespace llvm {
  class Function;
}

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
  const llvm::Function* llvmFunction;
  size_t  codeSize;

  /* use this constructor to map a function which is compiled by llvm  */
  inline Method(Code *c, size_t sz) {
    code(c);
    codeSize = sz;
  }

  virtual void print(PrintBuffer *buf) const;
  virtual void TRACER;
};


class Code : public Object {
public:
  inline Code() {}
  static VirtualTable* VT;
  Method* meth;

  inline Method *method(Method *m, size_t nbb) {
    return (meth = m);
  }

  inline Method *method(size_t nbb) {
    return meth;
  }
    
  inline Method *method(Method *m) { return meth = m; }
  inline Method *method() { return meth; }

  virtual void print(PrintBuffer *buf) const;
  virtual void TRACER;

  static Code* getCodeFromPointer(void* ptr) {
#ifdef MULTIPLE_GC
    return (mvm::Code*)mvm::Thread::get()->GC->begOf(ptr);
#else
    return (mvm::Code*)Collector::begOf(ptr);
#endif
  }
};

class ExceptionTable : public Object {
public:
  inline ExceptionTable() {}
  static VirtualTable* VT;
  void* framePtr;

  inline void *frameRegister(void *m, size_t nbb) {
    return (framePtr = m);
  }

  inline void *frameRegister(size_t nbb) {
    return framePtr;
  }
    
  inline void *frameRegister(void *m) { return (framePtr = m); }
  inline void *frameRegister() { return framePtr; }
  
  virtual void destroyer(size_t sz);
};

} // end namespace mvm

#endif // MVM_METHOD_H
