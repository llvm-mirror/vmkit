//===-------- JavaLLVMCompiler.cpp - A LLVM Compiler for J3 ---------------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/LLVMContext.h"
#include "llvm/Module.h"
#include "llvm/PassManager.h"
#include "llvm/Analysis/DIBuilder.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Target/TargetData.h"

#include "vmkit/JIT.h"

#include "JavaClass.h"
#include "JavaJIT.h"

#include "j3/JavaLLVMCompiler.h"

using namespace llvm;

namespace j3 {

JavaLLVMCompiler::JavaLLVMCompiler(const std::string& str) :
  TheModule(new llvm::Module(str, *(new LLVMContext()))),
  DebugFactory(new DIBuilder(*TheModule)) {

  enabledException = true;
  cooperativeGC = true;
}
  
void JavaLLVMCompiler::resolveVirtualClass(Class* cl) {
  // Lock here because we may be called by a class resolver
  vmkit::VmkitModule::protectIR();
  LLVMClassInfo* LCI = (LLVMClassInfo*)getClassInfo(cl);
  LCI->getVirtualType();
  vmkit::VmkitModule::unprotectIR();
}

void JavaLLVMCompiler::resolveStaticClass(Class* cl) {
  // Lock here because we may be called by a class initializer
  vmkit::VmkitModule::protectIR();
  LLVMClassInfo* LCI = (LLVMClassInfo*)getClassInfo(cl);
  LCI->getStaticType();
  vmkit::VmkitModule::unprotectIR();
}

Function* JavaLLVMCompiler::getMethod(JavaMethod* meth, Class* customizeFor) {
  return getMethodInfo(meth)->getMethod(customizeFor);
}

Function* JavaLLVMCompiler::parseFunction(JavaMethod* meth, Class* customizeFor) {
  assert(!isAbstract(meth->access));
  LLVMMethodInfo* LMI = getMethodInfo(meth);
  Function* func = LMI->getMethod(customizeFor);
  
  // We are jitting. Take the lock.
  vmkit::VmkitModule::protectIR();
  if (func->getLinkage() == GlobalValue::ExternalWeakLinkage) {
    JavaJIT jit(this, meth, func, customizeFor);
    if (isNative(meth->access)) {
      jit.nativeCompile();
      vmkit::VmkitModule::runPasses(func, JavaNativeFunctionPasses);
      vmkit::VmkitModule::runPasses(func, J3FunctionPasses);
    } else {
      jit.javaCompile();
      vmkit::VmkitModule::runPasses(func, JavaFunctionPasses);
      vmkit::VmkitModule::runPasses(func, J3FunctionPasses);
    }
    func->setLinkage(GlobalValue::ExternalLinkage);
    if (!LMI->isCustomizable && jit.isCustomizable) {
      // It's the first time we parsed the method and we just found
      // out it can be customized.
      meth->isCustomizable = true;
      LMI->isCustomizable = true;
      if (customizeFor != NULL) {
        LMI->setCustomizedVersion(customizeFor, func);
      }
    }
  }
  vmkit::VmkitModule::unprotectIR();

  return func;
}

JavaMethod* JavaLLVMCompiler::getJavaMethod(const llvm::Function& F) {
  function_iterator E = functions.end();
  function_iterator I = functions.find(&F);
  if (I == E) return 0;
  return I->second;
}

JavaLLVMCompiler::~JavaLLVMCompiler() {
  LLVMContext* Context = &(TheModule->getContext());
  delete TheModule;
  delete DebugFactory;
  delete JavaFunctionPasses;
  delete J3FunctionPasses;
  delete JavaNativeFunctionPasses;
  delete Context;
}

llvm::FunctionPass* createLowerConstantCallsPass(JavaLLVMCompiler* I);

void JavaLLVMCompiler::addJavaPasses() {
  JavaNativeFunctionPasses = new FunctionPassManager(TheModule);
  JavaNativeFunctionPasses->add(new TargetData(TheModule));

  J3FunctionPasses = new FunctionPassManager(TheModule);
  J3FunctionPasses->add(createLowerConstantCallsPass(this));
  
  JavaFunctionPasses = new FunctionPassManager(TheModule);
  JavaFunctionPasses->add(new TargetData(TheModule));
  vmkit::VmkitModule::addCommandLinePasses(JavaFunctionPasses);
}

} // end namespace j3
