#ifndef _J3_OBJECT_H_
#define _J3_OBJECT_H_

#include <pthread.h>
#include <stdint.h>

#include "vmkit/stack.h"

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
	class J3Monitor;

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
		friend class J3Trampoline;

	public:
		static const uint32_t nbInterfaceMethodTable = 41;
		static const uint32_t gepObjectClass = 0;
		static const uint32_t gepInterfaceMethods = 2;
		static const uint32_t gepVirtualMethods = 4;

	private:
		J3Type*               _type;
		J3TypeChecker         _checker;
		// see: Bowen Alpern, Anthony Cocchi, Stephen Fink, and David Grove. 2001. 
		// Efficient implementation of Java interfaces: Invokeinterface considered harmless. OOPSLA 2001.
		void*                 _interfaceMethodTable[nbInterfaceMethodTable];
		size_t                _nbVirtualMethods;
		void*                 _virtualMethods[1];

		J3VirtualTable(J3Type* type, J3Type* super, J3Type** interfaces, uint32_t nbInterfaces, bool isSecondary);
		void* operator new(size_t unused, vmkit::BumpAllocator* allocator, size_t n);
	public:
		static J3VirtualTable* create(J3Layout* cl);
		static J3VirtualTable* create(J3Class* cl);
		static J3VirtualTable* create(J3ArrayClass* cl);
		static J3VirtualTable* create(J3Primitive* prim);

		J3TypeChecker* checker() { return &_checker; }

		uint32_t      offset() { return _checker.offset; }
		bool          isPrimaryChecker() { return _checker.offset < J3TypeChecker::cacheOffset; }

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
		static const uint32_t gepHeader = 1;

	private:
		J3VirtualTable*     _vt;
		volatile uintptr_t  _header;
		/* 
		 *     biasable (not yet implemented):  0         | epoch | age        | 101
		 *     biased (not yet implemented):    thread_id | epoch | age        | 101
		 *     not locked:                      hash-code 24 bits | age 5 bits | 001
		 *     stack locked:                        pointer to lock record      | 00
		 *     inflated:                            pointer to monitor          | 10
		 */

		J3Object(); /* never directly allocate an object */

		J3Monitor* monitor();
		uint32_t   hashCode();

		static void monitorEnter(J3Object* obj);
		static void monitorExit(J3Object* obj);

		static J3Object* allocate(J3VirtualTable* vt, size_t n);
		static J3Object* doNewNoInit(J3Class* cl);
		static J3Object* doNew(J3Class* cl);
	public:

		J3VirtualTable*     vt();
		volatile uintptr_t* header();
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
		void*    content() { return this+1; }
	};

	class J3ObjectHandle {
		friend class J3LocalReferences;
		friend class J3GlobalReferences;
		friend class J3Method;

	public:
		static const uint32_t gepObj = 0;

	private:
		J3Object* volatile _obj;

	public:
		J3Object*           obj()   { return _obj; }
		J3ArrayObject*      array() { return (J3ArrayObject*)_obj; }
	public:
		J3VirtualTable*     vt()    { return obj()->vt(); }
		uint32_t            arrayLength() { return array()->length(); }

		J3ObjectHandle& operator=(const J3ObjectHandle& h) { _obj = h._obj; return *this; }

		static J3ObjectHandle* allocate(J3VirtualTable* vt, size_t n);
		static J3ObjectHandle* doNewObject(J3Class* cl);
		static J3ObjectHandle* doNewArray(J3ArrayClass* cl, uint32_t length);

		void            wait();

		bool            isSame(J3ObjectHandle* handle) { return obj() == handle->obj(); }

		void            harakiri() { _obj = 0; }

		uint32_t        hashCode();

		void            rawArrayCopyTo(uint32_t fromOffset, J3ObjectHandle* to, uint32_t toOffset, uint32_t nbb); 

		void            rawSetObject(uint32_t offset, J3ObjectHandle* v);
		J3ObjectHandle* rawGetObject(uint32_t offset);
		void            setObject(J3Field* f, J3ObjectHandle* v);
		J3ObjectHandle* getObject(J3Field* f);
		void            setObjectAt(uint32_t idx, J3ObjectHandle* v);
		J3ObjectHandle* getObjectAt(uint32_t idx);

#define defAccessor(name, ctype, llvmtype)															\
		void  rawSet##name(uint32_t offset, ctype value);										\
		ctype rawGet##name(uint32_t offset);																\
		void  set##name(J3Field* f, ctype value);														\
		ctype get##name(J3Field* f);																				\
		void  set##name##At(uint32_t idx, ctype value);											\
		ctype get##name##At(uint32_t idx);																	\
		void  setRegion##name(uint32_t selfIdx, const ctype* buf, uint32_t bufIdx, uint32_t len); \
		void  getRegion##name(uint32_t selfIdx, const ctype* buf, uint32_t bufIdx, uint32_t len);
		onJavaPrimitives(defAccessor);
#undef defAccessor
	};

	class J3LocalReferences : public vmkit::Stack<J3ObjectHandle> {
	public:
		J3LocalReferences(vmkit::BumpAllocator* _allocator) : vmkit::Stack<J3ObjectHandle>(_allocator) {}

		J3ObjectHandle* push(J3ObjectHandle* handle) { return push(handle->obj()); }
		J3ObjectHandle* push(J3Object* obj);
	};

	class J3GlobalReferences {
		pthread_mutex_t               mutex;
		vmkit::Stack<J3ObjectHandle>  references;
		vmkit::Stack<J3ObjectHandle*> emptySlots;
	public:
		J3GlobalReferences(vmkit::BumpAllocator* _allocator);
		
		J3ObjectHandle* add(J3ObjectHandle* handle);
		void            del(J3ObjectHandle* handle);
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
