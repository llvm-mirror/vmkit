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

void JavaJIT::invokeOnceVoid(Jnjvm* vm, JnjvmClassLoader* loader,
                             char const* className, char const* func,
                             char const* sign, int access, ...) {
  
  UserCommonClass* cl = loader->loadName(loader->asciizConstructUTF8(className),
                                         true, true);
  
  cl->initialiseClass(vm);
  bool stat = access == ACC_STATIC ? true : false;
  JavaMethod* method = cl->lookupMethod(loader->asciizConstructUTF8(func), 
                                        loader->asciizConstructUTF8(sign), stat,
                                        true);
  va_list ap;
  va_start(ap, access);
  if (stat) {
    method->invokeIntStaticAP(vm, (UserClass*)cl, ap);
  } else {
    JavaObject* obj = va_arg(ap, JavaObject*);
    method->invokeIntSpecialAP(vm, (UserClass*)cl, obj, ap);
  }
  va_end(ap);
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
TYPE JavaMethod::invoke##TYPE_NAME##VirtualAP(Jnjvm* vm, UserClass* cl, JavaObject* obj, va_list ap) { \
  if (!cl->isReady()) { \
    cl->classLoader->loadName(cl->getName(), true, true); \
    cl->initialiseClass(vm); \
  } \
  verifyNull(obj); \
  Signdef* sign = getSignature(); \
  void** buf = (void**)alloca(sign->args.size() * sizeof(uint64)); \
  void* _buf = (void*)buf; \
  readArgs(buf, sign, ap); \
  void* func = (((void***)obj)[0])[offset];\
  return ((FUNC_TYPE_VIRTUAL_BUF)sign->getVirtualCallBuf())(vm, cl->getCtpCache(), func, obj, _buf);\
}\
\
TYPE JavaMethod::invoke##TYPE_NAME##SpecialAP(Jnjvm* vm, UserClass* cl, JavaObject* obj, va_list ap) {\
  if (!cl->isReady()) { \
    cl->classLoader->loadName(cl->name, true, true); \
    cl->initialiseClass(vm); \
  } \
  \
  verifyNull(obj);\
  Signdef* sign = getSignature(); \
  void** buf = (void**)alloca(sign->args.size() * sizeof(uint64)); \
  void* _buf = (void*)buf; \
  readArgs(buf, sign, ap); \
  void* func = this->compiledPtr();\
  return ((FUNC_TYPE_VIRTUAL_BUF)sign->getVirtualCallBuf())(vm, cl->getCtpCache(), func, obj, _buf);\
}\
\
TYPE JavaMethod::invoke##TYPE_NAME##StaticAP(Jnjvm* vm, UserClass* cl, va_list ap) {\
  if (!cl->isReady()) { \
    cl->classLoader->loadName(cl->getName(), true, true); \
    cl->initialiseClass(vm); \
  } \
  \
  Signdef* sign = getSignature(); \
  void** buf = (void**)alloca(sign->args.size() * sizeof(uint64)); \
  void* _buf = (void*)buf; \
  readArgs(buf, sign, ap); \
  void* func = this->compiledPtr();\
  return ((FUNC_TYPE_STATIC_BUF)sign->getStaticCallBuf())(vm, cl->getCtpCache(), func, _buf);\
}\
\
TYPE JavaMethod::invoke##TYPE_NAME##VirtualBuf(Jnjvm* vm, UserClass* cl, JavaObject* obj, void* buf) {\
  if (!cl->isReady()) { \
    cl->classLoader->loadName(cl->getName(), true, true); \
    cl->initialiseClass(vm); \
  } \
  \
  verifyNull(obj);\
  JavaMethod* meth = obj->classOf->lookupMethod(name, type, false, true);\
  \
  Signdef* sign = getSignature(); \
  void* func = meth->compiledPtr();\
  return ((FUNC_TYPE_VIRTUAL_BUF)sign->getVirtualCallBuf())(vm, cl->getCtpCache(), func, obj, buf);\
}\
\
TYPE JavaMethod::invoke##TYPE_NAME##SpecialBuf(Jnjvm* vm, UserClass* cl, JavaObject* obj, void* buf) {\
  if (!cl->isReady()) { \
    cl->classLoader->loadName(cl->getName(), true, true); \
    cl->initialiseClass(vm); \
  } \
  \
  verifyNull(obj);\
  void* func = this->compiledPtr();\
  Signdef* sign = getSignature(); \
  return ((FUNC_TYPE_VIRTUAL_BUF)sign->getVirtualCallBuf())(vm, cl->getCtpCache(), func, obj, buf);\
}\
\
TYPE JavaMethod::invoke##TYPE_NAME##StaticBuf(Jnjvm* vm, UserClass* cl, void* buf) {\
  if (!cl->isReady()) { \
    cl->classLoader->loadName(cl->getName(), true, true); \
    cl->initialiseClass(vm); \
  } \
  \
  void* func = this->compiledPtr();\
  Signdef* sign = getSignature(); \
  return ((FUNC_TYPE_STATIC_BUF)sign->getStaticCallBuf())(vm, cl->getCtpCache(), func, buf);\
}\
\
TYPE JavaMethod::invoke##TYPE_NAME##Virtual(Jnjvm* vm, UserClass* cl, JavaObject* obj, ...) { \
  va_list ap;\
  va_start(ap, obj);\
  TYPE res = invoke##TYPE_NAME##VirtualAP(vm, cl, obj, ap);\
  va_end(ap); \
  return res; \
}\
\
TYPE JavaMethod::invoke##TYPE_NAME##Special(Jnjvm* vm, UserClass* cl, JavaObject* obj, ...) {\
  va_list ap;\
  va_start(ap, obj);\
  TYPE res = invoke##TYPE_NAME##SpecialAP(vm, cl, obj, ap);\
  va_end(ap); \
  return res; \
}\
\
TYPE JavaMethod::invoke##TYPE_NAME##Static(Jnjvm* vm, UserClass* cl, ...) {\
  va_list ap;\
  va_start(ap, cl);\
  TYPE res = invoke##TYPE_NAME##StaticAP(vm, cl, ap);\
  va_end(ap); \
  return res; \
}\

