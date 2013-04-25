#include "VMStaticInstance.h"
#include <stdlib.h>

extern "C" {

JNIEXPORT void JNICALL Java_sun_misc_Unsafe_unpark(
JavaObject* unsafe, JavaObject* thread) {
//return;
  llvm_gcroot(unsafe, 0);
  llvm_gcroot(thread, 0);
  JavaObject* vmthread;
  llvm_gcroot(vmthread, 0);
  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField* field = vm->upcalls->vmdataVMThread;
  JavaField* field2 = vm->upcalls->vmThread; 
  
  // It's possible that the thread to be interrupted has not finished
  // its initialization. Wait until the initialization is done.
  vmthread = field2->getInstanceObjectField(thread);
  while (vmthread == 0) {
  	JavaThread::yield();
  	vmthread = field2->getInstanceObjectField(thread);
  }
  JavaThread* th = (JavaThread*)field->getInstanceObjectField(vmthread);
  while (th == 0) {
  	JavaThread::yield();
  	fprintf(stderr, "Case 1\n");
  	th = (JavaThread*)field->getInstanceObjectField(vmthread);
  }
  //fprintf(stderr, "unparking started %lld\n", th->getThreadID());
  th->parkLock.unpark();
  //fprintf(stderr, "unparking done %lld\n", th->getThreadID());
}

}
