//===----- JavaMetaJIT.cpp - Caling Java methods from native code ---------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <cstdarg>
#include <cstring>
#include "j3/jni.h"

#include "debug.h"

#include "JavaClass.h"
#include "JavaObject.h"
#include "JavaThread.h"
#include "JavaTypes.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"

using namespace j3;


//===----------------------------------------------------------------------===//
// We do not need to have special care on the GC-pointers in the buffer
// manipulated in these functions because the objects in the buffer are
// addressed and never stored directly.
//===----------------------------------------------------------------------===//

jvalue* JavaMethod::marshalArguments(vmkit::ThreadAllocator& allocator, va_list ap)
{
	Signdef* signature = getSignature();
	if (signature->nbArguments == 0) return NULL;	//Zero arguments

	Typedef* const* arguments = signature->getArgumentsType();

	jvalue* buffer = (jvalue*)allocator.Allocate(signature->nbArguments * sizeof(jvalue));

	for (uint32 i = 0; i < signature->nbArguments; ++i) {
		const Typedef* type = arguments[i];

		if (type->isPrimitive()) {
			const PrimitiveTypedef* prim = (const PrimitiveTypedef*)type;

			     if (prim->isLong())	buffer[i].j = va_arg(ap, sint64);
			else if (prim->isInt())		buffer[i].i = va_arg(ap, sint32);
			else if (prim->isChar())	buffer[i].c = va_arg(ap, uint32);
			else if (prim->isShort())	buffer[i].s = va_arg(ap, sint32);
			else if (prim->isByte())	buffer[i].b = va_arg(ap, sint32);
			else if (prim->isBool())	buffer[i].z = va_arg(ap, uint32);
			else if (prim->isFloat())	buffer[i].f = (float)va_arg(ap, double);
			else if (prim->isDouble())	buffer[i].d = va_arg(ap, double);
			else assert(0 && "Unknown primitive type.");
		} else
			buffer[i].l = reinterpret_cast<jobject>(va_arg(ap, JavaObject**));
	}
	return buffer;
}

#define JavaMethod_INVOKE_VA(TYPE, TYPE_NAME, FUNC_TYPE_VIRTUAL_AP, FUNC_TYPE_STATIC_AP, FUNC_TYPE_VIRTUAL_BUF, FUNC_TYPE_STATIC_BUF) \
	TYPE JavaMethod::invoke##TYPE_NAME##Virtual(Jnjvm* vm, UserClass* cl, JavaObject* obj, ...) {	\
		llvm_gcroot(obj, 0);	\
		va_list ap;	\
		va_start(ap, obj);	\
		TYPE res = invokeVirtualAP<TYPE, FUNC_TYPE_VIRTUAL_BUF>(vm, cl, obj, ap);	\
		va_end(ap);	\
		return res;	\
	}	\
	TYPE JavaMethod::invoke##TYPE_NAME##Special(Jnjvm* vm, UserClass* cl, JavaObject* obj, ...) {	\
	  llvm_gcroot(obj, 0);	\
	  va_list ap;	\
	  va_start(ap, obj);	\
	  TYPE res = invokeSpecialAP<TYPE, FUNC_TYPE_VIRTUAL_BUF>(vm, cl, obj, ap);	\
	  va_end(ap);	\
	  return res;	\
	}	\
	TYPE JavaMethod::invoke##TYPE_NAME##Static(Jnjvm* vm, UserClass* cl, ...) {	\
	  va_list ap;	\
	  va_start(ap, cl);	\
	  TYPE res = invokeStaticAP<TYPE, FUNC_TYPE_STATIC_BUF>(vm, cl, ap);	\
	  va_end(ap);	\
	  return res;	\
	}

