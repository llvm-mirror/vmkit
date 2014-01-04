#ifndef _J3_CLASS_H_
#define _J3_CLASS_H_

#include <pthread.h>
#include <stdint.h>
#include <vector>

#include "vmkit/compiler.h"

#include "j3/j3object.h"

namespace llvm {
	class Type;
}

namespace vmkit {
	class Name;
}

namespace j3 {
	class J3ClassLoader;
	class J3ClassBytes;
	class J3Reader;
	class J3Primitive;
	class J3Class;
	class J3Layout;
	class J3StaticLayout;
	class J3ArrayClass;
	class J3ObjectType;
	class J3Method;
	class J3Field;
	class J3Attributes;
	class J3Attribute;
	class J3Signature;

	class J3InterfaceSlotDescriptor {
	public:
		uint32_t   nbMethods;
		J3Method** methods;
	};


	class J3Type : public vmkit::Symbol {
		pthread_mutex_t        _mutex;
		J3ClassLoader*         _loader;
		J3ArrayClass* volatile _array;

	protected:
		enum { LOADED, RESOLVED, INITED };

		const vmkit::Name*        _name;
		char*                     _nativeName;
		uint32_t                  _nativeNameLength;
		J3VirtualTable*           _vt; 
		J3ObjectHandle* volatile  _javaClass;

		volatile int           status;

		virtual void                doResolve(J3Field* hiddenFields, size_t nbHiddenFields) { status = RESOLVED; }
		virtual void                doInitialise() { status = INITED; }

		virtual void                doNativeName();
	public:
		J3Type(J3ClassLoader* loader, const vmkit::Name* name);

		J3ObjectHandle*             javaClass();

		virtual uint32_t            logSize() = 0;
		uint64_t                    getSizeInBits();

		void*                       getSymbolAddress();

		int                         isTypeDescriptor() { return 1; }

		bool                        isResolved() { return status >= RESOLVED; }
		bool                        isInitialised() { return status == INITED; }
		J3Type*                     resolve(J3Field* hiddenFields, uint32_t nbHiddenFields);
		J3Type*                     resolve();
		J3Type*                     initialise();

		J3VirtualTable*             vt();
		J3VirtualTable*             vtAndResolve();

		bool                        isAssignableTo(J3Type* parent);

		char*                       nativeName();
		uint32_t                    nativeNameLength();

		void                        lock() { pthread_mutex_lock(&_mutex); }
		void                        unlock() { pthread_mutex_unlock(&_mutex); }

		const vmkit::Name*          name() const { return _name; }
		J3ArrayClass*               getArray(uint32_t prof=1, const vmkit::Name* name=0);

		J3ClassLoader*              loader() { return _loader; }

		J3ObjectType*               asObjectType();
		J3Class*                    asClass();
		J3Primitive*                asPrimitive();
		J3ArrayClass*               asArrayClass();
		J3Layout*                   asLayout();
		J3StaticLayout*             asStaticLayout();

		virtual bool                isObjectType() { return 0; }
		virtual bool                isArrayClass() { return 0; }
		virtual bool                isStaticLayout() { return 0; }
		virtual bool                isLayout() { return 0; }
		virtual bool                isClass() { return 0; }
		virtual bool                isPrimitive() { return 0; }

		virtual llvm::Type*         llvmType() = 0;

		void dump();
	};

	class J3ObjectType : public J3Type {
		J3InterfaceSlotDescriptor _interfaceSlotDescriptors[J3VirtualTable::nbInterfaceMethodTable];

	public:
		J3ObjectType(J3ClassLoader* loader, const vmkit::Name* name);

		uint32_t                   logSize() { return sizeof(uintptr_t) == 8 ? 3 : 2; }
		llvm::Type*                llvmType();

		J3InterfaceSlotDescriptor* slotDescriptorAt(uint32_t index) { return &_interfaceSlotDescriptors[index]; }
		void                       prepareInterfaceTable();

		virtual J3Method*          findMethod(uint32_t access, const vmkit::Name* name, J3Signature* signature, bool error=1);

		bool                       isObjectType() { return 1; }

		static J3ObjectType*       nativeClass(J3ObjectHandle* handle);

		void                       dumpInterfaceSlotDescriptors();

		virtual J3ObjectHandle*    clone(J3ObjectHandle* obj);
	};

	class J3Layout : public J3ObjectType {
		friend class J3Class;

		size_t            _nbFields;
		size_t            _nbPublicFields;
		J3Field*          _fields;
		
		size_t            _nbMethods;
		size_t            _nbPublicMethods;
		J3Method**        _methods;

		uintptr_t         _structSize;
	public:
		J3Layout(J3ClassLoader* loader, const vmkit::Name* name);

		virtual bool      isLayout() { return 1; }

		uintptr_t         structSize();

		size_t            nbFields() { return _nbFields; }
		size_t            nbPublicFields() { return _nbPublicFields; }
		J3Field*          fields() { return _fields; }