#else

#define INVOKE(TYPE, TYPE_NAME, FUNC_TYPE_VIRTUAL_AP, FUNC_TYPE_STATIC_AP, FUNC_TYPE_VIRTUAL_BUF, FUNC_TYPE_STATIC_BUF) \
\
TYPE JavaMethod::invoke##TYPE_NAME##VirtualAP(Jnjvm* vm, UserClass* cl, JavaObject* obj, va_list ap) { \
  if (!cl->isReady()) { \
    cl->classLoader->loadName(cl->getName(), true, true); \
    cl->initialiseClass(vm); \
  } \
  \
  verifyNull(obj); \
  void* func = (((void***)obj)[0])[offset];\
  Signdef* sign = getSignature(); \
  return ((FUNC_TYPE_VIRTUAL_AP)sign->getVirtualCallAP())(vm, cl->getCtpCache(), func, obj, ap);\
}\
\
TYPE JavaMethod::invoke##TYPE_NAME##SpecialAP(Jnjvm* vm, UserClass* cl, JavaObject* obj, va_list ap) {\
  if (!cl->isReady()) { \
    cl->classLoader->loadName(cl->getName(), true, true); \
    cl->initialiseClass(vm); \
  } \
  \
  verifyNull(obj);\
  void* func = this->compiledPtr();\
  Signdef* sign = getSignature(); \
  return ((FUNC_TYPE_VIRTUAL_AP)sign->getVirtualCallAP())(vm, cl->getCtpCache(), func, obj, ap);\
}\
\
TYPE JavaMethod::invoke##TYPE_NAME##StaticAP(Jnjvm* vm, UserClass* cl, va_list ap) {\
  if (!cl->isReady()) { \
    cl->classLoader->loadName(cl->getName(), true, true); \
    cl->initialiseClass(vm); \
  } \
  \
  void* func = this->compiledPtr();\
  Signdef* sign = getSignature(); \
  return ((FUNC_TYPE_STATIC_AP)sign->getStaticCallAP())(vm, cl->getCtpCache(), func, ap);\
}\
\
TYPE JavaMethod::invoke##TYPE_NAME##VirtualBuf(Jnjvm* vm, UserClass* cl, JavaObject* obj, void* buf) {\
  if (!cl->isReady()) { \
    cl->classLoader->loadName(cl->getName(), true, true); \
    cl->initialiseClass(vm); \
  } \
  \
  verifyNull(obj);\
  void* func = (((void***)obj)[0])[offset];\
  Signdef* sign = getSignature(); \
  return ((FUNC_TYPE_VIRTUAL_BUF)sign->getVirtualCallBuf())(vm, cl->getCtpCache(), func, obj, buf);\
}\
\
TYPE JavaMethod::invoke##TYPE_NAME##SpecialBuf(Jnjvm* vm, UserClass* cl, JavaObject* obj, void* buf) {\
  if (!cl->isReady()) { \
    cl->classLoader->loadName(cl->getName(), true, true); \
    cl->initialiseClass(vm); \
  } \
  \
  verifyNull(obj);\
  void* func = this->compiledPtr();\
  Signdef* sign = getSignature(); \
  return ((FUNC_TYPE_VIRTUAL_BUF)sign->getVirtualCallBuf())(vm, cl->getCtpCache(), func, obj, buf);\
}\
\
TYPE JavaMethod::invoke##TYPE_NAME##StaticBuf(Jnjvm* vm, UserClass* cl, void* buf) {\
  if (!cl->isReady()) { \
    cl->classLoader->loadName(cl->getName(), true, true); \
    cl->initialiseClass(vm); \
  } \
  \
  void* func = this->compiledPtr();\
  Signdef* sign = getSignature(); \
  return ((FUNC_TYPE_STATIC_BUF)sign->getStaticCallBuf())(vm, cl->getCtpCache(), func, buf);\
}\
\
TYPE JavaMethod::invoke##TYPE_NAME##Virtual(Jnjvm* vm, UserClass* cl, JavaObject* obj, ...) { \
  va_list ap;\
  va_start(ap, obj);\
  TYPE res = invoke##TYPE_NAME##VirtualAP(vm, cl, obj, ap);\
  va_end(ap); \
  return res; \
}\
\
TYPE JavaMethod::invoke##TYPE_NAME##Special(Jnjvm* vm, UserClass* cl, JavaObject* obj, ...) {\
  va_list ap;\
  va_start(ap, obj);\
  TYPE res = invoke##TYPE_NAME##SpecialAP(vm, cl, obj, ap);\
  va_end(ap); \
  return res; \
}\
\
TYPE JavaMethod::invoke##TYPE_NAME##Static(Jnjvm* vm, UserClass* cl, ...) {\
  va_list ap;\
  va_start(ap, cl);\
  TYPE res = invoke##TYPE_NAME##StaticAP(vm, cl, ap);\
  va_end(ap); \
  return res; \
}\

