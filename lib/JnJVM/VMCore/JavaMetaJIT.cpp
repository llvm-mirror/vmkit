//===---- JavaMetaJIT.cpp - Functions for Java internal objects -----------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <stdarg.h>
#include <string.h>

#include <llvm/BasicBlock.h>
#include <llvm/GlobalVariable.h>
#include <llvm/Instructions.h>
#include <llvm/Module.h>
#include <llvm/Type.h>
#include "llvm/ExecutionEngine/GenericValue.h"

#include "mvm/JIT.h"
#include "mvm/Method.h"

#include "debug.h"
#include "JavaArray.h"
#include "JavaClass.h"
#include "JavaJIT.h"
#include "JavaObject.h"
#include "JavaThread.h"
#include "JavaTypes.h"
#include "Jnjvm.h"

using namespace jnjvm;
using namespace llvm;

Value* Class::staticVar(Module* compilingModule,  BasicBlock* currentBlock) {

  if (!_staticVar) {
    aquire();
    if (!_staticVar) {
#ifdef MULTIPLE_VM
      if (isolate == Jnjvm::bootstrapVM) {
        _staticVar = llvmVar(compilingModule);
      } else {
#endif
        JavaObject* obj = staticInstance();
        mvm::jit::protectConstants();//->lock();
        Constant* cons = 
          ConstantExpr::getIntToPtr(ConstantInt::get(Type::Int64Ty,
                                    uint64_t (obj)), JavaObject::llvmType);
        mvm::jit::unprotectConstants();//->unlock();
      
        isolate->protectModule->lock();
        _staticVar = new GlobalVariable(JavaObject::llvmType, true,
                                      GlobalValue::ExternalLinkage,
                                      cons, "",
                                      isolate->module);
        isolate->protectModule->unlock();
      }
#ifdef MULTIPLE_VM
    }
#endif
    release();
  }

#ifdef MULTIPLE_VM
  if (isolate == Jnjvm::bootstrapVM) {
    Value* ld = new LoadInst(_staticVar, "", currentBlock);
    return llvm::CallInst::Create(JavaJIT::getStaticInstanceLLVM, ld, "",
                                  currentBlock);
  } else {
#endif
    return new LoadInst(_staticVar, "", currentBlock);
#ifdef MULTIPLE_VM
  }
#endif
}

GlobalVariable* CommonClass::llvmVar(llvm::Module* compilingModule) {
  if (!_llvmVar) {
    aquire();
    if (!_llvmVar) {
#ifdef MULTIPLE_VM
      if (compilingModule == Jnjvm::bootstrapVM->module && isArray && isolate != Jnjvm::bootstrapVM) {
        // We know the array class can belong to bootstrap
        _llvmVar = Jnjvm::bootstrapVM->constructArray(this->name, 0)->llvmVar(compilingModule);
        release();
        return _llvmVar;
      }
#endif
      const Type* pty = mvm::jit::ptrType;
      
      mvm::jit::protectConstants();//->lock();
      Constant* cons = 
        ConstantExpr::getIntToPtr(ConstantInt::get(Type::Int64Ty, uint64_t (this)),
                                  pty);
      mvm::jit::unprotectConstants();//->unlock();
      
      isolate->protectModule->lock();
      _llvmVar = new GlobalVariable(pty, true,
                                    GlobalValue::ExternalLinkage,
                                    cons, "",
                                    isolate->module);
      isolate->protectModule->unlock();
    }
    release();
  }
  return _llvmVar;
}

Value* CommonClass::llvmDelegatee(llvm::Module* M, llvm::BasicBlock* BB) {
#ifndef MULTIPLE_VM
  if (!_llvmDelegatee) {
    aquire();
    if (!_llvmDelegatee) {
      const Type* pty = JavaObject::llvmType;
      
      JavaObject* obj = getClassDelegatee();
      mvm::jit::protectConstants();//->lock();
      Constant* cons = 
        ConstantExpr::getIntToPtr(ConstantInt::get(Type::Int64Ty, uint64(obj)),
                                    pty);
      mvm::jit::unprotectConstants();//->unlock();

      isolate->protectModule->lock();
      _llvmDelegatee = new GlobalVariable(pty, true,
                                    GlobalValue::ExternalLinkage,
                                    cons, "",
                                    isolate->module);
      isolate->protectModule->unlock();
    }
    release();
  }
  return new LoadInst(_llvmDelegatee, "", BB);
#else
  Value* ld = new LoadInst(llvmVar(M), "", BB);
  return llvm::CallInst::Create(JavaJIT::getClassDelegateeLLVM, ld, "", BB);
#endif
}

