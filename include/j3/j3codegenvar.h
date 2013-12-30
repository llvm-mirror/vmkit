#ifndef _J3_CODEGEN_VAR_H_
#define _J3_CODEGEN_VAR_H_

#include <stdint.h>

namespace llvm {
	class AllocaInst;
	class Value;
	class Type;
}

namespace j3 {
	class J3CodeGen;

	class J3CodeGenVar {
		void                killUnused(llvm::AllocaInst** stack, bool isObj);

		llvm::AllocaInst**  stackOf(llvm::Type* t);
	public:
		J3CodeGen*          codeGen;
		llvm::AllocaInst**  intStack;
		llvm::AllocaInst**  longStack;
		llvm::AllocaInst**  floatStack;
		llvm::AllocaInst**  doubleStack;
		llvm::AllocaInst**  refStack;
		llvm::Type**        metaStack;
		uint32_t            topStack;
		uint32_t            maxStack;

		static uint32_t     reservedSize(uint32_t max);
		void                init(J3CodeGen* _codeGen, uint32_t max, void* space);

		void                killUnused();

		uint32_t            metaStackSize();

		llvm::Value*        at(uint32_t idx);
		void                setAt(llvm::Value* value, uint32_t idx);
		void                drop(uint32_t n);
		llvm::Value*        top(uint32_t idx=0);
		llvm::Value*        pop();
		void                push(llvm::Value* value);
		void                dump();
	};
}

#endif
