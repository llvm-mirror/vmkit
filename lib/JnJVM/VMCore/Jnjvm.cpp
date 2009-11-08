//===---------- Jnjvm.cpp - Java virtual machine description --------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#define JNJVM_LOAD 0

#include <cfloat>
#include <climits>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include "debug.h"

#include "mvm/Threads/Thread.h"
#include "MvmGC.h"

#include "ClasspathReflect.h"
#include "JavaArray.h"
#include "JavaClass.h"
#include "JavaCompiler.h"
#include "JavaConstantPool.h"
#include "JavaString.h"
#include "JavaThread.h"
#include "JavaTypes.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"
#include "LinkJavaRuntime.h"
#include "LockedMap.h"
#include "Reader.h"
#include "Zip.h"

using namespace jnjvm;

const char* Jnjvm::dirSeparator = "/";
const char* Jnjvm::envSeparator = ":";
const unsigned int Jnjvm::Magic = 0xcafebabe;

#ifdef ISOLATE
Jnjvm* Jnjvm::RunningIsolates[NR_ISOLATES];
mvm::LockNormal Jnjvm::IsolateLock;
#endif


/// initialiseClass - Java class initialisation. Java specification §2.17.5.

void UserClass::initialiseClass(Jnjvm* vm) {
  
  // Primitives are initialized at boot time, arrays are initialized directly.
  
  // Assumes that the Class object has already been verified and prepared and
  // that the Class object contains state that can indicate one of four
  // situations:
  //
  //  * This Class object is verified and prepared but not initialized.
  //  * This Class object is being initialized by some particular thread T.
  //  * This Class object is fully initialized and ready for use.
  //  * This Class object is in an erroneous state, perhaps because the
  //    verification step failed or because initialization was attempted and
  //    failed.

  assert((isResolved() || getOwnerClass() || isReady() ||
         isErroneous()) && "Class in wrong state");
  
  if (getInitializationState() != ready) {
    
    // 1. Synchronize on the Class object that represents the class or 
    //    interface to be initialized. This involves waiting until the
    //    current thread can obtain the lock for that object
    //    (Java specification §8.13).
    acquire();
    JavaThread* self = JavaThread::get();

    if (getInitializationState() == inClinit) {
      // 2. If initialization by some other thread is in progress for the
      //    class or interface, then wait on this Class object (which 
      //    temporarily releases the lock). When the current thread awakens
      //    from the wait, repeat this step.
      if (getOwnerClass() != self) {
        while (getOwnerClass()) {
          waitClass();
        }
      } else {
        // 3. If initialization is in progress for the class or interface by
        //    the current thread, then this must be a recursive request for 
        //    initialization. Release the lock on the Class object and complete
        //    normally.
        release();
        return;
      }
    } 
    
    // 4. If the class or interface has already been initialized, then no 
    //    further action is required. Release the lock on the Class object
    //    and complete normally.
    if (getInitializationState() == ready) {
      release();
      return;
    }
    
    // 5. If the Class object is in an erroneous state, then initialization is
    //    not possible. Release the lock on the Class object and throw a
    //    NoClassDefFoundError.
    if (isErroneous()) {
      release();
      vm->noClassDefFoundError(name);
    }

    // 6. Otherwise, record the fact that initialization of the Class object is
    //    now in progress by the current thread and release the lock on the
    //    Class object.
    setOwnerClass(self);
    bool vmjced = (getInitializationState() == vmjc);
    setInitializationState(inClinit);
    UserClass* cl = (UserClass*)this;
#if defined(ISOLATE) || defined(ISOLATE_SHARING)
    // Isolate environments allocate the static instance on their own, not when
    // the class is being resolved.
    void* val = cl->allocateStaticInstance(vm);
#else
    // Single environment allocates the static instance during resolution, so
    // that compiled code can access it directly (with an initialization
    // check just before the access)
    void* val = cl->getStaticInstance();
    if (!val) {
      val = cl->allocateStaticInstance(vm);
    }
#endif
    release();
  

    // 7. Next, if the Class object represents a class rather than an interface, 
    //    and the direct superclass of this class has not yet been initialized,
    //    then recursively perform this entire procedure for the uninitialized 
    //    superclass. If the initialization of the direct superclass completes 
    //    abruptly because of a thrown exception, then lock this Class object, 
    //    label it erroneous, notify all waiting threads, release the lock, 
    //    and complete abruptly, throwing the same exception that resulted from 
    //    the initializing the superclass.
    UserClass* super = getSuper();
    if (super) {
      try {
        super->initialiseClass(vm);
      } catch(...) {
        acquire();
        setErroneous();
        setOwnerClass(0);
        broadcastClass();
        release();
        self->throwPendingException();
      }
    }
 
    JavaObject* exc = 0;
    JavaObject* obj = 0;
    llvm_gcroot(exc, 0);
    llvm_gcroot(obj, 0);
#ifdef SERVICE
    if (classLoader == classLoader->bootstrapLoader || 
        classLoader->getIsolate() == vm) {
#endif

    // 8. Next, execute either the class variable initializers and static
    //    initializers of the class or the field initializers of the interface,
    //    in textual order, as though they were a single block, except that
    //    final static variables and fields of interfaces whose values are 
    //    compile-time constants are initialized first.
    
    PRINT_DEBUG(JNJVM_LOAD, 0, COLOR_NORMAL, "; ", 0);
    PRINT_DEBUG(JNJVM_LOAD, 0, LIGHT_GREEN, "clinit ", 0);
    PRINT_DEBUG(JNJVM_LOAD, 0, COLOR_NORMAL, "%s\n", mvm::PrintString(this).cString());



    if (!vmjced) {
      JavaField* fields = cl->getStaticFields();
      for (uint32 i = 0; i < cl->nbStaticFields; ++i) {
        fields[i].initField(val, vm);
      }
    }
  
      
      
    JavaMethod* meth = lookupMethodDontThrow(vm->bootstrapLoader->clinitName,
                                             vm->bootstrapLoader->clinitType,
                                             true, false, 0);

    if (meth) {
      try{
        meth->invokeIntStatic(vm, cl);
      } catch(...) {
        exc = self->getJavaException();
        assert(exc && "no exception?");
        self->clearException();
      }
    }
#ifdef SERVICE
    }
#endif

    // 9. If the execution of the initializers completes normally, then lock
    //    this Class object, label it fully initialized, notify all waiting 
    //    threads, release the lock, and complete this procedure normally.
    if (!exc) {
      acquire();
      setInitializationState(ready);
      setOwnerClass(0);
      broadcastClass();
      release();
      return;
    }
    
    // 10. Otherwise, the initializers must have completed abruptly by
    //     throwing some exception E. If the class of E is not Error or one
    //     of its subclasses, then create a new instance of the class 
    //     ExceptionInInitializerError, with E as the argument, and use this
    //     object in place of E in the following step. But if a new instance of
    //     ExceptionInInitializerError cannot be created because an
    //     OutOfMemoryError occurs, then instead use an OutOfMemoryError object
    //     in place of E in the following step.
    if (exc->getClass()->isAssignableFrom(vm->upcalls->newException)) {
      Classpath* upcalls = classLoader->bootstrapLoader->upcalls;
      UserClass* clExcp = upcalls->ExceptionInInitializerError;
      Jnjvm* vm = self->getJVM();
      obj = clExcp->doNew(vm);
      if (!obj) {
        fprintf(stderr, "implement me");
        abort();
      }
      JavaMethod* init = upcalls->ErrorWithExcpExceptionInInitializerError;
      init->invokeIntSpecial(vm, clExcp, obj, exc);
      exc = obj;
    } 

    // 11. Lock the Class object, label it erroneous, notify all waiting
    //     threads, release the lock, and complete this procedure abruptly
    //     with reason E or its replacement as determined in the previous step.
    acquire();
    setErroneous();
    setOwnerClass(0);
    broadcastClass();
    release();
    self->throwException(exc);
  }
}
      
