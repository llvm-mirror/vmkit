#include "j3/j3symbol.h"
#include "j3/j3thread.h"
#include "j3/j3.h"

using namespace j3;

uint64_t J3Symbol::getSymbolAddress() {
	J3Thread::get()->vm()->internalError(L"implement me: getSymbolAddress");
}
