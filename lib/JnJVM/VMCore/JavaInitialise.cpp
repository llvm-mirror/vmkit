//===-------- JavaInitialise.cpp - Initialization of JnJVM ----------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//


#include <vector>

#include "mvm/VMLet.h"
#include "mvm/Threads/Locks.h"

#include "JavaArray.h"
#include "JavaCache.h"
#include "JavaClass.h"
#include "JavaConstantPool.h"
#include "JavaIsolate.h"
#include "JavaJIT.h"
#include "JavaObject.h"
#include "JavaString.h"
#include "JavaThread.h"
#include "JavaTypes.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"
#include "JnjvmModuleProvider.h"
#include "NativeUtil.h"
#include "Reader.h"
#include "LockedMap.h"
#include "Zip.h"




using namespace jnjvm;

ClassArray* JavaArray::ofByte = 0;
ClassArray* JavaArray::ofChar = 0;
ClassArray* JavaArray::ofInt = 0;
ClassArray* JavaArray::ofShort = 0;
ClassArray* JavaArray::ofBool = 0;
ClassArray* JavaArray::ofLong = 0;
ClassArray* JavaArray::ofFloat = 0;
ClassArray* JavaArray::ofDouble = 0;
ClassArray* JavaArray::ofString = 0;
ClassArray* JavaArray::ofObject = 0;

const UTF8* Attribut::codeAttribut = 0;
const UTF8* Attribut::exceptionsAttribut = 0;
const UTF8* Attribut::constantAttribut = 0;
const UTF8* Attribut::lineNumberTableAttribut = 0;
const UTF8* Attribut::innerClassesAttribut = 0;
const UTF8* Attribut::sourceFileAttribut = 0;

JavaObject* CommonClass::jnjvmClassLoader = 0;

CommonClass* ClassArray::SuperArray = 0;
std::vector<Class*> ClassArray::InterfacesArray;
std::vector<JavaMethod*> ClassArray::VirtualMethodsArray;
std::vector<JavaMethod*> ClassArray::StaticMethodsArray;
std::vector<JavaField*> ClassArray::VirtualFieldsArray;
std::vector<JavaField*> ClassArray::StaticFieldsArray;


llvm::Function* JavaJIT::getSJLJBufferLLVM = 0;
llvm::Function* JavaJIT::throwExceptionLLVM = 0;
llvm::Function* JavaJIT::getExceptionLLVM = 0;
llvm::Function* JavaJIT::getJavaExceptionLLVM = 0;
llvm::Function* JavaJIT::clearExceptionLLVM = 0;
llvm::Function* JavaJIT::compareExceptionLLVM = 0;
llvm::Function* JavaJIT::nullPointerExceptionLLVM = 0;
llvm::Function* JavaJIT::classCastExceptionLLVM = 0;
llvm::Function* JavaJIT::indexOutOfBoundsExceptionLLVM = 0;
llvm::Function* JavaJIT::markAndTraceLLVM = 0;
llvm::Function* JavaJIT::javaObjectTracerLLVM = 0;
llvm::Function* JavaJIT::virtualLookupLLVM = 0;
llvm::Function* JavaJIT::fieldLookupLLVM = 0;
llvm::Function* JavaJIT::UTF8AconsLLVM = 0;
llvm::Function* JavaJIT::Int8AconsLLVM = 0;
llvm::Function* JavaJIT::Int32AconsLLVM = 0;
llvm::Function* JavaJIT::Int16AconsLLVM = 0;
llvm::Function* JavaJIT::FloatAconsLLVM = 0;
llvm::Function* JavaJIT::DoubleAconsLLVM = 0;
llvm::Function* JavaJIT::LongAconsLLVM = 0;
llvm::Function* JavaJIT::ObjectAconsLLVM = 0;
llvm::Function* JavaJIT::printExecutionLLVM = 0;
llvm::Function* JavaJIT::printMethodStartLLVM = 0;
llvm::Function* JavaJIT::printMethodEndLLVM = 0;
llvm::Function* JavaJIT::jniProceedPendingExceptionLLVM = 0;
llvm::Function* JavaJIT::doNewLLVM = 0;
llvm::Function* JavaJIT::doNewUnknownLLVM = 0;
llvm::Function* JavaJIT::initialiseObjectLLVM = 0;
llvm::Function* JavaJIT::newLookupLLVM = 0;
llvm::Function* JavaJIT::instanceOfLLVM = 0;
llvm::Function* JavaJIT::aquireObjectLLVM = 0;
llvm::Function* JavaJIT::releaseObjectLLVM = 0;
llvm::Function* JavaJIT::multiCallNewLLVM = 0;
llvm::Function* JavaJIT::runtimeUTF8ToStrLLVM = 0;
llvm::Function* JavaJIT::getStaticInstanceLLVM = 0;
llvm::Function* JavaJIT::getClassDelegateeLLVM = 0;
llvm::Function* JavaJIT::arrayLengthLLVM = 0;
#ifndef SINGLE_VM
llvm::Function* JavaJIT::doNewIsolateLLVM = 0;
#endif


