//===------ CLIJitMeta.cpp - CLI class/method/field operators -------------===//
//
//                              N3
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <stdarg.h>

#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/GlobalVariable.h"

#include "types.h"

#include "mvm/JIT.h"

#include "CLIAccess.h"
#include "CLIJit.h"
#include "CLIString.h"
#include "N3.h"
#include "VMClass.h"
#include "VMObject.h"
#include "VMThread.h"

using namespace n3;
using namespace llvm;

// TODO: MUST CHECK the type!
//		if (llvm::isa<check>(signature->naturalType)) {									 
//		if (signature->naturalType == Type::getFloatTy(getGlobalContext())) {

#define IMPLEMENTS_VMFIELD_ASSESSORS(name, type, do_root)								\
	void VMField::set##name(VMObject* obj, type val) {										\
		llvm_gcroot(obj, 0);																								\
		do_root(val, 0);																										\
																																				\
		if (classDef->status < ready)																				\
			classDef->resolveType(true, true, NULL);													\
																																				\
		*(type*)((char *)obj + ptrOffset) = val;														\
																																				\
		return;																															\
	}																																			\
																																				\
	type VMField::get##name(VMObject* obj) {															\
		llvm_gcroot(obj, 0);																								\
		if (classDef->status < ready)																				\
			classDef->resolveType(true, true, NULL);													\
																																				\
		type res;																														\
		do_root(res, 0);																										\
		res = *(type *)((char *)obj + ptrOffset);														\
		return res;																													\
	}

ON_TYPES(IMPLEMENTS_VMFIELD_ASSESSORS, _F_NTR)

#undef IMPLEMENTS_VMFIELD_ASSESSORS

GenericValue VMMethod::invokeGeneric(std::vector<llvm::GenericValue>& args) {																		
	// at this step, we must not launch the gc because objects arguments are somewhere in the stack a copying collector
	// can not know where they are
	assert(code);									// compiling a method can trigger a gc
	return mvm::MvmModule::executionEngine->runFunction(methPtr, args);
}

GenericValue VMMethod::invokeGeneric(va_list ap) {																		
	Function* func = methPtr;																						
	std::vector<GenericValue> args;																			
	for (Function::arg_iterator i = func->arg_begin(), e = func->arg_end();	
			 i != e; ++i) {																									
		const Type* cur = i->getType();																		
		if (cur == Type::getInt8Ty(getGlobalContext())) {									
			GenericValue gv;																								
			gv.IntVal = APInt(8, va_arg(ap, int));													
			args.push_back(gv);																							
		} else if (cur == Type::getInt16Ty(getGlobalContext())) {					
			GenericValue gv;																								
			gv.IntVal = APInt(16, va_arg(ap, int));													
			args.push_back(gv);																							
		} else if (cur == Type::getInt32Ty(getGlobalContext())) {					
			GenericValue gv;																								
			gv.IntVal = APInt(32, va_arg(ap, int));													
			args.push_back(gv);																							
		} else if (cur == Type::getInt64Ty(getGlobalContext())) {					
			GenericValue gv1;																								
			gv1.IntVal = APInt(64, va_arg(ap, uint64));											
			args.push_back(gv1);																						
		} else if (cur == Type::getDoubleTy(getGlobalContext())) {				
			GenericValue gv1;																								
			gv1.DoubleVal = va_arg(ap, double);															
			args.push_back(gv1);																						
		} else if (cur == Type::getFloatTy(getGlobalContext())) {					
			GenericValue gv;																								
			gv.FloatVal = (float)(va_arg(ap, double));											
			args.push_back(gv);																							
		} else {																													
			GenericValue gv(va_arg(ap, VMObject*));													
			args.push_back(gv);																							
		}																																	
	}																																		
	
	return invokeGeneric(args);
}

GenericValue VMMethod::invokeGeneric(...) {
	va_list ap;
	va_start(ap, this);
	GenericValue res = invokeGeneric(ap);
	va_end(ap);
	return res;
}

#define DEFINE_INVOKE(name, type, extractor)														\
	type VMMethod::invoke##name(...) {																		\
		va_list ap;																													\
		va_start(ap, this);																									\
		GenericValue res = invokeGeneric(ap);																\
		va_end(ap);																													\
		return (type)res.extractor;																					\
	}

ON_TYPES(DEFINE_INVOKE, _F_NTE)

#undef DEFINE_INVOKE

#define DEFINE_INVOKE_VOID(name, type, extractor)	\
	type VMMethod::invoke##name(...) {																		\
		va_list ap;																													\
		va_start(ap, this);																									\
		invokeGeneric(ap);																									\
		va_end(ap);																													\
	}

ON_VOID(DEFINE_INVOKE_VOID, _F_NTE)

#undef DEFINE_INVOKE_VOID


// // mettre en param un Function *
// // materializeFunction avant
// GenericValue VMMethod::operator()(va_list ap) {
  
//   if (classDef->status < ready) 
//     classDef->resolveType(true, true, NULL);
  
//   Function* func = compiledPtr(NULL);
  
