#include "j3/j3signature.h"
#include "j3/j3object.h"
#include "j3/j3codegen.h"
#include "j3/j3.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"

using namespace j3;

J3LLVMSignature::J3LLVMSignature(llvm::FunctionType* functionType) {
	_functionType = functionType;
}

void J3LLVMSignature::generateCallerIR(J3CodeGen* codeGen, llvm::Module* module, const char* id) {
	llvm::Type* uint64Ty = llvm::Type::getInt64Ty(module->getContext());
	llvm::Type* callerIn[] = { llvm::Type::getInt8Ty(module->getContext())->getPointerTo(),
														 uint64Ty->getPointerTo() };
	llvm::Function*   caller = (llvm::Function*)module->getOrInsertFunction(id, llvm::FunctionType::get(uint64Ty, callerIn, 0));
	llvm::BasicBlock* bb = llvm::BasicBlock::Create(caller->getContext(), "entry", caller);
	llvm::IRBuilder<> builder(bb);

	llvm::Function::arg_iterator cur = caller->arg_begin();
	llvm::Value* method = builder.CreateBitCast(cur++, _functionType->getPointerTo());
	llvm::Value* ins = cur;

	llvm::Value* one = builder.getInt32(1);
	llvm::Value* gepHandle[] = { builder.getInt32(0), builder.getInt32(J3ObjectHandle::gepObj) };

	std::vector<llvm::Value*> params;

	for(llvm::FunctionType::param_iterator it=_functionType->param_begin(); it!=_functionType->param_end(); it++) {
		llvm::Type*  t = *it;
		llvm::Value* arg;

		if(t->isPointerTy()) {
			llvm::BasicBlock* ifnull = llvm::BasicBlock::Create(caller->getContext(), "if-arg-null", caller);
			llvm::BasicBlock* ifnotnull = llvm::BasicBlock::Create(caller->getContext(), "if-arg-notnull", caller);
			llvm::BasicBlock* after = llvm::BasicBlock::Create(caller->getContext(), "if-arg-after", caller);
			llvm::Value*      alloca = builder.CreateAlloca(codeGen->vm->typeJ3ObjectPtr);
			llvm::Value*      obj = builder.CreateLoad(ins);

			builder.CreateCondBr(builder.CreateIsNull(obj), ifnull, ifnotnull);

			builder.SetInsertPoint(ifnull);
			builder.CreateStore(codeGen->nullValue, alloca);
			builder.CreateBr(after);

			builder.SetInsertPoint(ifnotnull);
			builder.CreateStore(builder.CreateLoad(builder.CreateGEP(builder.CreateIntToPtr(obj,
																																										 codeGen->vm->typeJ3ObjectHandlePtr), gepHandle)),
													alloca);
			builder.CreateBr(after);

			builder.SetInsertPoint(after);
			arg = builder.CreateLoad(alloca);
		} else {
			arg = builder.CreateLoad(builder.CreateTruncOrBitCast(ins, t->getPointerTo()));
		}

		params.push_back(arg);
		ins = builder.CreateGEP(ins, one);
	}

	llvm::Value* res = builder.CreateCall(method, params);
	llvm::Type* ret = _functionType->getReturnType();

	if(ret != builder.getVoidTy()) {
		if(ret->isPointerTy()) {
			codeGen->builder = &builder;
			codeGen->currentThread();
			res = builder.CreatePtrToInt(builder.CreateCall2(codeGen->funcJ3ThreadPush, codeGen->currentThread(), res),
																	 uint64Ty);
		} else {
			if(ret->isFloatTy()) {
				llvm::Value* tmp = builder.CreateAlloca(ret);
				builder.CreateStore(res, tmp);
				res = builder.CreateLoad(builder.CreateBitCast(tmp, builder.getInt32Ty()->getPointerTo()));
			} 
			res = builder.CreateZExtOrBitCast(res, uint64Ty);
		}
	} else
		res = builder.getInt64(0);

	builder.CreateRet(res);

	//	caller->dump();
}
