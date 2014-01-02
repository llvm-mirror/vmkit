#include "j3/j3.h"
#include "j3/j3constants.h"
#include "j3/j3class.h"
#include "j3/j3classloader.h"
#include "j3/j3method.h"

#include "llvm/IR/DerivedTypes.h"

using namespace j3;

char    J3Cst::nativePrefix[] = "Java_";

#define defOpcode(ID, val, effect)											\
	#ID,
const char* J3Cst::opcodeNames[] = {
#include "j3/j3bc.def"
};
#undef defOpcode
