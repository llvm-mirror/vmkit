//===-------------------- JavaRuntimeJIT.cpp ------------------------------===//
//=== ---- Runtime functions called by code compiled by the JIT -----------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <cstdarg>

#include "mvm/JIT.h"
#include "mvm/Threads/Thread.h"

#include "JavaArray.h"
#include "JavaCache.h"
#include "JavaClass.h"
#include "JavaConstantPool.h"
#include "JavaJIT.h"
#include "JavaString.h"
#include "JavaThread.h"
#include "JavaTypes.h"
#include "Jnjvm.h"
#include "LockedMap.h"

using namespace jnjvm;

extern "C" void* jnjvmVirtualLookup(CacheNode* cache, JavaObject *obj) {
  Enveloppe* enveloppe = cache->enveloppe;
  UserCommonClass* ocl = obj->classOf;
  
#ifndef SERVICE
  assert((obj->classOf->isClass() && 
          obj->classOf->asClass()->isInitializing()) &&
         "Class not ready in a virtual lookup.");
#endif

  enveloppe->cacheLock.acquire();
  CacheNode* rcache = 0;
  CacheNode* tmp = enveloppe->firstCache;
  CacheNode* last = tmp;

  while (tmp) {
    if (ocl == tmp->lastCible) {
      rcache = tmp;
      break;
    } else {
      last = tmp;
      tmp = tmp->next;
    }
  }

  if (!rcache) {
    UserClass* methodCl = 0;
    UserClass* lookup = ocl->isArray() ? ocl->super : ocl->asClass();
    JavaMethod* dmeth = lookup->lookupMethod(enveloppe->methodName,
                                             enveloppe->methodSign,
                                             false, true, &methodCl);

#if !defined(ISOLATE_SHARING) && !defined(SERVICE)
    assert(dmeth->classDef->isInitializing() &&
           "Class not ready in a virtual lookup.");
#endif

    // Are we the first cache?
    if (cache != &(enveloppe->bootCache)) {
      mvm::BumpPtrAllocator& alloc = 
        enveloppe->classDef->classLoader->allocator;
      rcache = new(alloc) CacheNode(enveloppe);
    } else {
      rcache = cache;
    }
    
    rcache->methPtr = dmeth->compiledPtr();
    rcache->lastCible = (UserClass*)ocl;
    
  }

  if (enveloppe->firstCache != rcache) {
    CacheNode *f = enveloppe->firstCache;
    enveloppe->firstCache = rcache;
    last->next = rcache->next;
    rcache->next = f;
  }
  
  enveloppe->cacheLock.release();
  
  return rcache->methPtr;
}

extern "C" void* virtualFieldLookup(UserClass* caller, uint32 index) {
  UserConstantPool* ctpInfo = caller->getConstantPool();
  if (ctpInfo->ctpRes[index]) {
    return ctpInfo->ctpRes[index];
  }
  
  UserCommonClass* cl = 0;
  const UTF8* utf8 = 0;
  Typedef* sign = 0;
  
  ctpInfo->resolveField(index, cl, utf8, sign);
 
  UserClass* lookup = cl->isArray() ? cl->super : cl->asClass();
  JavaField* field = lookup->lookupField(utf8, sign->keyName, false, true, 0);
  
  ctpInfo->ctpRes[index] = (void*)field->ptrOffset;
  
  return (void*)field->ptrOffset;
}

extern "C" void* staticFieldLookup(UserClass* caller, uint32 index) {
  UserConstantPool* ctpInfo = caller->getConstantPool();
  
  if (ctpInfo->ctpRes[index]) {
    return ctpInfo->ctpRes[index];
  }
  
  UserCommonClass* cl = 0;
  UserClass* fieldCl = 0;
  const UTF8* utf8 = 0;
  Typedef* sign = 0;
  
  ctpInfo->resolveField(index, cl, utf8, sign);
 
  assert(cl->asClass() && "Lookup a field of something not an array");
  JavaField* field = cl->asClass()->lookupField(utf8, sign->keyName, true,
                                                true, &fieldCl);
  
  fieldCl->initialiseClass(JavaThread::get()->getJVM());
  void* obj = ((UserClass*)fieldCl)->getStaticInstance();
  
  assert(obj && "No static instance in static field lookup");
  
  void* ptr = (void*)((uint64)obj + field->ptrOffset);
  ctpInfo->ctpRes[index] = ptr;
   
  return ptr;
}

