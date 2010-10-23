//===---------- LLVMMaterializer.h - LLVM Materializer for J3 -------------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef J3_LLVM_MATERIALIZER_H
#define J3_LLVM_MATERIALIZER_H

#include <llvm/GVMaterializer.h>

#include <j3/JavaJITCompiler.h>

namespace j3 {

class LLVMMaterializer;

struct CallbackInfo {
  Class* cl;
  uint16 index;
  bool stat;

  CallbackInfo(Class* c, uint32 i, bool s) :
    cl(c), index(i), stat(s) {}

};

class JavaLLVMLazyJITCompiler : public JavaJITCompiler {
private:
  std::map<llvm::Function*, CallbackInfo> callbacks;
  typedef std::map<llvm::Function*, CallbackInfo>::iterator callback_iterator;

public:
  llvm::GVMaterializer* TheMaterializer;
  
  virtual llvm::Value* addCallback(Class* cl, uint16 index, Signdef* sign,
                                   bool stat, llvm::BasicBlock* insert);
  virtual uintptr_t getPointerOrStub(JavaMethod& meth, int side);
  
  virtual JavaCompiler* Create(const std::string& ModuleID) {
    return new JavaLLVMLazyJITCompiler(ModuleID);
  }

  JavaLLVMLazyJITCompiler(const std::string& ModuleID);
  
  virtual ~JavaLLVMLazyJITCompiler();
  
  virtual void* loadMethod(void* handle, const char* symbol);

  friend class LLVMMaterializer;
};

class LLVMMaterializer : public llvm::GVMaterializer {
public:
 
  JavaLLVMLazyJITCompiler* Comp;
  llvm::Module* TheModule;

  LLVMMaterializer(llvm::Module* M, JavaLLVMLazyJITCompiler* C);

  virtual bool Materialize(llvm::GlobalValue *GV, std::string *ErrInfo = 0);
  virtual bool isMaterializable(const llvm::GlobalValue*) const;
  virtual bool isDematerializable(const llvm::GlobalValue*) const {
    return false;
  }
  virtual bool MaterializeModule(llvm::Module*, std::string*) { return true; }
};

} // End j3 namespace

#endif
