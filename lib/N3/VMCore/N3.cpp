//===--------------- N3.cpp - The N3 virtual machine ----------------------===//
//
//                              N3
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//


#include <vector>
#include <stdarg.h>

#include "llvm/Function.h"
#include "llvm/Module.h"
#include "llvm/LLVMContext.h"
#include "llvm/Support/CommandLine.h"

#include "mvm/Object.h"
#include "mvm/PrintBuffer.h"
#include "mvm/Threads/Cond.h"
#include "mvm/Threads/Locks.h"
#include "mvm/JIT.h"

#include "types.h"

#include "Assembly.h"
#include "LinkN3Runtime.h"
#include "LockedMap.h"
#include "MSCorlib.h"
#include "N3.h"
#include "N3ModuleProvider.h"
#include "Reader.h"
#include "VMArray.h"
#include "VMClass.h"
#include "VMObject.h"
#include "VMThread.h"
#include "CLIJit.h"
#include "CLIString.h"


using namespace n3;

#define DECLARE_EXCEPTION(EXCP) \
  const char* N3::EXCP = #EXCP

DECLARE_EXCEPTION(SystemException);
DECLARE_EXCEPTION(OverFlowException);
DECLARE_EXCEPTION(OutOfMemoryException);
DECLARE_EXCEPTION(IndexOutOfRangeException);
DECLARE_EXCEPTION(NullReferenceException);
DECLARE_EXCEPTION(SynchronizationLocException);
DECLARE_EXCEPTION(ThreadInterruptedException);
DECLARE_EXCEPTION(MissingMethodException);
DECLARE_EXCEPTION(MissingFieldException);
DECLARE_EXCEPTION(ArrayTypeMismatchException);
DECLARE_EXCEPTION(ArgumentException);

/*
DECLARE_EXCEPTION(ArithmeticException);
DECLARE_EXCEPTION(InvocationTargetException);
DECLARE_EXCEPTION(ArrayStoreException);
DECLARE_EXCEPTION(ClassCastException);
DECLARE_EXCEPTION(ArrayIndexOutOfBoundsException);
DECLARE_EXCEPTION(SecurityException);
DECLARE_EXCEPTION(ClassFormatError);
DECLARE_EXCEPTION(ClassCircularityError);
DECLARE_EXCEPTION(NoClassDefFoundError);
DECLARE_EXCEPTION(UnsupportedClassVersionError);
DECLARE_EXCEPTION(NoSuchFieldError);
DECLARE_EXCEPTION(NoSuchMethodError);
DECLARE_EXCEPTION(InstantiationError);
DECLARE_EXCEPTION(IllegalAccessError);
DECLARE_EXCEPTION(IllegalAccessException);
DECLARE_EXCEPTION(VerifyError);
DECLARE_EXCEPTION(ExceptionInInitializerError);
DECLARE_EXCEPTION(LinkageError);
DECLARE_EXCEPTION(AbstractMethodError);
DECLARE_EXCEPTION(UnsatisfiedLinkError);
DECLARE_EXCEPTION(InternalError);
DECLARE_EXCEPTION(StackOverflowError);
DECLARE_EXCEPTION(ClassNotFoundException);
*/

#undef DECLARE_EXCEPTION

void ThreadSystem::print(mvm::PrintBuffer* buf) const {
  buf->write("ThreadSystem<>");
}

ThreadSystem* ThreadSystem::allocateThreadSystem() {
  ThreadSystem* res = gc_new(ThreadSystem)();
  res->nonDaemonThreads = 1;
  res->nonDaemonLock = new mvm::LockNormal();
  res->nonDaemonVar  = new mvm::Cond();
  return res;
}

N3::N3(mvm::BumpPtrAllocator &allocator, const char *name) : mvm::VirtualMachine(allocator) {
  this->module =            0;
  this->TheModuleProvider = 0;
	this->name =              name;

  this->scanner =           new mvm::UnpreciseStackScanner(); 
  this->LLVMModule =        new llvm::Module(name, llvm::getGlobalContext());
  this->module =            new mvm::MvmModule(this->LLVMModule);

  this->LLVMModule->setDataLayout(mvm::MvmModule::executionEngine->getTargetData()->getStringRepresentation());
  this->protectModule =     new mvm::LockNormal();

  this->functions =         new(allocator, "FunctionMap") FunctionMap();
  this->hashStr =           new(allocator, "StringMap")   StringMap();
  this->loadedAssemblies =  new(allocator, "AssemblyMap") AssemblyMap();

  this->TheModuleProvider = new N3ModuleProvider(this->LLVMModule, this->functions);
}

N3::~N3() {
  delete module;
  delete TheModuleProvider;
}

