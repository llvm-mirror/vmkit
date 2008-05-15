//===------JavaJITInitialise.cpp - Initialization of LLVM objects ---------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <stddef.h>

#include "llvm/CallingConv.h"
#include "llvm/Instructions.h"
#include "llvm/LinkAllPasses.h"
#include "llvm/Module.h"
#include "llvm/ParameterAttributes.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Support/MutexGuard.h"


#include "mvm/JIT.h"
#include "mvm/Method.h"

#include "JavaArray.h"
#include "JavaCache.h"
#include "JavaClass.h"
#include "JavaJIT.h"
#include "JavaObject.h"
#include "JavaThread.h"
#include "Jnjvm.h"
#include "JnjvmModuleProvider.h"
#ifdef SERVICE_VM
#include "ServiceDomain.h"
#endif

using namespace jnjvm;
using namespace llvm;

#ifdef WITH_TRACER
const llvm::FunctionType* JavaJIT::markAndTraceLLVMType = 0;
#endif

const llvm::Type* JavaObject::llvmType = 0;
const llvm::Type* JavaArray::llvmType = 0;
const llvm::Type* ArrayUInt8::llvmType = 0;
const llvm::Type* ArraySInt8::llvmType = 0;
const llvm::Type* ArrayUInt16::llvmType = 0;
const llvm::Type* ArraySInt16::llvmType = 0;
const llvm::Type* ArrayUInt32::llvmType = 0;
const llvm::Type* ArraySInt32::llvmType = 0;
const llvm::Type* ArrayFloat::llvmType = 0;
const llvm::Type* ArrayDouble::llvmType = 0;
const llvm::Type* ArrayLong::llvmType = 0;
const llvm::Type* ArrayObject::llvmType = 0;
const llvm::Type* CacheNode::llvmType = 0;
const llvm::Type* Enveloppe::llvmType = 0;

void JavaJIT::initialiseJITIsolateVM(Jnjvm* vm) {
  mvm::jit::protectEngine->lock();
  mvm::jit::executionEngine->addModuleProvider(vm->TheModuleProvider);
  mvm::jit::protectEngine->unlock();
}

#include "LLVMRuntime.cpp"

