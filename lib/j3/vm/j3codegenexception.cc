#include "j3/j3codegenexception.h"
#include "j3/j3codegen.h"
#include "j3/j3reader.h"
#include "j3/j3method.h"
#include "j3/j3class.h"
#include "j3/j3.h"

#include "vmkit/allocator.h"
#include "vmkit/names.h"

#include "llvm/IR/Function.h"

using namespace j3;

void J3ExceptionEntry::dump(uint32_t i) {
	fprintf(stderr, "  entry[%d]: %d %d %d %d\n", i, startPC, endPC, handlerPC, catchType);
}

void J3ExceptionNode::addEntry(J3CodeGen* codeGen, J3ExceptionEntry* entry) {
	if(!nbEntries) {
		landingPad = codeGen->newBB("landing-pad");
		curCheck = landingPad;
		codeGen->builder->SetInsertPoint(landingPad);

		llvm::LandingPadInst *caughtResult = codeGen->builder->CreateLandingPad(codeGen->vm->typeGXXException, 
																																						codeGen->funcGXXPersonality, 
																																						1, 
																																						"landing-pad");
		caughtResult->addClause(codeGen->gvTypeInfo);
		llvm::Value* excp = codeGen->builder->CreateBitCast(codeGen->builder->CreateCall(codeGen->funcCXABeginCatch, 
																																										 codeGen->builder->CreateExtractValue(caughtResult, 0)),
																												codeGen->vm->typeJ3ObjectPtr);
																							 
		codeGen->builder->CreateCall(codeGen->funcCXAEndCatch);

		codeGen->builder->CreateCall2(codeGen->funcEchoDebugExecute, 
																	codeGen->builder->getInt32(0), /* just to see my first exception :) */
																	codeGen->buildString("entering launchpad!\n"));
	}

	entries[nbEntries++] = entry;

	if(curCheck) { /* = 0 if I already have a finally */
		codeGen->builder->SetInsertPoint(curCheck);
		curCheck = codeGen->newBB("next-exception-check");

		if(entry->catchType) {
			codeGen->stack.metaStack[0] = codeGen->vm->typeJ3ObjectPtr;
			codeGen->stack.topStack = 1;
			codeGen->builder->CreateCondBr(codeGen->isAssignableTo(codeGen->stack.top(0), 
																														 codeGen->cl->classAt(entry->catchType)), 
																		 entry->bb, 
																		 curCheck);
		} else {
			codeGen->builder->CreateBr(entry->bb);
			curCheck = 0;
		}
	}
}

void J3ExceptionNode::close(J3CodeGen* codeGen) {
	if(curCheck) {
		codeGen->builder->SetInsertPoint(curCheck);
		codeGen->builder->CreateBr(codeGen->bbRet);
	} 
}

J3ExceptionNode** J3ExceptionTable::newNode(uint32_t pos, uint32_t pc) {
	J3ExceptionNode* res = &_exceptionNodes[nbNodes++];
	res->pc = pc;
	res->entries = (J3ExceptionEntry**)codeGen->allocator->allocate(sizeof(J3ExceptionEntry*)*nbEntries);
	nodes[pos] = res;
	if(pos > 1) {
		for(uint32_t i=0; i<nodes[pos-1]->nbEntries; i++)
			res->addEntry(codeGen, nodes[pos-1]->entries[i]);
	}
	return nodes+pos;
}

J3ExceptionNode** J3ExceptionTable::findPos(uint32_t pc) {
	for(uint32_t i=0; i<nbNodes; i++) {
		if(nodes[i]->pc == pc)
			return nodes+i;
		if(pc < nodes[i]->pc) {
			memmove(nodes+i+1, nodes+i, (nbNodes-i)*sizeof(J3ExceptionNode*));
			return newNode(i, pc);
		}
	}
	return newNode(nbNodes, pc);
}

void J3ExceptionTable::read(J3Reader* reader, uint32_t codeLength) {
	nbEntries = reader->readU2();
	entries = (J3ExceptionEntry*)codeGen->allocator->allocate(sizeof(J3ExceptionEntry)*nbEntries);
	_exceptionNodes = (J3ExceptionNode*)codeGen->allocator->allocate(sizeof(J3ExceptionNode)*(nbEntries*2 + 2));
	nodes = (J3ExceptionNode**)codeGen->allocator->allocate(sizeof(J3ExceptionNode*)*(nbEntries*2 + 2));
	nbNodes = 0;

	newNode(nbNodes, 0);
	for(uint32_t i=0; i<nbEntries; i++) {
		entries[i].startPC = reader->readU2();
		entries[i].endPC = reader->readU2();
		entries[i].handlerPC = reader->readU2();
		entries[i].catchType = reader->readU2();
		entries[i].bb = codeGen->forwardBranch("exception-handler", entries[i].handlerPC, 0, 1);
		codeGen->opInfos[entries[i].handlerPC].topStack = -1;

		J3ExceptionNode** cur = findPos(entries[i].startPC);
		J3ExceptionNode** end = findPos(entries[i].endPC);

		for(; cur<end; cur++)
			(*cur)->addEntry(codeGen, entries+i);
	}
	newNode(nbNodes, codeLength);

	for(uint32_t i=0; i<nbNodes; i++)
		nodes[i]->close(codeGen);
}

void J3ExceptionTable::dump(bool verbose) {
	if(nbEntries) {
		fprintf(stderr, "    ExceptionTable of %s::%s%s:\n", 
						codeGen->method->cl()->name()->cStr(), 
						codeGen->method->name()->cStr(),
						codeGen->method->signature()->name()->cStr());

		if(verbose) {
			for(uint32_t i=0; i<nbEntries; i++)
				entries[i].dump(i);
		}

		for(uint32_t i=0; i<nbNodes; i++) {
			fprintf(stderr, "      at pc %d:\n", nodes[i]->pc);
			for(uint32_t j=0; j<nodes[i]->nbEntries; j++)
				fprintf(stderr, "        catch %d at %d\n", nodes[i]->entries[j]->catchType, nodes[i]->entries[j]->handlerPC);
		}
	}
}

