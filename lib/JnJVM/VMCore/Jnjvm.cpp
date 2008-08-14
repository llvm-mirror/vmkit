//===---------- Jnjvm.cpp - Java virtual machine description --------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#define JNJVM_LOAD 0

#include <float.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"

#include "mvm/JIT.h"

#include "JavaArray.h"
#include "JavaClass.h"
#include "JavaConstantPool.h"
#include "JavaJIT.h"
#include "JavaString.h"
#include "JavaThread.h"
#include "JavaTypes.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"
#include "JnjvmModuleProvider.h"
#include "LockedMap.h"
#include "Reader.h"
#ifdef SERVICE_VM
#include "ServiceDomain.h"
#endif
#include "Zip.h"

using namespace jnjvm;

#define DEF_UTF8(var) \
  const UTF8* Jnjvm::var = 0
  
  DEF_UTF8(initName);
  DEF_UTF8(clinitName);
  DEF_UTF8(clinitType);
  DEF_UTF8(runName);
  DEF_UTF8(prelib);
  DEF_UTF8(postlib);
  DEF_UTF8(mathName);
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
  DEF_UTF8(finalize);

#undef DEF_UTF8

const char* Jnjvm::dirSeparator = "/";
const char* Jnjvm::envSeparator = ":";
const unsigned int Jnjvm::Magic = 0xcafebabe;

#define DECLARE_EXCEPTION(EXCP) \
  const char* Jnjvm::EXCP = "java/lang/"#EXCP

#define DECLARE_REFLECT_EXCEPTION(EXCP) \
  const char* Jnjvm::EXCP = "java/lang/reflect/"#EXCP

DECLARE_EXCEPTION(ArithmeticException);
DECLARE_REFLECT_EXCEPTION(InvocationTargetException);
DECLARE_EXCEPTION(ArrayStoreException);
DECLARE_EXCEPTION(ClassCastException);
DECLARE_EXCEPTION(IllegalMonitorStateException);
DECLARE_EXCEPTION(IllegalArgumentException);
DECLARE_EXCEPTION(InterruptedException);
DECLARE_EXCEPTION(IndexOutOfBoundsException);
DECLARE_EXCEPTION(ArrayIndexOutOfBoundsException);
DECLARE_EXCEPTION(NegativeArraySizeException);
DECLARE_EXCEPTION(NullPointerException);
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
DECLARE_EXCEPTION(OutOfMemoryError);
DECLARE_EXCEPTION(StackOverflowError);
DECLARE_EXCEPTION(UnknownError);
DECLARE_EXCEPTION(ClassNotFoundException);

typedef void (*clinit_t)(Jnjvm* vm);

void Jnjvm::initialiseClass(CommonClass* cl) {
  JavaState* status = cl->getStatus();
  if (cl->isArray || cl->isPrimitive) {
    *status = ready;
  } else if (!(*status == ready)) {
    cl->acquire();
    JavaState* status = cl->getStatus();
    if (*status == ready) {
      cl->release();
    } else if (*status >= resolved && *status != clinitParent &&
               *status != inClinit) {
      *status = clinitParent;
      cl->release();
      if (cl->super) {
        cl->super->initialiseClass();
      }

      cl->classLoader->TheModule->resolveStaticClass((Class*)cl);
      
      *status = inClinit;
      JavaMethod* meth = cl->lookupMethodDontThrow(clinitName, clinitType, true,
                                                   false);
      
      PRINT_DEBUG(JNJVM_LOAD, 0, COLOR_NORMAL, "; ", 0);
      PRINT_DEBUG(JNJVM_LOAD, 0, LIGHT_GREEN, "clinit ", 0);
      PRINT_DEBUG(JNJVM_LOAD, 0, COLOR_NORMAL, "%s::%s\n", printString(),
                  cl->printString());
      
      ((Class*)cl)->createStaticInstance();
      
      if (meth) {
        JavaObject* exc = 0;
        try{
          clinit_t pred = (clinit_t)(intptr_t)meth->compiledPtr();
          pred(JavaThread::get()->isolate);
        } catch(...) {
          exc = JavaThread::getJavaException();
          assert(exc && "no exception?");
          JavaThread::clearException();
        }
        if (exc) {
          if (exc->classOf->isAssignableFrom(Classpath::newException)) {
            JavaThread::get()->isolate->initializerError(exc);
          } else {
            JavaThread::throwException(exc);
          }
        }
      }
      
      *status = ready;
      cl->broadcastClass();
    } else if (*status < resolved) {
      cl->release();
      unknownError("try to clinit a not-read class...");
    } else {
      if (!cl->ownerClass()) {
        while (*status < ready) cl->waitClass();
        cl->release();
        initialiseClass(cl);
      } 
      cl->release();
    }
  }
}


void Jnjvm::errorWithExcp(const char* className, const JavaObject* excp) {
  JnjvmClassLoader* JCL = JnjvmClassLoader::bootstrapLoader;
  Class* cl = (Class*) JCL->loadName(JCL->asciizConstructUTF8(className),
                                     true, true, true);
  JavaObject* obj = cl->doNew(this);
  JavaJIT::invokeOnceVoid(this, JCL, className, "<init>",
                          "(Ljava/lang/Throwable;)V", ACC_VIRTUAL, obj, excp);
  JavaThread::throwException(obj);
}

