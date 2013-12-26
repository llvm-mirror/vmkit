#ifndef _J3_CODEGEN_EXCEPTION_H_
#define _J3_CODEGEN_EXCEPTION_H_

#include <stdint.h>

namespace llvm {
	class BasicBlock;
}

namespace j3 {
	class J3CodeGen;
	class J3Reader;

	class J3ExceptionEntry {
	public:
		uint32_t          startPC;
		uint32_t          endPC;
		uint32_t          handlerPC;
		uint32_t          catchType;
		llvm::BasicBlock* bb;

		void dump(uint32_t i);
	};

	class J3ExceptionNode {
	public:
		uint32_t           pc;
		uint32_t           nbEntries;
		J3ExceptionEntry** entries;
		llvm::BasicBlock*  landingPad;
		llvm::BasicBlock*  curCheck; /* last of the linked list of checker */

		void close(J3CodeGen* codeGen);
		void addEntry(J3CodeGen* codeGen, J3ExceptionEntry* entry);
	};

	class J3ExceptionTable {
	public:
		J3CodeGen*        codeGen;
		uint32_t          nbEntries;
		J3ExceptionEntry* entries;
		J3ExceptionNode*  _exceptionNodes;
		J3ExceptionNode** nodes;
		uint32_t          nbNodes;

		J3ExceptionTable(J3CodeGen* _codeGen) { codeGen = _codeGen; }

		J3ExceptionNode** newNode(uint32_t pos, uint32_t pc);
	
		J3ExceptionNode** findPos(uint32_t pc);
		void              read(J3Reader* reader, uint32_t codeLength);

		void              dump(bool verbose=0);
	};
}

#endif
