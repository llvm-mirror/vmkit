#include <stdio.h>

#include "j3/j3.h"
#include "j3/j3thread.h"
#include "j3/j3reader.h"
#include "j3/j3codegen.h"
#include "j3/j3class.h"
#include "j3/j3constants.h"
#include "j3/j3classloader.h"
#include "j3/j3method.h"
#include "j3/j3mangler.h"
#include "j3/j3jni.h"
#include "j3/j3object.h"
#include "j3/j3field.h"
#include "j3/j3attribute.h"
#include "j3/j3utf16.h"

#include "llvm/ExecutionEngine/ExecutionEngine.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/Intrinsics.h"

#include "llvm/DebugInfo.h"
#include "llvm/DIBuilder.h"

#define DEBUG_TYPE_INT    0
#define DEBUG_TYPE_FLOAT  1
#define DEBUG_TYPE_OBJECT 2

using namespace j3;

void J3CodeGen::echoDebugEnter(uint32_t isLeave, const char* msg, ...) {
	static __thread uint32_t prof = 0;
	if(J3Thread::get()->vm()->options()->debugExecute >= 1) {
		const char* enter = isLeave ? "leaving" : "entering";
		va_list va;
		va_start(va, msg);

		if(!isLeave)
			prof += J3Thread::get()->vm()->options()->debugEnterIndent;
		fprintf(stderr, "%*s", prof, "");
		if(isLeave)
			prof -= J3Thread::get()->vm()->options()->debugEnterIndent;
		
		fprintf(stderr, "%s ", enter);
		vfprintf(stderr, msg, va);
		va_end(va);
	}
}

void J3CodeGen::echoDebugExecute(uint32_t level, const char* msg, ...) {
	if(level == -1 || J3Thread::get()->vm()->options()->debugExecute >= level) {
		va_list va;
		va_start(va, msg);
		vfprintf(stderr, msg, va);
		va_end(va);
	}
}

void J3CodeGen::echoElement(uint32_t level, uint32_t type, uintptr_t elmt) {
	if(J3Thread::get()->vm()->options()->debugExecute >= level) {
		switch(type) {
			case DEBUG_TYPE_INT:    
				fprintf(stderr, "(long)%lld", (uint64_t)elmt); 
				break;

			case DEBUG_TYPE_FLOAT:  
				fprintf(stderr, "(double)%lf", *((double*)&elmt)); 
				break;

			case DEBUG_TYPE_OBJECT: 
				if(elmt) {
					J3Object* pobj = (J3Object*)elmt;
					J3* vm = J3Thread::get()->vm();
					J3ObjectHandle* prev = J3Thread::get()->tell();
					J3ObjectHandle* content = 0;
					
					if(pobj->vt()->type() == vm->typeCharacter->getArray())
						content = J3Thread::get()->push(pobj);
					else if(pobj->vt()->type() == vm->stringClass) {
						J3ObjectHandle* str = J3Thread::get()->push(pobj);
						content = str->getObject(vm->stringClassValue);
					}

					if(content) {
						char buf[J3Utf16Decoder::maxSize(content)];
						J3Utf16Decoder::decode(content, buf);
						J3Thread::get()->restore(prev);
						fprintf(stderr, "%s \"%s\"", pobj->vt()->type()->name()->cStr(), buf);
					} else
						fprintf(stderr, "%s@%p", pobj->vt()->type()->name()->cStr(), (void*)elmt);
				} else
					fprintf(stderr, "null");
				break;
		}
	}
}

void J3CodeGen::genEchoElement(const char* msg, uint32_t idx, llvm::Value* val) {
	llvm::Type* t = val->getType();
	builder->CreateCall3(funcEchoDebugExecute,
											 builder->getInt32(4),
											 buildString(msg),
											 builder->getInt32(idx));

	if(t->isIntegerTy())
		builder->CreateCall3(funcEchoElement, 
												 builder->getInt32(4),
												 builder->getInt32(DEBUG_TYPE_INT),
												 builder->CreateSExtOrBitCast(val, uintPtrTy));
	else if(t->isPointerTy())
		builder->CreateCall3(funcEchoElement, 
												 builder->getInt32(4),
												 builder->getInt32(DEBUG_TYPE_OBJECT),
												 builder->CreatePtrToInt(val, uintPtrTy));
	else {
		val = builder->CreateFPExt(val, builder->getDoubleTy());
		llvm::Value* loc = builder->CreateAlloca(val->getType());
		builder->CreateStore(val, loc);
		builder->CreateCall3(funcEchoElement, 
												 builder->getInt32(4),
												 builder->getInt32(DEBUG_TYPE_FLOAT),
												 builder->CreateLoad(builder->CreateBitCast(loc, uintPtrTy->getPointerTo())));
	}
	builder->CreateCall2(funcEchoDebugExecute,
											 builder->getInt32(4),
											 buildString("\n"));
}

void J3CodeGen::genDebugOpcode() {
	if(vm->options()->genDebugExecute) {
		llvm::BasicBlock* debug = newBB("debug");
		llvm::BasicBlock* after = newBB("after");
		builder
			->CreateCondBr(builder
										 ->CreateICmpSGT(builder->CreateLoad(builder
																												 ->CreateIntToPtr(llvm::ConstantInt::get(uintPtrTy, 
																																																 (uintptr_t)&vm->options()->debugExecute),
																																					uintPtrTy->getPointerTo())),
																		 llvm::ConstantInt::get(uintPtrTy, 0)),
										 debug, after);
		builder->SetInsertPoint(debug);

		if(vm->options()->genDebugExecute > 1) {
			for(uint32_t i=0; i<stack.topStack; i++) {
				genEchoElement("           stack[%d]: ", i, stack.at(i, stack.metaStack[i]));
			}
		}
		
		char buf[256];
		snprintf(buf, 256, "    [%4d] executing: %-20s in %s::%s", javaPC, 
						 J3Cst::opcodeNames[bc], cl->name()->cStr(), method->name()->cStr());
		builder->CreateCall3(funcEchoDebugExecute,
												 builder->getInt32(2),
												 buildString("%s\n"),
												 buildString(buf));

		builder->CreateBr(after);
		builder->SetInsertPoint(bb = after);
	}
}

void J3CodeGen::genDebugEnterLeave(bool isLeave) {
	if(vm->options()->genDebugExecute) {
		if(isLeave) {
			char buf[256];
			snprintf(buf, 256, "%s::%s", cl->name()->cStr(), method->name()->cStr());
			builder->CreateCall3(funcEchoDebugEnter,
													 builder->getInt32(1),
													 buildString("%s\n"),
													 buildString(buf));
		} else {
			char buf[256];
			snprintf(buf, 256, "%s::%s", cl->name()->cStr(), method->name()->cStr());
			builder->CreateCall3(funcEchoDebugEnter,
													 builder->getInt32(0),
													 buildString("%s\n"),
													 buildString(buf));
			
			if(vm->options()->genDebugExecute > 1) {
				uint32_t i = 0;
				for(llvm::Function::arg_iterator cur=llvmFunction->arg_begin(); cur!=llvmFunction->arg_end(); cur++) {
					genEchoElement("           arg[%d]: ", i++, cur);
				}
			}
		}
	}
}