//   std::vector<GenericValue> args;
//   for (Function::arg_iterator i = func->arg_begin(), e = func->arg_end();
//        i != e; ++i) {
//     const Type* type = i->getType();
//     if (type == Type::getInt8Ty(getGlobalContext())) {
//       GenericValue gv;
//       gv.IntVal = APInt(8, va_arg(ap, int));
//       args.push_back(gv);
//     } else if (type == Type::getInt16Ty(getGlobalContext())) {
//       GenericValue gv;
//       gv.IntVal = APInt(16, va_arg(ap, int));
//       args.push_back(gv);
//     } else if (type == Type::getInt32Ty(getGlobalContext())) {
//       GenericValue gv;
//       gv.IntVal = APInt(32, va_arg(ap, int));
//       args.push_back(gv);
//     } else if (type == Type::getInt64Ty(getGlobalContext())) {
//       GenericValue gv1;
//       gv1.IntVal = APInt(64, va_arg(ap, uint64));
//       args.push_back(gv1);
//     } else if (type == Type::getDoubleTy(getGlobalContext())) { 
//       GenericValue gv1;
//       gv1.DoubleVal = va_arg(ap, double);
//       args.push_back(gv1);
//     } else if (type == Type::getFloatTy(getGlobalContext())) {
//       GenericValue gv;
//       gv.FloatVal = (float)(va_arg(ap, double));
//       args.push_back(gv);
//     } else {
//       GenericValue gv(va_arg(ap, VMObject*));
//       args.push_back(gv);
//     }
//   }
  
//   return mvm::MvmModule::executionEngine->runFunction(func, args);
// }

// GenericValue VMMethod::operator()(VMObject* obj, va_list ap) {
  
//   if (classDef->status < ready) 
//     classDef->resolveType(true, true, NULL);
  
//   Function* func = compiledPtr(NULL);
  
//   std::vector<GenericValue> args;
//   GenericValue object(obj);
//   args.push_back(object);

//   for (Function::arg_iterator i = ++(func->arg_begin()), e = func->arg_end();
//        i != e; ++i) {
//     const Type* type = i->getType();
//     if (type == Type::getInt8Ty(getGlobalContext())) {
//       GenericValue gv;
//       gv.IntVal = APInt(8, va_arg(ap, int));
//       args.push_back(gv);
//     } else if (type == Type::getInt16Ty(getGlobalContext())) {
//       GenericValue gv;
//       gv.IntVal = APInt(16, va_arg(ap, int));
//       args.push_back(gv);
//     } else if (type == Type::getInt32Ty(getGlobalContext())) {
//       GenericValue gv;
//       gv.IntVal = APInt(32, va_arg(ap, int));
//       args.push_back(gv);
//     } else if (type == Type::getInt64Ty(getGlobalContext())) {
//       GenericValue gv1;
//       gv1.IntVal = APInt(64, va_arg(ap, uint64));
//       args.push_back(gv1);
//     } else if (type == Type::getDoubleTy(getGlobalContext())) { 
//       GenericValue gv1;
//       gv1.DoubleVal = va_arg(ap, double);
//       args.push_back(gv1);
//     } else if (type == Type::getFloatTy(getGlobalContext())) {
//       GenericValue gv;
//       gv.FloatVal = (float)(va_arg(ap, double));
//       args.push_back(gv);
//     } else {
//       GenericValue gv(va_arg(ap, VMObject*));
//       args.push_back(gv);
//     }
//   }
      
//   return mvm::MvmModule::executionEngine->runFunction(func, args);
// }


// GenericValue VMMethod::operator()(...) {
//   va_list ap;
//   va_start(ap, this);
//   GenericValue ret = (*this)(ap);
//   va_end(ap);
//   return  ret;
// }

// GenericValue VMMethod::run(...) {
//   va_list ap;
//   va_start(ap, this);
//   GenericValue ret = (*this)(ap);
//   va_end(ap);
//   return  ret;
// }

// GenericValue VMMethod::operator()(std::vector<GenericValue>& args) {
  
//   if (classDef->status < ready) 
//     classDef->resolveType(true, true, NULL);
  
//   Function* func = compiledPtr(NULL);
//   return mvm::MvmModule::executionEngine->runFunction(func, args);
// }

GlobalVariable* VMCommonClass::llvmVar() {
  if (!_llvmVar) {
    aquire();
    if (!_llvmVar) {
      Module* Mod = vm->getLLVMModule();
      const Type* pty = mvm::MvmModule::ptrType;
      Constant* cons = 
        ConstantExpr::getIntToPtr(ConstantInt::get(Type::getInt64Ty(getGlobalContext()), uint64_t (this)),
                                    pty);

      _llvmVar = new GlobalVariable(*Mod, pty, true,
                                    GlobalValue::ExternalLinkage,
                                    cons, "");
    
    }
    release();
  }
  return _llvmVar;
}

GlobalVariable* VMField::llvmVar() {
  if (!_llvmVar) {
    classDef->aquire();
    if (!_llvmVar) {
      const Type* pty = mvm::MvmModule::ptrType;
      Module* Mod = classDef->vm->getLLVMModule();
      Constant* cons = 
        ConstantExpr::getIntToPtr(ConstantInt::get(Type::getInt64Ty(getGlobalContext()), uint64_t (this)),
                                  pty);

      _llvmVar = new GlobalVariable(*Mod, pty, true,
                                    GlobalValue::ExternalLinkage,
                                    cons, "");
    }
    classDef->release();
  }
  return _llvmVar;
}

GlobalVariable* VMMethod::llvmVar() {
  if (!_llvmVar) {
    classDef->aquire();
    if (!_llvmVar) {
      Module* Mod = classDef->vm->getLLVMModule();
      const Type* pty = mvm::MvmModule::ptrType;
      Constant* cons = 
        ConstantExpr::getIntToPtr(ConstantInt::get(Type::getInt64Ty(getGlobalContext()), uint64_t (this)),
                                  pty);

      _llvmVar = new GlobalVariable(*Mod, pty, true,
                                    GlobalValue::ExternalLinkage,
                                    cons, "");
    
    }
    classDef->release();
  }
  return _llvmVar;
}

Constant* VMObject::classOffset() {
  return VMThread::get()->getVM()->module->constantOne;
}