void Jnjvm::errorWithExcp(UserClass* cl, JavaMethod* init,
                          const JavaObject* excp) {
  JavaObject* obj = cl->doNew(this);
  llvm_gcroot(obj, 0);
  init->invokeIntSpecial(this, cl, obj, excp);
  JavaThread::get()->throwException(obj);
}

JavaObject* Jnjvm::CreateError(UserClass* cl, JavaMethod* init,
                               const char* str) {
  JavaObject* obj = cl->doNew(this);
  llvm_gcroot(obj, 0);
  init->invokeIntSpecial(this, cl, obj, str ? asciizToStr(str) : 0);
  return obj;
}

JavaObject* Jnjvm::CreateError(UserClass* cl, JavaMethod* init,
                               JavaString* str) {
  JavaObject* obj = 0;
  llvm_gcroot(str, 0);
  llvm_gcroot(obj, 0);
  obj = cl->doNew(this);
  init->invokeIntSpecial(this, cl, obj, str);
  return obj;
}

void Jnjvm::error(UserClass* cl, JavaMethod* init, JavaString* str) {
  JavaObject* obj = 0;
  llvm_gcroot(obj, 0);
  llvm_gcroot(str, 0);
  obj = CreateError(cl, init, str);
  JavaThread::get()->throwException(obj);
}

void Jnjvm::arrayStoreException() {
  error(upcalls->ArrayStoreException,
        upcalls->InitArrayStoreException, (JavaString*)0);
}

void Jnjvm::indexOutOfBounds(const JavaObject* obj, sint32 entry) {
  JavaString* str = (JavaString*)
    upcalls->IntToString->invokeJavaObjectStatic(this, upcalls->intClass,
                                                 entry, 10);
  llvm_gcroot(str, 0);
  error(upcalls->ArrayIndexOutOfBoundsException,
        upcalls->InitArrayIndexOutOfBoundsException, str);
}

void Jnjvm::negativeArraySizeException(sint32 size) {
  JavaString* str = (JavaString*)
    upcalls->IntToString->invokeJavaObjectStatic(this, upcalls->intClass,
                                                 size, 10);
  llvm_gcroot(str, 0);
  error(upcalls->NegativeArraySizeException,
        upcalls->InitNegativeArraySizeException, str);
}

void Jnjvm::nullPointerException() {
  error(upcalls->NullPointerException,
        upcalls->InitNullPointerException, (JavaString*)0);
}

JavaObject* Jnjvm::CreateIndexOutOfBoundsException(sint32 entry) {
  JavaString* str = (JavaString*)
    upcalls->IntToString->invokeJavaObjectStatic(this, upcalls->intClass,
                                                 entry, 10);
  llvm_gcroot(str, 0);
  return CreateError(upcalls->ArrayIndexOutOfBoundsException,
                     upcalls->InitArrayIndexOutOfBoundsException, str);
}

JavaObject* Jnjvm::CreateNegativeArraySizeException() {
  return CreateError(upcalls->NegativeArraySizeException,
                     upcalls->InitNegativeArraySizeException,
                     (JavaString*)0);
}

JavaObject* Jnjvm::CreateUnsatisfiedLinkError(JavaMethod* meth) {
  JavaString* str = constructString(meth->toString());
  llvm_gcroot(str, 0);
  return CreateError(upcalls->UnsatisfiedLinkError,
                     upcalls->InitUnsatisfiedLinkError,
                     str);
}

JavaObject* Jnjvm::CreateArithmeticException() {
  JavaString* str = asciizToStr("/ by zero");
  llvm_gcroot(str, 0);
  return CreateError(upcalls->ArithmeticException,
                     upcalls->InitArithmeticException, str);
}

JavaObject* Jnjvm::CreateNullPointerException() {
  return CreateError(upcalls->NullPointerException,
                     upcalls->InitNullPointerException,
                     (JavaString*)0);
}

JavaObject* Jnjvm::CreateOutOfMemoryError() {
  JavaString* str = asciizToStr("Java heap space");
  llvm_gcroot(str, 0);
  return CreateError(upcalls->OutOfMemoryError,
                     upcalls->InitOutOfMemoryError, str);
}

JavaObject* Jnjvm::CreateStackOverflowError() {
  // Don't call init, or else we'll get a new stack overflow error.
  JavaObject* obj = upcalls->StackOverflowError->doNew(this);
  llvm_gcroot(obj, 0);
  ((JavaObjectThrowable*)obj)->fillInStackTrace();
  return obj;
}

JavaObject* Jnjvm::CreateArrayStoreException(JavaVirtualTable* VT) {
  JavaString* str = JavaString::internalToJava(VT->cl->name, this);
  llvm_gcroot(str, 0);
  return CreateError(upcalls->ArrayStoreException,
                     upcalls->InitArrayStoreException, str);
}

JavaObject* Jnjvm::CreateClassCastException(JavaObject* obj,
                                            UserCommonClass* cl) {
  llvm_gcroot(obj, 0);
  return CreateError(upcalls->ClassCastException,
                     upcalls->InitClassCastException,
                     (JavaString*)0);
}

JavaObject* Jnjvm::CreateLinkageError(const char* msg) {
  JavaString* str = asciizToStr(msg);
  llvm_gcroot(str, 0);
  return CreateError(upcalls->LinkageError,
                     upcalls->InitLinkageError, str);
}

