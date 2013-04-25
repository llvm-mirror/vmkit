#include "VMStaticInstance.h"
#include <stdlib.h>

extern "C" {

JNIEXPORT void JNICALL Java_sun_misc_Unsafe_unpark(
JavaObject* unsafe, JavaObject* thread) {
  llvm_gcroot(unsafe, 0);
  llvm_gcroot(thread, 0);
  Jnjvm* vm = JavaThread::get()->getJVM();
  
  // It's possible that the thread to be interrupted has not finished
  // its initialization. Wait until the initialization is done.
  //fprintf(stderr, "Case -1\n");
//  vmthread = field2->getInstanceObjectField(thread);
//  while (vmthread == 0) {
//  	JavaThread::yield();
//  	fprintf(stderr, "Case 0\n");
//  	vmthread = field2->getInstanceObjectField(thread);
//  }
  JavaThread* th = 0;
  
  // Get the eetop field
  sint64 newTh = vm->upcalls->eetop->getInstanceLongField(thread);
  th = (JavaThread*)newTh;
  while (th == 0) {
  	JavaThread::yield();
  	//fprintf(stderr, "Case 1\n");
  	newTh = vm->upcalls->eetop->getInstanceLongField(thread);
  	th = (JavaThread*)newTh;
  	//th = (JavaThread*)field->getInstanceObjectField(vmthread);
  }
  //fprintf(stderr, "unparking started %lld\n", th->getThreadID());
  th->parkLock.unpark();
  //fprintf(stderr, "Case 2\n");
  //fprintf(stderr, "unparking done %lld\n", th->getThreadID());
}

}