ConstantInt* JavaObject::classOffset() {
  return mvm::jit::constantOne;
}

ConstantInt* JavaArray::sizeOffset() {
  return mvm::jit::constantOne; 
}

ConstantInt* JavaArray::elementsOffset() {
  return mvm::jit::constantTwo; 
}

void JavaJIT::invokeOnceVoid(Jnjvm* vm, JavaObject* loader,
                             char const* className, char const* func,
                             char const* sign, int access, ...) {
  
  CommonClass* cl = vm->loadName(vm->asciizConstructUTF8(className), loader,
                                 true, true, true);
  
  bool stat = access == ACC_STATIC ? true : false;
  JavaMethod* method = cl->lookupMethod(vm->asciizConstructUTF8(func), 
                                        vm->asciizConstructUTF8(sign), stat,
                                        true);
  va_list ap;
  va_start(ap, access);
  if (stat) {
    method->invokeIntStaticAP(ap);
  } else {
    JavaObject* obj = va_arg(ap, JavaObject*);
    method->invokeIntSpecialAP(obj, ap);
  }
  va_end(ap);
}

VirtualTable* JavaJIT::makeVT(Class* cl, bool stat) {
  
  const Type* type = stat ? cl->staticType : cl->virtualType;
  std::vector<JavaField*> &fields = stat ? cl->staticFields : cl->virtualFields;
 
  cl->isolate->protectModule->lock();
  Function* func = Function::Create(markAndTraceLLVMType,
                                    GlobalValue::ExternalLinkage,
                                    "markAndTraceObject",
                                    cl->isolate->module);
  cl->isolate->protectModule->unlock();

  Constant* zero = mvm::jit::constantZero;
  Argument* arg = func->arg_begin();
  BasicBlock* block = BasicBlock::Create("", func);
  llvm::Value* realArg = new BitCastInst(arg, type, "", block);
  
  if (stat || cl->super == 0) {
    CallInst::Create(javaObjectTracerLLVM, arg, "", block);
  } else {
    CallInst::Create(((Class*)cl->super)->virtualTracer, arg, "", block);
  }

  for (std::vector<JavaField*>::iterator i = fields.begin(), 
            e = fields.end(); i!= e; ++i) {
    if ((*i)->signature->funcs->doTrace) {
      std::vector<Value*> args; //size = 2
      args.push_back(zero);
      args.push_back((*i)->offset);
      Value* ptr = GetElementPtrInst::Create(realArg, args.begin(), args.end(), "",
                                         block);
      Value* val = new LoadInst(ptr, "", block);
      Value* valCast = new BitCastInst(val, JavaObject::llvmType, "", block);
      CallInst::Create(markAndTraceLLVM, valCast, "", block);
    }
  }

  ReturnInst::Create(block);

  VirtualTable * res = malloc(VT_SIZE);
  memcpy(res, JavaObject::VT, VT_SIZE);
  void* codePtr = mvm::jit::executionEngine->getPointerToGlobal(func);
  ((void**)res)[VT_TRACER_OFFSET] = codePtr;
  
  if (!stat) {
    cl->virtualTracer = func;
    cl->codeVirtualTracer = (mvm::Code*)((unsigned*)codePtr - 1);
  } else {
    cl->staticTracer = func;
    cl->codeStaticTracer = (mvm::Code*)((unsigned*)codePtr - 1);
  }
  return res;
}


