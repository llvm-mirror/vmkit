//===---------------- N3.h - The N3 virtual machine -----------------------===//
//
//                              N3
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//


#ifndef N3_N3_H
#define N3_N3_H

#include "types.h"

#include <vector>

#include "llvm/Function.h"

#include "mvm/JIT.h"
#include "mvm/Object.h"
#include "mvm/PrintBuffer.h"
#include "mvm/Threads/Cond.h"
#include "mvm/Threads/Locks.h"

#include "types.h"

namespace n3 {

class ArrayObject;
class ArrayUInt8;
class Assembly;
class AssemblyMap;
class N3;
class N3ModuleProvider;
class StringMap;
class UTF8;
class UTF8Map;
class VMClass;
class VMClassArray;
class VMCommonClass;
class VMField;
class VMMethod;
class FunctionMap;
class N3ModuleProvider;
class UTF8;
class UTF8Map;
class VMMethod;
class VMObject;
class VMThread;
class ArrayUInt16;
class CLIString;

class ThreadSystem : public mvm::Object {
public:
  static VirtualTable* VT;
  uint16 nonDaemonThreads;
  mvm::Lock* nonDaemonLock;
  mvm::Cond* nonDaemonVar;

  virtual void print(mvm::PrintBuffer* buf) const;
  virtual void TRACER;

  static ThreadSystem* allocateThreadSystem();
};

class ClArgumentsInfo {
public:
  int argc;
  char** argv;
  uint32 appArgumentsPos;
  char* assembly;

  void readArgs(int argc, char** argv, N3 *vm);

  void printInformation();
  void nyi();
  void printVersion();
};


class N3 : public mvm::VirtualMachine {
public:
	// instance fields
  const char*              name;

  ClArgumentsInfo          argumentsInfo;
  AssemblyMap*             loadedAssemblies;
  std::vector<const char*> assemblyPath;
  Assembly*                coreAssembly;

  ThreadSystem*            threadSystem; 
  VMThread*                bootstrapThread;

  StringMap*               hashStr;
  UTF8Map*                 hashUTF8;

  mvm::Lock*               protectModule;
  FunctionMap*             functions;

  mvm::MvmModule*          module;
  llvm::Module*            LLVMModule;
  N3ModuleProvider*        TheModuleProvider;

	// constructors / destructors
	N3(mvm::BumpPtrAllocator &allocator, const char *name);
  ~N3();

	// virtual methods
  virtual void print(mvm::PrintBuffer* buf) const;
  virtual void TRACER;
  virtual void runApplication(int argc, char** argv);
  virtual void compile(const char* name);
  virtual void waitForExit();

	// non virtual methods
  ArrayUInt8*   openAssembly(const UTF8* name, const char* extension);
  Assembly*     loadAssembly(const UTF8* name, const char* extension);
  void          executeAssembly(const char* name, ArrayObject* args);
  void          runMain(int argc, char** argv);

  VMMethod*     lookupFunction(llvm::Function* F);

  llvm::Module* getLLVMModule() { return LLVMModule; }  
  

  Assembly*     constructAssembly(const UTF8* name);
  Assembly*     lookupAssembly(const UTF8* name);

	// usefull string, uint16 and utf8 functions

	char*            arrayToAsciiz(const ArrayUInt16 *array);
	ArrayUInt16*     asciizToArray(const char *asciiz);          // done
	ArrayUInt16*     bufToArray(const uint16 *buf, uint32 len);  // done
	ArrayUInt16*     UTF8ToArray(const UTF8 *utf8);              // done
	const UTF8*      asciizToUTF8(const char *asciiz);           // done
	const UTF8*      bufToUTF8(const uint16 *buf, uint32 len);   // done
	const UTF8*      arrayToUTF8(const ArrayUInt16 *array);      // done
	CLIString*       arrayToString(const ArrayUInt16 *array);

  /*
  void          illegalAccessException(const char* msg);
  void          initializerError(const VMObject* excp);
  void          invocationTargetException(const VMObject* obj);
  void          classCastException(const char* msg);
  void          errorWithExcp(const char* className, const VMObject* excp);*/
  void          illegalArgumentException(const char* msg);
  void          arrayStoreException();
  void          illegalMonitorStateException(const VMObject* obj);
  void          interruptedException(const VMObject* obj);
  void          nullPointerException(const char* fmt, ...);
  void          outOfMemoryError(sint32 n);
  void          indexOutOfBounds(const VMObject* obj, sint32 entry);
  void          negativeArraySizeException(int size);
  void          unknownError(const char* fmt, ...); 

  void          error(const char* fmt, ...);
  void          error(const char* className, const char* fmt, ...);
  void          error(const char* className, const char* fmt, va_list ap);

	// static fields
  static N3*         bootstrapVM;
 
  static const UTF8* clinitName;
  static const UTF8* ctorName;
  static const UTF8* invokeName;
  static const UTF8* math;
  static const UTF8* system;
  static const UTF8* sqrt;
  static const UTF8* sin;
  static const UTF8* cos;
  static const UTF8* exp;
  static const UTF8* log;
  static const UTF8* floor;
  static const UTF8* log10;
  static const UTF8* isNan;
  static const UTF8* pow;
  static const UTF8* floatName;
  static const UTF8* doubleName;
  static const UTF8* testInfinity;

  // Exceptions name
  static const char* SystemException;
  static const char* OverFlowException;
  static const char* OutOfMemoryException;
  static const char* IndexOutOfRangeException;
  static const char* SynchronizationLocException;
  static const char* NullReferenceException;
  static const char* ThreadInterruptedException;
  static const char* MissingMethodException;
  static const char* MissingFieldException;
  static const char* ArrayTypeMismatchException;
  static const char* ArgumentException;
  /*static const char* ArithmeticException;
  static const char* ClassNotFoundException;
  static const char* InvocationTargetException;
  static const char* ClassCastException;
  static const char* ArrayIndexOutOfBoundsException;
  static const char* SecurityException;
  static const char* ClassFormatError;
  static const char* ClassCircularityError;
  static const char* NoClassDefFoundError;
  static const char* UnsupportedClassVersionError;
  static const char* NoSuchFieldError;
  static const char* NoSuchMethodError;
  static const char* InstantiationError;
  static const char* IllegalAccessError;
  static const char* IllegalAccessException;
  static const char* VerifyError;
  static const char* ExceptionInInitializerError;
  static const char* LinkageError;
  static const char* AbstractMethodError;
  static const char* UnsatisfiedLinkError;
  static const char* InternalError;
  static const char* StackOverflowError;
  */
  // Exceptions

	// static methods
  static N3* allocateBootstrap();
  static N3* allocate(const char* name, N3* parent);
  static void mainCLIStart(VMThread* th);
};

} // end namespace n3

#endif
