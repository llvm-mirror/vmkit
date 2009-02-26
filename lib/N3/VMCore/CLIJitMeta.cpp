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
#include "VirtualMachine.h"
#include "VMClass.h"
#include "VMObject.h"
#include "VMThread.h"

using namespace n3;
using namespace llvm;

VMObject* VMClass::operator()() {
  if (status < ready) 
    resolveType(true, true, NULL);
  return doNew();
}

void VMField::operator()(VMObject* obj, float val) {
  
  if (classDef->status < ready) 
    classDef->resolveType(true, true, NULL);
  
  bool stat = isStatic(flags);
  if (stat) obj = classDef->staticInstance;
  void* ptr = (void*)((uint64)obj + ptrOffset);
  
  if (signature->naturalType == Type::FloatTy) {
    ((float*)ptr)[0] = val;
  } else {
    VMThread::get()->vm->unknownError("wrong type in field assignment");
  }

  return;
}

void VMField::operator()(VMObject* obj, double val) {
  
  if (classDef->status < ready) 
    classDef->resolveType(true, true, NULL);
  
  bool stat = isStatic(flags);
  if (stat) obj = classDef->staticInstance;
  void* ptr = (void*)((uint64)obj + ptrOffset);
  
  if (signature->naturalType == Type::DoubleTy) {
    ((double*)ptr)[0] = val;
  } else {
    VMThread::get()->vm->unknownError("wrong type in field assignment");
  }

  return;
}

void VMField::operator()(VMObject* obj, sint64 val) {
  
  if (classDef->status < ready) 
    classDef->resolveType(true, true, NULL);
  
  bool stat = isStatic(flags);
  if (stat) obj = classDef->staticInstance;
  void* ptr = (void*)((uint64)obj + ptrOffset);
  
  if (signature->naturalType == Type::Int64Ty) {
    ((uint64*)ptr)[0] = val;
  } else {
    VMThread::get()->vm->unknownError("wrong type in field assignment");
  }
  
  return;
}

void VMField::operator()(VMObject* obj, sint32 val) {
  
  if (classDef->status < ready) 
    classDef->resolveType(true, true, NULL);
  
  bool stat = isStatic(flags);
  if (stat) obj = classDef->staticInstance;
  void* ptr = (void*)((uint64)obj + ptrOffset);
  
  if (signature->naturalType == Type::Int32Ty) {
    ((uint32*)ptr)[0] = val;
  } else {
    VMThread::get()->vm->unknownError("wrong type in field assignment");
  }

  return;
}

void VMField::operator()(VMObject* obj, VMObject* val) {
  
  if (classDef->status < ready) 
    classDef->resolveType(true, true, NULL);
  
  bool stat = isStatic(flags);
  if (stat) obj = classDef->staticInstance;
  void* ptr = (void*)((uint64)obj + ptrOffset);
  
  if (llvm::isa<PointerType>(signature->naturalType)) {
    ((VMObject**)ptr)[0] = val;
  } else {
    VMThread::get()->vm->unknownError("wrong type in field assignment");
  }

  return;
}
void VMField::operator()(VMObject* obj, bool val) {
  
  if (classDef->status < ready) 
    classDef->resolveType(true, true, NULL);
  
  bool stat = isStatic(flags);
  if (stat) obj = classDef->staticInstance;
  void* ptr = (void*)((uint64)obj + ptrOffset);
  
  if (signature->naturalType == Type::Int1Ty) {
    ((bool*)ptr)[0] = val;
  } else {
    VMThread::get()->vm->unknownError("wrong type in field assignment");
  }

  return;
}

