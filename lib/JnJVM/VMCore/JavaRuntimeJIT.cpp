//===-------------------- JavaRuntimeJIT.cpp ------------------------------===//
//=== ---- Runtime functions called by code compiled by the JIT -----------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <stdarg.h>

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

#ifdef SERVICE_VM
#include "ServiceDomain.h"
#endif

using namespace jnjvm;

#ifdef MULTIPLE_VM
extern "C" JavaString* stringLookup(Class* cl, uint32 index) {
  const UTF8* utf8 = cl->ctpInfo->UTF8At(cl->ctpInfo->ctpDef[index]);
  JavaString* str = JavaThread::get()->isolate->UTF8ToStr(utf8);
  return str;
}
#endif

extern "C" void* jnjvmVirtualLookup(CacheNode* cache, JavaObject *obj) {
  Enveloppe* enveloppe = cache->enveloppe;
  JavaConstantPool* ctpInfo = enveloppe->ctpInfo;
  CommonClass* ocl = obj->classOf;
  CommonClass* cl = 0;
  const UTF8* utf8 = 0;
  Signdef* sign = 0;
  uint32 index = enveloppe->index;
  
  ctpInfo->resolveMethod(index, cl, utf8, sign);
  assert(obj->classOf->isReady() && "Class not ready in a virtual lookup.");

  enveloppe->cacheLock->lock();
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
    JavaMethod* dmeth = ocl->lookupMethod(utf8, sign->keyName, false, true);
    assert(dmeth->classDef->isReady() &&
           "Class not ready in a virtual lookup.");
    if (cache->methPtr) {
      rcache = new CacheNode(enveloppe);
    } else {
      rcache = cache;
    }
    
    rcache->methPtr = dmeth->compiledPtr();
    rcache->lastCible = (Class*)ocl;
    
  }

  if (enveloppe->firstCache != rcache) {
    CacheNode *f = enveloppe->firstCache;
    enveloppe->firstCache = rcache;
    last->next = rcache->next;
    rcache->next = f;
  }
  
  enveloppe->cacheLock->unlock();
  
  return rcache->methPtr;
}

extern "C" void* fieldLookup(JavaObject* obj, Class* caller, uint32 index,
                             uint32 stat) {
  JavaConstantPool* ctpInfo = caller->ctpInfo;
  if (ctpInfo->ctpRes[index]) {
    return ctpInfo->ctpRes[index];
  }
  
  CommonClass* cl = 0;
  const UTF8* utf8 = 0;
  Typedef* sign = 0;
  
  ctpInfo->resolveField(index, cl, utf8, sign);
  
  JavaField* field = cl->lookupField(utf8, sign->keyName, stat, true);
  
  void* ptr = 0;
  if (stat) {
    Class* fieldCl = field->classDef;
    JavaThread::get()->isolate->initialiseClass(fieldCl);
    ptr = (void*)((uint64)(fieldCl->staticInstance()) + field->ptrOffset);
#ifndef MULTIPLE_VM
    ctpInfo->ctpRes[index] = ptr;
#endif
  } else {
    ptr = (void*)((uint64)obj + field->ptrOffset);
    ctpInfo->ctpRes[index] = (void*)field->ptrOffset;
  }
  
  return ptr;
}

extern "C" void printMethodStart(JavaMethod* meth) {
  printf("[%d] executing %s\n", mvm::Thread::self(), meth->printString());
  fflush(stdout);
}

extern "C" void printMethodEnd(JavaMethod* meth) {
  printf("[%d] return from %s\n", mvm::Thread::self(), meth->printString());
  fflush(stdout);
}

extern "C" void printExecution(char* opcode, uint32 index, JavaMethod* meth) {
  printf("[%d] executing %s %s at %d\n", mvm::Thread::self(), meth->printString(), 
                                   opcode, index);
  fflush(stdout);
}

extern "C" void jniProceedPendingException() {
  JavaThread* th = JavaThread::get();
  jmp_buf* buf = th->sjlj_buffers.back();
  th->sjlj_buffers.pop_back();
  free(buf);
  if (JavaThread::get()->pendingException) {
    th->throwPendingException();
  }
}

extern "C" void* getSJLJBuffer() {
  JavaThread* th = JavaThread::get();
  void** buf = (void**)malloc(sizeof(jmp_buf));
  th->sjlj_buffers.push_back((jmp_buf*)buf);
  return (void*)buf;
}

extern "C" void jnjvmNullPointerException() {
  JavaThread::get()->isolate->nullPointerException("null");
}

extern "C" void negativeArraySizeException(sint32 val) {
  JavaThread::get()->isolate->negativeArraySizeException(val);
}

extern "C" void outOfMemoryError(sint32 val) {
  JavaThread::get()->isolate->outOfMemoryError(val);
}

extern "C" void jnjvmClassCastException(JavaObject* obj, CommonClass* cl) {
  JavaThread::get()->isolate->classCastException("");
}

extern "C" void indexOutOfBoundsException(JavaObject* obj, sint32 index) {
  JavaThread::get()->isolate->indexOutOfBounds(obj, index);
}

#ifdef MULTIPLE_VM
extern "C" JavaObject* getStaticInstance(Class* cl, Jnjvm* vm) {
  std::pair<JavaState, JavaObject*>* val = vm->statics->lookup(cl);
  if (!val || !(val->second)) {
    vm->initialiseClass(cl);
    val = vm->statics->lookup(cl);
  }
  return val->second;
}
#endif