static void _initField(JavaField* field) {
  ConstantInt* offset = field->offset;
  const TargetData* targetData = mvm::jit::executionEngine->getTargetData();
  bool stat = isStatic(field->access);
  const Type* clType = stat ? field->classDef->staticType :
                              field->classDef->virtualType;
  
  const StructLayout* sl =
    targetData->getStructLayout((StructType*)(clType->getContainedType(0)));
  uint64 ptrOffset = sl->getElementOffset(offset->getZExtValue());
  
  field->ptrOffset = ptrOffset;
}

void JavaJIT::initField(JavaField* field, JavaObject* obj, uint64 val) {
  _initField(field);
  
  const AssessorDesc* funcs = field->signature->funcs;
  if (funcs == AssessorDesc::dLong) {
    ((sint64*)((uint64)obj + field->ptrOffset))[0] = val;
  } else if (funcs == AssessorDesc::dInt) {
    ((sint32*)((uint64)obj + field->ptrOffset))[0] = (sint32)val;
  } else if (funcs == AssessorDesc::dChar) {
    ((uint16*)((uint64)obj + field->ptrOffset))[0] = (uint16)val;
  } else if (funcs == AssessorDesc::dShort) {
    ((sint16*)((uint64)obj + field->ptrOffset))[0] = (sint16)val;
  } else if (funcs == AssessorDesc::dByte) {
    ((sint8*)((uint64)obj + field->ptrOffset))[0] = (sint8)val;
  } else if (funcs == AssessorDesc::dBool) {
    ((uint8*)((uint64)obj + field->ptrOffset))[0] = (uint8)val;
  } else {
    // 0 value for everything else
    ((sint32*)((uint64)obj + field->ptrOffset))[0] = (sint32)val;
  }
}

void JavaJIT::initField(JavaField* field, JavaObject* obj, JavaObject* val) {
  _initField(field);
  ((JavaObject**)((uint64)obj + field->ptrOffset))[0] = val;
}

void JavaJIT::initField(JavaField* field, JavaObject* obj, double val) {
  _initField(field);
  ((double*)((uint64)obj + field->ptrOffset))[0] = val;
}

void JavaJIT::initField(JavaField* field, JavaObject* obj, float val) {
  _initField(field);
  ((float*)((uint64)obj + field->ptrOffset))[0] = val;
}

JavaObject* Class::operator()(Jnjvm* vm) {
  if (!isReady()) 
    isolate->loadName(name, classLoader, true, true, true);
  return doNew(vm);
}

void JavaField::operator()(JavaObject* obj, float val) {
  if (!classDef->isReady()) 
    classDef->isolate->loadName(classDef->name, classDef->classLoader, true, true, true);
  
  bool stat = isStatic(access);
  if (stat) obj = classDef->staticInstance();
  void* ptr = (void*)((uint64)obj + ptrOffset);
  
  if (signature->funcs->llvmType == Type::FloatTy) {
    ((float*)ptr)[0] = val;
  } else {
    JavaThread::get()->isolate->illegalArgumentException("wrong type in field assignment");
  }
}

void JavaField::operator()(JavaObject* obj, double val) {
  if (!classDef->isReady())
    classDef->isolate->loadName(classDef->name, classDef->classLoader, true, true, true);
  
  bool stat = isStatic(access);
  if (stat) obj = classDef->staticInstance();
  void* ptr = (void*)((uint64)obj + ptrOffset);
  
  if (signature->funcs->llvmType == Type::DoubleTy) {
    ((double*)ptr)[0] = val;
  } else {
    JavaThread::get()->isolate->illegalArgumentException("wrong type in field assignment");
  }
}

void JavaField::operator()(JavaObject* obj, sint64 val) {
  if (!classDef->isReady())
    classDef->isolate->loadName(classDef->name, classDef->classLoader, true, true, true);
  
  bool stat = isStatic(access);
  if (stat) obj = classDef->staticInstance();
  void* ptr = (void*)((uint64)obj + ptrOffset);
  
  if (signature->funcs == AssessorDesc::dLong) {
    ((uint64*)ptr)[0] = val;
  } else {
    JavaThread::get()->isolate->illegalArgumentException("wrong type in field assignment");
  }
}

