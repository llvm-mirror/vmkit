#ifndef _INLINER_H_
#define _INLINER_H_

#include "llvm/Pass.h"

namespace vmkit {
	class CompilationUnit;

	llvm::FunctionPass* createFunctionInlinerPass(CompilationUnit* compiler, bool onlyAlwaysInline);
}

#endif
