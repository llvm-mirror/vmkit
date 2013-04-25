#include "VMStaticInstance.h"
#include <stdlib.h>

extern "C" {

JNIEXPORT void JNICALL Java_sun_misc_Unsafe_unpark(
JavaObject* unsafe, JavaObject* thread) {
  llvm_gcroot(unsafe, 0);
  llvm_gcroot(thread, 0);
  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField * field = vm->upcalls->eetop;
  
  // Get the eetop field
  // It's possible that the thread to be interrupted has not finished
  // its initialization. Wait until the initialization is done.
  while (field->getInstanceLongField(thread) == 0)
    vmkit::Thread::yield();
  
  JavaThread* jth = (JavaThread*)field->getInstanceLongField(thread);

  jth->parkLock.unpark();
}

}