GenericValue VMField::operator()(VMObject* obj) {
  
  if (classDef->status < ready) 
    classDef->resolveType(true, true, NULL);
  
  bool stat = isStatic(flags);
  if (stat) {
    if (obj != 0) {
      // Assignment to a static var
      void* ptr = (void*)((uint64)(classDef->staticInstance) + ptrOffset);
      ((VMObject**)ptr)[0] = obj;
      return GenericValue(0);
    } else {
      // Get a static var
      obj = classDef->staticInstance;
    }
  }
  
  void* ptr = (void*)((uint64)obj + ptrOffset);
  const Type* type = signature->naturalType;
  if (type == Type::Int8Ty) {
    GenericValue gv;
    gv.IntVal = APInt(8, ((uint8*)ptr)[0]);
    return gv;
  } else if (type == Type::Int16Ty) {
    GenericValue gv;
    gv.IntVal = APInt(16, ((uint16*)ptr)[0]);
    return gv;
  } else if (type == Type::Int32Ty) {
    GenericValue gv;
    gv.IntVal = APInt(32, ((uint32*)ptr)[0]);
    return gv;
  } else if (type == Type::Int64Ty) {
    GenericValue gv;
    gv.IntVal = APInt(64, ((uint64*)ptr)[0]);
    return gv;
  } else if (type == Type::DoubleTy) { 
    GenericValue gv;
    gv.DoubleVal = ((double*)ptr)[0];
    return gv;
  } else if (type == Type::FloatTy) {
    GenericValue gv;
    gv.FloatVal = ((float*)ptr)[0];
    return gv;
  } else {
    GenericValue gv(((VMObject**)ptr)[0]);
    return gv;
  }
}

GenericValue VMMethod::operator()(va_list ap) {
  
  if (classDef->status < ready) 
    classDef->resolveType(true, true, NULL);
  
  Function* func = compiledPtr(NULL);
  
  std::vector<GenericValue> args;
  for (Function::arg_iterator i = func->arg_begin(), e = func->arg_end();
       i != e; ++i) {
    const Type* type = i->getType();
    if (type == Type::Int8Ty) {
      GenericValue gv;
      gv.IntVal = APInt(8, va_arg(ap, int));
      args.push_back(gv);
    } else if (type == Type::Int16Ty) {
      GenericValue gv;
      gv.IntVal = APInt(16, va_arg(ap, int));
      args.push_back(gv);
    } else if (type == Type::Int32Ty) {
      GenericValue gv;
      gv.IntVal = APInt(32, va_arg(ap, int));
      args.push_back(gv);
    } else if (type == Type::Int64Ty) {
      GenericValue gv1;
      gv1.IntVal = APInt(64, va_arg(ap, uint64));
      args.push_back(gv1);
    } else if (type == Type::DoubleTy) { 
      GenericValue gv1;
      gv1.DoubleVal = va_arg(ap, double);
      args.push_back(gv1);
    } else if (type == Type::FloatTy) {
      GenericValue gv;
      gv.FloatVal = (float)(va_arg(ap, double));
      args.push_back(gv);
    } else {
      GenericValue gv(va_arg(ap, VMObject*));
      args.push_back(gv);
    }
  }
  
  return mvm::MvmModule::executionEngine->runFunction(func, args);
}

GenericValue VMMethod::operator()(VMObject* obj, va_list ap) {
  
  if (classDef->status < ready) 
    classDef->resolveType(true, true, NULL);
  
  Function* func = compiledPtr(NULL);
  
  std::vector<GenericValue> args;
  GenericValue object(obj);
  args.push_back(object);

  for (Function::arg_iterator i = ++(func->arg_begin()), e = func->arg_end();
       i != e; ++i) {
    const Type* type = i->getType();
    if (type == Type::Int8Ty) {
      GenericValue gv;
      gv.IntVal = APInt(8, va_arg(ap, int));
      args.push_back(gv);
    } else if (type == Type::Int16Ty) {
      GenericValue gv;
      gv.IntVal = APInt(16, va_arg(ap, int));
      args.push_back(gv);
    } else if (type == Type::Int32Ty) {
      GenericValue gv;
      gv.IntVal = APInt(32, va_arg(ap, int));
      args.push_back(gv);
    } else if (type == Type::Int64Ty) {
      GenericValue gv1;
      gv1.IntVal = APInt(64, va_arg(ap, uint64));
      args.push_back(gv1);
    } else if (type == Type::DoubleTy) { 
      GenericValue gv1;
      gv1.DoubleVal = va_arg(ap, double);
      args.push_back(gv1);
    } else if (type == Type::FloatTy) {
      GenericValue gv;
      gv.FloatVal = (float)(va_arg(ap, double));
      args.push_back(gv);
    } else {
      GenericValue gv(va_arg(ap, VMObject*));
      args.push_back(gv);
    }
  }
      
  return mvm::MvmModule::executionEngine->runFunction(func, args);
}


