//===----------- N3Initialise.cpp - Initialization of N3 ------------------===//
//
//                              N3
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//


#include <vector>

#include "mvm/CompilationUnit.h"
#include "mvm/JIT.h"
#include "mvm/Threads/Locks.h"

#include "Assembly.h"
#include "CLIString.h"
#include "CLIJit.h"
#include "LockedMap.h"
#include "NativeUtil.h"
#include "MSCorlib.h"
#include "N3.h"
#include "N3ModuleProvider.h"
#include "Reader.h"
#include "VirtualMachine.h"
#include "VMArray.h"
#include "VMCache.h"
#include "VMClass.h"
#include "VMObject.h"
#include "VMThread.h"

using namespace n3;

N3* N3::bootstrapVM = 0;
mvm::Lock* VMObject::globalLock = 0;

VMCommonClass* VMClassArray::SuperArray = 0;
std::vector<VMClass*> VMClassArray::InterfacesArray;
std::vector<VMMethod*> VMClassArray::VirtualMethodsArray;
std::vector<VMMethod*> VMClassArray::StaticMethodsArray;
std::vector<VMField*> VMClassArray::VirtualFieldsArray;
std::vector<VMField*> VMClassArray::StaticFieldsArray;

VMClass* MSCorlib::pVoid = 0;
VMClass* MSCorlib::pBoolean= 0;
VMClass* MSCorlib::pChar = 0;
VMClass* MSCorlib::pSInt8 = 0;
VMClass* MSCorlib::pUInt8 = 0;
VMClass* MSCorlib::pSInt16 = 0;
VMClass* MSCorlib::pUInt16 = 0;
VMClass* MSCorlib::pSInt32 = 0;
VMClass* MSCorlib::pUInt32 = 0;
VMClass* MSCorlib::pSInt64 = 0;
VMClass* MSCorlib::pUInt64 = 0;
VMClass* MSCorlib::pFloat = 0;
VMClass* MSCorlib::pDouble = 0;
VMClass* MSCorlib::pIntPtr = 0;
VMClass* MSCorlib::pUIntPtr = 0;
VMClass* MSCorlib::pObject = 0;
VMClass* MSCorlib::pString = 0;
VMClass* MSCorlib::pValue = 0;
VMClass* MSCorlib::pEnum = 0;
VMClass* MSCorlib::pArray = 0;
VMClass* MSCorlib::pDelegate = 0;
VMClass* MSCorlib::pException = 0;
VMClassArray* MSCorlib::arrayChar = 0;
VMClassArray* MSCorlib::arrayString = 0;
VMClassArray* MSCorlib::arrayObject = 0;
VMClassArray* MSCorlib::arrayByte = 0;
VMMethod* MSCorlib::ctorPropertyType;
VMMethod* MSCorlib::ctorMethodType;
VMMethod* MSCorlib::ctorClrType;
VMClass* MSCorlib::clrType;
VMField* MSCorlib::typeClrType;
VMField* MSCorlib::propertyPropertyType;
VMField* MSCorlib::methodMethodType;
VMMethod* MSCorlib::ctorAssemblyReflection;
VMClass* MSCorlib::assemblyReflection;
VMClass* MSCorlib::typedReference;
VMField* MSCorlib::assemblyAssemblyReflection;
VMClass* MSCorlib::propertyType;
VMClass* MSCorlib::methodType;
VMClass* MSCorlib::resourceStreamType;
VMMethod* MSCorlib::ctorResourceStreamType;

VMField* MSCorlib::ctorBoolean;
VMField* MSCorlib::ctorUInt8;
VMField* MSCorlib::ctorSInt8;
VMField* MSCorlib::ctorChar;
VMField* MSCorlib::ctorSInt16;
VMField* MSCorlib::ctorUInt16;
VMField* MSCorlib::ctorSInt32;
VMField* MSCorlib::ctorUInt32;
VMField* MSCorlib::ctorSInt64;
VMField* MSCorlib::ctorUInt64;
VMField* MSCorlib::ctorIntPtr;
VMField* MSCorlib::ctorUIntPtr;
VMField* MSCorlib::ctorDouble;
VMField* MSCorlib::ctorFloat;

const UTF8* N3::clinitName = 0;
const UTF8* N3::ctorName = 0;
const UTF8* N3::invokeName = 0;
const UTF8* N3::math = 0;
const UTF8* N3::system = 0;
const UTF8* N3::sqrt = 0;
const UTF8* N3::sin = 0;
const UTF8* N3::cos = 0;
const UTF8* N3::exp = 0;
const UTF8* N3::log = 0;
const UTF8* N3::floor = 0;
const UTF8* N3::log10 = 0;
const UTF8* N3::isNan = 0;
const UTF8* N3::pow = 0;
const UTF8* N3::floatName = 0;
const UTF8* N3::doubleName = 0;
const UTF8* N3::testInfinity = 0;