void Jnjvm::illegalAccessException(const char* msg) {
  JavaString* str = asciizToStr(msg);
  llvm_gcroot(str, 0);
  error(upcalls->IllegalAccessException,
        upcalls->InitIllegalAccessException, str);
}

void Jnjvm::illegalMonitorStateException(const JavaObject* obj) {
  llvm_gcroot(obj, 0);
  error(upcalls->IllegalMonitorStateException,
        upcalls->InitIllegalMonitorStateException,
        (JavaString*)0);
}

void Jnjvm::interruptedException(const JavaObject* obj) {
  llvm_gcroot(obj, 0);
  error(upcalls->InterruptedException,
        upcalls->InitInterruptedException,
        (JavaString*)0);
}


void Jnjvm::initializerError(const JavaObject* excp) {
  llvm_gcroot(excp, 0);
  errorWithExcp(upcalls->ExceptionInInitializerError,
                upcalls->ErrorWithExcpExceptionInInitializerError,
                excp);
}

void Jnjvm::invocationTargetException(const JavaObject* excp) {
  llvm_gcroot(excp, 0);
  errorWithExcp(upcalls->InvocationTargetException,
                upcalls->ErrorWithExcpInvocationTargetException,
                excp);
}

void Jnjvm::outOfMemoryError() {
  JavaString* str = asciizToStr("Java heap space");
  llvm_gcroot(str, 0);
  error(upcalls->OutOfMemoryError,
        upcalls->InitOutOfMemoryError, str);
}

void Jnjvm::illegalArgumentException(const char* msg) {
  JavaString* str = asciizToStr(msg);
  llvm_gcroot(str, 0);
  error(upcalls->IllegalArgumentException,
        upcalls->InitIllegalArgumentException, str);
}

void Jnjvm::classCastException(JavaObject* obj, UserCommonClass* cl) {
  llvm_gcroot(obj, 0);
  error(upcalls->ClassCastException,
        upcalls->InitClassCastException,
        (JavaString*)0);
}

void Jnjvm::noClassDefFoundError(JavaObject* obj) {
  llvm_gcroot(obj, 0);
  errorWithExcp(upcalls->NoClassDefFoundError,
        upcalls->ErrorWithExcpNoClassDefFoundError, 
        obj);
}

void Jnjvm::instantiationException(UserCommonClass* cl) {
  JavaString* str = internalUTF8ToStr(cl->name);
  llvm_gcroot(str, 0);
  error(upcalls->InstantiationException, upcalls->InitInstantiationException,
        str);
}

void Jnjvm::instantiationError(UserCommonClass* cl) {
  JavaString* str = internalUTF8ToStr(cl->name);
  llvm_gcroot(str, 0);
  error(upcalls->InstantiationError, upcalls->InitInstantiationError, str);
}
  

static JavaString* CreateNoSuchMsg(CommonClass* cl, const UTF8* name,
                                   Jnjvm* vm) {
  ArrayUInt16* msg = (ArrayUInt16*)
    vm->upcalls->ArrayOfChar->doNew(19 + cl->name->size + name->size, vm);
  JavaString* str = 0;
  llvm_gcroot(msg, 0);
  llvm_gcroot(str, 0);

  uint32 i = 0;


  msg->elements[i++] = 'u';
  msg->elements[i++] = 'n';
  msg->elements[i++] = 'a';
  msg->elements[i++] = 'b';
  msg->elements[i++] = 'l';
  msg->elements[i++] = 'e';
  msg->elements[i++] = ' ';
  msg->elements[i++] = 't';
  msg->elements[i++] = 'o';
  msg->elements[i++] = ' ';
  msg->elements[i++] = 'f';
  msg->elements[i++] = 'i';
  msg->elements[i++] = 'n';
  msg->elements[i++] = 'd';
  msg->elements[i++] = ' ';

  for (sint32 j = 0; j < name->size; ++j)
    msg->elements[i++] = name->elements[j];

  msg->elements[i++] = ' ';
  msg->elements[i++] = 'i';
  msg->elements[i++] = 'n';
  msg->elements[i++] = ' ';
  
  for (sint32 j = 0; j < cl->name->size; ++j) {
    if (cl->name->elements[j] == '/') msg->elements[i++] = '.';
    else msg->elements[i++] = cl->name->elements[j];
  }

  str = vm->constructString(msg);

  return str;
}

void Jnjvm::noSuchFieldError(CommonClass* cl, const UTF8* name) { 
  JavaString* str = CreateNoSuchMsg(cl, name, this);
  llvm_gcroot(str, 0);
  error(upcalls->NoSuchFieldError,
        upcalls->InitNoSuchFieldError, str);
}

void Jnjvm::noSuchMethodError(CommonClass* cl, const UTF8* name) {
  JavaString* str = CreateNoSuchMsg(cl, name, this);
  llvm_gcroot(str, 0);
  error(upcalls->NoSuchMethodError,
        upcalls->InitNoSuchMethodError, str);
}

static JavaString* CreateUnableToLoad(const UTF8* name, Jnjvm* vm) {
  ArrayUInt16* msg = (ArrayUInt16*)
    vm->upcalls->ArrayOfChar->doNew(15 + name->size, vm);
  JavaString* str = 0;
  llvm_gcroot(msg, 0);
  llvm_gcroot(str, 0);
  uint32 i = 0;


  msg->elements[i++] = 'u';
  msg->elements[i++] = 'n';
  msg->elements[i++] = 'a';
  msg->elements[i++] = 'b';
  msg->elements[i++] = 'l';
  msg->elements[i++] = 'e';
  msg->elements[i++] = ' ';
  msg->elements[i++] = 't';
  msg->elements[i++] = 'o';
  msg->elements[i++] = ' ';
  msg->elements[i++] = 'l';
  msg->elements[i++] = 'o';
  msg->elements[i++] = 'a';
  msg->elements[i++] = 'd';
  msg->elements[i++] = ' ';

  for (sint32 j = 0; j < name->size; ++j) {
    if (name->elements[j] == '/') msg->elements[i++] = '.';
    else msg->elements[i++] = name->elements[j];
  }

  str = vm->constructString(msg);

  return str;
}