#endif

typedef uint32 (*uint32_virtual_ap)(Jnjvm*, UserConstantPool*, void*, JavaObject*, va_list);
typedef sint64 (*sint64_virtual_ap)(Jnjvm*, UserConstantPool*, void*, JavaObject*, va_list);
typedef float  (*float_virtual_ap)(Jnjvm*, UserConstantPool*, void*, JavaObject*, va_list);
typedef double (*double_virtual_ap)(Jnjvm*, UserConstantPool*, void*, JavaObject*, va_list);
typedef JavaObject* (*object_virtual_ap)(Jnjvm*, UserConstantPool*, void*, JavaObject*, va_list);

typedef uint32 (*uint32_static_ap)(Jnjvm*, UserConstantPool*, void*, va_list);
typedef sint64 (*sint64_static_ap)(Jnjvm*, UserConstantPool*, void*, va_list);
typedef float  (*float_static_ap)(Jnjvm*, UserConstantPool*, void*, va_list);
typedef double (*double_static_ap)(Jnjvm*, UserConstantPool*, void*, va_list);
typedef JavaObject* (*object_static_ap)(Jnjvm*, UserConstantPool*, void*, va_list);

typedef uint32 (*uint32_virtual_buf)(Jnjvm*, UserConstantPool*, void*, JavaObject*, void*);
typedef sint64 (*sint64_virtual_buf)(Jnjvm*, UserConstantPool*, void*, JavaObject*, void*);
typedef float  (*float_virtual_buf)(Jnjvm*, UserConstantPool*, void*, JavaObject*, void*);
typedef double (*double_virtual_buf)(Jnjvm*, UserConstantPool*, void*, JavaObject*, void*);
typedef JavaObject* (*object_virtual_buf)(Jnjvm*, UserConstantPool*, void*, JavaObject*, void*);

typedef uint32 (*uint32_static_buf)(Jnjvm*, UserConstantPool*, void*, void*);
typedef sint64 (*sint64_static_buf)(Jnjvm*, UserConstantPool*, void*, void*);
typedef float  (*float_static_buf)(Jnjvm*, UserConstantPool*, void*, void*);
typedef double (*double_static_buf)(Jnjvm*, UserConstantPool*, void*, void*);
typedef JavaObject* (*object_static_buf)(Jnjvm*, UserConstantPool*, void*, void*);

INVOKE(uint32, Int, uint32_virtual_ap, uint32_static_ap, uint32_virtual_buf, uint32_static_buf)
INVOKE(sint64, Long, sint64_virtual_ap, sint64_static_ap, sint64_virtual_buf, sint64_static_buf)
INVOKE(float,  Float, float_virtual_ap,  float_static_ap,  float_virtual_buf,  float_static_buf)
INVOKE(double, Double, double_virtual_ap, double_static_ap, double_virtual_buf, double_static_buf)
INVOKE(JavaObject*, JavaObject, object_virtual_ap, object_static_ap, object_virtual_buf, object_static_buf)

#undef INVOKE
