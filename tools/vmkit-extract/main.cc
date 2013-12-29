#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cxxabi.h>

#include "llvm/IR/LLVMContext.h"
//#include "llvm/ADT/SetVector.h"
//#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/Assembly/PrintModulePass.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Module.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/PassManager.h"
//#include "llvm/Support/CommandLine.h"
//#include "llvm/Support/ManagedStatic.h"
//#include "llvm/Support/PrettyStackTrace.h"
//#include "llvm/Support/Regex.h"
//#include "llvm/Support/Signals.h"
#include "llvm/Support/SourceMgr.h"
//#include "llvm/Support/SystemUtils.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Transforms/IPO.h"
//#include <memory>

void add(std::map<std::string, llvm::GlobalValue*>& mangler, llvm::GlobalValue* cur) {
	const char* id = cur->getName().data();
	int   status;
	char* realname;
	realname = abi::__cxa_demangle(id, 0, 0, &status);
	const char* tmp = realname ? realname : id;
	mangler[std::string(tmp)] = cur;
}

int main(int argc, char** argv) {
	if(argc != 3) {
		fprintf(stderr, "usage: %s input.bc file.def\n", argv[0]);
		abort();
	}

	llvm::LLVMContext& context = llvm::getGlobalContext();
	llvm::SMDiagnostic err;
	llvm::OwningPtr<llvm::Module> module;
  module.reset(llvm::getLazyIRFileModule(argv[1], err, context));

	std::map<std::string, llvm::GlobalValue*> mangler;
	std::vector<llvm::GlobalValue*> extracted;

	for(llvm::Module::iterator cur=module->begin(); cur!=module->end(); cur++)
		add(mangler, cur);

	//	for(llvm::Module::global_iterator cur=module->global_begin(); cur!=module->global_end(); cur++)
	//		add(mangler, cur);

	FILE* fp = fopen(argv[2], "r");
	size_t linecapp = 0;
	char* buf;

	while(getline(&buf, &linecapp, fp) > 0) { 
		char* p = strchr(buf, '"') + 1;
		char* e = strchr(p, '"');
		*e = 0;
		llvm::GlobalValue* gv = mangler[p];

		//gv->setName(p);

		if(gv) {
			extracted.push_back(gv);
			if (gv->isMaterializable()) {
				std::string ErrInfo;
				if(gv->Materialize(&ErrInfo)) {
					fprintf(stderr, "%s: error reading input: %s\n", argv[0], ErrInfo.c_str());
					return 1;
				}
			}
			//fprintf(stderr, "extracting: %s (%s)\n", p, gv->getName().data());
			//gv->dump();
		} else {
			fprintf(stderr, "unable to find symbol %s\n", p);
			return 1;
		}
	}

  // directly from llvm-extract
	llvm::PassManager Passes;
  Passes.add(new llvm::DataLayout(module.get())); // Use correct DataLayout

	Passes.add(llvm::createGVExtractionPass(extracted, 0));
	Passes.add(llvm::createGlobalDCEPass());             // Delete unreachable globals
	Passes.add(llvm::createStripDeadDebugInfoPass());    // Remove dead debug info
	Passes.add(llvm::createStripDeadPrototypesPass());   // Remove dead func decls

  std::string ErrorInfo;
	llvm::tool_output_file Out("-", ErrorInfo, llvm::sys::fs::F_Binary);
  if (!ErrorInfo.empty()) {
		llvm::errs() << ErrorInfo << '\n';
    return 1;
  }

	//Passes.add(llvm::createPrintModulePass(&Out.os()));
	Passes.add(createBitcodeWriterPass(Out.os()));

  Passes.run(*module.get());

  // Declare success.
  Out.keep();

	return 0;
}
