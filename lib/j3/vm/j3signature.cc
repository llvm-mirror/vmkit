#include "j3/j3signature.h"
#include "j3/j3object.h"
#include "j3/j3codegen.h"
#include "j3/j3classloader.h"
#include "j3/j3class.h"
#include "j3/j3.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Type.h"

using namespace j3;

J3Signature::J3Signature(J3ClassLoader* loader, const vmkit::Name* name) {
	_loader = loader;
	_name = name;
}

void J3Signature::checkInOut() {
	if(!_out) {
		J3Type*            args[1+name()->length()];
		uint32_t           nbArgs = 0;
		uint32_t           cur = 1;
		
		if(name()->cStr()[0] != J3Cst::ID_Left)
			loader()->wrongType(0, name());

		while(name()->cStr()[cur] != J3Cst::ID_Right)
			args[nbArgs++] = loader()->getTypeInternal(0, name(), cur, &cur, 0);

		if(nbArgs != nbIns())
			J3::internalError("should not happen %d %d", nbArgs, nbIns());

		_ins = (J3Type**)loader()->allocator()->allocate(nbArgs*sizeof(J3Type*));
		memcpy(_ins, args, nbArgs*sizeof(J3Type*));

		_out = loader()->getTypeInternal(0, name(), cur+1, &cur, 0); /* out has to be the last (thread safety) */
		if(cur != name()->length())
			loader()->wrongType(0, name());
	}
}

J3LLVMSignature* J3Signature::buildLLVMSignature(llvm::FunctionType* fType) {
	J3LLVMSignature* res = loader()->vm()->llvmSignatures[fType];
	if(!res) {
		loader()->vm()->llvmSignatures[fType] = res = new(loader()->vm()->allocator()) J3LLVMSignature();
		res->functionType = fType;
	}
	return res;
}

void J3Signature::checkFunctionType() {
	if(!_virtualLLVMSignature) {
		std::vector<llvm::Type*> vins;
		std::vector<llvm::Type*> sins;
		uint32_t                 cur = 1;

		vins.push_back(loader()->vm()->objectClass->llvmType());

		if(name()->cStr()[0] != J3Cst::ID_Left)
			loader()->wrongType(0, name());

		while(name()->cStr()[cur] != J3Cst::ID_Right) {
			llvm::Type* in = loader()->getTypeInternal(0, name(), cur, &cur, 1)->llvmType();
			sins.push_back(in);
			vins.push_back(in);
		}

		llvm::Type* out = loader()->getTypeInternal(0, name(), cur+1, &cur, 1)->llvmType();
		
		_staticLLVMSignature = buildLLVMSignature(llvm::FunctionType::get(out, sins, 0));
		_virtualLLVMSignature = buildLLVMSignature(llvm::FunctionType::get(out, vins, 0));
	}
}

J3LLVMSignature* J3Signature::llvmSignature(uint32_t access) { 
	checkFunctionType();
	return J3Cst::isStatic(access) ? _staticLLVMSignature : _virtualLLVMSignature; 
}

llvm::FunctionType* J3Signature::functionType(uint32_t access) {
	return llvmSignature(access)->functionType;
}

uint32_t J3Signature::nbIns() {
	return functionType(J3Cst::ACC_STATIC)->getNumParams();
}

J3Signature::function_t J3Signature::caller(uint32_t access) {
	if(!_virtualLLVMSignature)
		J3::internalError("sould not happen");
	return llvmSignature(access)->caller;
}

void J3Signature::setCaller(uint32_t access, J3Signature::function_t caller) {
	llvmSignature(access)->caller = caller;
}

void J3Signature::generateCallerIR(uint32_t access, J3CodeGen* codeGen, llvm::Module* module, const char* id) {
	llvm::FunctionType* fType = functionType(access);
	llvm::Type* uint64Ty = llvm::Type::getInt64Ty(module->getContext());
	llvm::Type* callerIn[] = { llvm::Type::getInt8Ty(module->getContext())->getPointerTo(),
														 uint64Ty->getPointerTo() };
	llvm::Function*   caller = (llvm::Function*)module->getOrInsertFunction(id, llvm::FunctionType::get(uint64Ty, callerIn, 0));
	llvm::BasicBlock* bb = llvm::BasicBlock::Create(caller->getContext(), "entry", caller);
	llvm::IRBuilder<> builder(bb);

	llvm::Function::arg_iterator cur = caller->arg_begin();
	llvm::Value* method = builder.CreateBitCast(cur++, fType->getPointerTo());
	llvm::Value* ins = cur;

	llvm::Value* one = builder.getInt32(1);
	llvm::Value* gepHandle[] = { builder.getInt32(0), builder.getInt32(J3ObjectHandle::gepObj) };

	std::vector<llvm::Value*> params;

	for(llvm::FunctionType::param_iterator it=fType->param_begin(); it!=fType->param_end(); it++) {
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
	llvm::Type* ret = fType->getReturnType();

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
}