static JavaString* CreateUnableToLoad(JavaString* name, Jnjvm* vm) {
  JavaString* str = 0;
  ArrayUInt16* msg = (ArrayUInt16*)
    vm->upcalls->ArrayOfChar->doNew(15 + name->count, vm);
  llvm_gcroot(msg, 0);
  llvm_gcroot(str, 0);
  uint32 i = 0;


  msg->elements[i++] = 'u';
  msg->elements[i++] = 'n';
  msg->elements[i++] = 'a';
  msg->elements[i++] = 'b';
  msg->elements[i++] = 'l';
  msg->elements[i++] = 'e';
  msg->elements[i++] = ' ';
  msg->elements[i++] = 't';
  msg->elements[i++] = 'o';
  msg->elements[i++] = ' ';
  msg->elements[i++] = 'l';
  msg->elements[i++] = 'o';
  msg->elements[i++] = 'a';
  msg->elements[i++] = 'd';
  msg->elements[i++] = ' ';

  for (sint32 j = name->offset; j < name->offset + name->count; ++j) {
    if (name->value->elements[j] == '/') msg->elements[i++] = '.';
    else msg->elements[i++] = name->value->elements[j];
  }

  str = vm->constructString(msg);

  return str;
}



void Jnjvm::noClassDefFoundError(const UTF8* name) {
  JavaString* str = CreateUnableToLoad(name, this);
  llvm_gcroot(str, 0);
  error(upcalls->NoClassDefFoundError,
        upcalls->InitNoClassDefFoundError, str);
}

void Jnjvm::classNotFoundException(JavaString* name) {
  JavaString* str = CreateUnableToLoad(name, this);
  llvm_gcroot(str, 0);
  error(upcalls->ClassNotFoundException,
        upcalls->InitClassNotFoundException, str);
}

void Jnjvm::classFormatError(UserClass* cl, const UTF8* name) {
  uint32 size = 35 + name->size + cl->name->size;
  ArrayUInt16* msg = (ArrayUInt16*)upcalls->ArrayOfChar->doNew(size, this);
  JavaString* str = 0;
  llvm_gcroot(msg, 0);
  llvm_gcroot(str, 0);
  uint32 i = 0;


  msg->elements[i++] = 't';
  msg->elements[i++] = 'r';
  msg->elements[i++] = 'y';
  msg->elements[i++] = ' ';
  msg->elements[i++] = 't';
  msg->elements[i++] = 'o';
  msg->elements[i++] = ' ';
  msg->elements[i++] = 'l';
  msg->elements[i++] = 'o';
  msg->elements[i++] = 'a';
  msg->elements[i++] = 'd';
  msg->elements[i++] = ' ';

  for (sint32 j = 0; j < cl->name->size; ++j) {
    if (cl->name->elements[j] == '/') msg->elements[i++] = '.';
    else msg->elements[i++] = cl->name->elements[j];
  }
  
  msg->elements[i++] = ' ';
  msg->elements[i++] = 'a';
  msg->elements[i++] = 'n';
  msg->elements[i++] = 'd';
  msg->elements[i++] = ' ';
  msg->elements[i++] = 'f';
  msg->elements[i++] = 'o';
  msg->elements[i++] = 'u';
  msg->elements[i++] = 'n';
  msg->elements[i++] = 'd';
  msg->elements[i++] = ' ';
  msg->elements[i++] = 'c';
  msg->elements[i++] = 'l';
  msg->elements[i++] = 'a';
  msg->elements[i++] = 's';
  msg->elements[i++] = 's';
  msg->elements[i++] = ' ';
  msg->elements[i++] = 'n';
  msg->elements[i++] = 'a';
  msg->elements[i++] = 'm';
  msg->elements[i++] = 'e';
  msg->elements[i++] = 'd';
  msg->elements[i++] = ' ';
  
  for (sint32 j = 0; j < name->size; ++j) {
    if (name->elements[j] == '/') msg->elements[i++] = '.';
    else msg->elements[i++] = name->elements[j];
  }
 
  assert(i == size && "Array overflow");

  str = constructString(msg);
  error(upcalls->ClassFormatError, upcalls->InitClassFormatError, str);
}


void Jnjvm::classFormatError(const char* msg) {
  JavaString* str = asciizToStr(msg);
  llvm_gcroot(str, 0);
  error(upcalls->ClassFormatError, upcalls->InitClassFormatError, str);
}

JavaString* Jnjvm::internalUTF8ToStr(const UTF8* utf8) {
  uint32 size = utf8->size;
  ArrayUInt16* tmp = (ArrayUInt16*)upcalls->ArrayOfChar->doNew(size, this);
  llvm_gcroot(tmp, 0);
  uint16* buf = tmp->elements;
  
  for (uint32 i = 0; i < size; i++) {
    buf[i] = utf8->elements[i];
  }
  
  return hashStr.lookupOrCreate((const ArrayUInt16*&)tmp, this,
                                JavaString::stringDup);
}

JavaString* Jnjvm::constructString(const ArrayUInt16* array) { 
  llvm_gcroot(array, 0);
  JavaString* res = hashStr.lookupOrCreate(array, this, JavaString::stringDup);
  return res;
}

JavaString* Jnjvm::asciizToStr(const char* asciiz) {
  assert(asciiz && "No asciiz given");
  ArrayUInt16* var = 0;
  llvm_gcroot(var, 0);
  var = asciizToArray(asciiz);
  return constructString(var);
}

void Jnjvm::addProperty(char* key, char* value) {
  postProperties.push_back(std::make_pair(key, value));
}

// Mimic what's happening in Classpath when creating a java.lang.Class object.
JavaObject* UserCommonClass::getClassDelegatee(Jnjvm* vm, JavaObject* pd) {
  JavaObjectClass* delegatee = 0;
  JavaObjectClass* base = 0;
  llvm_gcroot(pd, 0);
  llvm_gcroot(delegatee, 0);
  llvm_gcroot(base, 0);

  if (!getDelegatee()) {
    UserClass* cl = vm->upcalls->newClass;
    delegatee = (JavaObjectClass*)cl->doNew(vm);
    delegatee->vmdata = this;
    if (!pd && isArray()) {
      base = (JavaObjectClass*)
        asArrayClass()->baseClass()->getClassDelegatee(vm, pd);
      delegatee->pd = base->pd;
    } else {
      delegatee->pd = pd;
    }
    setDelegatee(delegatee);
  }
  return getDelegatee();
}

JavaObject* const* UserCommonClass::getClassDelegateePtr(Jnjvm* vm, JavaObject* pd) {
  llvm_gcroot(pd, 0);
  // Make sure it's created.
  getClassDelegatee(vm, pd);
  return getDelegateePtr();
}

//===----------------------------------------------------------------------===//
// The command line parsing tool does not manipulate any GC-allocated objects.
// The ArrayUInt8 is allocated with malloc and free'd after parsing.
//===----------------------------------------------------------------------===//

