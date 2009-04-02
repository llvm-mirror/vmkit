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

namespace jnjvm {

class Class;
class JavaMethod;
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

};

}

#endif