void JavaField::operator()(JavaObject* obj, uint32 val) {
  if (!classDef->isReady())
    classDef->isolate->loadName(classDef->name, classDef->classLoader, true, true, true);
  
  bool stat = isStatic(access);
  if (stat) obj = classDef->staticInstance();
  void* ptr = (void*)((uint64)obj + ptrOffset);
  
  if (signature->funcs == AssessorDesc::dInt) {
    ((sint32*)ptr)[0] = (sint32)val;
  } else if (signature->funcs == AssessorDesc::dShort) {
    ((sint16*)ptr)[0] = (sint16)val;
  } else if (signature->funcs == AssessorDesc::dByte) {
    ((sint8*)ptr)[0] = (sint8)val;
  } else if (signature->funcs == AssessorDesc::dBool) {
    ((uint8*)ptr)[0] = (uint8)val;
  } else if (signature->funcs == AssessorDesc::dChar) {
    ((uint16*)ptr)[0] = (uint16)val;
  } else {
    JavaThread::get()->isolate->illegalArgumentException("wrong type in field assignment");
  }
}

void JavaField::operator()(JavaObject* obj, JavaObject* val) {
  if (!classDef->isReady())
    classDef->isolate->loadName(classDef->name, classDef->classLoader, true, true, true);
  
  bool stat = isStatic(access);
  if (stat) obj = classDef->staticInstance();
  void* ptr = (void*)((uint64)obj + ptrOffset);
  
  if (signature->funcs->llvmType == JavaObject::llvmType) {
    ((JavaObject**)ptr)[0] = val;
  } else {
    JavaThread::get()->isolate->illegalArgumentException("wrong type in field assignment");
  }
}

GenericValue JavaField::operator()(JavaObject* obj) {
  if (!classDef->isReady())
    classDef->isolate->loadName(classDef->name, classDef->classLoader, true, true, true);
  
  bool stat = isStatic(access);
  if (stat) {
    if (obj != 0) {
      // Assignment to a static var
      void* ptr = (void*)((uint64)(classDef->staticInstance()) + ptrOffset);
      ((JavaObject**)ptr)[0] = obj;
      return GenericValue(0);
    } else {
      // Get a static var
      obj = classDef->staticInstance();
    }
  }
  
  assert(obj && "getting a field from a null value");
  
  void* ptr = (void*)((uint64)obj + ptrOffset);
  const Type* type = signature->funcs->llvmType;
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
  } else if (type == JavaObject::llvmType) {
    GenericValue gv(((JavaObject**)ptr)[0]);
    return gv;
  } else {
    assert(0 && "Unknown type!");
    return GenericValue(0);
  }
}

#define readArgs(buf, signature, ap) \
  for (std::vector<Typedef*>::iterator i = signature->args.begin(), \
            e = signature->args.end(); i!= e; i++) { \
    const AssessorDesc* funcs = (*i)->funcs; \
    if (funcs == AssessorDesc::dLong) { \
      ((sint64*)buf)[0] = va_arg(ap, sint64); \
      buf += 2; \
    } else if (funcs == AssessorDesc::dInt) { \
      ((sint32*)buf)[0] = va_arg(ap, sint32); \
      buf++; \
    } else if (funcs == AssessorDesc::dChar) { \
      ((uint32*)buf)[0] = va_arg(ap, uint32); \
      buf++; \
    } else if (funcs == AssessorDesc::dShort) { \
      ((uint32*)buf)[0] = va_arg(ap, uint32); \
      buf++; \
    } else if (funcs == AssessorDesc::dByte) { \
      ((uint32*)buf)[0] = va_arg(ap, uint32); \
      buf++; \
    } else if (funcs == AssessorDesc::dBool) { \
      ((uint32*)buf)[0] = va_arg(ap, uint32); \
      buf++; \
    } else if (funcs == AssessorDesc::dFloat) { \
      ((float*)buf)[0] = (float)va_arg(ap, double); \
      buf++; \
    } else if (funcs == AssessorDesc::dDouble) { \
      ((double*)buf)[0] = va_arg(ap, double); \
      buf += 2; \
    } else if (funcs == AssessorDesc::dRef || funcs == AssessorDesc::dTab) { \
      ((JavaObject**)buf)[0] = va_arg(ap, JavaObject*); \
      buf++; \
    } else { \
      assert(0 && "Should not be here"); \
    } \
  } \


