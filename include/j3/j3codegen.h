#ifndef _J3_CODE_GEN_H_
#define _J3_CODE_GEN_H_

#include "llvm/IR/IRBuilder.h"
#include "j3/j3codegenvar.h"
#include "j3/j3codegenexception.h"

namespace llvm {
	class Function;
	class Value;
}

namespace vmkit {
	class BumpAllocator;
}

namespace j3 {
	class J3;
	class J3Type;
	class J3Class;
	class J3ArrayClass;	
	class J3ClassLoader;
	class J3Field;
	class J3Method;
	class J3Signature;
	class J3Reader;
	class J3ObjectType;
	class J3Signature;

	class J3OpInfo {
	public:
		llvm::Instruction* insn;
		llvm::BasicBlock*  bb;
		llvm::Type**       metaStack;
		size_t             topStack;
	};

	class J3CodeGen {
	public:
		enum {
			WithMethod = 1,
			WithCaller = 2,
			OnlyTranslate = 4,
			NotUseStub = 8,
			NotNeedGC = 16,
			SupposeClinited = 32,
			NoRuntimeCheck = 64
		};

		bool withMethod() { return mode & WithMethod; }
		bool withCaller() { return mode & WithCaller; }
		bool onlyTranslate() { return mode & OnlyTranslate; }
		bool useStub() { return !(mode & NotUseStub); }
		bool needGC() { return !(mode & NotNeedGC); }
		bool supposeClinited() { return mode & SupposeClinited; }
		bool noRuntimeCheck() { return mode & NoRuntimeCheck; }

	private:
		friend class J3CodeGenVar;
		friend class J3ExceptionTable;
		friend class J3ExceptionNode;
		friend class J3Signature;

		uint32_t               mode;

		vmkit::BumpAllocator*  allocator;
		llvm::Module*          module;
		llvm::IRBuilder<>      builder;
		llvm::Function*        llvmFunction;

		J3*                    vm;
		J3Class*               cl;
		J3ClassLoader*         loader;
		J3Method*              method;
		J3Signature*           signature;
		J3Reader*              codeReader;
		uint32_t               access;

		llvm::BasicBlock*      bbCheckCastFailed;
		llvm::BasicBlock*      bbNullCheckFailed;
		llvm::BasicBlock*      bbRet;

		J3ExceptionTable       exceptions;
		uint32_t               curExceptionNode;

		J3OpInfo*              opInfos;
		uint32_t*              pendingBranchs;
		uint32_t               topPendingBranchs;

		llvm::Type*            uintPtrTy;
		llvm::Value*           nullValue;

		llvm::MDNode*          dbgInfo;
		uint32_t               javaPC;
		uint32_t               bc;

		J3CodeGenVar           locals;
		J3CodeGenVar           stack;
		J3CodeGenVar           ret;

		bool                   closeBB;

		bool                   isWide;

		uint32_t            wideReadU1();
		uint32_t            wideReadS1();

		llvm::Function*     buildFunction(J3Method* method, bool isStub=1);
		llvm::Value*        typeDescriptor(J3ObjectType* objectType, llvm::Type* type);

		llvm::Value*        spToCurrentThread(llvm::Value* sp);
		llvm::Value*        currentThread();

		llvm::Value*        nullCheck(llvm::Value* obj);

		llvm::BasicBlock*   newBB(const char* name);
		bool                onEndPoint();
		llvm::BasicBlock*   forwardBranch(const char* id, uint32_t pc, bool doAlloc, bool doPush);
		void                condBr(llvm::Value* op);

		llvm::Value*        flatten(llvm::Value* v);
		llvm::Value*        unflatten(llvm::Value* v, llvm::Type* type);

		llvm::Value*        handleToObject(llvm::Value* obj);
		llvm::Value*        javaClass(J3ObjectType* type, bool doPush);
		llvm::Value*        staticObject(J3Class* cl);
		llvm::Value*        vt(J3ObjectType* cl);
		llvm::Value*        vt(llvm::Value* obj);
		void                resolveJ3ObjectType(J3ObjectType* cl);
		void                initialiseJ3ObjectType(J3ObjectType* cl);

		void                monitorEnter(llvm::Value* obj);
		void                monitorExit(llvm::Value* obj);

		llvm::CallInst*     isAssignableTo(llvm::Value* obj, J3ObjectType* type);
		void                inlineCall(llvm::CallInst* call);
		void                instanceof(llvm::Value* obj, J3ObjectType* type);
		void                checkCast(llvm::Value* obj, J3ObjectType* type);

		void                floatToInteger(J3Type* from, J3Type* to);
		void                compareFP(bool isL);
		void                compareLong();

		llvm::Value*        fieldOffset(llvm::Value* obj, J3Field* f);

		void                get(llvm::Value* obj, J3Field* field);
		void                getField(uint32_t idx);
		void                getStatic(uint32_t idx);

		void                put(llvm::Value* obj, llvm::Value* val, J3Field* field);
		void                putField(uint32_t idx);
		void                putStatic(uint32_t idx);

		void                invoke(uint32_t access, J3Method* method, llvm::Value* func);
		void                invokeVirtual(uint32_t idx);
		void                invokeStatic(uint32_t idx);
		void                invokeSpecial(uint32_t idx);
		void                invokeInterface(uint32_t idx);

		llvm::Value*        arrayContent(J3Type* cType, llvm::Value* array, llvm::Value* idx);

		void                arrayBoundCheck(llvm::Value* obj, llvm::Value* idx);
		llvm::Value*        arrayLength(llvm::Value* obj);
		llvm::Value*        arrayLengthPtr(llvm::Value* obj);
		void                arrayStore(J3Type* cType);
		void                arrayLoad(J3Type* cType);

		void                multianewArray();
		void                newArray(uint8_t atype);
		void                newArray(J3ArrayClass* type);
		void                newObject(J3Class* cl);

		void                lookupSwitch();
		void                tableSwitch();

		void                ldc(uint32_t idx);

		void                selectExceptionNode(uint32_t idx);

		void                translate();
		void                z_translate();

		void                initExceptionNode(J3ExceptionNode** pnode, uint32_t pc, J3ExceptionNode* node);
		void                addToExceptionNode(J3ExceptionNode* node, J3ExceptionEntry* entry);
		void                closeExceptionNode(J3ExceptionNode* node);

		void                generateJava();
		void                generateNative();
		llvm::Function*     lookupNative();
		llvm::Type*         doNativeType(llvm::Type* type);

		llvm::Value*        buildString(const char* msg);

		static void         echoDebugEnter(uint32_t isLeave, const char* msg, ...);
		static void         echoDebugExecute(uint32_t level, const char* msg, ...);
		static void         echoElement(uint32_t level, uint32_t type, uintptr_t elmt);
		void                genEchoElement(const char* msg, uint32_t idx, llvm::Value* val);
		void                genDebugOpcode();
		void                genDebugEnterLeave(bool isLeave);

#define _x(name, id, forceInline)								\
		llvm::Function* name;
#include "j3/j3meta.def"
#undef _x
		llvm::GlobalValue* gvTypeInfo;            /* typename void* */
		llvm::Function*    gcRoot;             
		llvm::Function*    frameAddress;
		llvm::Function*    stackMap;
		llvm::Function*    patchPoint64;
		llvm::Function*    patchPointVoid;

		J3CodeGen(vmkit::BumpAllocator* _allocator, J3Method* method, uint32_t mode);
		~J3CodeGen();

		void* operator new(size_t n, vmkit::BumpAllocator* _allocator);
		void  operator delete(void* ptr);
	public:
		static void translate(J3Method* method, uint32_t mode);
	};
}

#endif