extern "C" CommonClass* initialisationCheck(CommonClass* cl) {
  JavaThread::get()->isolate->initialiseClass(cl);
  return cl;
}

extern "C" JavaObject* getClassDelegatee(CommonClass* cl) {
  Jnjvm* vm = JavaThread::get()->isolate;
  return vm->getClassDelegatee(cl);
}

extern "C" Class* newLookup(Class* caller, uint32 index) { 
  JavaConstantPool* ctpInfo = caller->ctpInfo;
  Class* cl = (Class*)ctpInfo->loadClass(index);
  return cl;
}

#ifndef WITHOUT_VTABLE
extern "C" uint32 vtableLookup(JavaObject* obj, Class* caller, uint32 index) {
  CommonClass* cl = 0;
  const UTF8* utf8 = 0;
  Signdef* sign = 0;
  
  caller->ctpInfo->resolveMethod(index, cl, utf8, sign);
  JavaMethod* dmeth = cl->lookupMethodDontThrow(utf8, sign->keyName, false,
                                                true);
  assert(obj->classOf->isReady() && "Class not ready in a virtual lookup.");
  if (!dmeth) {
    // Arg, it should have been an invoke interface.... Perform the lookup
    // on the object class and do not update offset.
    dmeth = obj->classOf->lookupMethod(utf8, sign->keyName, false, true);
  } else {
    caller->ctpInfo->ctpRes[index] = (void*)dmeth->offset;
  }
  
  assert(dmeth->classDef->isReady() && "Class not ready in a virtual lookup.");

  return dmeth->offset;
}
#endif


static JavaArray* multiCallNewIntern(arrayCtor_t ctor, ClassArray* cl,
                                     uint32 len,
                                     sint32* dims,
                                     Jnjvm* vm) {
  if (len <= 0) JavaThread::get()->isolate->unknownError("Can not happen");
  JavaArray* _res = ctor(dims[0], cl, vm);
  if (len > 1) {
    ArrayObject* res = (ArrayObject*)_res;
    CommonClass* _base = cl->baseClass();
    if (_base->isArray) {
      ClassArray* base = (ClassArray*)_base;
      AssessorDesc* func = base->funcs();
      arrayCtor_t newCtor = func->arrayCtor;
      if (dims[0] > 0) {
        for (sint32 i = 0; i < dims[0]; ++i) {
          res->elements[i] = multiCallNewIntern(newCtor, base, (len - 1),
                                                &dims[1], vm);
        }
      } else {
        for (uint32 i = 1; i < len; ++i) {
          sint32 p = dims[i];
          if (p < 0) JavaThread::get()->isolate->negativeArraySizeException(p);
        }
      }
    } else {
      JavaThread::get()->isolate->unknownError("Can not happen");
    }
  }
  return _res;
}

extern "C" JavaArray* multiCallNew(ClassArray* cl, uint32 len, ...) {
  va_list ap;
  va_start(ap, len);
  sint32* dims = (sint32*)alloca(sizeof(sint32) * len);
  for (uint32 i = 0; i < len; ++i){
    dims[i] = va_arg(ap, int);
  }
#ifdef MULTIPLE_VM
  Jnjvm* vm = va_arg(ap, Jnjvm*);
#else
  Jnjvm* vm = 0;
#endif
  return multiCallNewIntern((arrayCtor_t)ArrayObject::acons, cl, len, dims, vm);
}

extern "C" void JavaObjectAquire(JavaObject* obj) {
#ifdef SERVICE_VM
  ServiceDomain* vm = (ServiceDomain*)JavaThread::get()->isolate;
  if (!(vm->GC->isMyObject(obj))) {
    vm->serviceError(vm, "I'm locking an object I don't own");
  }
#endif
  obj->acquire();
}


extern "C" void JavaObjectRelease(JavaObject* obj) {
  verifyNull(obj);
#ifdef SERVICE_VM
  ServiceDomain* vm = (ServiceDomain*)JavaThread::get()->isolate;
  if (!(vm->GC->isMyObject(obj))) {
    vm->serviceError(vm, "I'm unlocking an object I don't own");
  }
#endif
  obj->release();
}

#ifdef SERVICE_VM
extern "C" void JavaObjectAquireInSharedDomain(JavaObject* obj) {
  verifyNull(obj);
  obj->acquire();
}

extern "C" void JavaObjectReleaseInSharedDomain(JavaObject* obj) {
  verifyNull(obj);
  obj->release();
}
#endif

extern "C" bool instanceOf(JavaObject* obj, CommonClass* cl) {
  return obj->instanceOf(cl);
}

extern "C" bool instantiationOfArray(CommonClass* cl1, ClassArray* cl2) {
  return cl1->instantiationOfArray(cl2);
}

extern "C" bool implements(CommonClass* cl1, CommonClass* cl2) {
  return cl1->implements(cl2);
}

extern "C" bool isAssignableFrom(CommonClass* cl1, CommonClass* cl2) {
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

extern "C" bool JavaThreadCompareException(Class* cl) {
  return JavaThread::compareException(cl);
}

extern "C" void JavaThreadClearException() {
  return JavaThread::clearException();
}

extern "C" uint32 getThreadID() {
  return JavaThread::get()->threadID;
}

extern "C" void overflowThinLock(JavaObject* obj) {
  obj->overflowThinlock();
}
