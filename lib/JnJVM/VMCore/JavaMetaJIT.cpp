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
#include "JnjvmModule.h"

using namespace jnjvm;
using namespace llvm;

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
    method->invokeIntStaticAP(vm, ap);
  } else {
    JavaObject* obj = va_arg(ap, JavaObject*);
    method->invokeIntSpecialAP(vm, obj, ap);
  }
  va_end(ap);
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
  
  if (signature->funcs == AssessorDesc::dRef || 
      signature->funcs == AssessorDesc::dTab) {
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
  } else if (type == JnjvmModule::JavaObjectType) {
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
TYPE JavaMethod::invoke##TYPE_NAME##VirtualAP(Jnjvm* vm, JavaObject* obj, va_list ap) { \
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
  return ((FUNC_TYPE_VIRTUAL_BUF)signature->virtualCallBuf())(vm, func, obj, _buf);\
}\
\
TYPE JavaMethod::invoke##TYPE_NAME##SpecialAP(Jnjvm* vm, JavaObject* obj, va_list ap) {\
  if (!classDef->isReady())\
    classDef->isolate->loadName(classDef->name, classDef->classLoader, true, true, true);\
  \
  verifyNull(obj);\
  void** buf = (void**)alloca(this->signature->args.size() * sizeof(uint64)); \
  void* _buf = (void*)buf; \
  readArgs(buf, signature, ap); \
  void* func = this->compiledPtr();\
  return ((FUNC_TYPE_VIRTUAL_BUF)signature->virtualCallBuf())(vm, func, obj, _buf);\
}\
\
TYPE JavaMethod::invoke##TYPE_NAME##StaticAP(Jnjvm* vm, va_list ap) {\
  if (!classDef->isReady())\
    classDef->isolate->loadName(classDef->name, classDef->classLoader, true, true, true);\
  \
  void** buf = (void**)alloca(this->signature->args.size() * sizeof(uint64)); \
  void* _buf = (void*)buf; \
  readArgs(buf, signature, ap); \
  void* func = this->compiledPtr();\
  return ((FUNC_TYPE_STATIC_BUF)signature->staticCallBuf())(vm, func, _buf);\
}\
\
TYPE JavaMethod::invoke##TYPE_NAME##VirtualBuf(Jnjvm* vm, JavaObject* obj, void* buf) {\
  if (!classDef->isReady())\
    classDef->isolate->loadName(classDef->name, classDef->classLoader, true, true, true);\
  \
  verifyNull(obj);\
  JavaMethod* meth = obj->classOf->lookupMethod(name, type, false, true);\
  \
  void* func = meth->compiledPtr();\
  return ((FUNC_TYPE_VIRTUAL_BUF)signature->virtualCallBuf())(vm, func, obj, buf);\
}\
\
TYPE JavaMethod::invoke##TYPE_NAME##SpecialBuf(Jnjvm* vm, JavaObject* obj, void* buf) {\
  if (!classDef->isReady())\
    classDef->isolate->loadName(classDef->name, classDef->classLoader, true, true, true);\
  \
  verifyNull(obj);\
  void* func = this->compiledPtr();\
  return ((FUNC_TYPE_VIRTUAL_BUF)signature->virtualCallBuf())(vm, func, obj, buf);\
}\
\
TYPE JavaMethod::invoke##TYPE_NAME##StaticBuf(Jnjvm* vm, void* buf) {\
  if (!classDef->isReady())\
    classDef->isolate->loadName(classDef->name, classDef->classLoader, true, true, true);\
  \
  void* func = this->compiledPtr();\
  return ((FUNC_TYPE_STATIC_BUF)signature->staticCallBuf())(vm, func, buf);\
}\
\
TYPE JavaMethod::invoke##TYPE_NAME##Virtual(Jnjvm* vm, JavaObject* obj, ...) { \
  va_list ap;\
  va_start(ap, obj);\
  TYPE res = invoke##TYPE_NAME##VirtualAP(vm, obj, ap);\
  va_end(ap); \
  return res; \
}\
\
TYPE JavaMethod::invoke##TYPE_NAME##Special(Jnjvm* vm, JavaObject* obj, ...) {\
  va_list ap;\
  va_start(ap, obj);\
  TYPE res = invoke##TYPE_NAME##SpecialAP(vm, obj, ap);\
  va_end(ap); \
  return res; \
}\
\
TYPE JavaMethod::invoke##TYPE_NAME##Static(Jnjvm* vm, ...) {\
  va_list ap;\
  va_start(ap, vm);\
  TYPE res = invoke##TYPE_NAME##StaticAP(vm, ap);\
  va_end(ap); \
  return res; \
}\

#else

