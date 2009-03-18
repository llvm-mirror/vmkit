
#ifndef JAVA_COMPILER_H
#define JAVA_COMPILER_H

namespace llvm {
  class Function;
}

namespace jnjvm {

class Class;
class JavaMethod;
class Signdef;

class JavaCompiler {
public:
  
  virtual void* materializeFunction(JavaMethod* meth) = 0;
  virtual void setMethod(JavaMethod* meth, void* ptr, const char* name) = 0;
  virtual bool isStaticCompiling() = 0;
  virtual void resolveVirtualClass(Class* cl) = 0;
  virtual void resolveStaticClass(Class* cl) = 0;

  virtual JavaCompiler* Create(std::string) = 0;

  virtual void staticCallBuf(Signdef* sign) = 0;
  virtual void virtualCallBuf(Signdef* sign) = 0;
  virtual void staticCallAP(Signdef* sign) = 0;
  virtual void virtualCallAP(Signdef* sign) = 0;
  
  virtual ~JavaCompiler() {}

};

}

#endif