#define PATH_MANIFEST "META-INF/MANIFEST.MF"
#define MAIN_CLASS "Main-Class: "
#define MAIN_LOWER_CLASS "Main-class: "
#define PREMAIN_CLASS "Premain-Class: "
#define BOOT_CLASS_PATH "Boot-Class-Path: "
#define CAN_REDEFINE_CLASS_PATH "Can-Redefine-Classes: "

#define LENGTH_MAIN_CLASS 12
#define LENGTH_PREMAIN_CLASS 15
#define LENGTH_BOOT_CLASS_PATH 17

extern "C" struct JNINativeInterface JNI_JNIEnvTable;
extern "C" const struct JNIInvokeInterface JNI_JavaVMTable;

void ClArgumentsInfo::javaAgent(char* cur) {
  assert(0 && "implement me");
}

extern "C" int sys_strnstr(const char *haystack, const char *needle) {
  const char* res = strstr(haystack, needle);
  if (res) return res - haystack;
  else return -1; 
}


static char* findInformation(Jnjvm* vm, ArrayUInt8* manifest, const char* entry,
                             uint32 len) {
  uint8* ptr = manifest->elements;
  sint32 index = sys_strnstr((char*)ptr, entry);
  if (index != -1) {
    index += len;
    sint32 end = sys_strnstr((char*)&(ptr[index]), "\n");
    if (end == -1) end = manifest->size;
    else end += index;

    sint32 length = end - index - 1;
    char* name = (char*)vm->allocator.Allocate(length + 1, "class name");
    memcpy(name, &(ptr[index]), length);
    name[length] = 0;
    return name;
  } else {
    return 0;
  }
}

void ClArgumentsInfo::extractClassFromJar(Jnjvm* vm, int argc, char** argv, 
                                          int i) {
  jarFile = argv[i];
  uint32 size = 2 + strlen(vm->classpath) + strlen(jarFile);
  char* temp = (char*)vm->allocator.Allocate(size, "jar file");

  sprintf(temp, "%s:%s", vm->classpath, jarFile);
  vm->setClasspath(temp);
  
  ArrayUInt8* bytes = Reader::openFile(vm->bootstrapLoader, jarFile, true);

  if (!bytes) {
    printf("Unable to access jarfile %s\n", jarFile);
    return;
  }

  ZipArchive archive(bytes, vm->allocator);
  if (archive.getOfscd() != -1) {
    ZipFile* file = archive.getFile(PATH_MANIFEST);
    if (file) {
      UserClassArray* array = vm->bootstrapLoader->upcalls->ArrayOfByte;
      ArrayUInt8* res = (ArrayUInt8*)array->doNew(file->ucsize, vm->allocator,
                                                  true);
      int ok = archive.readFile(res, file);
      if (ok) {
        char* mainClass = findInformation(vm, res, MAIN_CLASS,
                                          LENGTH_MAIN_CLASS);
        if (!mainClass)
          mainClass = findInformation(vm, res, MAIN_LOWER_CLASS,
                                      LENGTH_MAIN_CLASS);
        if (mainClass) {
          className = mainClass;
        } else {
          printf("No Main-Class:  in Manifest of archive %s.\n", jarFile);
        }
      } else {
        printf("Can't extract Manifest file from archive %s\n", jarFile);
      }
      free(res);
    } else {
      printf("Can't find Manifest file in archive %s\n", jarFile);
    }
  } else {
    printf("Can't find archive %s\n", jarFile);
  }
  free(bytes);
}

void ClArgumentsInfo::nyi() {
  fprintf(stdout, "Not yet implemented\n");
}

void ClArgumentsInfo::printVersion() {
  fprintf(stdout, "JnJVM for Java 1.1 -- 1.5\n");
}

void ClArgumentsInfo::printInformation() {
  fprintf(stdout, 
  "Usage: java [-options] class [args...] (to execute a class)\n"
   "or  java [-options] -jar jarfile [args...]\n"
           "(to execute a jar file) where options include:\n"
    "-client       to select the \"client\" VM\n"
    "-server       to select the \"server\" VM\n"
    "-hotspot      is a synonym for the \"client\" VM  [deprecated]\n"
    "              The default VM is client.\n"
    "\n"
    "-cp <class search path of directories and zip/jar files>\n"
    "-classpath <class search path of directories and zip/jar files>\n"
    "              A : separated list of directories, JAR archives,\n"
    "              and ZIP archives to search for class files.\n"
    "-D<name>=<value>\n"
    "              set a system property\n"
    "-verbose[:class|gc|jni]\n"
    "              enable verbose output\n"
    "-version      print product version and exit\n"
    "-version:<value>\n"
    "              require the specified version to run\n"
    "-showversion  print product version and continue\n"
    "-jre-restrict-search | -jre-no-restrict-search\n"
    "              include/exclude user private JREs in the version search\n"
    "-? -help      print this help message\n"
    "-X            print help on non-standard options\n"
    "-ea[:<packagename>...|:<classname>]\n"
    "-enableassertions[:<packagename>...|:<classname>]\n"
    "              enable assertions\n"
    "-da[:<packagename>...|:<classname>]\n"
    "-disableassertions[:<packagename>...|:<classname>]\n"
    "              disable assertions\n"
    "-esa | -enablesystemassertions\n"
    "              enable system assertions\n"
    "-dsa | -disablesystemassertions\n"
    "              disable system assertions\n"
    "-agentlib:<libname>[=<options>]\n"
    "              load native agent library <libname>, e.g. -agentlib:hprof\n"
    "                see also, -agentlib:jdwp=help and -agentlib:hprof=help\n"
    "-agentpath:<pathname>[=<options>]\n"
    "              load native agent library by full pathname\n"
    "-javaagent:<jarpath>[=<options>]\n"
    "       load Java programming language agent, see java.lang.instrument\n");
}