const llvm::FunctionType* JavaJIT::markAndTraceLLVMType = 0;

mvm::Lock* JavaObject::globalLock = 0;
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
const llvm::Type* UTF8::llvmType = 0;
const llvm::Type* CacheNode::llvmType = 0;
const llvm::Type* Enveloppe::llvmType = 0;


mvm::Key<JavaThread>* JavaThread::threadKey = 0;


Jnjvm* Jnjvm::bootstrapVM = 0;
const UTF8* Jnjvm::initName = 0;
const UTF8* Jnjvm::clinitName = 0;
const UTF8* Jnjvm::clinitType = 0;
const UTF8* Jnjvm::runName = 0;
const UTF8* Jnjvm::prelib = 0;
const UTF8* Jnjvm::postlib = 0;
const UTF8* Jnjvm::mathName = 0;

#define DEF_UTF8(var) \
  const UTF8* Jnjvm::var = 0;
  
  DEF_UTF8(abs);
  DEF_UTF8(sqrt);
  DEF_UTF8(sin);
  DEF_UTF8(cos);
  DEF_UTF8(tan);
  DEF_UTF8(asin);
  DEF_UTF8(acos);
  DEF_UTF8(atan);
  DEF_UTF8(atan2);
  DEF_UTF8(exp);
  DEF_UTF8(log);
  DEF_UTF8(pow);
  DEF_UTF8(ceil);
  DEF_UTF8(floor);
  DEF_UTF8(rint);
  DEF_UTF8(cbrt);
  DEF_UTF8(cosh);
  DEF_UTF8(expm1);
  DEF_UTF8(hypot);
  DEF_UTF8(log10);
  DEF_UTF8(log1p);
  DEF_UTF8(sinh);
  DEF_UTF8(tanh);

#undef DEF_UTF8


static void initialiseVT() {

# define INIT(X) { \
  X fake; \
  X::VT = ((void**)(void*)(&fake))[0]; }

  INIT(JavaArray);
  INIT(ArrayUInt8);
  INIT(ArraySInt8);
  INIT(ArrayUInt16);
  INIT(ArraySInt16);
  INIT(ArrayUInt32);
  INIT(ArraySInt32);
  INIT(ArrayLong);
  INIT(ArrayFloat);
  INIT(ArrayDouble);
  INIT(ArrayObject);
  INIT(UTF8);
  INIT(Attribut);
  INIT(CommonClass);
  INIT(Class);
  INIT(ClassArray);
  INIT(JavaMethod);
  INIT(JavaField);
  INIT(JavaCtpInfo);
  INIT(Exception);
  INIT(JavaJIT);
  INIT(JavaCond);
  INIT(LockObj);
  INIT(JavaObject);
  INIT(JavaThread);
  //mvm::Key<JavaThread>::VT = mvm::ThreadKey::VT;
  INIT(AssessorDesc);
  INIT(Typedef);
  INIT(Signdef);
  INIT(ThreadSystem);
  INIT(Jnjvm);
  INIT(Reader);
  INIT(ZipFile);
  INIT(ZipArchive);
  INIT(UTF8Map);
  INIT(ClassMap);
  INIT(FieldMap);
  INIT(MethodMap);
  INIT(ZipFileMap);
  INIT(StringMap);
  INIT(jnjvm::TypeMap);
  INIT(JavaIsolate);
  INIT(JavaString);
  INIT(CacheNode);
  INIT(Enveloppe);
#undef INIT

}

