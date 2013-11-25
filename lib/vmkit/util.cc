#include "vmkit/util.h"

#include <string.h>
#include <wchar.h>

using namespace vmkit;

struct Util::char_less_t    Util::char_less;
struct Util::wchar_t_less_t Util::wchar_t_less;

bool Util::char_less_t::operator()(const char* s1, const char* s2) const {
	return strcmp(s1, s2) < 0; 
}

bool Util::wchar_t_less_t::operator()(const wchar_t* lhs, const wchar_t* rhs) const {
	//printf("Compare: %ls - %ls - %d\n", lhs, rhs, wcscmp(lhs, rhs));
	return wcscmp(lhs, rhs) < 0;
}
