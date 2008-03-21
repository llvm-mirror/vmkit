//===----------- N3Initialise.cpp - Initialization of N3 ------------------===//
//
//                              N3
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//


#include <vector>

#include "mvm/JIT.h"
#include "mvm/Threads/Locks.h"

#include "Assembly.h"
#include "CLIString.h"
#include "CLIJit.h"
#include "LockedMap.h"
#include "NativeUtil.h"
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
mvm::Key<VMThread>* VMThread::threadKey = 0;

VMCommonClass* VMClassArray::SuperArray = 0;
std::vector<VMClass*> VMClassArray::InterfacesArray;
std::vector<VMMethod*> VMClassArray::VirtualMethodsArray;
std::vector<VMMethod*> VMClassArray::StaticMethodsArray;
std::vector<VMField*> VMClassArray::VirtualFieldsArray;
std::vector<VMField*> VMClassArray::StaticFieldsArray;

VMClass* N3::pVoid = 0;
VMClass* N3::pBoolean= 0;
VMClass* N3::pChar = 0;
VMClass* N3::pSInt8 = 0;
VMClass* N3::pUInt8 = 0;
VMClass* N3::pSInt16 = 0;
VMClass* N3::pUInt16 = 0;
VMClass* N3::pSInt32 = 0;
VMClass* N3::pUInt32 = 0;
VMClass* N3::pSInt64 = 0;
VMClass* N3::pUInt64 = 0;
VMClass* N3::pFloat = 0;
VMClass* N3::pDouble = 0;
VMClass* N3::pIntPtr = 0;
VMClass* N3::pUIntPtr = 0;
VMClass* N3::pObject = 0;
VMClass* N3::pString = 0;
VMClass* N3::pValue = 0;
VMClass* N3::pEnum = 0;
VMClass* N3::pArray = 0;
VMClass* N3::pDelegate = 0;
VMClass* N3::pException = 0;
VMClassArray* N3::arrayChar = 0;
VMClassArray* N3::arrayString = 0;
VMClassArray* N3::arrayObject = 0;
VMClassArray* N3::arrayByte = 0;
VMMethod* N3::ctorPropertyType;
VMMethod* N3::ctorMethodType;
VMMethod* N3::ctorClrType;
VMClass* N3::clrType;
VMField* N3::typeClrType;
VMField* N3::propertyPropertyType;
VMField* N3::methodMethodType;
VMMethod* N3::ctorAssemblyReflection;
VMClass* N3::assemblyReflection;
VMClass* N3::typedReference;
VMField* N3::assemblyAssemblyReflection;
VMClass* N3::propertyType;
VMClass* N3::methodType;
VMClass* N3::resourceStreamType;
VMMethod* N3::ctorResourceStreamType;

VMField* N3::ctorBoolean;
VMField* N3::ctorUInt8;
VMField* N3::ctorSInt8;
VMField* N3::ctorChar;
VMField* N3::ctorSInt16;
VMField* N3::ctorUInt16;
VMField* N3::ctorSInt32;
VMField* N3::ctorUInt32;
VMField* N3::ctorSInt64;
VMField* N3::ctorUInt64;
VMField* N3::ctorIntPtr;
VMField* N3::ctorUIntPtr;
VMField* N3::ctorDouble;
VMField* N3::ctorFloat;

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

llvm::Function* CLIJit::markAndTraceLLVM;
const llvm::FunctionType* CLIJit::markAndTraceLLVMType;
llvm::Function* CLIJit::vmObjectTracerLLVM;
llvm::Function* CLIJit::initialiseClassLLVM;
llvm::Function* CLIJit::virtualLookupLLVM;
llvm::Function* CLIJit::arrayConsLLVM;
llvm::Function* CLIJit::arrayMultiConsLLVM;
llvm::Function* CLIJit::objConsLLVM;
llvm::Function* CLIJit::objInitLLVM;
llvm::Function* CLIJit::throwExceptionLLVM;
llvm::Function* CLIJit::instanceOfLLVM;
llvm::Function* CLIJit::isInCodeLLVM;
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
  X::VT = ((void**)(void*)(&fake))[0]; }
  
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
  INIT(VMClassArray);
  INIT(VMMethod);
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

static void loadStringClass(N3* vm) {
  VMClass* type = (VMClass*)vm->coreAssembly->loadTypeFromName(
                                           vm->asciizConstructUTF8("String"),
                                           vm->asciizConstructUTF8("System"),
                                           false, false, false, true);
  N3::pString = type;
  type->resolveType(false, false);

  uint64 size = mvm::jit::getTypeSize(type->virtualType->getContainedType(0)) + sizeof(const UTF8*) + sizeof(llvm::GlobalVariable*);
  type->virtualInstance = 
    (VMObject*)mvm::Object::gcmalloc(size, type->virtualInstance->getVirtualTable());
  type->virtualInstance->initialise(type);
}