void ClArgumentsInfo::readArgs(Jnjvm* vm) {
  className = 0;
  appArgumentsPos = 0;
  sint32 i = 1;
  if (i == argc) printInformation();
  while (i < argc) {
    char* cur = argv[i];
    if (!(strcmp(cur, "-client"))) {
      nyi();
    } else if (!(strcmp(cur, "-server"))) {
      nyi();
    } else if (!(strcmp(cur, "-classpath"))) {
      ++i;
      if (i == argc) printInformation();
      else vm->setClasspath(argv[i]);
    } else if (!(strcmp(cur, "-cp"))) {
      ++i;
      if (i == argc) printInformation();
      else vm->setClasspath(argv[i]);
    } else if (!(strcmp(cur, "-debug"))) {
      nyi();
    } else if (!(strncmp(cur, "-D", 2))) {
      uint32 len = strlen(cur);
      if (len == 2) {
        printInformation();
      } else {
        char* key = &cur[2];
        char* value = strchr(key, '=');
        if (!value) {
          printInformation();
          return;
        } else {
          value[0] = 0;
          vm->addProperty(key, &value[1]);
        }
      }
    } else if (!(strncmp(cur, "-Xbootclasspath:", 16))) {
      uint32 len = strlen(cur);
      if (len == 16) {
        printInformation();
      } else {
        char* path = &cur[16];
        vm->bootstrapLoader->analyseClasspathEnv(path);
      }
    } else if (!(strcmp(cur, "-enableassertions"))) {
      nyi();
    } else if (!(strcmp(cur, "-ea"))) {
      nyi();
    } else if (!(strcmp(cur, "-disableassertions"))) {
      nyi();
    } else if (!(strcmp(cur, "-da"))) {
      nyi();
    } else if (!(strcmp(cur, "-enablesystemassertions"))) {
      nyi();
    } else if (!(strcmp(cur, "-esa"))) {
      nyi();
    } else if (!(strcmp(cur, "-disablesystemassertions"))) {
      nyi();
    } else if (!(strcmp(cur, "-dsa"))) {
      nyi();
    } else if (!(strcmp(cur, "-jar"))) {
      ++i;
      if (i == argc) {
        printInformation();
      } else {
        extractClassFromJar(vm, argc, argv, i);
        appArgumentsPos = i;
        return;
      }
    } else if (!(strcmp(cur, "-jre-restrict-research"))) {
      nyi();
    } else if (!(strcmp(cur, "-jre-no-restrict-research"))) {
      nyi();
    } else if (!(strcmp(cur, "-noclassgc"))) {
      nyi();
    } else if (!(strcmp(cur, "-ms"))) {
      nyi();
    } else if (!(strcmp(cur, "-mx"))) {
      nyi();
    } else if (!(strcmp(cur, "-ss"))) {
      nyi();
    } else if (!(strcmp(cur, "-verbose"))) {
      nyi();
    } else if (!(strcmp(cur, "-verbose:class"))) {
      nyi();
    } else if (!(strcmp(cur, "-verbosegc"))) {
      nyi();
    } else if (!(strcmp(cur, "-verbose:gc"))) {
      nyi();
    } else if (!(strcmp(cur, "-verbose:jni"))) {
      nyi();
    } else if (!(strcmp(cur, "-version"))) {
      printVersion();
    } else if (!(strcmp(cur, "-showversion"))) {
      nyi();
    } else if (!(strcmp(cur, "-?"))) {
      printInformation();
    } else if (!(strcmp(cur, "-help"))) {
      printInformation();
    } else if (!(strcmp(cur, "-X"))) {
      nyi();
    } else if (!(strcmp(cur, "-agentlib"))) {
      nyi();
    } else if (!(strcmp(cur, "-agentpath"))) {
      nyi();
    } else if (cur[0] == '-') {
    } else if (!(strcmp(cur, "-javaagent"))) {
      javaAgent(cur);
    } else {
      className = cur;
      appArgumentsPos = i;
      return;
    }
    ++i;
  }
}


JnjvmClassLoader* Jnjvm::loadAppClassLoader() {
  JavaObject* loader = 0;
  llvm_gcroot(loader, 0);
  
  if (appClassLoader == 0) {
    UserClass* cl = upcalls->newClassLoader;
    loader = upcalls->getSystemClassLoader->invokeJavaObjectStatic(this, cl);
    appClassLoader = JnjvmClassLoader::getJnjvmLoaderFromJavaObject(loader,
                                                                    this);
    if (argumentsInfo.jarFile)
      appClassLoader->loadLibFromJar(this, argumentsInfo.jarFile,
                                     argumentsInfo.className);
    else if (argumentsInfo.className)
      appClassLoader->loadLibFromFile(this, argumentsInfo.className);
  }
  return appClassLoader;
}