		size_t            nbMethods() { return _nbMethods; }
		size_t            nbPublicMethods() { return _nbPublicMethods; }
		J3Method**        methods() { return _methods; }

		J3Method*         localFindMethod(const vmkit::Name* name, J3Signature* signature);
		J3Field*          localFindField(const vmkit::Name* name, const J3Type* type);

		virtual J3ObjectHandle* extractAttribute(J3Attribute* attr) = 0;
	};

	class J3StaticLayout : public J3Layout {
		J3Class* _cl;
	public:
		J3StaticLayout(J3ClassLoader* loader, J3Class* cl, const vmkit::Name* name);

		J3Class* cl() { return _cl; }

		virtual bool      isStaticLayout() { return 1; }
		J3ObjectHandle*   extractAttribute(J3Attribute* attr);
	};

	class J3Class : public J3Layout {
		J3StaticLayout     _staticLayout;

		uint16_t           _access;

		size_t             _nbInterfaces;
		J3Class**          _interfaces;
		J3Class*           _super;       /* this for root */

		J3Attributes*      _attributes;

		J3ClassBytes*      _bytes;
		size_t             nbCtp;
		uint8_t*           ctpTypes;
		uint32_t*          ctpValues;
		void**             ctpResolved;

		size_t             _nbConstructors;
		size_t             _nbPublicConstructors;

		/* GC Object */
		J3ObjectHandle*    _staticInstance;

		J3Attributes* readAttributes(J3Reader* reader);
		void          readClassBytes(J3Field* hiddenFields, uint32_t nbHiddenFields);

		void          check(uint16_t idx, uint32_t id=-1);

		void          fillFields(J3Field** fields, size_t n);

		void          doNativeName();

		void          doResolve(J3Field* hiddenFields, size_t nbHiddenFields);
		void          doInitialise();

		J3Method*     interfaceOrMethodAt(uint16_t idx, uint16_t access);
	public:
		J3Class(J3ClassLoader* loader, const vmkit::Name* name, J3ClassBytes* bytes);

		J3ObjectHandle*     clone(J3ObjectHandle* obj);

		size_t              nbConstructors() { return _nbConstructors; }
		size_t              nbPublicConstructors() { return _nbPublicConstructors; }

		J3ObjectHandle*     extractAttribute(J3Attribute* attr);

		J3StaticLayout*     staticLayout() { return &_staticLayout; }

		size_t              nbInterfaces() { return _nbInterfaces; }
		J3Class**           interfaces() { return _interfaces; }
		J3Class*            super() { return _super; }
		uint16_t            access() { return _access; }

		J3ObjectHandle*     staticInstance();

		void                registerNative(const vmkit::Name* methName, const vmkit::Name* methSign, void* fnPtr);

		uint8_t             getCtpType(uint16_t idx);
		void*               getCtpResolved(uint16_t idx);

		const vmkit::Name*  nameAt(uint16_t idx);
		float               floatAt(uint16_t idx);
		double              doubleAt(uint16_t idx);
		uint32_t            integerAt(uint16_t idx);
		uint64_t            longAt(uint16_t idx);
		J3ObjectHandle*     stringAt(uint16_t idx);
		J3ObjectType*       classAt(uint16_t idx);
		J3Method*           interfaceMethodAt(uint16_t idx, uint16_t access);
		J3Method*           methodAt(uint16_t idx, uint16_t access);
		J3Field*            fieldAt(uint16_t idx, uint16_t access);

		J3ClassBytes*       bytes() { return _bytes; }

		bool                isClass() { return 1; }

		J3Method*           findMethod(uint32_t access, const vmkit::Name* name, J3Signature* signature, bool error=1);
		J3Field*            findField(uint32_t access, const vmkit::Name* name, J3Type* type, bool error=1);
	};

	class J3ArrayClass : public J3ObjectType {
		J3Type*             _component;

		void                doResolve(J3Field* hiddenFields, size_t nbHiddenFields);
		void                doInitialise();

		void                doNativeName();
	public:
		J3ArrayClass(J3ClassLoader* loader, J3Type* component, const vmkit::Name* name);

		J3ObjectHandle*     clone(J3ObjectHandle* obj);

		J3Type*             component() { return _component; }
		bool                isArrayClass() { return 1; }

		J3Method*           findMethod(uint32_t access, const vmkit::Name* name, J3Signature* signature, bool error=1);
	};

	class J3Primitive : public J3Type {
		uint32_t     _logSize;
		llvm::Type*  _llvmType;

	public:
		J3Primitive(J3ClassLoader* loader, char id, llvm::Type* type, uint32_t logSize);

		uint32_t    logSize()     { return _logSize; }
		bool        isPrimitive() { return 1; }
		llvm::Type* llvmType()    { return _llvmType; }
	};
}

#endif
