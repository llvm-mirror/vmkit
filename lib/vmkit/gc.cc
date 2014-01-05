#include "vmkit/gc.h"
#include <stdlib.h>
#include <stdio.h>

using namespace vmkit;

void* GC::allocate(uintptr_t sz) {
	return calloc(sz, 1);
}
