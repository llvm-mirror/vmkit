#include "j3/j3codegenvar.h"
#include "j3/j3codegen.h"
#include "j3/j3.h"
#include "j3/j3classloader.h"

#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Function.h"

using namespace j3;

void J3CodeGenVar::killUnused(llvm::AllocaInst** stack, bool isObj) {
	llvm::Type* i8ptr = codeGen->builder->getInt8Ty()->getPointerTo();
	llvm::Value* meta = codeGen->builder->CreateBitCast(codeGen->nullValue, i8ptr);
	llvm::Type* i8ptrptr = i8ptr->getPointerTo();

	for(uint32_t i=0; i<maxStack; i++) {
		llvm::AllocaInst* cur = stack[i];

    unsigned uses = cur->getNumUses();
    if (!uses) {
      cur->eraseFromParent();
    } else if (uses == 1 && llvm::dyn_cast<llvm::StoreInst>(*(cur->use_begin()))) {
			llvm::dyn_cast<llvm::StoreInst>(*(cur->use_begin()))->eraseFromParent();
      cur->eraseFromParent();
    } else if(isObj) {
			codeGen->builder->SetInsertPoint(cur->getNextNode());
			codeGen->builder->CreateCall2(codeGen->gcRoot, codeGen->builder->CreateBitCast(refStack[i], i8ptrptr), meta);
			//codeGen->builder->CreateStore(codeGen->nullValue, cur);
			//
		}
	}
}

void J3CodeGenVar::killUnused() {
	killUnused(intStack, 0);
	killUnused(longStack, 0);
	killUnused(floatStack, 0);
	killUnused(doubleStack, 0);
	killUnused(refStack, 1);
}

uint32_t J3CodeGenVar::reservedSize(uint32_t max) {
	return max*5*sizeof(llvm::AllocaInst*) + max*sizeof(llvm::Type*);
}

uint32_t J3CodeGenVar::metaStackSize() {
	return topStack*sizeof(llvm::Type*);
}

void J3CodeGenVar::init(J3CodeGen* _codeGen, uint32_t max, void* space) {
	codeGen = _codeGen;
	maxStack = max;
	intStack = (llvm::AllocaInst**)space;
	longStack = intStack + max;
	floatStack = longStack + max;
	doubleStack = floatStack + max;
	refStack = doubleStack + max;
	metaStack = (llvm::Type**)(refStack + max);
	topStack = 0;
	
	for(uint32_t i=0; i<max; i++) {
		intStack[i] = codeGen->builder->CreateAlloca(codeGen->builder->getInt32Ty());
		longStack[i] = codeGen->builder->CreateAlloca(codeGen->builder->getInt64Ty());
		floatStack[i] = codeGen->builder->CreateAlloca(codeGen->builder->getFloatTy());
		doubleStack[i] = codeGen->builder->CreateAlloca(codeGen->builder->getDoubleTy());
		refStack[i] = codeGen->builder->CreateAlloca(codeGen->vm->typeJ3ObjectPtr);
	}
}

void J3CodeGenVar::dump() {
	for(uint32_t i=0; i<topStack; i++) {
		fprintf(stderr, "  [%u]: ", i);
		llvm::Type*  t = metaStack[i];
		llvm::Value* v;

		if(t->isIntegerTy(64))
			v = doubleStack[i];
		else if(t->isIntegerTy())
			v = intStack[i];
		else if(t->isFloatTy())
			v = floatStack[i];
		else if(t->isDoubleTy())
			v = doubleStack[i];
		else if(t->isPointerTy())
			v = refStack[i];
		else {
			t->dump();
			J3::internalError(L"should not happen");
		}

		v->dump();
		//fprintf(stderr, "\n");
	}
}

void J3CodeGenVar::drop(uint32_t n) {
	topStack -= n;
}

void J3CodeGenVar::setAt(llvm::Value* value, uint32_t idx) {
	//if(idx >= nbLocals + maxStack)
	//J3::classFormatError(cl, L"bad index %lu", idx);
	llvm::Type*   t = value->getType();
	llvm::AllocaInst** stack;

	if(t->isIntegerTy(64)) {
		stack = longStack;
	} else if(t->isIntegerTy()) {
		stack = intStack;
	} else if(t->isFloatTy()) {
		stack = floatStack;
	} else if(t->isDoubleTy()) {
		stack = doubleStack;
	} else if(t->isPointerTy()) {
		stack = refStack;
	} else
		J3::internalError(L"should not happen");

	//	fprintf(stderr, "setAt[%u]: ", idx);
	//	value->dump();
	//	fprintf(stderr, "\n");

	metaStack[idx] = t;

	codeGen->builder->CreateStore(value, stack[idx]);
}

llvm::Value* J3CodeGenVar::at(uint32_t idx) {
	//if(idx >= nbLocals + maxStack)
	//vm->classFormatError(cl, L"bad index %lu", idx);
	llvm::Type* t = metaStack[idx];
	llvm::Value* res;

	if(t->isIntegerTy(64)) {
		res = codeGen->builder->CreateLoad(longStack[idx]);
	} else if(t->isIntegerTy()) {
		res = codeGen->builder->CreateLoad(intStack[idx]);
	} else if(t->isFloatTy()) {
		res = codeGen->builder->CreateLoad(floatStack[idx]);
	} else if(t->isDoubleTy()) {
		res = codeGen->builder->CreateLoad(doubleStack[idx]);
	} else if(t->isPointerTy()) {
		res = codeGen->builder->CreateLoad(refStack[idx]);
	} else
		J3::internalError(L"should not happen");

	//	fprintf(stderr, "top: ");
	//	res->dump();
	//	fprintf(stderr, "\n");

	return res;
}

void J3CodeGenVar::push(llvm::Value* value) {
	//	fprintf(stderr, "push: ");
	//	value->dump();
	//	fprintf(stderr, "\n");
	if(topStack >= maxStack)
		J3::classFormatError(codeGen->cl, L"too many push in... ");

	setAt(value, topStack++);
}

llvm::Value* J3CodeGenVar::pop() {
	if(!topStack)
		J3::classFormatError(codeGen->cl, L"too many pop in... ");

	llvm::Value* res = at(--topStack);
	return res;
}

llvm::Value* J3CodeGenVar::top(uint32_t idx) {
	if(topStack <= idx)
		J3::classFormatError(codeGen->cl, L"too large top in... ");
	
	return at(topStack - idx - 1);
}