#if 1//defined(__PPC__) && !defined(__MACH__)
#define INVOKE(TYPE, TYPE_NAME, FUNC_TYPE_VIRTUAL_AP, FUNC_TYPE_STATIC_AP, FUNC_TYPE_VIRTUAL_BUF, FUNC_TYPE_STATIC_BUF) \
\
TYPE JavaMethod::invoke##TYPE_NAME##VirtualAP(JavaObject* obj, va_list ap) { \
  if (!classDef->isReady()) \
    classDef->isolate->loadName(classDef->name, classDef->classLoader, true, true, true); \
  \
  verifyNull(obj); \
  JavaMethod* meth = obj->classOf->lookupMethod(name, type, false, true); \
  \
  void** buf = (void**)alloca(meth->signature->args.size() * sizeof(uint64)); \
  void* _buf = (void*)buf; \
  readArgs(buf, signature, ap); \
  void* func = meth->compiledPtr();\
  return ((FUNC_TYPE_VIRTUAL_BUF)signature->virtualCallBuf())(func, obj, _buf);\
}\
\
TYPE JavaMethod::invoke##TYPE_NAME##SpecialAP(JavaObject* obj, va_list ap) {\
  if (!classDef->isReady())\
    classDef->isolate->loadName(classDef->name, classDef->classLoader, true, true, true);\
  \
  verifyNull(obj);\
  void** buf = (void**)alloca(this->signature->args.size() * sizeof(uint64)); \
  void* _buf = (void*)buf; \
  readArgs(buf, signature, ap); \
  void* func = this->compiledPtr();\
  return ((FUNC_TYPE_VIRTUAL_BUF)signature->virtualCallBuf())(func, obj, _buf);\
}\
\
TYPE JavaMethod::invoke##TYPE_NAME##StaticAP(va_list ap) {\
  if (!classDef->isReady())\
    classDef->isolate->loadName(classDef->name, classDef->classLoader, true, true, true);\
  \
  void** buf = (void**)alloca(this->signature->args.size() * sizeof(uint64)); \
  void* _buf = (void*)buf; \
  readArgs(buf, signature, ap); \
  void* func = this->compiledPtr();\
  return ((FUNC_TYPE_STATIC_BUF)signature->staticCallBuf())(func, _buf);\
}\
\
TYPE JavaMethod::invoke##TYPE_NAME##VirtualBuf(JavaObject* obj, void* buf) {\
  if (!classDef->isReady())\
    classDef->isolate->loadName(classDef->name, classDef->classLoader, true, true, true);\
  \
  verifyNull(obj);\
  JavaMethod* meth = obj->classOf->lookupMethod(name, type, false, true);\
  \
  void* func = meth->compiledPtr();\
  return ((FUNC_TYPE_VIRTUAL_BUF)signature->virtualCallBuf())(func, obj, buf);\
}\
\
TYPE JavaMethod::invoke##TYPE_NAME##SpecialBuf(JavaObject* obj, void* buf) {\
  if (!classDef->isReady())\
    classDef->isolate->loadName(classDef->name, classDef->classLoader, true, true, true);\
  \
  verifyNull(obj);\
  void* func = this->compiledPtr();\
  return ((FUNC_TYPE_VIRTUAL_BUF)signature->virtualCallBuf())(func, obj, buf);\
}\
\
TYPE JavaMethod::invoke##TYPE_NAME##StaticBuf(void* buf) {\
  if (!classDef->isReady())\
    classDef->isolate->loadName(classDef->name, classDef->classLoader, true, true, true);\
  \
  void* func = this->compiledPtr();\
  return ((FUNC_TYPE_STATIC_BUF)signature->staticCallBuf())(func, buf);\
}\
\
TYPE JavaMethod::invoke##TYPE_NAME##Virtual(JavaObject* obj, ...) { \
  va_list ap;\
  va_start(ap, obj);\
  TYPE res = invoke##TYPE_NAME##VirtualAP(obj, ap);\
  va_end(ap); \
  return res; \
}\
\
TYPE JavaMethod::invoke##TYPE_NAME##Special(JavaObject* obj, ...) {\
  va_list ap;\
  va_start(ap, obj);\
  TYPE res = invoke##TYPE_NAME##SpecialAP(obj, ap);\
  va_end(ap); \
  return res; \
}\
\
TYPE JavaMethod::invoke##TYPE_NAME##Static(...) {\
  va_list ap;\
  va_start(ap, this);\
  TYPE res = invoke##TYPE_NAME##StaticAP(ap);\
  va_end(ap); \
  return res; \
}\

