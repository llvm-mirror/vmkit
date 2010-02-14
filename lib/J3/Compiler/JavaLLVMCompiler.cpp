//===-------- JavaLLVMCompiler.cpp - A LLVM Compiler for J3 ---------------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/BasicBlock.h"
#include "llvm/CallingConv.h"
#include "llvm/Constants.h"
#include "llvm/Instructions.h"
#include "llvm/LLVMContext.h"
#include "llvm/Module.h"
#include "llvm/PassManager.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Target/TargetData.h"

#include "mvm/JIT.h"

#include "JavaArray.h"
#include "JavaClass.h"
#include "JavaJIT.h"
#include "JavaTypes.h"

#include "j3/J3Intrinsics.h"
#include "j3/LLVMMaterializer.h"

using namespace j3;
using namespace llvm;

JavaLLVMCompiler::JavaLLVMCompiler(const std::string& str) :
  TheModule(new llvm::Module(str, getGlobalContext())),
  DebugFactory(new DIFactory(*TheModule)),
  JavaIntrinsics(TheModule) {

  enabledException = true;
#ifdef WITH_LLVM_GCC
  cooperativeGC = true;
#else
  cooperativeGC = false;
#endif
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
  J3Intrinsics::protectIR();
  if (func->getLinkage() == GlobalValue::ExternalWeakLinkage) {
    JavaJIT jit(this, meth, func);
    if (isNative(meth->access)) {
      jit.nativeCompile();
      J3Intrinsics::runPasses(func, JavaNativeFunctionPasses);
    } else {
      jit.javaCompile();
      J3Intrinsics::runPasses(func, J3Intrinsics::globalFunctionPasses);
      J3Intrinsics::runPasses(func, JavaFunctionPasses);
    }
    func->setLinkage(GlobalValue::ExternalLinkage);
  }
  J3Intrinsics::unprotectIR();

  return func;
}

JavaMethod* JavaLLVMCompiler::getJavaMethod(llvm::Function* F) {
  function_iterator E = functions.end();
  function_iterator I = functions.find(F);
  if (I == E) return 0;
  return I->second;
}

MDNode* JavaLLVMCompiler::GetDbgSubprogram(JavaMethod* meth) {
  if (getMethodInfo(meth)->getDbgSubprogram() == NULL) {
    MDNode* node = DebugFactory->CreateSubprogram(DIDescriptor(), "", "",
                                                  "", DICompileUnit(), 0,
                                                  DIType(), false,
                                                  false).getNode();
    DbgInfos.insert(std::make_pair(node, meth));
    getMethodInfo(meth)->setDbgSubprogram(node);
  }
  return getMethodInfo(meth)->getDbgSubprogram();
}

JavaLLVMCompiler::~JavaLLVMCompiler() {
  delete JavaFunctionPasses;
  delete JavaNativeFunctionPasses;
}

namespace mvm {
  llvm::FunctionPass* createEscapeAnalysisPass();
  llvm::LoopPass* createLoopSafePointsPass();
}

namespace j3 {
  llvm::FunctionPass* createLowerConstantCallsPass(J3Intrinsics* I);
}

void JavaLLVMCompiler::addJavaPasses() {
  JavaNativeFunctionPasses = new FunctionPassManager(TheModule);
  JavaNativeFunctionPasses->add(new TargetData(TheModule));
  // Lower constant calls to lower things like getClass used
  // on synchronized methods.
  JavaNativeFunctionPasses->add(createLowerConstantCallsPass(getIntrinsics()));
  
  JavaFunctionPasses = new FunctionPassManager(TheModule);
  JavaFunctionPasses->add(new TargetData(TheModule));
  if (cooperativeGC)
    JavaFunctionPasses->add(mvm::createLoopSafePointsPass());

  // Re-enable this when the pointers in stack-allocated objects can
  // be given to the GC.
  //JavaFunctionPasses->add(mvm::createEscapeAnalysisPass());
  JavaFunctionPasses->add(createLowerConstantCallsPass(getIntrinsics()));
}