static void initialiseStatics() {
  JavaObject::globalLock = mvm::Lock::allocNormal();
  //mvm::Object::pushRoot((mvm::Object*)JavaObject::globalLock);

  JavaThread::threadKey = new mvm::Key<JavaThread>();
  //JavaThread::threadKey = gc_new(mvm::Key<JavaThread>);
  //mvm::Object::pushRoot((mvm::Object*)JavaThread::threadKey);
  
  Jnjvm* vm = JavaIsolate::bootstrapVM = JavaIsolate::allocateBootstrap();
  mvm::Object::pushRoot((mvm::Object*)JavaIsolate::bootstrapVM);
  
  // Array initialization
  const UTF8* utf8OfChar = vm->asciizConstructUTF8("[C");
  JavaArray::ofChar = vm->constructArray(utf8OfChar,
                                         CommonClass::jnjvmClassLoader);
  ((UTF8*)utf8OfChar)->classOf = JavaArray::ofChar;
  
  AssessorDesc::initialise(vm);


  
  ClassArray::InterfacesArray.push_back(
    vm->constructClass(vm->asciizConstructUTF8("java/lang/Cloneable"),
                       CommonClass::jnjvmClassLoader));
  
  ClassArray::InterfacesArray.push_back(
    vm->constructClass(vm->asciizConstructUTF8("java/io/Serializable"),
                       CommonClass::jnjvmClassLoader));
  
  ClassArray::SuperArray = 
    vm->constructClass(vm->asciizConstructUTF8("java/lang/Object"),
                       CommonClass::jnjvmClassLoader);
  
  JavaArray::ofChar->interfaces = ClassArray::InterfacesArray;
  JavaArray::ofChar->super = ClassArray::SuperArray;
  
  JavaArray::ofByte = vm->constructArray(vm->asciizConstructUTF8("[B"),
                                         CommonClass::jnjvmClassLoader);
  JavaArray::ofString = 
    vm->constructArray(vm->asciizConstructUTF8("[Ljava/lang/String;"),
                       CommonClass::jnjvmClassLoader);
  
  JavaArray::ofObject = 
    vm->constructArray(vm->asciizConstructUTF8("[Ljava/lang/Object;"),
                       CommonClass::jnjvmClassLoader);
  
  JavaArray::ofInt = vm->constructArray(vm->asciizConstructUTF8("[I"), 
                                        CommonClass::jnjvmClassLoader);
  
  JavaArray::ofBool = vm->constructArray(vm->asciizConstructUTF8("[Z"), 
                                        CommonClass::jnjvmClassLoader);
  
  JavaArray::ofLong = vm->constructArray(vm->asciizConstructUTF8("[J"), 
                                        CommonClass::jnjvmClassLoader);
  
  JavaArray::ofFloat = vm->constructArray(vm->asciizConstructUTF8("[F"), 
                                        CommonClass::jnjvmClassLoader);
  
  JavaArray::ofDouble = vm->constructArray(vm->asciizConstructUTF8("[D"), 
                                        CommonClass::jnjvmClassLoader);
  
  JavaArray::ofShort = vm->constructArray(vm->asciizConstructUTF8("[S"), 
                                        CommonClass::jnjvmClassLoader);
  
  // End array initialization

  Attribut::codeAttribut = vm->asciizConstructUTF8("Code");
  Attribut::exceptionsAttribut = vm->asciizConstructUTF8("Exceptions");
  Attribut::constantAttribut = vm->asciizConstructUTF8("ConstantValue");
  Attribut::lineNumberTableAttribut =
    vm->asciizConstructUTF8("LineNumberTable");
  Attribut::innerClassesAttribut = vm->asciizConstructUTF8("InnerClasses");
  Attribut::sourceFileAttribut = vm->asciizConstructUTF8("SourceFile");
  
  Jnjvm::initName = vm->asciizConstructUTF8("<init>");
  Jnjvm::clinitName = vm->asciizConstructUTF8("<clinit>");
  Jnjvm::clinitType = vm->asciizConstructUTF8("()V");
  Jnjvm::runName = vm->asciizConstructUTF8("run");
  Jnjvm::prelib = vm->asciizConstructUTF8("lib");
  Jnjvm::postlib = vm->asciizConstructUTF8(".so");
  Jnjvm::mathName = vm->asciizConstructUTF8("java/lang/Math");

#define DEF_UTF8(var) \
  Jnjvm::var = vm->asciizConstructUTF8(#var)
  
  DEF_UTF8(abs);
  DEF_UTF8(sqrt);
  DEF_UTF8(sin);
  DEF_UTF8(cos);
  DEF_UTF8(tan);
  DEF_UTF8(asin);
  DEF_UTF8(acos);
  DEF_UTF8(atan);
  DEF_UTF8(atan2);
  DEF_UTF8(exp);
  DEF_UTF8(log);
  DEF_UTF8(pow);
  DEF_UTF8(ceil);
  DEF_UTF8(floor);
  DEF_UTF8(rint);
  DEF_UTF8(cbrt);
  DEF_UTF8(cosh);
  DEF_UTF8(expm1);
  DEF_UTF8(hypot);
  DEF_UTF8(log10);
  DEF_UTF8(log1p);
  DEF_UTF8(sinh);
  DEF_UTF8(tanh);

#undef DEF_UTF8
 
}

extern "C" void sigsegv_handler(int val, void* addr) {
  printf("SIGSEGV in JnJVM at %p\n", addr);
  JavaJIT::printBacktrace();
  assert(0);
}


extern "C" int boot() {
  JavaJIT::initialise();
  initialiseVT();
  initialiseStatics();
  Classpath::initialiseClasspath(JavaIsolate::bootstrapVM);
  //mvm::VMLet::register_sigsegv_handler(sigsegv_handler);
  return 0; 
}

extern "C" int start_app(int argc, char** argv) {
#if defined(SINGLE_VM) || defined(SERVICE_VM)
  JavaIsolate* vm = (JavaIsolate*)JavaIsolate::bootstrapVM;
#else
  JavaIsolate* vm = JavaIsolate::allocateIsolate(JavaIsolate::bootstrapVM);
#endif
  vm->runMain(argc, argv);
  return 0;
}
