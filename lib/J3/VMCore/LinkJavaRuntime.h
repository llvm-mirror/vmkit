//===--------------------- LinkJavaRuntime.h ------------------------------===//
//=== ------------- Reference all runtime functions -----------------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef JNJVM_LINK_JAVA_RUNTIME_H
#define JNJVM_LINK_JAVA_RUNTIME_H


namespace jnjvm {
  class JavaObject;
  class UserClass;
  class UserClassArray;
  class UserCommonClass;
  class UserConstantPool;
  class JavaVirtualTable;
  class JavaMethod;
  class Jnjvm;
}

using namespace jnjvm;

extern "C" void* jnjvmInterfaceLookup(UserClass* caller, uint32 index);
extern "C" void* jnjvmVirtualFieldLookup(UserClass* caller, uint32 index);
extern "C" void* jnjvmStaticFieldLookup(UserClass* caller, uint32 index);
extern "C" void* jnjvmVirtualTableLookup(UserClass* caller, uint32 index, ...);
extern "C" void* jnjvmStringLookup(UserClass* cl, uint32 index);
extern "C" void* jnjvmClassLookup(UserClass* caller, uint32 index);
extern "C" UserCommonClass* jnjvmRuntimeInitialiseClass(UserClass* cl);
extern "C" JavaObject* jnjvmRuntimeDelegatee(UserCommonClass* cl);
extern "C" JavaArray* jnjvmMultiCallNew(UserClassArray* cl, uint32 len, ...);
extern "C" UserClassArray* jnjvmGetArrayClass(UserCommonClass*,
                                              UserClassArray**);
extern "C" void jnjvmEndJNI();
extern "C" void* jnjvmStartJNI();
extern "C" void jnjvmJavaObjectAquire(JavaObject* obj);
extern "C" void jnjvmJavaObjectRelease(JavaObject* obj);
extern "C" void jnjvmThrowException(JavaObject* obj);
extern "C" void jnjvmOverflowThinLock(JavaObject* obj);
extern "C" JavaObject* jnjvmNullPointerException();
extern "C" JavaObject* jnjvmNegativeArraySizeException(sint32 val);
extern "C" JavaObject* jnjvmOutOfMemoryError(sint32 val);
extern "C" JavaObject* jnjvmStackOverflowError();
extern "C" JavaObject* jnjvmArithmeticException();
extern "C" JavaObject* jnjvmClassCastException(JavaObject* obj,
                                               UserCommonClass* cl);
extern "C" JavaObject* jnjvmIndexOutOfBoundsException(JavaObject* obj,
                                                      sint32 index);
extern "C" JavaObject* jnjvmArrayStoreException(JavaVirtualTable* VT);
extern "C" void jnjvmThrowExceptionFromJIT();
extern "C" void jnjvmPrintMethodStart(JavaMethod* meth);
extern "C" void jnjvmPrintMethodEnd(JavaMethod* meth);
extern "C" void jnjvmPrintExecution(uint32 opcode, uint32 index,
                                    JavaMethod* meth);


#ifdef SERVICE
extern "C" void jnjvmServiceCallStart(Jnjvm* OldService,
                                      Jnjvm* NewService);
extern "C" void jnjvmServiceCallStop(Jnjvm* OldService,
                                     Jnjvm* NewService);
#endif




#ifdef ISOLATE_SHARING
extern "C" void* jnjvmStaticCtpLookup(UserClass* cl, uint32 index);
extern "C" UserConstantPool* jnjvmSpecialCtpLookup(UserConstantPool* ctpInfo,
                                                   uint32 index,
                                                   UserConstantPool** res);
#endif


namespace {
  struct ForceRuntimeLinking {
    ForceRuntimeLinking() {
      // We must reference the passes in such a way that compilers will not
      // delete it all as dead code, even with whole program optimization,
      // yet is effectively a NO-OP. As the compiler isn't smart enough
      // to know that getenv() never returns -1, this will do the job.
      if (std::getenv("bar") != (char*) -1) 
        return;
      
      (void) jnjvmInterfaceLookup(0, 0);
      (void) jnjvmVirtualFieldLookup(0, 0);
      (void) jnjvmStaticFieldLookup(0, 0);
      (void) jnjvmVirtualTableLookup(0, 0);
      (void) jnjvmClassLookup(0, 0);
      (void) jnjvmRuntimeInitialiseClass(0);
      (void) jnjvmRuntimeDelegatee(0);
      (void) jnjvmMultiCallNew(0, 0);
      (void) jnjvmGetArrayClass(0, 0);
      (void) jnjvmEndJNI();
      (void) jnjvmStartJNI();
      (void) jnjvmJavaObjectAquire(0);
      (void) jnjvmJavaObjectRelease(0);
      (void) jnjvmThrowException(0);
      (void) jnjvmOverflowThinLock(0);
      (void) jnjvmNullPointerException();
      (void) jnjvmNegativeArraySizeException(0);
      (void) jnjvmOutOfMemoryError(0);
      (void) jnjvmStackOverflowError();
      (void) jnjvmArithmeticException();
      (void) jnjvmClassCastException(0, 0);
      (void) jnjvmIndexOutOfBoundsException(0, 0);
      (void) jnjvmArrayStoreException(0);
      (void) jnjvmThrowExceptionFromJIT();
      (void) jnjvmPrintMethodStart(0);
      (void) jnjvmPrintMethodEnd(0);
      (void) jnjvmPrintExecution(0, 0, 0);
      (void) jnjvmStringLookup(0, 0);
#ifdef SERVICE
      (void) jnjvmServiceCallStart(0, 0);
      (void) jnjvmServiceCallStop(0, 0);
#endif

#ifdef ISOLATE_SHARING
      (void) jnjvmStaticCtpLookup(0, 0);
      (void) jnjvmSpecialCtpLookup(0, 0, 0);
#endif

    }
  } ForcePassLinking; // Force link by creating a global definition.
}
  


#endif //JNJVM_LINK_JAVA_RUNTIME_H