#else

#define INVOKE(TYPE, TYPE_NAME, FUNC_TYPE_VIRTUAL_AP, FUNC_TYPE_STATIC_AP, FUNC_TYPE_VIRTUAL_BUF, FUNC_TYPE_STATIC_BUF) \
\
TYPE JavaMethod::invoke##TYPE_NAME##VirtualAP(JavaObject* obj, va_list ap) { \
  if (!classDef->isReady()) \
    classDef->isolate->loadName(classDef->name, classDef->classLoader, true, true, true); \
  \
  verifyNull(obj); \
  JavaMethod* meth = obj->classOf->lookupMethod(name, type, false, true); \
  \
  void* func = meth->compiledPtr();\
  return ((FUNC_TYPE_VIRTUAL_AP)signature->virtualCallAP())(func, obj, ap);\
}\
\
TYPE JavaMethod::invoke##TYPE_NAME##SpecialAP(JavaObject* obj, va_list ap) {\
  if (!classDef->isReady())\
    classDef->isolate->loadName(classDef->name, classDef->classLoader, true, true, true);\
  \
  verifyNull(obj);\
  void* func = this->compiledPtr();\
  return ((FUNC_TYPE_VIRTUAL_AP)signature->virtualCallAP())(func, obj, ap);\
}\
\
TYPE JavaMethod::invoke##TYPE_NAME##StaticAP(va_list ap) {\
  if (!classDef->isReady())\
    classDef->isolate->loadName(classDef->name, classDef->classLoader, true, true, true);\
  \
  void* func = this->compiledPtr();\
  return ((FUNC_TYPE_STATIC_AP)signature->staticCallAP())(func, ap);\
}\
\
TYPE JavaMethod::invoke##TYPE_NAME##VirtualBuf(JavaObject* obj, void* buf) {\
  if (!classDef->isReady())\
    classDef->isolate->loadName(classDef->name, classDef->classLoader, true, true, true);\
  \
  verifyNull(obj);\
  JavaMethod* meth = obj->classOf->lookupMethod(name, type, false, true);\
  \
  void* func = meth->compiledPtr();\
  return ((FUNC_TYPE_VIRTUAL_BUF)signature->virtualCallBuf())(func, obj, buf);\
}\
\
TYPE JavaMethod::invoke##TYPE_NAME##SpecialBuf(JavaObject* obj, void* buf) {\
  if (!classDef->isReady())\
    classDef->isolate->loadName(classDef->name, classDef->classLoader, true, true, true);\
  \
  verifyNull(obj);\
  void* func = this->compiledPtr();\
  return ((FUNC_TYPE_VIRTUAL_BUF)signature->virtualCallBuf())(func, obj, buf);\
}\
\
TYPE JavaMethod::invoke##TYPE_NAME##StaticBuf(void* buf) {\
  if (!classDef->isReady())\
    classDef->isolate->loadName(classDef->name, classDef->classLoader, true, true, true);\
  \
  void* func = this->compiledPtr();\
  return ((FUNC_TYPE_STATIC_BUF)signature->staticCallBuf())(func, buf);\
}\
\
TYPE JavaMethod::invoke##TYPE_NAME##Virtual(JavaObject* obj, ...) { \
  va_list ap;\
  va_start(ap, obj);\
  TYPE res = invoke##TYPE_NAME##VirtualAP(obj, ap);\
  va_end(ap); \
  return res; \
}\
\
TYPE JavaMethod::invoke##TYPE_NAME##Special(JavaObject* obj, ...) {\
  va_list ap;\
  va_start(ap, obj);\
  TYPE res = invoke##TYPE_NAME##SpecialAP(obj, ap);\
  va_end(ap); \
  return res; \
}\
\
TYPE JavaMethod::invoke##TYPE_NAME##Static(...) {\
  va_list ap;\
  va_start(ap, this);\
  TYPE res = invoke##TYPE_NAME##StaticAP(ap);\
  va_end(ap); \
  return res; \
}\