void JavaJIT::initialiseJITBootstrapVM(Jnjvm* vm) {
  Module* module = vm->module;
  mvm::jit::protectEngine->lock();
  mvm::jit::executionEngine->addModuleProvider(vm->TheModuleProvider); 
  mvm::jit::protectEngine->unlock();

  mvm::jit::protectTypes();
  makeLLVMModuleContents(module);
  
  VTType = module->getTypeByName("VT");
  
  JavaObject::llvmType = 
    PointerType::getUnqual(module->getTypeByName("JavaObject"));

  JavaArray::llvmType =
    PointerType::getUnqual(module->getTypeByName("JavaArray"));
  
  JavaClassType =
    PointerType::getUnqual(module->getTypeByName("JavaClass"));

#define ARRAY_TYPE(name, type) \
  name::llvmType = PointerType::getUnqual(module->getTypeByName(#name));
  ARRAY_TYPE(ArrayUInt8, Type::Int8Ty);
  ARRAY_TYPE(ArraySInt8, Type::Int8Ty);
  ARRAY_TYPE(ArrayUInt16, Type::Int16Ty);
  ARRAY_TYPE(ArraySInt16, Type::Int16Ty);
  ARRAY_TYPE(ArrayUInt32, Type::Int32Ty);
  ARRAY_TYPE(ArraySInt32, Type::Int32Ty);
  ARRAY_TYPE(ArrayLong, Type::Int64Ty);
  ARRAY_TYPE(ArrayDouble, Type::DoubleTy);
  ARRAY_TYPE(ArrayFloat, Type::FloatTy);
  ARRAY_TYPE(ArrayObject, JavaObject::llvmType);
#undef ARRAY_TYPE

  CacheNode::llvmType =
    PointerType::getUnqual(module->getTypeByName("CacheNode"));
  
  Enveloppe::llvmType =
    PointerType::getUnqual(module->getTypeByName("Enveloppe"));

  mvm::jit::unprotectTypes();

  virtualLookupLLVM = module->getFunction("virtualLookup");
  multiCallNewLLVM = module->getFunction("multiCallNew");
  initialisationCheckLLVM = module->getFunction("initialisationCheck");
  forceInitialisationCheckLLVM = 
    module->getFunction("forceInitialisationCheck");

  arrayLengthLLVM = module->getFunction("arrayLength");
  getVTLLVM = module->getFunction("getVT");
  getClassLLVM = module->getFunction("getClass");
  newLookupLLVM = module->getFunction("newLookup");
  getVTFromClassLLVM = module->getFunction("getVTFromClass");
  getObjectSizeFromClassLLVM = module->getFunction("getObjectSizeFromClass");
 
  getClassDelegateeLLVM = module->getFunction("getClassDelegatee");
  instanceOfLLVM = module->getFunction("JavaObjectInstanceOf");
  aquireObjectLLVM = module->getFunction("JavaObjectAquire");
  releaseObjectLLVM = module->getFunction("JavaObjectRelease");

  fieldLookupLLVM = module->getFunction("fieldLookup");
  
  getExceptionLLVM = module->getFunction("JavaThreadGetException");
  getJavaExceptionLLVM = module->getFunction("JavaThreadGetJavaException");
  compareExceptionLLVM = module->getFunction("JavaThreadCompareException");
  jniProceedPendingExceptionLLVM = 
    module->getFunction("jniProceedPendingException");
  getSJLJBufferLLVM = module->getFunction("getSJLJBuffer");
  
  nullPointerExceptionLLVM = module->getFunction("nullPointerException");
  classCastExceptionLLVM = module->getFunction("classCastException");
  indexOutOfBoundsExceptionLLVM = 
    module->getFunction("indexOutOfBoundsException");
  negativeArraySizeExceptionLLVM = 
    module->getFunction("negativeArraySizeException");
  outOfMemoryErrorLLVM = module->getFunction("outOfMemoryError");

  javaObjectAllocateLLVM = module->getFunction("gcmalloc");

  printExecutionLLVM = module->getFunction("printExecution");
  printMethodStartLLVM = module->getFunction("printMethodStart");
  printMethodEndLLVM = module->getFunction("printMethodEnd");

  throwExceptionLLVM = module->getFunction("JavaThreadThrowException");

  clearExceptionLLVM = module->getFunction("JavaThreadClearException");
  

#ifdef MULTIPLE_VM
  getStaticInstanceLLVM = module->getFunction("getStaticInstance");
  runtimeUTF8ToStrLLVM = module->getFunction("runtimeUTF8ToStr");
#endif
  
#ifdef SERVICE_VM
  aquireObjectInSharedDomainLLVM = 
    module->getFunction("JavaObjectAquireInSharedDomain");
  releaseObjectInSharedDomainLLVM = 
    module->getFunction("JavaObjectReleaseInSharedDomain");
  ServiceDomain::serviceCallStartLLVM = module->getFunction("serviceCallStart");
  ServiceDomain::serviceCallStopLLVM = module->getFunction("serviceCallStop");
#endif
    
#ifdef WITH_TRACER
  markAndTraceLLVM = module->getFunction("MarkAndTrace");
  markAndTraceLLVMType = markAndTraceLLVM->getFunctionType();
  javaObjectTracerLLVM = module->getFunction("JavaObjectTracer");
#endif

#ifndef WITHOUT_VTABLE
  vtableLookupLLVM = module->getFunction("vtableLookup");
#endif

#ifdef MULTIPLE_GC
  getCollectorLLVM = module->getFunction("getCollector");
#endif

  
  mvm::jit::protectConstants();
  constantUTF8Null = Constant::getNullValue(ArrayUInt16::llvmType); 
  constantJavaClassNull = Constant::getNullValue(JavaClassType); 
  constantJavaObjectNull = Constant::getNullValue(JavaObject::llvmType);
  constantMaxArraySize = ConstantInt::get(Type::Int32Ty,
                                          JavaArray::MaxArraySize);
  constantJavaObjectSize = ConstantInt::get(Type::Int32Ty, sizeof(JavaObject));
  
  
  Constant* cons = 
    ConstantExpr::getIntToPtr(ConstantInt::get(Type::Int64Ty,
                              uint64_t (JavaObject::VT)), VTType);
      
  JavaObjectVT = new GlobalVariable(VTType, true,
                                    GlobalValue::ExternalLinkage,
                                    cons, "",
                                    module);
  
  cons = 
    ConstantExpr::getIntToPtr(ConstantInt::get(Type::Int64Ty,
                              uint64_t (ArrayObject::VT)), VTType);
  
  ArrayObjectVT = new GlobalVariable(VTType, true,
                                    GlobalValue::ExternalLinkage,
                                    cons, "",
                                    module);
  mvm::jit::unprotectConstants();
  
  constantOffsetObjectSizeInClass = mvm::jit::constantOne;
  constantOffsetVTInClass = mvm::jit::constantTwo;
  
}

llvm::Constant*    JavaJIT::constantJavaObjectNull;
llvm::Constant*    JavaJIT::constantUTF8Null;
llvm::Constant*    JavaJIT::constantJavaClassNull;
llvm::Constant*    JavaJIT::constantMaxArraySize;
llvm::Constant*    JavaJIT::constantJavaObjectSize;
llvm::GlobalVariable*    JavaJIT::JavaObjectVT;
llvm::GlobalVariable*    JavaJIT::ArrayObjectVT;
llvm::ConstantInt* JavaJIT::constantOffsetObjectSizeInClass;
llvm::ConstantInt* JavaJIT::constantOffsetVTInClass;
const llvm::Type* JavaJIT::JavaClassType;

namespace mvm {

llvm::FunctionPass* createEscapeAnalysisPass(llvm::Function*, llvm::Function*);
llvm::FunctionPass* createLowerConstantCallsPass();
//llvm::FunctionPass* createArrayChecksPass();

}

static void addPass(FunctionPassManager *PM, Pass *P) {
  // Add the pass to the pass manager...
  PM->add(P);
}

void JavaJIT::AddStandardCompilePasses(FunctionPassManager *PM) {
  llvm::MutexGuard locked(mvm::jit::executionEngine->lock);
  // LLVM does not allow calling functions from other modules in verifier
  //PM->add(llvm::createVerifierPass());                  // Verify that input is correct
  
  addPass(PM, llvm::createCFGSimplificationPass());    // Clean up disgusting code
  addPass(PM, llvm::createPromoteMemoryToRegisterPass());// Kill useless allocas
  addPass(PM, llvm::createInstructionCombiningPass()); // Clean up after IPCP & DAE
  addPass(PM, llvm::createCFGSimplificationPass());    // Clean up after IPCP & DAE
  
  addPass(PM, createTailDuplicationPass());      // Simplify cfg by copying code
  addPass(PM, createSimplifyLibCallsPass());     // Library Call Optimizations
  addPass(PM, createInstructionCombiningPass()); // Cleanup for scalarrepl.
  addPass(PM, createJumpThreadingPass());        // Thread jumps.
  addPass(PM, createCFGSimplificationPass());    // Merge & remove BBs
  addPass(PM, createScalarReplAggregatesPass()); // Break up aggregate allocas
  addPass(PM, createInstructionCombiningPass()); // Combine silly seq's
  addPass(PM, createCondPropagationPass());      // Propagate conditionals
  
  addPass(PM, createTailCallEliminationPass());  // Eliminate tail calls
  addPass(PM, createCFGSimplificationPass());    // Merge & remove BBs
  addPass(PM, createReassociatePass());          // Reassociate expressions
  addPass(PM, createLoopRotatePass());
  addPass(PM, createLICMPass());                 // Hoist loop invariants
  addPass(PM, createLoopUnswitchPass());         // Unswitch loops.
  addPass(PM, createLoopIndexSplitPass());       // Index split loops.
  addPass(PM, createIndVarSimplifyPass());       // Canonicalize indvars
  addPass(PM, createLoopDeletionPass());         // Delete dead loops
  addPass(PM, createLoopUnrollPass());           // Unroll small loops
  addPass(PM, createInstructionCombiningPass()); // Clean up after the unroller
  addPass(PM, createGVNPass());                  // Remove redundancies
  addPass(PM, createMemCpyOptPass());            // Remove memcpy / form memset
  addPass(PM, createSCCPPass());                 // Constant prop with SCCP
  
  addPass(PM, mvm::createLowerConstantCallsPass());

  // Run instcombine after redundancy elimination to exploit opportunities
  // opened up by them.
  addPass(PM, createInstructionCombiningPass());
  addPass(PM, createCondPropagationPass());      // Propagate conditionals

  addPass(PM, createDeadStoreEliminationPass()); // Delete dead stores
  addPass(PM, createAggressiveDCEPass());        // SSA based 'Aggressive DCE'
  addPass(PM, createCFGSimplificationPass());    // Merge & remove BBs

}

