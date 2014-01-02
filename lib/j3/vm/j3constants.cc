#include "j3/j3.h"
#include "j3/j3constants.h"
#include "j3/j3class.h"
#include "j3/j3classloader.h"
#include "j3/j3method.h"

#include "llvm/IR/DerivedTypes.h"

using namespace j3;

char    J3Cst::nativePrefix[] = "Java_";

const vmkit::Name* J3Cst::rewrite(J3ObjectType* cl, const vmkit::Name* orig) {
	const vmkit::Name* clName = cl->name();
	char res[clName->length() + orig->length() + 3];
	uint32_t d;

	res[0] = ID_Left;
	if(cl->isClass()) {
		res[1] = ID_Classname;
		memcpy(res+2, clName->cStr(), sizeof(char)*clName->length());
		res[2 + clName->length()] = ID_End;
		d = 2;
	} else {
		memcpy(res+1, clName->cStr(), sizeof(char)*clName->length());
		d = 0;
	}

	memcpy(res+1+d+clName->length(), orig->cStr()+1, sizeof(char)*(orig->length()-1));
	res[d + clName->length() + orig->length()] = 0;

	return cl->loader()->vm()->names()->get(res);
}

#define defOpcode(ID, val, effect)											\
	#ID,
const char* J3Cst::opcodeNames[] = {
#include "j3/j3bc.def"
};
#undef defOpcode