void Jnjvm::loadBootstrap() {
  JnjvmClassLoader* loader = bootstrapLoader;
  
  // First create system threads.
  finalizerThread = new JavaThread(0, 0, this);
  finalizerThread->start((void (*)(mvm::Thread*))finalizerStart);
    
  enqueueThread = new JavaThread(0, 0, this);
  enqueueThread->start((void (*)(mvm::Thread*))enqueueStart);
  
  // Initialise the bootstrap class loader if it's not
  // done already.
  if (!bootstrapLoader->upcalls->newString)
    bootstrapLoader->upcalls->initialiseClasspath(bootstrapLoader);
  
#define LOAD_CLASS(cl) \
  cl->resolveClass(); \
  cl->initialiseClass(this);
  
  // If a string belongs to the vm hashmap, we must remove it when
  // it's destroyed. So we define a new VT for strings that will be
  // placed in the hashmap. This VT will have its destructor set so
  // that the string is removed when deallocated.
  upcalls->newString->resolveClass();
  if (!JavaString::internStringVT) {
    JavaVirtualTable* stringVT = upcalls->newString->getVirtualVT();
    uint32 size = upcalls->newString->virtualTableSize * sizeof(uintptr_t);
    
    JavaString::internStringVT = 
      (JavaVirtualTable*)bootstrapLoader->allocator.Allocate(size, "String VT");

    memcpy(JavaString::internStringVT, stringVT, size);
    
    JavaString::internStringVT->destructor = 
      (uintptr_t)JavaString::stringDestructor;

    // Tell the finalizer that this is a native destructor.
    JavaString::internStringVT->operatorDelete = 
      (uintptr_t)JavaString::stringDestructor;
  }
  upcalls->newString->initialiseClass(this);

#ifdef SERVICE
  if (!IsolateID)
#endif
  // The initialization code of the classes initialized below may require
  // to get the Java thread, so we create the Java thread object first.
  upcalls->InitializeThreading(this);
  
  LOAD_CLASS(upcalls->newClass);
  LOAD_CLASS(upcalls->newConstructor);
  LOAD_CLASS(upcalls->newField);
  LOAD_CLASS(upcalls->newMethod);
  LOAD_CLASS(upcalls->newVMThread);
  LOAD_CLASS(upcalls->newStackTraceElement);
  LOAD_CLASS(upcalls->newVMThrowable);
  LOAD_CLASS(upcalls->boolClass);
  LOAD_CLASS(upcalls->byteClass);
  LOAD_CLASS(upcalls->charClass);
  LOAD_CLASS(upcalls->shortClass);
  LOAD_CLASS(upcalls->intClass);
  LOAD_CLASS(upcalls->longClass);
  LOAD_CLASS(upcalls->floatClass);
  LOAD_CLASS(upcalls->doubleClass);
  LOAD_CLASS(upcalls->InvocationTargetException);
  LOAD_CLASS(upcalls->ArrayStoreException);
  LOAD_CLASS(upcalls->ClassCastException);
  LOAD_CLASS(upcalls->IllegalMonitorStateException);
  LOAD_CLASS(upcalls->IllegalArgumentException);
  LOAD_CLASS(upcalls->InterruptedException);
  LOAD_CLASS(upcalls->IndexOutOfBoundsException);
  LOAD_CLASS(upcalls->ArrayIndexOutOfBoundsException);
  LOAD_CLASS(upcalls->NegativeArraySizeException);
  LOAD_CLASS(upcalls->NullPointerException);
  LOAD_CLASS(upcalls->SecurityException);
  LOAD_CLASS(upcalls->ClassFormatError);
  LOAD_CLASS(upcalls->ClassCircularityError);
  LOAD_CLASS(upcalls->NoClassDefFoundError);
  LOAD_CLASS(upcalls->UnsupportedClassVersionError);
  LOAD_CLASS(upcalls->NoSuchFieldError);
  LOAD_CLASS(upcalls->NoSuchMethodError);
  LOAD_CLASS(upcalls->InstantiationError);
  LOAD_CLASS(upcalls->IllegalAccessError);
  LOAD_CLASS(upcalls->IllegalAccessException);
  LOAD_CLASS(upcalls->VerifyError);
  LOAD_CLASS(upcalls->ExceptionInInitializerError);
  LOAD_CLASS(upcalls->LinkageError);
  LOAD_CLASS(upcalls->AbstractMethodError);
  LOAD_CLASS(upcalls->UnsatisfiedLinkError);
  LOAD_CLASS(upcalls->InternalError);
  LOAD_CLASS(upcalls->OutOfMemoryError);
  LOAD_CLASS(upcalls->StackOverflowError);
  LOAD_CLASS(upcalls->UnknownError);
  LOAD_CLASS(upcalls->ClassNotFoundException); 
  LOAD_CLASS(upcalls->ArithmeticException); 
  LOAD_CLASS(upcalls->InstantiationException);
#undef LOAD_CLASS

  loadAppClassLoader();
  JavaObject* obj = JavaThread::get()->currentThread();
  JavaObject* javaLoader = appClassLoader->getJavaClassLoader();

#ifdef SERVICE
  if (!IsolateID)
#endif
  upcalls->setContextClassLoader->invokeIntSpecial(this, upcalls->newThread,
                                                   obj, javaLoader);
  // load and initialise math since it is responsible for dlopen'ing 
  // libjavalang.so and we are optimizing some math operations
  UserCommonClass* math = 
    loader->loadName(loader->asciizConstructUTF8("java/lang/Math"), true, true);
  math->asClass()->initialiseClass(this);
}

void Jnjvm::executeClass(const char* className, ArrayObject* args) {
  JavaObject* exc = 0;
  JavaObject* obj = 0;
  JavaObject* group = 0;
  
  llvm_gcroot(args, 0);
  llvm_gcroot(exc, 0);
  llvm_gcroot(obj, 0);
  llvm_gcroot(group, 0);

  try {
    
    // First try to see if we are a self-contained executable.
    UserClass* cl = appClassLoader->loadClassFromSelf(this, className);
    
    // If not, load the class.
    if (!cl) {
      const UTF8* name = appClassLoader->asciizConstructUTF8(className);
      cl = (UserClass*)appClassLoader->loadName(name, true, true);
    }
    
    cl->initialiseClass(this);
  
    const UTF8* funcSign = 
      appClassLoader->asciizConstructUTF8("([Ljava/lang/String;)V");
    const UTF8* funcName = appClassLoader->asciizConstructUTF8("main");
    JavaMethod* method = cl->lookupMethod(funcName, funcSign, true, true, 0);
  
    method->invokeIntStatic(this, method->classDef, args);
  }catch(...) {
  }

  exc = JavaThread::get()->pendingException;
  if (exc) {
    JavaThread* th = JavaThread::get();
    th->clearException();
    obj = th->currentThread();
    group = upcalls->group->getObjectField(obj);
    try{
      upcalls->uncaughtException->invokeIntSpecial(this, upcalls->threadGroup,
                                                   group, obj, exc);
    }catch(...) {
      fprintf(stderr, "Exception in thread \"main\": "
                      "Can not print stack trace.\n");
    }
  }
}

void Jnjvm::executePremain(const char* className, JavaString* args,
                             JavaObject* instrumenter) {
  llvm_gcroot(instrumenter, 0);
  try {
    const UTF8* name = appClassLoader->asciizConstructUTF8(className);
    UserClass* cl = (UserClass*)appClassLoader->loadName(name, true, true);
    cl->initialiseClass(this);
  
    const UTF8* funcSign = appClassLoader->asciizConstructUTF8(
      "(Ljava/lang/String;Ljava/lang/instrument/Instrumentation;)V");
    const UTF8* funcName = appClassLoader->asciizConstructUTF8("premain");
    JavaMethod* method = cl->lookupMethod(funcName, funcSign, true, true, 0);
  
    method->invokeIntStatic(this, method->classDef, args, instrumenter);
  } catch(...) {
    JavaThread::get()->clearException();
  }
}

void Jnjvm::waitForExit() { 
  
  threadSystem.nonDaemonLock.lock();
  
  while (threadSystem.nonDaemonThreads) {
    threadSystem.nonDaemonVar.wait(&threadSystem.nonDaemonLock);
  }
  
  threadSystem.nonDaemonLock.unlock();

  return;
}

void Jnjvm::mainJavaStart(JavaThread* thread) {

  JavaString* str = 0;
  JavaObject* instrumenter = 0;
  ArrayObject* args = 0;

  llvm_gcroot(str, 0);
  llvm_gcroot(instrumenter, 0);
  llvm_gcroot(args, 0);

  Jnjvm* vm = thread->getJVM();
  vm->mainThread = thread;

  vm->loadBootstrap();

#ifdef SERVICE
  thread->ServiceException = vm->upcalls->newThrowable->doNew(vm);
#endif

  ClArgumentsInfo& info = vm->argumentsInfo;
  
  if (info.agents.size()) {
    assert(0 && "implement me");
    instrumenter = 0;//createInstrumenter();
    for (std::vector< std::pair<char*, char*> >::iterator i = 
                                                info.agents.begin(),
            e = info.agents.end(); i!= e; ++i) {
      str = vm->asciizToStr(i->second);
      vm->executePremain(i->first, str, instrumenter);
    }
  }
    
  UserClassArray* array = vm->bootstrapLoader->upcalls->ArrayOfString;
  args = (ArrayObject*)array->doNew(info.argc - 2, vm);
  for (int i = 2; i < info.argc; ++i) {
    args->elements[i - 2] = (JavaObject*)vm->asciizToStr(info.argv[i]);
  }

  vm->executeClass(info.className, args);
  
  vm->threadSystem.nonDaemonLock.lock();
  --(vm->threadSystem.nonDaemonThreads);
  if (vm->threadSystem.nonDaemonThreads == 0)
      vm->threadSystem.nonDaemonVar.signal();
  vm->threadSystem.nonDaemonLock.unlock();  
}