GenericValue VMMethod::operator()(...) {
  va_list ap;
  va_start(ap, this);
  GenericValue ret = (*this)(ap);
  va_end(ap);
  return  ret;
}

GenericValue VMMethod::run(...) {
  va_list ap;
  va_start(ap, this);
  GenericValue ret = (*this)(ap);
  va_end(ap);
  return  ret;
}

GenericValue VMMethod::operator()(std::vector<GenericValue>& args) {
  
  if (classDef->status < ready) 
    classDef->resolveType(true, true, NULL);
  
  Function* func = compiledPtr(NULL);
  return mvm::MvmModule::executionEngine->runFunction(func, args);
}

GenericValue VMObject::operator()(VMField* field) {
  return (*field)(this);
}

void VMObject::operator()(VMField* field, float val) {
  return (*field)(this, val);
}

void VMObject::operator()(VMField* field, double val) {
  return (*field)(this, val);
}

void VMObject::operator()(VMField* field, sint32 val) {
  return (*field)(this, val);
}

void VMObject::operator()(VMField* field, sint64 val) {
  return (*field)(this, val);
}

void VMObject::operator()(VMField* field, VMObject* val) {
  return (*field)(this, val);
}

void VMObject::operator()(VMField* field, bool val) {
  return (*field)(this, val);
}

void VMField::operator()(float val) {
  VMField * field = this;
  return (*field)(classDef->virtualInstance, val);
}

void VMField::operator()(double val) {
  VMField * field = this;
  return (*field)(classDef->virtualInstance, val);
}

void VMField::operator()(sint64 val) {
  VMField * field = this;
  return (*field)(classDef->virtualInstance, val);
}

void VMField::operator()(sint32 val) {
  VMField * field = this;
  return (*field)(classDef->virtualInstance, val);
}

void VMField::operator()(bool val) {
  VMField * field = this;
  return (*field)(classDef->virtualInstance, val);
}

GlobalVariable* VMCommonClass::llvmVar() {
  if (!_llvmVar) {
    aquire();
    if (!_llvmVar) {
      const Type* pty = mvm::MvmModule::ptrType;
      Constant* cons = 
        ConstantExpr::getIntToPtr(ConstantInt::get(Type::Int64Ty, uint64_t (this)),
                                    pty);

      _llvmVar = new GlobalVariable(pty, true,
                                    GlobalValue::ExternalLinkage,
                                    cons, "",
                                    vm->module->getLLVMModule());
    
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
      Constant* cons = 
        ConstantExpr::getIntToPtr(ConstantInt::get(Type::Int64Ty, uint64_t (this)),
                                  pty);

      _llvmVar = new GlobalVariable(pty, true,
                                    GlobalValue::ExternalLinkage,
                                    cons, "",
                                    classDef->vm->module->getLLVMModule());
    }
    classDef->release();
  }
  return _llvmVar;
}

GlobalVariable* VMMethod::llvmVar() {
  if (!_llvmVar) {
    classDef->aquire();
    if (!_llvmVar) {
      const Type* pty = mvm::MvmModule::ptrType;
      Constant* cons = 
        ConstantExpr::getIntToPtr(ConstantInt::get(Type::Int64Ty, uint64_t (this)),
                                  pty);

      _llvmVar = new GlobalVariable(pty, true,
                                    GlobalValue::ExternalLinkage,
                                    cons, "",
                                    classDef->vm->module->getLLVMModule());
    
    }
    classDef->release();
  }
  return _llvmVar;
}

ConstantInt* VMObject::classOffset() {
  return mvm::MvmModule::constantOne;
}
