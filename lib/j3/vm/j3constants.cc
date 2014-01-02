#include "j3/j3.h"
#include "j3/j3constants.h"
#include "j3/j3class.h"
#include "j3/j3classloader.h"
#include "j3/j3method.h"

#include "llvm/IR/DerivedTypes.h"

using namespace j3;

char    J3Cst::nativePrefix[] = "Java_";

const vmkit::Name* J3Cst::rewrite(J3* vm, const vmkit::Name* clName, const vmkit::Name* orig) {
	wchar_t res[clName->length() + orig->length() + 3];

	res[0] = ID_Left;
	res[1] = ID_Classname;
	memcpy(res+2, clName->cStr(), sizeof(wchar_t)*clName->length());
	res[2 + clName->length()] = ID_End;
	memcpy(res+3+clName->length(), orig->cStr()+1, sizeof(wchar_t)*(orig->length()-1));
	res[2 + clName->length() + orig->length()] = 0;

	return vm->names()->get(res);
}

#define defOpcode(ID, val, effect)											\
	#ID,
const char* J3Cst::opcodeNames[] = {
#include "j3/j3bc.def"
};
#undef defOpcode