const llvm::Type* VMArray::llvmType;
const llvm::Type* VMObject::llvmType;
const llvm::Type* Enveloppe::llvmType;
const llvm::Type* CacheNode::llvmType;

llvm::Function* CLIJit::printExecutionLLVM;
llvm::Function* CLIJit::indexOutOfBoundsExceptionLLVM;
llvm::Function* CLIJit::nullPointerExceptionLLVM;
llvm::Function* CLIJit::classCastExceptionLLVM;
llvm::Function* CLIJit::clearExceptionLLVM;
llvm::Function* CLIJit::compareExceptionLLVM;
llvm::Function* CLIJit::arrayLengthLLVM;

#ifdef WITH_TRACER
llvm::Function* CLIJit::markAndTraceLLVM;
const llvm::FunctionType* CLIJit::markAndTraceLLVMType;
#endif
llvm::Function* CLIJit::vmObjectTracerLLVM;
llvm::Function* CLIJit::initialiseClassLLVM;
llvm::Function* CLIJit::virtualLookupLLVM;
llvm::Function* CLIJit::arrayConsLLVM;
llvm::Function* CLIJit::arrayMultiConsLLVM;
llvm::Function* CLIJit::objConsLLVM;
llvm::Function* CLIJit::objInitLLVM;
llvm::Function* CLIJit::throwExceptionLLVM;
llvm::Function* CLIJit::instanceOfLLVM;
llvm::Function* CLIJit::getCLIExceptionLLVM;
llvm::Function* CLIJit::getCppExceptionLLVM;
llvm::Function* CLIJit::newStringLLVM;


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



static void initialiseVT() {

# define INIT(X) { \
  X fake; \
  X::VT = ((VirtualTable**)(void*)(&fake))[0]; }
  
  INIT(Assembly);
  INIT(Header);
  INIT(Property);
  INIT(Param);
  INIT(Section);
  INIT(Table);
  INIT(VMArray);
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
  INIT(VMCommonClass);
  INIT(VMClass);
  INIT(VMGenericClass);
  INIT(VMClassArray);
  INIT(VMMethod);
  INIT(VMGenericMethod);
  INIT(VMField);
  INIT(VMCond);
  INIT(LockObj);
  INIT(VMObject);
  INIT(VMThread);
  //mvm::Key<VMThread>::VT = mvm::ThreadKey::VT;
  INIT(ThreadSystem);
  INIT(N3);
  INIT(Reader);
  INIT(UTF8Map);
  INIT(ClassNameMap);
  INIT(ClassTokenMap);
  INIT(FieldTokenMap);
  INIT(MethodTokenMap);
  INIT(StringMap);
  INIT(FunctionMap);
  INIT(VirtualMachine);
  INIT(CLIString);
  INIT(CLIJit);
  INIT(CacheNode);
  INIT(Enveloppe);
  INIT(Opinfo);
  INIT(Exception);

#undef INIT

}

VMThread* VMThread::TheThread = 0;

