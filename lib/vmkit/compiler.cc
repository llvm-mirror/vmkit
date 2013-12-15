#include "vmkit/compiler.h"
#include "vmkit/thread.h"
#include "vmkit/vmkit.h"

using namespace vmkit;

uint8_t* Symbol::getSymbolAddress() {
	Thread::get()->vm()->internalError(L"implement me: getSymbolAddress");
}
