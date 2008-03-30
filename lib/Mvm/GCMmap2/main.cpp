//===---------------- main.cc - Mvm Garbage Collector ---------------------===//
//
//                              Mvm
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "mvm/GC/GC.h"
#include <stdio.h>

void destr(gc *me, size_t sz) {
 	printf("Destroy %p\n", me);
}

void trace(gc *me, size_t sz) {
	// 	printf("Trace %p\n", me);
}

void marker(void) {
	//	printf("Marker...\n");
}

int main(int argc, char **argv) {
  Collector::initialise(marker, 0);

	Collector::destroy();
 	return 0;
}






