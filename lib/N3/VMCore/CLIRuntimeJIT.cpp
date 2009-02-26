//===--- CLIRuntimeJIT.cpp - Runtime functions for the JIT compiled code --===//
//
//                              The vmkit project
//
// This file is distributed under the University Of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//


#include <cstdio>

#include "llvm/DerivedTypes.h"
#include "llvm/Instructions.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"

#include "mvm/JIT.h"
#include "mvm/Object.h"
#include "mvm/PrintBuffer.h"
#include "mvm/Threads/Locks.h"

#include "CLIString.h"
#include "MSCorlib.h"
#include "N3.h"
#include "VMArray.h"
#include "VMCache.h"
#include "VMClass.h"
#include "VMObject.h"
#include "VMThread.h"

#include <cstdarg>

using namespace n3;
using namespace llvm;

extern "C" VMObject* initialiseClass(VMClass* cl) {
  cl->clinitClass(NULL);
  return cl->staticInstance;
}

extern "C" void n3ClassCastException() {
  fflush(stdout);
  assert(0 && "implement class cast exception");
}

extern "C" void n3NullPointerException() {
  fflush(stdout);
  assert(0 && "implement null pointer exception");
}

extern "C" void indexOutOfBounds() {
  fflush(stdout);
  assert(0 && "implement index out of bounds exception");
}

extern "C" VMObject* newString(const UTF8* utf8) {
  CLIString * str = 
    (CLIString*)(((N3*)VMThread::get()->vm)->UTF8ToStr(utf8));
  return str;
}

extern "C" bool n3InstanceOf(VMObject* obj, VMCommonClass* cl) {
  return obj->instanceOf(cl);
}

extern "C" void* GetCppException() {
  return VMThread::getCppException();
}

extern "C" void ThrowException(VMObject* obj) {
  return VMThread::throwException(obj);
}

extern "C" VMObject* GetCLIException() {
  return VMThread::getCLIException();
}

extern "C" bool CompareException(VMClass* cl) {
  return VMThread::compareException(cl);
}

extern "C" void ClearException() {
  return VMThread::get()->clearException();
}

static VMObject* doMultiNewIntern(VMClassArray* cl, uint32 dim, sint32* buf) {
  if (dim <= 0) VMThread::get()->vm->error("Can't happen");
  sint32 n = buf[0];
  if (n < 0) VMThread::get()->vm->negativeArraySizeException(n);
  
  VMArray* res = (VMArray*)cl->doNew(n);
  if (dim > 1) {
    VMCommonClass* base = cl->baseClass;
    if (n > 0) {
      for (sint32 i = 0; i < n; ++i) {
        res->elements[i] = doMultiNewIntern((VMClassArray*)base, dim - 1, &(buf[1]));
      }   
    }   
    for (uint32 i = 1; i < dim; ++i) {
      if (buf[i] < 0) VMThread::get()->vm->negativeArraySizeException(buf[i]);
    }   
  }
  return res;
}

extern "C" VMObject* doMultiNew(VMClassArray* cl, ...) {
  sint32* dimSizes = (sint32*)alloca(cl->dims * sizeof(sint32));
  va_list ap; 
  va_start(ap, cl);
  for (uint32 i = 0; i < cl->dims; ++i) {
    dimSizes[i] = va_arg(ap, sint32);
  }
  va_end(ap);
  return doMultiNewIntern(cl, cl->dims, dimSizes);
}

extern "C" CacheNode* n3VirtualLookup(CacheNode* cache, VMObject *obj) {
  Enveloppe* enveloppe = cache->enveloppe;
  VMCommonClass* ocl = obj->classOf;
  VMMethod* orig = enveloppe->originalMethod;
  
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
    VMMethod* dmeth = ocl->lookupMethodDontThrow(orig->name,
                                                 orig->parameters,
                                                 false, true);
    if (dmeth == 0) {
      char* methAsciiz = orig->name->UTF8ToAsciiz();
      char* nameAsciiz = orig->classDef->name->UTF8ToAsciiz();
      char* nameSpaceAsciiz = orig->classDef->nameSpace->UTF8ToAsciiz();

      char *buf = (char*)alloca(3 + strlen(methAsciiz) + 
                                    strlen(nameAsciiz) + 
                                    strlen(nameSpaceAsciiz));
      sprintf(buf, "%s.%s.%s", nameSpaceAsciiz, nameAsciiz, methAsciiz);
      const UTF8* newName = VMThread::get()->vm->asciizConstructUTF8(buf);
      dmeth = ocl->lookupMethod(newName, orig->parameters, false, true);
    }
    
    if (cache->methPtr) {
      rcache = CacheNode::allocate();
      rcache->enveloppe = enveloppe;
    } else {
      rcache = cache;
    }
    
    Function* func = dmeth->compiledPtr(NULL);
    rcache->methPtr = mvm::MvmModule::executionEngine->getPointerToGlobal(func);
    rcache->lastCible = (VMClass*)ocl;
    rcache->box = (dmeth->classDef->super == MSCorlib::pValue);
  }

  if (enveloppe->firstCache != rcache) {
    CacheNode *f = enveloppe->firstCache;
    enveloppe->firstCache = rcache;
    last->next = rcache->next;
    rcache->next = f;

  }
  
  enveloppe->cacheLock->unlock();
  
  return rcache;
}

extern "C" VMObject* newObject(VMClass* cl) {
  return cl->doNew();
}

extern "C" VMObject* newArray(VMClassArray* cl, sint32 nb) {
  return cl->doNew(nb);
}
