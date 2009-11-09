//===------- JavaCompiler.h - Jnjvm interface for the compiler ------------===//
//
//                           The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef JAVA_COMPILER_H
#define JAVA_COMPILER_H

#include <cstdio>
#include <cstdlib>
#include <string>
#include <dlfcn.h>

#include "mvm/GC/GC.h"

namespace mvm {
  class UTF8;
}

namespace jnjvm {

class Class;
class JavaMethod;
class JavaVirtualTable;
class Signdef;

class JavaCompiler {
public:
  
  virtual JavaCompiler* Create(const std::string&) {
    return this;
  }
  
  virtual void* materializeFunction(JavaMethod* meth) {
    fprintf(stderr, "Materializing a function in an empty compiler");
    abort();
    return 0;
  }

  virtual void setMethod(JavaMethod* meth, void* ptr, const char* name) {
  }
  
  virtual bool isStaticCompiling() {
    return false;
  }

  virtual bool emitFunctionName() {
    return false;
  }

  virtual void resolveVirtualClass(Class* cl) {
    fprintf(stderr, "Resolving a class in an empty compiler");
    abort();
  }

  virtual void resolveStaticClass(Class* cl) {
    fprintf(stderr, "Resolving a class in an empty compiler");
    abort();
  }


  virtual void staticCallBuf(Signdef* sign) {
    fprintf(stderr, "Asking for a callback in an empty compiler");
    abort();
  }

  virtual void virtualCallBuf(Signdef* sign) {
    fprintf(stderr, "Asking for a callback in an empty compiler");
    abort();
  }

  virtual void staticCallAP(Signdef* sign) {
    fprintf(stderr, "Asking for a callback in an empty compiler");
    abort();
  }

  virtual void virtualCallAP(Signdef* sign) {
    fprintf(stderr, "Asking for a callback in an empty compiler");
    abort();
  }
  
  virtual ~JavaCompiler() {}

  virtual mvm::StackScanner* createStackScanner() {
    return new mvm::UnpreciseStackScanner();
  }

  virtual void* loadMethod(void* handle, const char* symbol) {
    return dlsym(handle, symbol);
  }

  static const mvm::UTF8* InlinePragma;
  static const mvm::UTF8* NoInlinePragma;
};

}

#endif