#ifdef ISOLATE
extern "C" void* stringLookup(UserClass* cl, uint32 index) {
  UserConstantPool* ctpInfo = cl->getConstantPool();
  const UTF8* utf8 = ctpInfo->UTF8AtForString(index);
  JavaString* str = JavaThread::get()->getJVM()->internalUTF8ToStr(utf8);
#ifdef ISOLATE_SHARING
  ctpInfo->ctpRes[index] = str;
#endif
  return (void*)str;
}

#ifdef ISOLATE_SHARING
extern "C" void* enveloppeLookup(UserClass* cl, uint32 index) {
  UserConstantPool* ctpInfo = cl->getConstantPool();
  mvm::Allocator* allocator = cl->classLoader->allocator;
  Enveloppe* enveloppe = new(allocator) Enveloppe(ctpInfo, index);
  ctpInfo->ctpRes[index] = enveloppe;
  return (void*)enveloppe;
}

extern "C" void* staticCtpLookup(UserClass* cl, uint32 index) {
  UserConstantPool* ctpInfo = cl->getConstantPool();
  JavaConstantPool* shared = ctpInfo->getSharedPool();
  uint32 clIndex = shared->getClassIndexFromMethod(index);
  UserClass* refCl = (UserClass*)ctpInfo->loadClass(clIndex);
  refCl->initialiseClass(JavaThread::get()->getJVM());

  CommonClass* baseCl = 0;
  const UTF8* utf8 = 0;
  Signdef* sign = 0;

  shared->resolveMethod(index, baseCl, utf8, sign);
  UserClass* methodCl = 0;
  refCl->lookupMethod(utf8, sign->keyName, true, true, &methodCl);
  ctpInfo->ctpRes[index] = methodCl->getConstantPool();
  shared->ctpRes[clIndex] = refCl->classDef;
  return (void*)methodCl->getConstantPool();
}

extern "C" UserConstantPool* specialCtpLookup(UserConstantPool* ctpInfo,
                                              uint32 index,
                                              UserConstantPool** res) {
  JavaConstantPool* shared = ctpInfo->getSharedPool();
  uint32 clIndex = shared->getClassIndexFromMethod(index);
  UserClass* refCl = (UserClass*)ctpInfo->loadClass(clIndex);

  CommonClass* baseCl = 0;
  const UTF8* utf8 = 0;
  Signdef* sign = 0;

  shared->resolveMethod(index, baseCl, utf8, sign);
  UserClass* methodCl = 0;
  refCl->lookupMethod(utf8, sign->keyName, false, true, &methodCl);
  shared->ctpRes[clIndex] = refCl->classDef;
  *res = methodCl->getConstantPool();
  return methodCl->getConstantPool();
}

#endif

#endif

