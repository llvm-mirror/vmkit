#include "vmkit/gc.h"
#include <stdlib.h>

using namespace vmkit;

void* GC::allocate(size_t sz) {
	return calloc(sz, 1);
}
