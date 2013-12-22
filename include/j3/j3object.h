#ifndef _J3_OBJECT_H_
#define _J3_OBJECT_H_

#include <pthread.h>
#include <stdint.h>

#include "j3/j3typesdef.h"

namespace vmkit {
	class BumpAllocator;
}

namespace j3 {
	class J3Type;
	class J3Class;
	class J3Layout;
	class J3ArrayClass;
	class J3Primitive;
	class J3Field;
	class J3VirtualTable;
	class J3FixedPoint;
	class J3Method;

	// see: Cliff Click and John Rose. 2002. Fast subtype checking in the HotSpot JVM. 
	// In Proceedings of the 2002 joint ACM-ISCOPE conference on Java Grande (JGI '02). ACM, New York, NY, USA, 96-107. 
	class J3TypeChecker {
	public:
		static const uint32_t displayLength = 9;
		static const uint32_t cacheOffset = displayLength - 1;

		J3VirtualTable*  display[displayLength];
		J3VirtualTable** secondaryTypes; 
		uint32_t         nbSecondaryTypes;
		uint32_t         offset;                 /* offset between 1 and 8 if class, cache otherwise */

		void dump();
	};

	class J3VirtualTable {
	public:
		static const uint32_t gepObjectClass = 0;
		static const uint32_t gepVirtualMethods = 3;

	private:
		J3Type*               _type;
		J3TypeChecker         checker;
		size_t                _nbVirtualMethods;
		void*                 _virtualMethods[1];

		J3VirtualTable(J3Type* type, J3Type* super, J3Type** interfaces, uint32_t nbInterfaces, bool isSecondary);
		void* operator new(size_t unused, vmkit::BumpAllocator* allocator, size_t n);
	public:
		static J3VirtualTable* create(J3Layout* cl);
		static J3VirtualTable* create(J3Class* cl);
		static J3VirtualTable* create(J3ArrayClass* cl);
		static J3VirtualTable* create(J3Primitive* prim);

		uint32_t      offset() { return checker.offset; }
		bool          isPrimaryChecker() { return checker.offset < J3TypeChecker::cacheOffset; }

		bool          slowIsAssignableTo(J3VirtualTable* parent);
		bool          fastIsAssignableToPrimaryChecker(J3VirtualTable* parent, uint32_t parentOffset);
		bool          fastIsAssignableToNonPrimaryChecker(J3VirtualTable* parent);
		bool          isAssignableTo(J3VirtualTable* parent);

		J3Type*       type() const { return _type; }
		void**        virtualMethods() const { return (void**)_virtualMethods; }
		size_t        nbVirtualMethods() const { return _nbVirtualMethods; }

		void dump();
	};

	class J3Object {
		friend class J3ArrayObject;
		friend class J3ObjectHandle;
	public:
		static const uint32_t gepVT = 0;

	private:
		J3VirtualTable* _vt;
		uintptr_t       _header;

		J3Object(); /* never directly allocate an object */

		static J3Object* allocate(J3VirtualTable* vt, size_t n);
		static J3Object* doNewNoInit(J3Class* cl);
		static J3Object* doNew(J3Class* cl);
	public:

		J3VirtualTable* vt();
		uintptr_t*      header();
	};

	class J3ArrayObject : public J3Object {
		friend class J3ObjectHandle;
	public:
		static const uint32_t gepLength = 1;
		static const uint32_t gepContent = 2;

	private:
		uint32_t _length;
		static J3Object* doNew(J3ArrayClass* cl, uint32_t length);

	public:

		uint32_t length() { return _length; }
	};

	class J3ObjectHandle {
		friend class J3FixedPoint;
		friend class J3Method;

	public:
		static const uint32_t gepObj = 0;

	private:
		J3Object* volatile _obj;

		J3Object*           obj()   { return _obj; }
		J3ArrayObject*      array() { return (J3ArrayObject*)_obj; }
	public:
		J3VirtualTable*     vt()    { return obj()->vt(); }
		uint32_t            arrayLength() { return array()->length(); }

		static J3ObjectHandle* allocate(J3VirtualTable* vt, size_t n);
		static J3ObjectHandle* doNewObject(J3Class* cl);
		static J3ObjectHandle* doNewArray(J3ArrayClass* cl, uint32_t length);

		static void*           trampoline(J3Object* obj, J3Method* ref);

		void            harakiri() { _obj = 0; }

		uint32_t        hashCode();

		void            rawSetObject(uint32_t offset, J3ObjectHandle* v);
		J3ObjectHandle* rawGetObject(uint32_t offset);
		void            setObject(J3Field* f, J3ObjectHandle* v);
		J3ObjectHandle* getObject(J3Field* f);
		void            setObjectAt(uint32_t idx, J3ObjectHandle* v);
		J3ObjectHandle* getObjectAt(uint32_t idx);

#define defAccessor(name, ctype, llvmtype)						\
		void  rawSet##name(uint32_t offset, ctype value); \
		ctype rawGet##name(uint32_t offset);							\
		void  set##name(J3Field* f, ctype value);					\
		ctype get##name(J3Field* f);											\
		void  set##name##At(uint32_t idx, ctype value);		\
		ctype get##name##At(uint32_t idx);
		onJavaPrimitives(defAccessor);
#undef defAccessor
	};

	class J3FixedPointNode {
	public:
		J3FixedPointNode* nextFree;
		J3FixedPointNode* nextBusy;
		J3ObjectHandle*   top;
		J3ObjectHandle*   max;
	};

	class J3FixedPoint {
		static const uint32_t defaultNodeCapacity = 256;

		pthread_mutex_t       mutex;
		vmkit::BumpAllocator* allocator;
		J3FixedPointNode*     head;

		void createNode(uint32_t capacity=defaultNodeCapacity);
	public:
		J3FixedPoint(vmkit::BumpAllocator* _allocator);

		void            unsyncEnsureCapacity(uint32_t capacity);

		J3ObjectHandle* syncPush(J3ObjectHandle* handle) { return syncPush(handle->obj()); }
		J3ObjectHandle* syncPush(J3Object* obj);
		J3ObjectHandle* unsyncPush(J3ObjectHandle* handle) { return unsyncPush(handle->obj()); }
		J3ObjectHandle* unsyncPush(J3Object* obj);
		void            unsyncPop();

		J3ObjectHandle* unsyncTell() { return head->top; }
		void            unsyncRestore(J3ObjectHandle* ptr);
	};

	class J3Value {
	public:
		union {
#define doIt(name, ctype, llvmtype) \
			ctype val##name;
			onJavaFields(doIt);
#undef doIt
		};

#define doIt(name, ctype, llvmtype) \
		J3Value(ctype val) { val##name = val; }
		onJavaFields(doIt);
#undef doIt

		J3Value() {}
	};
}

#endif
