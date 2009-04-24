//===--------- Object.cc - Common objects for vmlets ----------------------===//
//
//                     The Micro Virtual Machine
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <cstdio>
#include <cstdlib>

#include "MvmGC.h"
#include "mvm/Allocator.h"
#include "mvm/Object.h"
#include "mvm/PrintBuffer.h"
#include "mvm/VirtualMachine.h"
#include "mvm/Threads/Thread.h"

using namespace mvm;


VirtualTable *NativeString::VT = 0;
VirtualTable *PrintBuffer::VT = 0;

extern "C" void printFloat(float f) {
  fprintf(stderr, "%f\n", f);
}

extern "C" void printDouble(double d) {
  fprintf(stderr, "%f\n", d);
}

extern "C" void printLong(sint64 l) {
  fprintf(stderr, "%lld\n", (long long int)l);
}

extern "C" void printInt(sint32 i) {
  fprintf(stderr, "%d\n", i);
}

extern "C" void printObject(mvm::Object* obj) {
  fprintf(stderr, "%s\n", obj->printString());
}


void Object::initialise() {
# define INIT(X) { \
  X fake; \
  X::VT = ((VirtualTable**)(void*)(&fake))[0]; }
  
  INIT(NativeString);
  INIT(PrintBuffer);
  
#undef INIT
}

void PrintBuffer::TRACER {
  ((PrintBuffer *)this)->contents()->MARK_AND_TRACE;
}


PrintBuffer *PrintBuffer::writeObj(const Object *obj) {
  Object *beg = (Object*)Collector::begOf(obj);
  
  if(beg) {
    if(beg == obj) {
      obj->print((mvm::PrintBuffer*)this);
    } else {
      write("<In Object [");
      beg->print(this);
      write("] -- offset ");
      writeS4((intptr_t)obj - (intptr_t)beg);
      write(">");
    }
  } else {
    write("<DirectValue: ");
    writeS4((intptr_t)obj);
    write(">");
  }
  return this;
}

extern "C" void write_ptr(PrintBuffer* buf, void* obj) {
  buf->writePtr(obj);
}

extern "C" void write_int(PrintBuffer* buf, int a) {
  buf->writeS4(a);
}

extern "C" void write_str(PrintBuffer* buf, char* a) {
  buf->write(a);
}

char *Object::printString(void) const {
  PrintBuffer *buf= PrintBuffer::alloc();
  buf->writeObj(this);
  return buf->contents()->cString();
}

void Object::print(PrintBuffer *buf) const {
  buf->write("<Object@");
  buf->writePtr((void*)this);
  buf->write(">");
}

void NativeString::print(PrintBuffer *buf) const {
  NativeString *const self= (NativeString *)this;
  buf->write("\"");
  for (size_t i= 0; i < strlen(self->cString()); ++i) {
    int c= self->cString()[i];
    switch (c) {
      case '\b': buf->write("\\b"); break;
      case '\f': buf->write("\\f"); break;
      case '\n': buf->write("\\n"); break;
      case '\r': buf->write("\\r"); break;
      case '\t': buf->write("\\t"); break;
      case '"':  buf->write("\\\""); break;
      default: {
        char esc[32];
        if (c < 32)
          sprintf(esc, "\\x%02x", c);
        else
          sprintf(esc, "%c", c);
        buf->write(esc);
      }
    }
  }
  buf->write("\"");
}


void VirtualMachine::finalizerStart(mvm::Thread* th) {
  VirtualMachine* vm = th->MyVM;

  while (true) {
    vm->FinalizationLock.lock();
    while (vm->CurrentFinalizedIndex == 0) {
      vm->FinalizationCond.wait(&vm->FinalizationLock);
    }
    vm->FinalizationLock.unlock();

    while (true) {
      vm->FinalizationQueueLock.acquire();
      gc* res = 0;
      if (vm->CurrentFinalizedIndex != 0) {
        res = vm->ToBeFinalized[--vm->CurrentFinalizedIndex];
      }
      vm->FinalizationQueueLock.release();
      if (!res) break;

      VirtualTable* VT = res->getVirtualTable();
      try {
        if (VT->operatorDelete) {
          // It's a native method!
          destructor_t dest = (destructor_t)VT->destructor;
          dest(res);
        } else {
          vm->invokeFinalizer(res);
        }
      } catch(...) {
      }
    }
  }
}

void VirtualMachine::growQueue() {
  if (CurrentIndex >= QueueLength) {
    uint32 newLength = QueueLength * GROW_FACTOR;
    gc** newQueue = new gc*[newLength];
    for (uint32 i = 0; i < QueueLength; ++i) newQueue[i] = FinalizationQueue[i];
    FinalizationQueue = newQueue;
    QueueLength = newLength;
    
    newLength = ToBeFinalizedLength * GROW_FACTOR;
    newQueue = new gc*[newLength];
    for (uint32 i = 0; i < ToBeFinalizedLength; ++i) newQueue[i] = ToBeFinalized[i];
    ToBeFinalized = newQueue;
    ToBeFinalizedLength = newLength;
  }
}


void VirtualMachine::addFinalizationCandidate(gc* obj) {
  FinalizationQueueLock.acquire();
 
  if (CurrentIndex >= QueueLength) {
    growQueue();
  }
  
  FinalizationQueue[CurrentIndex++] = obj;
  FinalizationQueueLock.release();
}

void VirtualMachine::scanFinalizationQueue() {
  FinalizationQueueLock.acquire();
  uint32 NewIndex = 0;
  for (uint32 i = 0; i < CurrentIndex; ++i) {
    gc* obj = FinalizationQueue[i];

    if (!Collector::isLive(obj)) {
      obj->markAndTrace();
      /* Add to object table */
      ToBeFinalized[CurrentFinalizedIndex++] = obj;
    } else {
      FinalizationQueue[NewIndex++] = obj;
    }
  }
  CurrentIndex = NewIndex;

  for (uint32 i = 0; i < CurrentFinalizedIndex; ++i) {
    gc* obj = ToBeFinalized[i];
    obj->markAndTrace();
  }
  FinalizationQueueLock.release();

}