#ifndef WITHOUT_VTABLE
extern "C" void* vtableLookup(UserClass* caller, uint32 index, ...) {
  UserCommonClass* cl = 0;
  const UTF8* utf8 = 0;
  Signdef* sign = 0;
  
  caller->getConstantPool()->resolveMethod(index, cl, utf8, sign);
  UserClass* lookup = cl->isArray() ? cl->super : cl->asClass();
  JavaMethod* dmeth = lookup->lookupMethodDontThrow(utf8, sign->keyName, false,
                                                    true, 0);
  if (!dmeth) {
    va_list ap;
    va_start(ap, index);
    JavaObject* obj = va_arg(ap, JavaObject*);
    va_end(ap);
    assert((obj->classOf->isClass() && 
            obj->classOf->asClass()->isInitializing()) &&
           "Class not ready in a virtual lookup.");
    // Arg, the bytecode is buggy! Perform the lookup on the object class
    // and do not update offset.
    lookup = obj->classOf->isArray() ? obj->classOf->super : 
                                       obj->classOf->asClass();
    dmeth = lookup->lookupMethod(utf8, sign->keyName, false, true, 0);
  } else {
    caller->getConstantPool()->ctpRes[index] = (void*)dmeth->offset;
  }

#if !defined(ISOLATE_SHARING) && !defined(SERVICE)
  assert(dmeth->classDef->isInitializing() && 
         "Class not ready in a virtual lookup.");
#endif

  return (void*)dmeth->offset;
}
#endif

extern "C" void* classLookup(UserClass* caller, uint32 index) { 
  UserConstantPool* ctpInfo = caller->getConstantPool();
  UserClass* cl = (UserClass*)ctpInfo->loadClass(index);
  // We can not initialize here, because bytecodes such as CHECKCAST
  // or classes used in catch clauses do not trigger class initialization.
  // This is really sad, because we need to insert class initialization checks
  // in the LLVM code.
  assert(cl && "No cl after class lookup");
  return (void*)cl;
}


extern "C" void printMethodStart(JavaMethod* meth) {
  printf("[%p] executing %s\n", (void*)mvm::Thread::get(),
         meth->printString());
  fflush(stdout);
}

extern "C" void printMethodEnd(JavaMethod* meth) {
  printf("[%p] return from %s\n", (void*)mvm::Thread::get(),
         meth->printString());
  fflush(stdout);
}

extern "C" void printExecution(char* opcode, uint32 index, JavaMethod* meth) {
  printf("[%p] executing %s %s at %d\n", (void*)mvm::Thread::get(),
         meth->printString(), opcode, index);
  fflush(stdout);
}

extern "C" void jniProceedPendingException() {
  JavaThread* th = JavaThread::get();
  jmp_buf* buf = th->sjlj_buffers.back();
  th->sjlj_buffers.pop_back();
  mvm::Allocator& allocator = th->getJVM()->gcAllocator;
  allocator.freeTemporaryMemory(buf);
  if (JavaThread::get()->pendingException) {
    th->throwPendingException();
  }
}

extern "C" void* getSJLJBuffer() {
  JavaThread* th = JavaThread::get();
  mvm::Allocator& allocator = th->getJVM()->gcAllocator;
  void** buf = (void**)allocator.allocateTemporaryMemory(sizeof(jmp_buf));
  th->sjlj_buffers.push_back((jmp_buf*)buf);
  return (void*)buf;
}

extern "C" void jnjvmNullPointerException() {
  JavaThread::get()->getJVM()->nullPointerException("null");
}

extern "C" void negativeArraySizeException(sint32 val) {
  JavaThread::get()->getJVM()->negativeArraySizeException(val);
}

extern "C" void outOfMemoryError(sint32 val) {
  JavaThread::get()->getJVM()->outOfMemoryError(val);
}

extern "C" void jnjvmClassCastException(JavaObject* obj, UserCommonClass* cl) {
  JavaThread::get()->getJVM()->classCastException(obj, cl);
}

extern "C" void indexOutOfBoundsException(JavaObject* obj, sint32 index) {
  JavaThread::get()->getJVM()->indexOutOfBounds(obj, index);
}

extern "C" UserCommonClass* jnjvmRuntimeInitialiseClass(UserClass* cl) {
  cl->initialiseClass(JavaThread::get()->getJVM());
  return cl;
}

extern "C" JavaObject* jnjvmRuntimeDelegatee(UserCommonClass* cl) {
  Jnjvm* vm = JavaThread::get()->getJVM();
  return cl->getClassDelegatee(vm);
}