void N3::error(const char* className, const char* fmt, va_list ap) {
  fprintf(stderr, "Internal exception of type %s during bootstrap: ", className);
  vfprintf(stderr, fmt, ap);
  throw 1;
}

void N3::indexOutOfBounds(const VMObject* obj, sint32 entry) {
  error(IndexOutOfRangeException, "%d", entry);
}

void N3::negativeArraySizeException(sint32 size) {
  error(OverFlowException, "%d", size);
}

void N3::nullPointerException(const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  error(NullReferenceException, fmt, va_arg(ap, char*));
}


void N3::illegalMonitorStateException(const VMObject* obj) {
  error(SynchronizationLocException, "");
}

void N3::interruptedException(const VMObject* obj) {
  error(ThreadInterruptedException, "");
}

void N3::outOfMemoryError(sint32 n) {
  error(OutOfMemoryException, "");
}

void N3::arrayStoreException() {
  error(ArrayTypeMismatchException, "");
}

void N3::illegalArgumentException(const char* name) {
  error(ArgumentException, name);
}

void N3::unknownError(const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  error(SystemException, fmt, ap);
}

void N3::error(const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  error(SystemException, fmt, ap);
}

void N3::error(const char* name, const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  error(name, fmt, ap);
}




using namespace n3;

void N3::print(mvm::PrintBuffer* buf) const {
  buf->write("N3 virtual machine<>");
}

static Assembly* assemblyDup(const UTF8*& name, N3* vm) {
	mvm::BumpPtrAllocator *a = new mvm::BumpPtrAllocator();
  return new(*a, "Assembly") Assembly(*a, name);
}

Assembly* N3::constructAssembly(const UTF8* name) {
  return loadedAssemblies->lookupOrCreate(name, this, assemblyDup);
}

Assembly* N3::lookupAssembly(const UTF8* name) {
  return loadedAssemblies->lookup(name);
}

VMMethod* N3::lookupFunction(Function* F) {
  return functions->lookup(F);
}


N3* N3::allocateBootstrap() {
  mvm::BumpPtrAllocator *a = new mvm::BumpPtrAllocator();
  N3 *vm= new(*a, "VM") N3(*a, "bootstrapN3");
  
  vm->hashUTF8 =         new(vm->allocator, "UTF8Map")     UTF8Map();

  CLIJit::initialiseBootstrapVM(vm);
  
  return vm;
}


N3* N3::allocate(const char* name, N3* parent) {
  mvm::BumpPtrAllocator *a = new mvm::BumpPtrAllocator();
  N3 *vm= new(*a, "VM") N3(*a, name);

  vm->hashUTF8 = parent->hashUTF8;
  
  vm->threadSystem = ThreadSystem::allocateThreadSystem();

  vm->assemblyPath = parent->assemblyPath;
  vm->coreAssembly = parent->coreAssembly;
  vm->loadedAssemblies->hash(parent->coreAssembly->name, parent->coreAssembly);

  CLIJit::initialiseAppDomain(vm);
  
  return vm; 
}

ArrayUInt8* N3::openAssembly(const UTF8* name, const char* ext) {
  char* asciiz = name->UTF8ToAsciiz();
  uint32 alen = strlen(asciiz);
 
  ArrayUInt8* res = 0;
  uint32 idx = 0;

  while ((res == 0) && (idx < assemblyPath.size())) {
    const char* cur = assemblyPath[idx];
    uint32 strLen = strlen(cur);
    char* buf = (char*)alloca(strLen + alen + 16);

    if (ext != 0) {
      sprintf(buf, "%s%s.%s", cur, asciiz, ext);
    } else {
      sprintf(buf, "%s%s", cur, asciiz);
    }
    
    res = Reader::openFile(buf);
    ++idx;
  }

  return res;
}

void ClArgumentsInfo::nyi() {
  fprintf(stdout, "Not yet implemented\n");
}

void ClArgumentsInfo::printVersion() {
  fprintf(stdout, "N3 -- a VVM implementation of the Common Language Infrastructure\n");
}

void ClArgumentsInfo::printInformation() {
  fprintf(stdout, 
  "Usage: n3 [-options] assembly [args...] \n"
    "No option is available\n");
}

void ClArgumentsInfo::readArgs(int argc, char** argv, N3* n3) {
  assembly = 0;
  appArgumentsPos = 0;
  sint32 i = 1;
  if (i == argc) printInformation();
  while (i < argc) {
    char* cur = argv[i];
    if (cur[0] == '-') {
    } else {
      assembly = cur;
      appArgumentsPos = i;
      return;
    }
    ++i;
  }
}


