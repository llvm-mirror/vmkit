//===------- VirtualMachine.h - Virtual machine description ---------------===//
//
//                              N3
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef N3_VIRTUAL_MACHINE_H
#define N3_VIRTUAL_MACHINE_H

#include <vector>

#include "llvm/Function.h"

#include "mvm/JIT.h"
#include "mvm/Object.h"
#include "mvm/PrintBuffer.h"
#include "mvm/VirtualMachine.h"
#include "mvm/Threads/Cond.h"
#include "mvm/Threads/Locks.h"

#include "types.h"

namespace n3 {

class FunctionMap;
class N3ModuleProvider;
class UTF8;
class UTF8Map;
class VMObject;
class VMThread;

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

class VirtualMachine : public mvm::VirtualMachine {
public:
  static VirtualTable* VT;
  ThreadSystem* threadSystem; 
 
  const UTF8*   asciizConstructUTF8(const char* asciiz);
  const UTF8*   readerConstructUTF8(const uint16* buf, uint32 len);
  UTF8Map * hashUTF8;

  
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
  
  /*
  void illegalAccessException(const char* msg);
  void initializerError(const VMObject* excp);
  void invocationTargetException(const VMObject* obj);
  void classCastException(const char* msg);
  void errorWithExcp(const char* className, const VMObject* excp);*/
  void illegalArgumentException(const char* msg);
  void arrayStoreException();
  void illegalMonitorStateException(const VMObject* obj);
  void interruptedException(const VMObject* obj);
  void nullPointerException(const char* fmt, ...);
  void outOfMemoryError(sint32 n);
  void indexOutOfBounds(const VMObject* obj, sint32 entry);
  void negativeArraySizeException(int size);
  void unknownError(const char* fmt, ...); 
  void error(const char* fmt, ...);
  void error(const char* className, const char* fmt, ...);
  void error(const char* className, const char* fmt, va_list ap);

  
  virtual void TRACER;
  virtual void print(mvm::PrintBuffer* buf) const {
    buf->write("Virtual Machine<>");
  }

  ~VirtualMachine();
  VirtualMachine();
  
  mvm::Lock* protectModule;
  FunctionMap* functions;
  mvm::MvmModule* module;
  N3ModuleProvider* TheModuleProvider;
  VMThread* bootstrapThread;

  virtual void runApplication(int argc, char** argv);

};

} // end namespace n3

#endif