static void initialiseStatics() {
  NativeUtil::initialise();
  CLIJit::initialise();

  VMObject::globalLock = mvm::Lock::allocNormal();
  //mvm::Object::pushRoot((mvm::Object*)VMObject::globalLock);

  VMThread::threadKey = new mvm::Key<VMThread>();
  //mvm::Object::pushRoot((mvm::Object*)VMThread::threadKey);
  
  N3* vm = N3::bootstrapVM = N3::allocateBootstrap();
  mvm::Object::pushRoot((mvm::Object*)N3::bootstrapVM);

  
  
  char* assemblyName = getenv("MSCORLIB");
  if (assemblyName == 0)
    VMThread::get()->vm->error("can not find mscorlib.dll. Abort");

  vm->assemblyPath.push_back("");
  vm->assemblyPath.push_back(assemblyName);
  
  const UTF8* mscorlib = vm->asciizConstructUTF8("mscorlib");
  Assembly* ass = vm->loadAssembly(mscorlib, "dll");
  if (ass == 0)
    VMThread::get()->vm->error("can not find mscorlib.dll. Abort");

  vm->coreAssembly = ass;

  // Array initialization
  const UTF8* System = vm->asciizConstructUTF8("System");
  const UTF8* utf8OfChar = vm->asciizConstructUTF8("Char");
  
  N3::arrayChar = ass->constructArray(utf8OfChar, System, 1);
  ((UTF8*)System)->classOf = N3::arrayChar;
  ((UTF8*)utf8OfChar)->classOf = N3::arrayChar;
  ((UTF8*)mscorlib)->classOf = N3::arrayChar;

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

  INIT(N3::pObject,   "System", "Object", VMObject::llvmType, false);
  INIT(N3::pValue,    "System", "ValueType", 0, false);
  INIT(N3::pVoid,     "System", "Void", llvm::Type::VoidTy, true);
  INIT(N3::pBoolean,  "System", "Boolean", llvm::Type::Int1Ty, true);
  INIT(N3::pUInt8,    "System", "Byte", llvm::Type::Int8Ty, true);
  INIT(N3::pSInt8,    "System", "SByte", llvm::Type::Int8Ty, true);
  INIT(N3::pChar,     "System", "Char", llvm::Type::Int16Ty, true);
  INIT(N3::pSInt16,   "System", "Int16", llvm::Type::Int16Ty, true);
  INIT(N3::pUInt16,   "System", "UInt16", llvm::Type::Int16Ty, true);
  INIT(N3::pSInt32,   "System", "Int32", llvm::Type::Int32Ty, true);
  INIT(N3::pUInt32,   "System", "UInt32", llvm::Type::Int32Ty, true);
  INIT(N3::pSInt64,   "System", "Int64", llvm::Type::Int64Ty, true);
  INIT(N3::pUInt64,   "System", "UInt64", llvm::Type::Int64Ty, true);
  INIT(N3::pIntPtr,   "System", "IntPtr", llvm::PointerType::getUnqual(llvm::Type::Int8Ty), true);
  INIT(N3::pUIntPtr,  "System", "UIntPtr", llvm::PointerType::getUnqual(llvm::Type::Int8Ty), true);
  INIT(N3::pDouble,   "System", "Double", llvm::Type::DoubleTy, true);
  INIT(N3::pFloat,    "System", "Single", llvm::Type::FloatTy, true);
  INIT(N3::pEnum,     "System", "Enum", llvm::Type::Int32Ty, true);
  INIT(N3::pArray,    "System", "Array", 0, true);
  INIT(N3::pException,"System", "Exception", 0, false);
  INIT(N3::pDelegate, "System", "Delegate", 0, false);
  INIT(N3::clrType,   "System.Reflection", "ClrType", 0, false);
  INIT(N3::assemblyReflection,   "System.Reflection", "Assembly", 0, false);
  INIT(N3::typedReference,   "System", "TypedReference", 0, false);
  INIT(N3::propertyType,   "System.Reflection", "ClrProperty", 0, false);
  INIT(N3::methodType,   "System.Reflection", "ClrMethod", 0, false);
  INIT(N3::resourceStreamType,   "System.Reflection", "ClrResourceStream", 0, false);

#undef INIT
  

  N3::arrayChar->baseClass = N3::pChar;
  VMClassArray::SuperArray = N3::pArray;
  N3::arrayChar->super = N3::pArray;
  
  loadStringClass(vm);
  
  N3::arrayString = ass->constructArray(vm->asciizConstructUTF8("String"),
                                        System, 1);
  N3::arrayString->baseClass = N3::pString;
  
  N3::arrayByte = ass->constructArray(vm->asciizConstructUTF8("Byte"),
                                        System, 1);
  N3::arrayByte->baseClass = N3::pUInt8;
  
  N3::arrayObject = ass->constructArray(vm->asciizConstructUTF8("Object"),
                                        System, 1);
  N3::arrayObject->baseClass = N3::pObject;
  

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
  
  {
  N3::clrType->resolveType(false, false);
  std::vector<VMCommonClass*> args;
  args.push_back(N3::pVoid);
  args.push_back(N3::clrType);
  N3::ctorClrType = N3::clrType->lookupMethod(vm->asciizConstructUTF8(".ctor"), args, false, false);
  N3::typeClrType = N3::clrType->lookupField(vm->asciizConstructUTF8("privateData"), N3::pIntPtr, false, false);
  }

  {
  N3::assemblyReflection->resolveType(false, false);
  std::vector<VMCommonClass*> args;
  args.push_back(N3::pVoid);
  args.push_back(N3::assemblyReflection);
  N3::ctorAssemblyReflection = N3::assemblyReflection->lookupMethod(vm->asciizConstructUTF8(".ctor"), args, false, false);
  N3::assemblyAssemblyReflection = N3::assemblyReflection->lookupField(vm->asciizConstructUTF8("privateData"), N3::pIntPtr, false, false);
  }
  
  {
  N3::propertyType->resolveType(false, false);
  std::vector<VMCommonClass*> args;
  args.push_back(N3::pVoid);
  args.push_back(N3::propertyType);
  N3::ctorPropertyType = N3::propertyType->lookupMethod(vm->asciizConstructUTF8(".ctor"), args, false, false);
  N3::propertyPropertyType = N3::propertyType->lookupField(vm->asciizConstructUTF8("privateData"), N3::pIntPtr, false, false);
  }
  
  {
  N3::methodType->resolveType(false, false);
  std::vector<VMCommonClass*> args;
  args.push_back(N3::pVoid);
  args.push_back(N3::methodType);
  N3::ctorMethodType = N3::methodType->lookupMethod(vm->asciizConstructUTF8(".ctor"), args, false, false);
  N3::methodMethodType = N3::methodType->lookupField(vm->asciizConstructUTF8("privateData"), N3::pIntPtr, false, false);
  }
  
  {
  N3::resourceStreamType->resolveType(false, false);
  std::vector<VMCommonClass*> args;
  args.push_back(N3::pVoid);
  args.push_back(N3::resourceStreamType);
  args.push_back(N3::pIntPtr);
  args.push_back(N3::pSInt64);
  args.push_back(N3::pSInt64);
  N3::ctorResourceStreamType = N3::resourceStreamType->lookupMethod(vm->asciizConstructUTF8(".ctor"), args, false, false);
  }
  
  VMCommonClass* voidPtr = vm->coreAssembly->constructPointer(N3::pVoid, 1);
#define INIT(var, cl, type) {\
    cl->resolveType(false, false); \
    var = cl->lookupField(vm->asciizConstructUTF8("value_"), type, false, false); \
  }
  
  INIT(N3::ctorBoolean,  N3::pBoolean, N3::pBoolean);
  INIT(N3::ctorUInt8, N3::pUInt8, N3::pUInt8);
  INIT(N3::ctorSInt8, N3::pSInt8, N3::pSInt8);
  INIT(N3::ctorChar,  N3::pChar, N3::pChar);
  INIT(N3::ctorSInt16, N3::pSInt16, N3::pSInt16);
  INIT(N3::ctorUInt16, N3::pUInt16, N3::pUInt16);
  INIT(N3::ctorSInt32, N3::pSInt32, N3::pSInt32);
  INIT(N3::ctorUInt32, N3::pUInt32, N3::pUInt32);
  INIT(N3::ctorSInt64, N3::pSInt64, N3::pSInt64);
  INIT(N3::ctorUInt64, N3::pUInt64, N3::pUInt64);
  INIT(N3::ctorIntPtr, N3::pIntPtr, voidPtr);
  INIT(N3::ctorUIntPtr, N3::pUIntPtr, voidPtr);
  INIT(N3::ctorDouble, N3::pDouble, N3::pDouble);
  INIT(N3::ctorFloat, N3::pFloat, N3::pFloat);

#undef INIT
}


extern "C" int boot(int argc, char **argv, char **envp) {
  initialiseVT();
  initialiseStatics();
  return 0;
}

extern "C" int start_app(int argc, char** argv) {
  N3* vm = N3::allocate("", N3::bootstrapVM);
  vm->runMain(argc, argv);
  return 0; 
}
