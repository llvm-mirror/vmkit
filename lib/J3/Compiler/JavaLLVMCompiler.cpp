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
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Target/TargetData.h"

#include "mvm/JIT.h"

#include "JavaClass.h"
#include "JavaJIT.h"

#include "j3/JavaLLVMCompiler.h"

using namespace j3;
using namespace llvm;

JavaLLVMCompiler::JavaLLVMCompiler(const std::string& str) :
  TheModule(new llvm::Module(str, *(new LLVMContext()))),
  DebugFactory(new DIFactory(*TheModule)),
  JavaIntrinsics(TheModule) {

  enabledException = true;
#ifdef WITH_LLVM_GCC
  cooperativeGC = true;
#else
  cooperativeGC = false;
#endif
  initialiseAssessorInfo();
}
  
void JavaLLVMCompiler::resolveVirtualClass(Class* cl) {
  // Lock here because we may be called by a class resolver
  mvm::MvmModule::protectIR();
  LLVMClassInfo* LCI = (LLVMClassInfo*)getClassInfo(cl);
  LCI->getVirtualType();
  mvm::MvmModule::unprotectIR();
}

void JavaLLVMCompiler::resolveStaticClass(Class* cl) {
  // Lock here because we may be called by a class initializer
  mvm::MvmModule::protectIR();
  LLVMClassInfo* LCI = (LLVMClassInfo*)getClassInfo(cl);
  LCI->getStaticType();
  mvm::MvmModule::unprotectIR();
}

Function* JavaLLVMCompiler::getMethod(JavaMethod* meth) {
  return getMethodInfo(meth)->getMethod();
}

Function* JavaLLVMCompiler::parseFunction(JavaMethod* meth) {
  LLVMMethodInfo* LMI = getMethodInfo(meth);
  Function* func = LMI->getMethod();
  
  // We are jitting. Take the lock.
  mvm::MvmModule::protectIR();
  if (func->getLinkage() == GlobalValue::ExternalWeakLinkage) {
    JavaJIT jit(this, meth, func);
    if (isNative(meth->access)) {
      jit.nativeCompile();
      mvm::MvmModule::runPasses(func, JavaNativeFunctionPasses);
    } else {
      jit.javaCompile();
      mvm::MvmModule::runPasses(func, JavaFunctionPasses);
    }
    func->setLinkage(GlobalValue::ExternalLinkage);
  }
  mvm::MvmModule::unprotectIR();

  return func;
}

JavaMethod* JavaLLVMCompiler::getJavaMethod(const llvm::Function& F) {
  function_iterator E = functions.end();
  function_iterator I = functions.find(&F);
  if (I == E) return 0;
  return I->second;
}

MDNode* JavaLLVMCompiler::GetDbgSubprogram(JavaMethod* meth) {
  if (getMethodInfo(meth)->getDbgSubprogram() == NULL) {
    MDNode* node = DebugFactory->CreateSubprogram(DIDescriptor(), "", "",
                                                  "", DIFile(), 0,
                                                  DIType(), false,
                                                  false);
    DbgInfos.insert(std::make_pair(node, meth));
    getMethodInfo(meth)->setDbgSubprogram(node);
  }
  return getMethodInfo(meth)->getDbgSubprogram();
}

JavaLLVMCompiler::~JavaLLVMCompiler() {
  LLVMContext* Context = &(TheModule->getContext());
  delete TheModule;
  delete DebugFactory;
  delete JavaFunctionPasses;
  delete JavaNativeFunctionPasses;
  delete Context;
}

namespace mvm {
  llvm::FunctionPass* createEscapeAnalysisPass();
  llvm::LoopPass* createLoopSafePointsPass();
}

namespace j3 {
  llvm::FunctionPass* createLowerConstantCallsPass(JavaLLVMCompiler* I);
}

void JavaLLVMCompiler::addJavaPasses() {
  JavaNativeFunctionPasses = new FunctionPassManager(TheModule);
  JavaNativeFunctionPasses->add(new TargetData(TheModule));
  // Lower constant calls to lower things like getClass used
  // on synchronized methods.
  JavaNativeFunctionPasses->add(createLowerConstantCallsPass(this));
  
  JavaFunctionPasses = new FunctionPassManager(TheModule);
  if (cooperativeGC)
    JavaFunctionPasses->add(mvm::createLoopSafePointsPass());
  // Add other passes after the loop pass, because safepoints may move objects.
  // Moving objects disable many optimizations.
  mvm::MvmModule::addCommandLinePasses(JavaFunctionPasses);

  // Re-enable this when the pointers in stack-allocated objects can
  // be given to the GC.
  //JavaFunctionPasses->add(mvm::createEscapeAnalysisPass());
  JavaFunctionPasses->add(createLowerConstantCallsPass(this));
}
