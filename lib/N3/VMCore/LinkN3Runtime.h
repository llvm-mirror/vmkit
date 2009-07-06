//===----------------------- LinkN3Runtime.h ------------------------------===//
//=== ------------- Reference all runtime functions -----------------------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef JNJVM_LINK_N3_RUNTIME_H
#define JNJVM_LINK_N3_RUNTIME_H


namespace n3 {
  class CacheNode;
  class UTF8;
  class VMClass;
  class VMClassArray;
  class VMCommonClass;
  class VMObject;
}

using namespace n3;


extern "C" VMObject* initialiseClass(VMClass* cl);
extern "C" void n3ClassCastException();
extern "C" void n3NullPointerException();
extern "C" void indexOutOfBounds();
extern "C" VMObject* newString(const UTF8* utf8);
extern "C" bool n3InstanceOf(VMObject* obj, VMCommonClass* cl);
extern "C" void* GetCppException();
extern "C" void ThrowException(VMObject* obj);
extern "C" VMObject* GetCLIException();
extern "C" bool CompareException(VMClass* cl);
extern "C" void ClearException();
extern "C" VMObject* doMultiNew(VMClassArray* cl, ...);
extern "C" CacheNode* n3VirtualLookup(CacheNode* cache, VMObject *obj);
extern "C" VMObject* newObject(VMClass* cl);
extern "C" VMObject* newArray(VMClassArray* cl, sint32 nb);

namespace {
  struct ForceRuntimeLinking {
    ForceRuntimeLinking() {
      // We must reference the passes in such a way that compilers will not
      // delete it all as dead code, even with whole program optimization,
      // yet is effectively a NO-OP. As the compiler isn't smart enough
      // to know that getenv() never returns -1, this will do the job.
      if (std::getenv("bar") != (char*) -1) 
        return;
      (void) initialiseClass(0);
      (void) n3ClassCastException();
      (void )n3NullPointerException();
      (void) indexOutOfBounds();
      (void) newString(0);
      (void) n3InstanceOf(0, 0);
      (void) GetCppException();
      (void) ThrowException(0);
      (void) GetCLIException();
      (void) CompareException(0);
      (void) ClearException();
      (void) doMultiNew(0);
      (void) n3VirtualLookup(0, 0);
      (void) newObject(0);
      (void) newArray(0, 0);
    }
  } ForcePassLinking; // Force link by creating a global definition.
}
  


#endif //JNJVM_LINK_N3_RUNTIME_H