#endif

typedef uint32 (*uint32_virtual_ap)(void*, JavaObject*, va_list);
typedef sint64 (*sint64_virtual_ap)(void*, JavaObject*, va_list);
typedef float  (*float_virtual_ap)(void*, JavaObject*, va_list);
typedef double (*double_virtual_ap)(void*, JavaObject*, va_list);
typedef JavaObject* (*object_virtual_ap)(void*, JavaObject*, va_list);

typedef uint32 (*uint32_static_ap)(void*, va_list);
typedef sint64 (*sint64_static_ap)(void*, va_list);
typedef float  (*float_static_ap)(void*, va_list);
typedef double (*double_static_ap)(void*, va_list);
typedef JavaObject* (*object_static_ap)(void*, va_list);

typedef uint32 (*uint32_virtual_buf)(void*, JavaObject*, void*);
typedef sint64 (*sint64_virtual_buf)(void*, JavaObject*, void*);
typedef float  (*float_virtual_buf)(void*, JavaObject*, void*);
typedef double (*double_virtual_buf)(void*, JavaObject*, void*);
typedef JavaObject* (*object_virtual_buf)(void*, JavaObject*, void*);

typedef uint32 (*uint32_static_buf)(void*, void*);
typedef sint64 (*sint64_static_buf)(void*, void*);
typedef float  (*float_static_buf)(void*, void*);
typedef double (*double_static_buf)(void*, void*);
typedef JavaObject* (*object_static_buf)(void*, void*);

INVOKE(uint32, Int, uint32_virtual_ap, uint32_static_ap, uint32_virtual_buf, uint32_static_buf)
INVOKE(sint64, Long, sint64_virtual_ap, sint64_static_ap, sint64_virtual_buf, sint64_static_buf)
INVOKE(float,  Float, float_virtual_ap,  float_static_ap,  float_virtual_buf,  float_static_buf)
INVOKE(double, Double, double_virtual_ap, double_static_ap, double_virtual_buf, double_static_buf)
INVOKE(JavaObject*, JavaObject, object_virtual_ap, object_static_ap, object_virtual_buf, object_static_buf)

#undef INVOKE

GenericValue JavaObject::operator()(JavaField* field) {
  return (*field)(this);
}

void JavaObject::operator()(JavaField* field, float val) {
  return (*field)(this, val);
}

void JavaObject::operator()(JavaField* field, double val) {
  return (*field)(this, val);
}

void JavaObject::operator()(JavaField* field, uint32 val) {
  return (*field)(this, val);
}

void JavaObject::operator()(JavaField* field, sint64 val) {
  return (*field)(this, val);
}

void JavaObject::operator()(JavaField* field, JavaObject* val) {
  return (*field)(this, val);
}

void JavaField::operator()(float val) {
  JavaField * field = this;
  return (*field)(classDef->virtualInstance, val);
}

void JavaField::operator()(double val) {
  JavaField * field = this;
  return (*field)(classDef->virtualInstance, val);
}

void JavaField::operator()(sint64 val) {
  JavaField * field = this;
  return (*field)(classDef->virtualInstance, val);
}

void JavaField::operator()(uint32 val) {
  JavaField * field = this;
  return (*field)(classDef->virtualInstance, val);
}

