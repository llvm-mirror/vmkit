//===---------------- main.cc - Mvm Garbage Collector ---------------------===//
//
//                              Mvm
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "mvm/GC/GC.h"
#include "mvm/Threads/Thread.h"
#include <stdio.h>

mvm::Key<mvm::Thread>* mvm::Thread::threadKey = 0;

void destr(gc *me, size_t sz) {
 	printf("Destroy %p\n", me);
}

void trace(gc *me, size_t sz) {
	// 	printf("Trace %p\n", me);
}

void marker(void*) {
	//	printf("Marker...\n");
}

int main(int argc, char **argv) {
  mvm::Thread::initialise();
  Collector::initialise(marker, 0);
#ifdef MULTIPLE_GC
  mvm::Thread::get()->GC->destroy();
#else
  Collector::destroy();
#endif
  return 0;
}