#ifdef SERVICE

#include <signal.h>

extern void terminationHandler(int);

static void serviceCPUMonitor(mvm::Thread* th) {
  while (true) {
    sleep(1);
    for(JavaThread* cur = (Java*)th->next(); cur != th;
        cur = (JavaThread*)cur->next()) {
        JavaThread* th = (JavaThread*)cur;
        if (!(th->StateWaiting)) {
          mvm::VirtualMachine* executingVM = cur->MyVM;
          assert(executingVM && "Thread with no VM!");
          ++executingVM->executionTime;
      }
    }
  }
}
#endif

void Jnjvm::runApplication(int argc, char** argv) {
  argumentsInfo.argc = argc;
  argumentsInfo.argv = argv;
  argumentsInfo.readArgs(this);
  if (argumentsInfo.className) {
    int pos = argumentsInfo.appArgumentsPos;
    
    argumentsInfo.argv = argumentsInfo.argv + pos - 1;
    argumentsInfo.argc = argumentsInfo.argc - pos + 1;
#ifdef SERVICE
    struct sigaction sa;
    sigset_t mask;
    sigfillset(&mask);
    sigaction(SIGUSR1, 0, &sa);
    sa.sa_mask = mask;
    sa.sa_handler = terminationHandler;
    sa.sa_flags |= SA_RESTART;
    sigaction(SIGUSR1, &sa, NULL);

    mvm::Thread* th = new JavaThread(0, 0, this);
    th->start(serviceCPUMonitor);
#endif
   
    mainThread = new JavaThread(0, 0, this);
    mainThread->start((void (*)(mvm::Thread*))mainJavaStart);
  } else {
    threadSystem.nonDaemonThreads = 0;
  }
}

Jnjvm::Jnjvm(mvm::BumpPtrAllocator& Alloc, JnjvmBootstrapLoader* loader) : 
  VirtualMachine(Alloc), lockSystem(this) {

  classpath = getenv("CLASSPATH");
  if (!classpath) classpath = ".";
  
  appClassLoader = 0;
  jniEnv = &JNI_JNIEnvTable;
  javavmEnv = &JNI_JavaVMTable;
  

  bootstrapLoader = loader;
  upcalls = bootstrapLoader->upcalls;

  throwable = upcalls->newThrowable;

  StringList* end = loader->strings;
  while (end) {
    for (uint32 i = 0; i < end->length; ++i) {
      JavaString* obj = end->strings[i];
      hashStr.insert(obj);
    }
    end = end->prev;
  }

  bootstrapLoader->insertAllMethodsInVM(this);  

#ifdef ISOLATE
  IsolateLock.lock();
  for (uint32 i = 0; i < NR_ISOLATES; ++i) {
    if (RunningIsolates[i] == 0) {
      RunningIsolates[i] = this;
      IsolateID = i;
      break;
    }
  }
  IsolateLock.unlock();
#endif

}

Jnjvm::~Jnjvm() {
#ifdef ISOLATE
  RunningIsolates[IsolateID] = 0;
#endif
}

const UTF8* Jnjvm::asciizToInternalUTF8(const char* asciiz) {
  uint32 size = strlen(asciiz);
  UTF8* tmp = (UTF8*)upcalls->ArrayOfChar->doNew(size, this);
  uint16* buf = tmp->elements;
  
  for (uint32 i = 0; i < size; i++) {
    if (asciiz[i] == '.') buf[i] = '/';
    else buf[i] = asciiz[i];
  }
  return (const UTF8*)tmp;

}
  
ArrayUInt16* Jnjvm::asciizToArray(const char* asciiz) {
  uint32 size = strlen(asciiz);
  ArrayUInt16* tmp = (ArrayUInt16*)upcalls->ArrayOfChar->doNew(size, this);
  uint16* buf = tmp->elements;
  
  for (uint32 i = 0; i < size; i++) {
    buf[i] = asciiz[i];
  }
  return tmp;
}

void Jnjvm::internalRemoveMethods(JnjvmClassLoader* loader, mvm::FunctionMap& Map) {
  // Loop over all methods in the map to find which ones belong
  // to this class loader.
  Map.FunctionMapLock.acquire();
  std::map<void*, mvm::MethodInfo*>::iterator temp;
  for (std::map<void*, mvm::MethodInfo*>::iterator i = Map.Functions.begin(), 
       e = Map.Functions.end(); i != e;) {
    JavaMethod* meth = (JavaMethod*)i->second->getMetaInfo();
    if (meth->classDef->classLoader == loader) {
      temp = i;
      ++i;
      Map.Functions.erase(temp);
    } else {
      ++i;
    }
  }
  Map.FunctionMapLock.release();
}

void Jnjvm::removeMethodsInFunctionMaps(JnjvmClassLoader* loader) {
  internalRemoveMethods(loader, RuntimeFunctions);
  internalRemoveMethods(loader, StaticFunctions);
}


/// JavaStaticCompiler - Compiler for AOT-compiled programs that
/// do not use the JIT.
class JavaStaticCompiler : public JavaCompiler {
public:
#ifdef WITH_LLVM_GCC
  virtual mvm::StackScanner* createStackScanner() {
    return new mvm::CamlStackScanner();
  }
#endif
};


// Helper function to run Jnjvm without JIT.
extern "C" int StartJnjvmWithoutJIT(int argc, char** argv, char* mainClass) {
  mvm::Collector::initialise();
  
  char** newArgv = new char*[argc + 1];
  memcpy(newArgv + 1, argv, argc * sizeof(char*));
  newArgv[0] = newArgv[1];
  newArgv[1] = mainClass;
 
  JavaCompiler* Comp = new JavaStaticCompiler();
  JnjvmClassLoader* JCL = mvm::VirtualMachine::initialiseJVM(Comp);
  mvm::VirtualMachine* vm = mvm::VirtualMachine::createJVM(JCL);
  vm->runApplication(argc + 1, newArgv);
  vm->waitForExit();

  delete[] newArgv;
  
  return 0; 
}
