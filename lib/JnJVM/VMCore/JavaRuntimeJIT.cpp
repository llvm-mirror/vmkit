//===-------------------- JavaRuntimeJIT.cpp ------------------------------===//
//=== ---- Runtime functions called by code compiled by the JIT -----------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/Function.h"

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

extern "C" JavaString* runtimeUTF8ToStr(const UTF8* val) {
  Jnjvm* vm = JavaThread::get()->isolate;
  return vm->UTF8ToStr(val);
}

extern "C" void* virtualLookup(CacheNode* cache, JavaObject *obj) {
  Enveloppe* enveloppe = cache->enveloppe;
  JavaCtpInfo* ctpInfo = enveloppe->ctpInfo;
  CommonClass* ocl = obj->classOf;
  CommonClass* cl = 0;
  const UTF8* utf8 = 0;
  Signdef* sign = 0;
  uint32 index = enveloppe->index;
  
  ctpInfo->resolveInterfaceOrMethod(index, cl, utf8, sign);

  CacheNode* rcache = 0;
  CacheNode* tmp = enveloppe->firstCache;
  CacheNode* last = tmp;
  enveloppe->cacheLock->lock();

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
    if (cache->methPtr) {
      rcache = CacheNode::allocate();
      rcache->enveloppe = enveloppe;
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
                             uint32 stat, void** ifStatic, uint32* offset) {
  JavaCtpInfo* ctpInfo = caller->ctpInfo;
  if (ctpInfo->ctpRes[index]) {
    JavaField* field = (JavaField*)(ctpInfo->ctpRes[index]);
    field->classDef->initialiseClass();
    if (stat) obj = field->classDef->staticInstance();
    void* ptr = (void*)(field->ptrOffset + (uint64)obj);
    if (stat) *ifStatic = ptr;
    *offset = (uint32)field->ptrOffset;
    return ptr;
  }
  
  CommonClass* cl = 0;
  const UTF8* utf8 = 0;
  Typedef* sign = 0;
  
  ctpInfo->resolveField(index, cl, utf8, sign);
  
  JavaField* field = cl->lookupField(utf8, sign->keyName, stat, true);
  field->classDef->initialiseClass();
  
  if (stat) obj = ((Class*)cl)->staticInstance();
  void* ptr = (void*)((uint64)obj + field->ptrOffset);
  
  ctpInfo->ctpRes[index] = field;

  if (stat) *ifStatic = ptr;
  *offset = (uint32)field->ptrOffset;

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

extern "C" void nullPointerException() {
  JavaThread::get()->isolate->nullPointerException("null");
}

extern "C" void classCastException(JavaObject* obj, CommonClass* cl) {
  JavaThread::get()->isolate->classCastException("");
}

extern "C" void indexOutOfBoundsException(JavaObject* obj, sint32 index) {
  JavaThread::get()->isolate->indexOutOfBounds(obj, index);
}

#ifndef SINGLE_VM
extern "C" JavaObject* getStaticInstance(Class* cl) {
  if (cl->isolate == Jnjvm::bootstrapVM) {
    Jnjvm* vm = JavaThread::get()->isolate;
    std::pair<uint8, JavaObject*>* val = vm->statics->lookup(cl);
    if (!val) {
      cl->initialiseClass();
      val = vm->statics->lookup(cl);
    }
    return val->second;
  } else {
    return cl->_staticInstance;
  }
}

extern "C" JavaObject* getClassDelegatee(CommonClass* cl) {
  Jnjvm* vm = JavaThread::get()->isolate;
  return vm->getClassDelegatee(cl);
}
#endif

void JavaJIT::runtimeInitialise() {
  void* p;
  p = (void*)&runtimeUTF8ToStr;
  p = (void*)&fieldLookup;
  p = (void*)&virtualLookup;
  p = (void*)&printExecution;
  p = (void*)&jniProceedPendingException;
  p = (void*)&nullPointerException;
  p = (void*)&classCastException;
  p = (void*)&indexOutOfBoundsException;
#ifndef SINGLE_VM
  p = (void*)&getStaticInstance;
  p = (void*)&getClassDelegatee;
#endif
}

extern "C" Class* newLookup(Class* caller, uint32 index, Class** toAlloc) { 
  JavaCtpInfo* ctpInfo = caller->ctpInfo;
  Class* cl = (Class*)ctpInfo->loadClass(index);
  cl->resolveClass(true);
  
  *toAlloc = cl;
  return cl;
}