void Jnjvm::error(const char* className, const char* fmt, ...) {
  JnjvmClassLoader* JCL = JnjvmClassLoader::bootstrapLoader;
  char* tmp = (char*)alloca(4096);
  Class* cl = (Class*) JCL->loadName(JCL->asciizConstructUTF8(className),
                                     true, true, true);
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(tmp, 4096, fmt, ap);
  va_end(ap);

  JavaObject* obj = cl->doNew(this);
  JavaJIT::invokeOnceVoid(this, JCL, className, "<init>",
                          "(Ljava/lang/String;)V", ACC_VIRTUAL, obj, 
                          this->asciizToStr(tmp));
  JavaThread::throwException(obj);
}


void Jnjvm::verror(const char* className, const char* fmt, va_list ap) {
  JnjvmClassLoader* JCL = JnjvmClassLoader::bootstrapLoader;
  char* tmp = (char*)alloca(4096);
  Class* cl = (Class*) JCL->loadName(JCL->asciizConstructUTF8(className),
                                     true, true, true);
  vsnprintf(tmp, 4096, fmt, ap);
  va_end(ap);
  JavaObject* obj = cl->doNew(this);
  JavaJIT::invokeOnceVoid(this, JCL, className, "<init>",
                          "(Ljava/lang/String;)V", ACC_VIRTUAL, obj, 
                          this->asciizToStr(tmp));

  JavaThread::throwException(obj);
}

void Jnjvm::arrayStoreException() {
  error(ArrayStoreException, "");
}

void Jnjvm::indexOutOfBounds(const JavaObject* obj, sint32 entry) {
  error(ArrayIndexOutOfBoundsException, "%d", entry);
}

void Jnjvm::negativeArraySizeException(sint32 size) {
  error(NegativeArraySizeException, "%d", size);
}

void Jnjvm::nullPointerException(const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  char* val = va_arg(ap, char*);
  va_end(ap);
  error(NullPointerException, fmt, val);
}

void Jnjvm::illegalAccessException(const char* msg) {
  error(IllegalAccessException, msg);
}

void Jnjvm::illegalMonitorStateException(const JavaObject* obj) {
  error(IllegalMonitorStateException, "");
}

void Jnjvm::interruptedException(const JavaObject* obj) {
  error(InterruptedException, "");
}


void Jnjvm::initializerError(const JavaObject* excp) {
  errorWithExcp(ExceptionInInitializerError, excp);
}

void Jnjvm::invocationTargetException(const JavaObject* excp) {
  errorWithExcp(InvocationTargetException, excp);
}

void Jnjvm::outOfMemoryError(sint32 n) {
  error(OutOfMemoryError, "");
}

void Jnjvm::illegalArgumentExceptionForMethod(JavaMethod* meth, 
                                               CommonClass* required,
                                               CommonClass* given) {
  error(IllegalArgumentException, "for method %s", meth->printString());
}

void Jnjvm::illegalArgumentExceptionForField(JavaField* field, 
                                              CommonClass* required,
                                              CommonClass* given) {
  error(IllegalArgumentException, "for field %s", field->printString());
}

void Jnjvm::illegalArgumentException(const char* msg) {
  error(IllegalArgumentException, msg);
}

void Jnjvm::classCastException(const char* msg) {
  error(ClassCastException, msg);
}

void Jnjvm::unknownError(const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  verror(UnknownError, fmt, ap);
}

JavaString* Jnjvm::UTF8ToStr(const UTF8* utf8) { 
  JavaString* res = hashStr->lookupOrCreate(utf8, this, JavaString::stringDup);
  return res;
}

JavaString* Jnjvm::asciizToStr(const char* asciiz) {
  // asciizToStr is called by jnjvm code, so utf8s created
  // by this method are stored in the bootstrap class loader
  JnjvmClassLoader* JCL = JnjvmClassLoader::bootstrapLoader;
  const UTF8* var = JCL->asciizConstructUTF8(asciiz);
  return UTF8ToStr(var);
}

void Jnjvm::addProperty(char* key, char* value) {
  postProperties.push_back(std::make_pair(key, value));
}

#ifndef MULTIPLE_VM
JavaObject* Jnjvm::getClassDelegatee(CommonClass* cl) {
  cl->acquire();
  if (!(cl->delegatee)) {
    JavaObject* delegatee = Classpath::newClass->doNew(this);
    cl->delegatee = delegatee;
    Classpath::initClass->invokeIntSpecial(this, delegatee, cl);
  } else if (cl->delegatee->classOf != Classpath::newClass) {
    JavaObject* pd = cl->delegatee;
    JavaObject* delegatee = Classpath::newClass->doNew(this);
    cl->delegatee = delegatee;;
    Classpath::initClassWithProtectionDomain->invokeIntSpecial(this, delegatee,
                                                               cl, pd);
  }
  cl->release();
  return cl->delegatee;
}
#else
JavaObject* Jnjvm::getClassDelegatee(CommonClass* cl) {
  cl->acquire();
  JavaObject* val = delegatees->lookup(cl);
  if (!val) {
    val = Classpath::newClass->doNew(this);
    delegatees->hash(cl, val);
    Classpath::initClass->invokeIntSpecial(this, val, cl);
  } else if (val->classOf != Classpath::newClass) {
    JavaObject* pd = val;
    val = Classpath::newClass->doNew(this);
    delegatees->remove(cl);
    delegatees->hash(cl, val);
    Classpath::initClassWithProtectionDomain->invokeIntSpecial(this, val, cl,
                                                               pd);
  }
  cl->release();
  return val;
}
#endif

Jnjvm::~Jnjvm() {
#ifdef MULTIPLE_GC
  GC->destroy();
  delete GC;
#endif
  
  delete hashStr;  
  delete globalRefsLock;
}

Jnjvm::Jnjvm() {
#ifdef MULTIPLE_GC
  GC = 0;
#endif
  hashStr = 0;
  globalRefsLock = 0;
}