static JavaArray* multiCallNewIntern(UserClassArray* cl, uint32 len,
                                     sint32* dims, Jnjvm* vm) {
  if (len <= 0) JavaThread::get()->getJVM()->unknownError("Can not happen");
  JavaArray* _res = cl->doNew(dims[0], vm);
  if (len > 1) {
    ArrayObject* res = (ArrayObject*)_res;
    UserCommonClass* _base = cl->baseClass();
    if (_base->isArray()) {
      UserClassArray* base = (UserClassArray*)_base;
      if (dims[0] > 0) {
        for (sint32 i = 0; i < dims[0]; ++i) {
          res->elements[i] = multiCallNewIntern(base, (len - 1),
                                                &dims[1], vm);
        }
      } else {
        for (uint32 i = 1; i < len; ++i) {
          sint32 p = dims[i];
          if (p < 0) JavaThread::get()->getJVM()->negativeArraySizeException(p);
        }
      }
    } else {
      JavaThread::get()->getJVM()->unknownError("Can not happen");
    }
  }
  return _res;
}

extern "C" JavaArray* multiCallNew(UserClassArray* cl, uint32 len, ...) {
  va_list ap;
  va_start(ap, len);
  sint32* dims = (sint32*)alloca(sizeof(sint32) * len);
  for (uint32 i = 0; i < len; ++i){
    dims[i] = va_arg(ap, int);
  }
  Jnjvm* vm = JavaThread::get()->getJVM();
  return multiCallNewIntern(cl, len, dims, vm);
}

extern "C" UserClassArray* getArrayClass(UserCommonClass* cl,
                                         UserClassArray** dcl) {
  JnjvmClassLoader* JCL = cl->classLoader;
  if (cl->asClass()) cl->asClass()->resolveClass();
  const UTF8* arrayName = JCL->constructArrayName(1, cl->getName());
  
  UserClassArray* result = JCL->constructArray(arrayName);
  if (dcl) *dcl = result;
  return result;
}

extern "C" void JavaObjectAquire(JavaObject* obj) {
  obj->acquire();
}

extern "C" void JavaObjectRelease(JavaObject* obj) {
  obj->release();
}

extern "C" bool instanceOf(JavaObject* obj, UserCommonClass* cl) {
  return obj->instanceOf(cl);
}

extern "C" bool instantiationOfArray(UserCommonClass* cl1,
                                     UserClassArray* cl2) {
  return cl1->instantiationOfArray(cl2);
}

extern "C" bool implements(UserCommonClass* cl1, UserCommonClass* cl2) {
  return cl1->implements(cl2);
}

extern "C" bool isAssignableFrom(UserCommonClass* cl1, UserCommonClass* cl2) {
  return cl1->isAssignableFrom(cl2);
}

extern "C" void* JavaThreadGetException() {
  return JavaThread::getException();
}

extern "C" void JavaThreadThrowException(JavaObject* obj) {
  return JavaThread::throwException(obj);
}

extern "C" JavaObject* JavaThreadGetJavaException() {
  return JavaThread::getJavaException();
}

extern "C" bool JavaThreadCompareException(UserClass* cl) {
  return JavaThread::compareException(cl);
}

extern "C" void JavaThreadClearException() {
  return JavaThread::clearException();
}

extern "C" void overflowThinLock(JavaObject* obj) {
  obj->overflowThinLock();
}

#ifdef SERVICE

extern "C" void serviceCallStart(Jnjvm* OldService,
                                 Jnjvm* NewService) {
  fprintf(stderr, "I have switched from %d to %d\n", OldService->IsolateID,
          NewService->IsolateID);

  fprintf(stderr, "Now the thread id is %d\n", mvm::Thread::get()->IsolateID);
}

extern "C" void serviceCallStop(Jnjvm* OldService,
                                Jnjvm* NewService) {
  fprintf(stderr, "End service call\n");
}

#endif