#define INVOKE(TYPE, TYPE_NAME, FUNC_TYPE_VIRTUAL_AP, FUNC_TYPE_STATIC_AP, FUNC_TYPE_VIRTUAL_BUF, FUNC_TYPE_STATIC_BUF) \
\
TYPE JavaMethod::invoke##TYPE_NAME##VirtualAP(Jnjvm* vm, JavaObject* obj, va_list ap) { \
  if (!classDef->isReady()) \
    classDef->isolate->loadName(classDef->name, classDef->classLoader, true, true, true); \
  \
  verifyNull(obj); \
  JavaMethod* meth = obj->classOf->lookupMethod(name, type, false, true); \
  \
  void* func = meth->compiledPtr();\
  return ((FUNC_TYPE_VIRTUAL_AP)signature->virtualCallAP())(vm, func, obj, ap);\
}\
\
TYPE JavaMethod::invoke##TYPE_NAME##SpecialAP(Jnjvm* vm, JavaObject* obj, va_list ap) {\
  if (!classDef->isReady())\
    classDef->isolate->loadName(classDef->name, classDef->classLoader, true, true, true);\
  \
  verifyNull(obj);\
  void* func = this->compiledPtr();\
  return ((FUNC_TYPE_VIRTUAL_AP)signature->virtualCallAP())(vm, func, obj, ap);\
}\
\
TYPE JavaMethod::invoke##TYPE_NAME##StaticAP(Jnjvm* vm, va_list ap) {\
  if (!classDef->isReady())\
    classDef->isolate->loadName(classDef->name, classDef->classLoader, true, true, true);\
  \
  void* func = this->compiledPtr();\
  return ((FUNC_TYPE_STATIC_AP)signature->staticCallAP())(vm, func, ap);\
}\
\
TYPE JavaMethod::invoke##TYPE_NAME##VirtualBuf(Jnjvm* vm, JavaObject* obj, void* buf) {\
  if (!classDef->isReady())\
    classDef->isolate->loadName(classDef->name, classDef->classLoader, true, true, true);\
  \
  verifyNull(obj);\
  JavaMethod* meth = obj->classOf->lookupMethod(name, type, false, true);\
  \
  void* func = meth->compiledPtr();\
  return ((FUNC_TYPE_VIRTUAL_BUF)signature->virtualCallBuf())(vm, func, obj, buf);\
}\
\
TYPE JavaMethod::invoke##TYPE_NAME##SpecialBuf(Jnjvm* vm, JavaObject* obj, void* buf) {\
  if (!classDef->isReady())\
    classDef->isolate->loadName(classDef->name, classDef->classLoader, true, true, true);\
  \
  verifyNull(obj);\
  void* func = this->compiledPtr();\
  return ((FUNC_TYPE_VIRTUAL_BUF)signature->virtualCallBuf())(vm, func, obj, buf);\
}\
\
TYPE JavaMethod::invoke##TYPE_NAME##StaticBuf(Jnjvm* vm, void* buf) {\
  if (!classDef->isReady())\
    classDef->isolate->loadName(classDef->name, classDef->classLoader, true, true, true);\
  \
  void* func = this->compiledPtr();\
  return ((FUNC_TYPE_STATIC_BUF)signature->staticCallBuf())(vm, func, buf);\
}\
\
TYPE JavaMethod::invoke##TYPE_NAME##Virtual(Jnjvm* vm,JavaObject* obj, ...) { \
  va_list ap;\
  va_start(ap, obj);\
  TYPE res = invoke##TYPE_NAME##VirtualAP(vm, obj, ap);\
  va_end(ap); \
  return res; \
}\
\
TYPE JavaMethod::invoke##TYPE_NAME##Special(Jnjvm* vm, JavaObject* obj, ...) {\
  va_list ap;\
  va_start(ap, obj);\
  TYPE res = invoke##TYPE_NAME##SpecialAP(vm, obj, ap);\
  va_end(ap); \
  return res; \
}\
\
TYPE JavaMethod::invoke##TYPE_NAME##Static(Jnjvm* vm, ...) {\
  va_list ap;\
  va_start(ap, this);\
  TYPE res = invoke##TYPE_NAME##StaticAP(vm, ap);\
  va_end(ap); \
  return res; \
}\

#endif

typedef uint32 (*uint32_virtual_ap)(Jnjvm*, void*, JavaObject*, va_list);
typedef sint64 (*sint64_virtual_ap)(Jnjvm*, void*, JavaObject*, va_list);
typedef float  (*float_virtual_ap)(Jnjvm*, void*, JavaObject*, va_list);
typedef double (*double_virtual_ap)(Jnjvm*, void*, JavaObject*, va_list);
typedef JavaObject* (*object_virtual_ap)(Jnjvm*, void*, JavaObject*, va_list);

typedef uint32 (*uint32_static_ap)(Jnjvm*, void*, va_list);
typedef sint64 (*sint64_static_ap)(Jnjvm*, void*, va_list);
typedef float  (*float_static_ap)(Jnjvm*, void*, va_list);
typedef double (*double_static_ap)(Jnjvm*, void*, va_list);
typedef JavaObject* (*object_static_ap)(Jnjvm*, void*, va_list);

typedef uint32 (*uint32_virtual_buf)(Jnjvm*, void*, JavaObject*, void*);
typedef sint64 (*sint64_virtual_buf)(Jnjvm*, void*, JavaObject*, void*);
typedef float  (*float_virtual_buf)(Jnjvm*, void*, JavaObject*, void*);
typedef double (*double_virtual_buf)(Jnjvm*, void*, JavaObject*, void*);
typedef JavaObject* (*object_virtual_buf)(Jnjvm*, void*, JavaObject*, void*);

typedef uint32 (*uint32_static_buf)(Jnjvm*, void*, void*);
typedef sint64 (*sint64_static_buf)(Jnjvm*, void*, void*);
typedef float  (*float_static_buf)(Jnjvm*, void*, void*);
typedef double (*double_static_buf)(Jnjvm*, void*, void*);
typedef JavaObject* (*object_static_buf)(Jnjvm*, void*, void*);

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
  return (*field)(0, val);
}

void JavaField::operator()(double val) {
  JavaField * field = this;
  return (*field)(0, val);
}

void JavaField::operator()(sint64 val) {
  JavaField * field = this;
  return (*field)(0, val);
}

void JavaField::operator()(uint32 val) {
  JavaField * field = this;
  return (*field)(0, val);
}
