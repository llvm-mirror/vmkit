//===----------- JavaObject.h - Java object definition -------------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef JNJVM_JAVA_OBJECT_H
#define JNJVM_JAVA_OBJECT_H

#include "mvm/Object.h"
#include "mvm/Threads/Locks.h"
#include "mvm/Threads/Thread.h"

#include "types.h"

#include "JavaLocks.h"
#include "JnjvmConfig.h"

namespace jnjvm {

class JavaObject;
class JavaThread;
class Jnjvm;
class Typedef;
class UserCommonClass;

/// JavaVirtualTable - This class is the virtual table of instances of
/// Java classes. Besides holding function pointers for virtual calls,
/// it contains a bunch of information useful for fast dynamic type checking.
/// These are placed here for fast access of information from a Java object
/// (that only points to the VT, not the class).
///
class JavaVirtualTable : public VirtualTable {
public:

  /// cl - The class which defined this virtual table.
  ///
  CommonClass* cl;

  /// depth - The super hierarchy depth of the class.
  ///
  size_t depth;

  /// offset - Offset in the virtual table where this virtual
  /// table may be pointed. The offset is the cache if the class
  /// is an interface or depth is too big, or an offset in the display.
  ///
  size_t offset;

  /// cache - The cached result for better type checks on secondary types.
  ///
  JavaVirtualTable* cache;

  /// display - Array of super classes.
  ///
  JavaVirtualTable* display[8];

  /// nbSecondaryTypes - The length of the secondary type list.
  ///
  size_t nbSecondaryTypes;

  /// secondaryTypes - The list of secondary types of this type. These
  /// are the interface and all the supers whose depth is too big.
  ///
  JavaVirtualTable** secondaryTypes;

  /// baseClass - Holds the base class VT of an array, or the array class VT
  /// of a regular class. Used for AASTORE checks.
  ///
  JavaVirtualTable* baseClassVT;

  /// Java methods for the virtual table functions.
  uintptr_t init;
  uintptr_t equals;
  uintptr_t hashCode;
  uintptr_t toString;
  uintptr_t clone;
  uintptr_t getClass;
  uintptr_t notify;
  uintptr_t notifyAll;
  uintptr_t waitIndefinitely;
  uintptr_t waitMs;
  uintptr_t waitMsNs;
  uintptr_t virtualMethods[1];

  /// operator new - Allocates a JavaVirtualTable with the given size. The
  /// size must contain the additional information for type checking, as well
  /// as the function pointers.
  ///
  void* operator new(size_t sz, mvm::BumpPtrAllocator& allocator,
                     uint32 nbMethods) {
    return allocator.Allocate(sizeof(uintptr_t) * (nbMethods), "Virtual table");
  }

  /// JavaVirtualTable - Create JavaVirtualTable objects for classes, array
  /// classes and primitive classes.
  ///
  JavaVirtualTable(Class* C);
  JavaVirtualTable(ClassArray* C);
  JavaVirtualTable(ClassPrimitive* C);


  /// getFirstJavaMethod - Get the byte offset of the first Java method
  /// (<init>).
  ///
  uintptr_t* getFirstJavaMethod() {
    return &init;
  }
  
  /// getFirstJavaMethodIndex - Get the word offset of the first Java method.
  ///
  static uint32_t getFirstJavaMethodIndex() {
    return 18;
  }
   
  /// getBaseSize - Get the size of the java.lang.Object virtual table.
  ///
  static uint32_t getBaseSize() {
    return 29;
  }
  
  /// getNumJavaMethods - Get the number of methods of the java.lang.Object
  /// class.
  ///
  static uint32_t getNumJavaMethods() {
    return 11;
  }

  /// getDisplayLength - Get the length of the display (primary type) array.
  ///
  static uint32_t getDisplayLength() {
    return 8;
  }
  
  /// getCacheIndex - Get the word offset of the type cache.
  ///
  static uint32_t getCacheIndex() {
    return 6;
  }
  
  /// getOffsetIndex - Get the word offset of the type cache.
  ///
  static uint32_t getOffsetIndex() {
    return 5;
  }
  
  /// getSecondaryTypesIndex - Get the word offset of the secondary types
  /// list.
  ///
  static uint32_t getSecondaryTypesIndex() {
    return 16;
  }
  
  /// getNumSecondaryTypesIndex - Get the word offset of the number of
  /// secondary types.
  ///
  static uint32_t getNumSecondaryTypesIndex() {
    return 15;
  }

  /// isSubtypeOf - Returns true if the given VT is a subtype of the this
  /// VT.
  ///
  bool isSubtypeOf(JavaVirtualTable* VT);

  /// setNativeTracer - Set the tracer of this virtual table as a method
  /// defined by JnJVM.
  ///
  void setNativeTracer(uintptr_t tracer, const char* name);
  
  /// setNativeDestructor - Set the destructor of this virtual table as a method
  /// defined by JnJVM.
  ///
  void setNativeDestructor(uintptr_t tracer, const char* name);

};


/// JavaObject - This class represents a Java object.
///
class JavaObject : public gc {
private:
  
  /// waitIntern - internal wait on a monitor
  ///
  void waitIntern(struct timeval *info, bool timed);
  
public:

  /// getClass - Returns the class of this object.
  ///
  UserCommonClass* getClass() const {
    return ((JavaVirtualTable*)getVirtualTable())->cl;
  }

  /// lock - The monitor of this object. Most of the time null.
  ///
  mvm::ThinLock<JavaLock, JavaObject, mvm::FatLockWithGC> lock;

  /// wait - Java wait. Makes the current thread waiting on a monitor.
  ///
  void wait();

  /// timedWait - Java timed wait. Makes the current thread waiting on a
  /// monitor for the given amount of time.
  ///
  void timedWait(struct timeval &info);
  
  /// notify - Java notify. Notifies a thread from the availability of the
  /// monitor.
  ///
  void notify();
  
  /// notifyAll - Java notifyAll. Notifies all threads from the availability of
  /// the monitor.
  ///
  void notifyAll();
 
  /// overflowThinLock - Notify that the thin lock has overflowed.
  ///
  void overflowThinLock() {
    lock.overflowThinLock();
  }

  /// instanceOf - Is this object's class of type the given class?
  ///
  bool instanceOf(UserCommonClass* cl);

  /// acquire - Acquire the lock on this object.
  void acquire() {
    JavaObject* self = this;
    llvm_gcroot(self, 0);
    self->lock.acquire(self);
  }

  /// release - Release the lock on this object
  void release() {
    JavaObject* self = this;
    llvm_gcroot(self, 0);
    lock.release(self);
  }

  /// owner - Returns true if the current thread is the owner of this object's
  /// lock.
  bool owner() {
    return lock.owner();
  }

#ifdef SIGSEGV_THROW_NULL
  #define verifyNull(obj) {}
#else
  #define verifyNull(obj) \
    if (obj == 0) JavaThread::get()->getJVM()->nullPointerException();
#endif
  
  /// lockObj - Get the LockObj if the lock is a fat lock.
  JavaLock* lockObj() {
    return lock.getFatLock();
  }

  /// decapsulePrimitive - Based on the signature argument, decapsule
  /// obj as a primitive and put it in the buffer.
  ///
  void decapsulePrimitive(Jnjvm* vm, uintptr_t &buf, const Typedef* signature);

};


} // end namespace jnjvm

#endif
