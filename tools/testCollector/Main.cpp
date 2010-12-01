//===---------------- main.cc - Mvm Garbage Collector ---------------------===//
//
//                              Mvm
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "mvm/GC.h"
#include "mvm/Threads/Thread.h"
#include <stdio.h>

void destr(gc *me, size_t sz) {
 	printf("Destroy %p\n", (void*)me);
}

void trace(gc *me, size_t sz) {
	// printf("Trace %p\n", (void*)me);
}

void marker(void*) {
	// printf("Marker...\n");
}

int main(int argc, char **argv) {
  mvm::Collector::initialise();
#ifdef MULTIPLE_GC
  mvm::Thread::get()->GC->destroy();
#else
  mvm::Collector::destroy();
#endif
  return 0;
}