void N3::waitForExit() { 
  threadSystem->nonDaemonLock->lock();
  
  while (threadSystem->nonDaemonThreads) {
    threadSystem->nonDaemonVar->wait(threadSystem->nonDaemonLock);
  }

  threadSystem->nonDaemonLock->unlock();

  return;
}

Assembly* N3::loadAssembly(const UTF8* name, const char* ext) {
  Assembly* ass = lookupAssembly(name);
  if (ass == 0 || !ass->isRead) {
    ArrayUInt8* bytes = openAssembly(name, ext);
    if (bytes != 0) {
      if (ass == 0) ass = constructAssembly(name);
      ass->bytes = bytes;
      ass->vm = this;
      ass->read();
      ass->isRead = true;
    }
  }
  return ass;
}

void N3::executeAssembly(const char* _name, ArrayObject* args) {
  const UTF8* name = asciizToUTF8(_name);
  Assembly* assembly = loadAssembly(name, 0);
  if (assembly == 0) {
    error("Can not find assembly %s", _name);
  } else {
    uint32 entryPoint = assembly->entryPoint;
    uint32 table = entryPoint >> 24;
    if (table != CONSTANT_MethodDef) {
      error("Entry point does not point to a method");
    } else {
      uint32 typeToken = assembly->getTypeDefTokenFromMethod(entryPoint);
      assembly->loadType(this, typeToken, true, true, true ,true, NULL, NULL);
      VMMethod* mainMeth = assembly->lookupMethodFromToken(entryPoint);
      (*mainMeth)(args);
    }
  }
}

void N3::runMain(int argc, char** argv) {
  ClArgumentsInfo& info = argumentsInfo;

  info.readArgs(argc, argv, this);
  if (info.assembly) {
    info.argv = argv + info.appArgumentsPos - 1;
    info.argc = argc - info.appArgumentsPos + 1;
    
    
    bootstrapThread = VMThread::TheThread;
    bootstrapThread->vm = this;
    bootstrapThread->start((void (*)(mvm::Thread*))mainCLIStart);

  } else {
    --(threadSystem->nonDaemonThreads);
  }
}

void N3::mainCLIStart(VMThread* th) {
  N3* vm = (N3*)th->vm;
  MSCorlib::loadBootstrap(vm);
  
  ClArgumentsInfo& info = vm->argumentsInfo;  
  ArrayObject* args = (ArrayObject*)MSCorlib::arrayString->doNew(info.argc-2);
  for (int i = 2; i < info.argc; ++i) {
    args->setAt(i - 2, (VMObject*)vm->arrayToString(vm->asciizToArray(info.argv[i])));
  }
  
  try{
    vm->executeAssembly(info.assembly, args);
  }catch(...) {
    VMObject* exc = th->pendingException;
    printf("N3 caught %s\n", exc->printString());
  }

  vm->threadSystem->nonDaemonLock->lock();
  --(vm->threadSystem->nonDaemonThreads);
  if (vm->threadSystem->nonDaemonThreads == 0)
    vm->threadSystem->nonDaemonVar->signal();
  vm->threadSystem->nonDaemonLock->unlock();
}



ArrayUInt16* N3::asciizToArray(const char* asciiz) {
	uint32 len = strlen(asciiz);
	ArrayUInt16 *res = (ArrayUInt16*)MSCorlib::arrayChar->doNew(len);
	for(uint32 i=0; i<len; i++)
		res->elements[i] = asciiz[i];
	return res;
}

ArrayUInt16* N3::bufToArray(const uint16* buf, uint32 size) {
	ArrayUInt16 *res = (ArrayUInt16*)MSCorlib::arrayChar->doNew(size);
	memcpy(res->elements, buf, size<<1);
	return res;
}

ArrayUInt16* N3::UTF8ToArray(const UTF8 *utf8) {
  return bufToArray(utf8->elements, utf8->size);
}

const UTF8* N3::asciizToUTF8(const char* asciiz) {
  return hashUTF8->lookupOrCreateAsciiz(asciiz);
}

const UTF8* N3::bufToUTF8(const uint16* buf, uint32 len) {
  return hashUTF8->lookupOrCreateReader(buf, len);
}

const UTF8* N3::arrayToUTF8(const ArrayUInt16 *array) {
  return bufToUTF8(array->elements, array->size);
}

CLIString *N3::arrayToString(const ArrayUInt16 *array) {
  return (CLIString*)CLIString::stringDup(array, this);
}

char* N3::arrayToAsciiz(const ArrayUInt16 *array) {
	int size = array->size;
  mvm::NativeString* buf = mvm::NativeString::alloc(size + 1);
  for (sint32 i = 0; i < size; ++i) {
    buf->setAt(i, array->elements[i]);
  }
  buf->setAt(size, 0);
  return buf->cString();
}

#include "MSCorlib.inc"