static void initialiseStatics() {
  CLIJit::initialise();

  VMObject::globalLock = new mvm::LockNormal();

  N3* vm = N3::bootstrapVM = N3::allocateBootstrap();
  VMThread::TheThread = VMThread::allocate(0, vm);
  
  
  vm->assemblyPath.push_back("");
  vm->assemblyPath.push_back(MSCorlib::libsPath);
  
  const UTF8* mscorlib = vm->asciizConstructUTF8("mscorlib");
  Assembly* ass = vm->loadAssembly(mscorlib, "dll");
  if (ass == 0)
    VMThread::get()->vm->error("can not load mscorlib.dll. Abort");

  vm->coreAssembly = ass;

  // Array initialization
  const UTF8* System = vm->asciizConstructUTF8("System");
  const UTF8* utf8OfChar = vm->asciizConstructUTF8("Char");
  
  MSCorlib::arrayChar = ass->constructArray(utf8OfChar, System, 1);
  ((UTF8*)System)->classOf = MSCorlib::arrayChar;
  ((UTF8*)utf8OfChar)->classOf = MSCorlib::arrayChar;
  ((UTF8*)mscorlib)->classOf = MSCorlib::arrayChar;

#define INIT(var, nameSpace, name, type, prim) {\
  var = (VMClass*)vm->coreAssembly->loadTypeFromName( \
                                           vm->asciizConstructUTF8(name),     \
                                           vm->asciizConstructUTF8(nameSpace),\
                                           false, false, false, true); \
  var->isPrimitive = prim; \
  if (type) { \
    var->naturalType = type;  \
    var->virtualType = type;  \
  }}

  INIT(MSCorlib::pObject,   "System", "Object", VMObject::llvmType, false);
  INIT(MSCorlib::pValue,    "System", "ValueType", 0, false);
  INIT(MSCorlib::pVoid,     "System", "Void", llvm::Type::VoidTy, true);
  INIT(MSCorlib::pBoolean,  "System", "Boolean", llvm::Type::Int1Ty, true);
  INIT(MSCorlib::pUInt8,    "System", "Byte", llvm::Type::Int8Ty, true);
  INIT(MSCorlib::pSInt8,    "System", "SByte", llvm::Type::Int8Ty, true);
  INIT(MSCorlib::pChar,     "System", "Char", llvm::Type::Int16Ty, true);
  INIT(MSCorlib::pSInt16,   "System", "Int16", llvm::Type::Int16Ty, true);
  INIT(MSCorlib::pUInt16,   "System", "UInt16", llvm::Type::Int16Ty, true);
  INIT(MSCorlib::pSInt32,   "System", "Int32", llvm::Type::Int32Ty, true);
  INIT(MSCorlib::pUInt32,   "System", "UInt32", llvm::Type::Int32Ty, true);
  INIT(MSCorlib::pSInt64,   "System", "Int64", llvm::Type::Int64Ty, true);
  INIT(MSCorlib::pUInt64,   "System", "UInt64", llvm::Type::Int64Ty, true);
  INIT(MSCorlib::pIntPtr,   "System", "IntPtr", llvm::PointerType::getUnqual(llvm::Type::Int8Ty), true);
  INIT(MSCorlib::pUIntPtr,  "System", "UIntPtr", llvm::PointerType::getUnqual(llvm::Type::Int8Ty), true);
  INIT(MSCorlib::pDouble,   "System", "Double", llvm::Type::DoubleTy, true);
  INIT(MSCorlib::pFloat,    "System", "Single", llvm::Type::FloatTy, true);
  INIT(MSCorlib::pEnum,     "System", "Enum", llvm::Type::Int32Ty, true);
  INIT(MSCorlib::pArray,    "System", "Array", 0, true);
  INIT(MSCorlib::pException,"System", "Exception", 0, false);
  INIT(MSCorlib::pDelegate, "System", "Delegate", 0, false);

#undef INIT
  

  MSCorlib::arrayChar->baseClass = MSCorlib::pChar;
  VMClassArray::SuperArray = MSCorlib::pArray;
  MSCorlib::arrayChar->super = MSCorlib::pArray;
  
  MSCorlib::loadStringClass(vm);

  MSCorlib::arrayString = ass->constructArray(vm->asciizConstructUTF8("String"),
                                        System, 1);
  MSCorlib::arrayString->baseClass = MSCorlib::pString;
  
  MSCorlib::arrayByte = ass->constructArray(vm->asciizConstructUTF8("Byte"),
                                        System, 1);
  MSCorlib::arrayByte->baseClass = MSCorlib::pUInt8;
  
  MSCorlib::arrayObject = ass->constructArray(vm->asciizConstructUTF8("Object"),
                                        System, 1);
  MSCorlib::arrayObject->baseClass = MSCorlib::pObject;
  

  N3::clinitName = vm->asciizConstructUTF8(".cctor");
  N3::ctorName = vm->asciizConstructUTF8(".ctor");
  N3::invokeName = vm->asciizConstructUTF8("Invoke");
  N3::math = vm->asciizConstructUTF8("Math");
  N3::system = vm->asciizConstructUTF8("System");
  N3::sqrt = vm->asciizConstructUTF8("Sqrt");
  N3::sin = vm->asciizConstructUTF8("Sin");
  N3::cos = vm->asciizConstructUTF8("Cos");
  N3::exp = vm->asciizConstructUTF8("Exp");
  N3::log = vm->asciizConstructUTF8("Log");
  N3::floor = vm->asciizConstructUTF8("Floor");
  N3::log10 = vm->asciizConstructUTF8("Log10");
  N3::isNan = vm->asciizConstructUTF8("IsNaN");
  N3::pow = vm->asciizConstructUTF8("Pow");
  N3::floatName = vm->asciizConstructUTF8("Float");
  N3::doubleName = vm->asciizConstructUTF8("Double");
  N3::testInfinity = vm->asciizConstructUTF8("TestInfinity");
  

  MSCorlib::initialise(vm);
   
}


mvm::CompilationUnit* mvm::VirtualMachine::initialiseCLIVM() {
  if (!N3::bootstrapVM) {
    initialiseVT();
    initialiseStatics();
  }
  return 0;
}

void VirtualMachine::runApplication(int argc, char** argv) {
  ((N3*)this)->runMain(argc, argv);
}

void VirtualMachine::compile(const char* argv) {
  assert(0 && "This virtual machine does not perform static compilation yet!\n");
}

mvm::VirtualMachine* mvm::VirtualMachine::createCLIVM(mvm::CompilationUnit* C) {
  N3* vm = N3::allocate("", N3::bootstrapVM);
  return vm;
}