#define JavaMethod_INVOKE_AP(TYPE, TYPE_NAME, FUNC_TYPE_VIRTUAL_AP, FUNC_TYPE_STATIC_AP, FUNC_TYPE_VIRTUAL_BUF, FUNC_TYPE_STATIC_BUF)	\
	TYPE JavaMethod::invoke##TYPE_NAME##VirtualAP(Jnjvm* vm, UserClass* cl, JavaObject* obj, va_list ap) {	\
		llvm_gcroot(obj, 0);																				\
		return invokeVirtualAP<TYPE, FUNC_TYPE_VIRTUAL_BUF>(vm, cl, obj, ap);								\
	}																										\
	TYPE JavaMethod::invoke##TYPE_NAME##SpecialAP(Jnjvm* vm, UserClass* cl, JavaObject* obj, va_list ap) {	\
		llvm_gcroot(obj, 0);																				\
		return invokeSpecialAP<TYPE, FUNC_TYPE_VIRTUAL_BUF>(vm, cl, obj, ap);								\
	}																										\
	TYPE JavaMethod::invoke##TYPE_NAME##StaticAP(Jnjvm* vm, UserClass* cl, va_list ap) {					\
		return invokeStaticAP<TYPE, FUNC_TYPE_STATIC_BUF>(vm, cl, ap);										\
	}

#define JavaMethod_INVOKE_BUF(TYPE, TYPE_NAME, FUNC_TYPE_VIRTUAL_AP, FUNC_TYPE_STATIC_AP, FUNC_TYPE_VIRTUAL_BUF, FUNC_TYPE_STATIC_BUF)	\
	TYPE JavaMethod::invoke##TYPE_NAME##VirtualBuf(Jnjvm* vm, UserClass* cl, JavaObject* obj, void* buf) {	\
		llvm_gcroot(obj, 0); 																				\
		return invokeVirtualBuf<TYPE, FUNC_TYPE_VIRTUAL_BUF>(vm, cl, obj, buf); 							\
	} 																										\
	TYPE JavaMethod::invoke##TYPE_NAME##SpecialBuf(Jnjvm* vm, UserClass* cl, JavaObject* obj, void* buf) {	\
		llvm_gcroot(obj, 0); 																				\
		return invokeSpecialBuf<TYPE, FUNC_TYPE_VIRTUAL_BUF>(vm, cl, obj, buf); 							\
	}																										\
	TYPE JavaMethod::invoke##TYPE_NAME##StaticBuf(Jnjvm* vm, UserClass* cl, void* buf) {					\
		return invokeStaticBuf<TYPE, FUNC_TYPE_STATIC_BUF>(vm, cl, buf); 									\
	}

#define JavaMethod_INVOKE(TYPE, TYPE_NAME) \
	typedef TYPE (* func_virtual_ap_##TYPE_NAME)(UserConstantPool*, void*, JavaObject*, va_list);	\
	typedef TYPE (* func_static_ap_##TYPE_NAME)(UserConstantPool*, void*, va_list);					\
	typedef TYPE (* func_virtual_buf_##TYPE_NAME)(UserConstantPool*, void*, JavaObject*, void*);	\
	typedef TYPE (* func_static_buf_##TYPE_NAME)(UserConstantPool*, void*, void*);					\
	JavaMethod_INVOKE_AP(TYPE, TYPE_NAME, func_virtual_ap_##TYPE_NAME, func_static_ap_##TYPE_NAME, func_virtual_buf_##TYPE_NAME, func_static_buf_##TYPE_NAME) \
	JavaMethod_INVOKE_BUF(TYPE, TYPE_NAME, func_virtual_ap_##TYPE_NAME, func_static_ap_##TYPE_NAME, func_virtual_buf_##TYPE_NAME, func_static_buf_##TYPE_NAME) \
	JavaMethod_INVOKE_VA(TYPE, TYPE_NAME, func_virtual_ap_##TYPE_NAME, func_static_ap_##TYPE_NAME, func_virtual_buf_##TYPE_NAME, func_static_buf_##TYPE_NAME)

JavaMethod_INVOKE(uint32, Int)
JavaMethod_INVOKE(sint64, Long)
JavaMethod_INVOKE(float,  Float)
JavaMethod_INVOKE(double, Double)
JavaMethod_INVOKE(JavaObject*, JavaObject)