Function* Signdef::createFunctionCallBuf(bool virt) {
  
  ConstantInt* CI = mvm::jit::constantZero;
  std::vector<Value*> Args;

  isolate->protectModule->lock();
  Function* res = Function::Create(virt ? virtualBufType : staticBufType,
                                      GlobalValue::ExternalLinkage,
                                      this->printString(),
                                      isolate->module);
  isolate->protectModule->unlock();
  
  BasicBlock* currentBlock = BasicBlock::Create("enter", res);
  Function::arg_iterator i = res->arg_begin();
  Value *obj, *ptr, *func;
  func = i;
  ++i;
  if (virt) {
    obj = i;
    ++i;
    Args.push_back(obj);
  }
  ptr = i;

  for (std::vector<Typedef*>::iterator i = args.begin(), 
            e = args.end(); i!= e; ++i) {
  
    const AssessorDesc* funcs = (*i)->funcs;
    ptr = GetElementPtrInst::Create(ptr, CI, "", currentBlock);
    Value* val = new BitCastInst(ptr, funcs->llvmTypePtr, "", currentBlock);
    Value* arg = new LoadInst(val, "", currentBlock);
    Args.push_back(arg);
    if (funcs == AssessorDesc::dLong || funcs == AssessorDesc::dDouble) {
      CI = mvm::jit::constantTwo;
    } else {
      CI = mvm::jit::constantOne;
    }
  }

#ifdef MULTIPLE_VM
  Args.push_back(mvm::jit::constantPtrNull);
#endif

  Value* val = CallInst::Create(func, Args.begin(), Args.end(), "", currentBlock);
  if (ret->funcs != AssessorDesc::dVoid)
    ReturnInst::Create(val, currentBlock);
  else
    ReturnInst::Create(currentBlock);
  
  return res;
}

Function* Signdef::createFunctionCallAP(bool virt) {
  
  std::vector<Value*> Args;

  isolate->protectModule->lock();
  Function* res = Function::Create(virt ? virtualBufType : staticBufType,
                                      GlobalValue::ExternalLinkage,
                                      this->printString(),
                                      isolate->module);
  isolate->protectModule->unlock();
  
  BasicBlock* currentBlock = BasicBlock::Create("enter", res);
  Function::arg_iterator i = res->arg_begin();
  Value *obj, *ap, *func;
  func = i;
  ++i;
  if (virt) {
    obj = i;
    Args.push_back(obj);
    ++i;
  }
  ap = i;

  for (std::vector<Typedef*>::iterator i = args.begin(), 
            e = args.end(); i!= e; i++) {

    Args.push_back(new VAArgInst(ap, (*i)->funcs->llvmType, "", currentBlock));
  }

#ifdef MULTIPLE_VM
  Args.push_back(mvm::jit::constantPtrNull);
#endif

  Value* val = CallInst::Create(func, Args.begin(), Args.end(), "", currentBlock);
  if (ret->funcs != AssessorDesc::dVoid)
    ReturnInst::Create(val, currentBlock);
  else
    ReturnInst::Create(currentBlock);
  
  return res;
}

void* Signdef::staticCallBuf() {
  if (!_staticCallBuf) {
    Function* statBuf = createFunctionCallBuf(false);
    _staticCallBuf = (mvm::Code*)mvm::jit::executionEngine->getPointerToGlobal(statBuf);
  }
  return _staticCallBuf;
}

void* Signdef::virtualCallBuf() {
  if (!_virtualCallBuf) {
    Function* virtBuf = createFunctionCallBuf(true);
    _virtualCallBuf = (mvm::Code*)mvm::jit::executionEngine->getPointerToGlobal(virtBuf);
  }
  return _virtualCallBuf;
}

void* Signdef::staticCallAP() {
  if (!_staticCallAP) {
    Function* statAP = createFunctionCallAP(false);
    _staticCallAP = (mvm::Code*)mvm::jit::executionEngine->getPointerToGlobal(statAP);
  }
  return _staticCallAP;
}

void* Signdef::virtualCallAP() {
  if (!_virtualCallAP) {
    Function* virtAP = createFunctionCallAP(true);
    _virtualCallAP = (mvm::Code*)mvm::jit::executionEngine->getPointerToGlobal(virtAP);
  }
  return _virtualCallAP;
}
