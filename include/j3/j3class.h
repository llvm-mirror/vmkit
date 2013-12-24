#ifndef _J3_CLASS_H_
#define _J3_CLASS_H_

#include <pthread.h>
#include <stdint.h>
#include <vector>

#include "vmkit/compiler.h"

#include "j3/j3object.h"

namespace llvm {
	class Type;
	class GlobalValue;
	class Module;
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
	class J3ArrayClass;
	class J3ObjectType;
	class J3Method;
	class J3Field;

	class J3Type : public vmkit::Symbol {
		pthread_mutex_t        _mutex;
		J3ClassLoader*         _loader;
		J3ArrayClass* volatile _array;

	protected:
		enum { CITED, LOADED, RESOLVED, INITED };

		const vmkit::Name*     _name;
		char*                  _nativeName;
		uint32_t               _nativeNameLength;
		J3VirtualTable*        _vt;

		volatile int           status;

		virtual void                doResolve(J3Field* hiddenFields, size_t nbHiddenFields) { status = RESOLVED; }
		virtual void                doInitialise() { status = INITED; }

	public:
		J3Type(J3ClassLoader* loader, const vmkit::Name* name);

		void*                       getSymbolAddress();

		virtual llvm::GlobalValue*  llvmDescriptor(llvm::Module* module) { return 0; }

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

		virtual bool                isObjectType() { return 0; }
		virtual bool                isArrayClass() { return 0; }
		virtual bool                isLayout() { return 0; }
		virtual bool                isClass() { return 0; }
		virtual bool                isPrimitive() { return 0; }
		virtual llvm::Type*         llvmType() = 0;

		void dump();
	};

	class J3Attribute : public vmkit::PermanentObject {
		friend class J3Class;

		const vmkit::Name* _id;
		uint32_t           _offset;
	public:
		
		const vmkit::Name* id() { return _id; }
		uint32_t           offset() { return _offset; }
	};

	class J3Attributes : public vmkit::PermanentObject {
		size_t       _nbAttributes;
		J3Attribute  _attributes[1];
	public:
		J3Attributes(size_t n) { _nbAttributes = n; }

		void* operator new(size_t unused, vmkit::BumpAllocator* allocator, size_t n) {
			return vmkit::PermanentObject::operator new(sizeof(J3Attributes) + (n - 1) * sizeof(J3Attribute), allocator);
		}

		size_t       nbAttributes() { return _nbAttributes; }
		J3Attribute* attribute(size_t n);
		
		J3Attribute* lookup(const vmkit::Name* attr);
	};

	class J3Field : public vmkit::PermanentObject {
		friend class J3Class;

		J3Class*           _cl;
		uint16_t           _access;
		const vmkit::Name* _name;
		J3Type*            _type;
		J3Attributes*      _attributes;
		uint32_t           _num;

	public:
		J3Field() {}
		J3Field(uint16_t access, const vmkit::Name* name, J3Type* type) { _access = access; _name = name; _type = type; }

		J3Attributes*      attributes() const { return _attributes; }
		uint16_t           access() { return _access; }
		J3Class*           cl()  { return _cl; }
		const vmkit::Name* name() { return _name; }
		J3Type*            type() { return _type; }

		uint32_t           num() { return _num; }

		void               dump();
	};

	class J3InterfaceSlotDescriptor {
	public:
		uint32_t   nbMethods;
		J3Method** methods;
	};

	class J3ObjectType : public J3Type {
		J3ObjectHandle*           _javaClass;
		J3InterfaceSlotDescriptor _interfaceSlotDescriptors[J3VirtualTable::nbInterfaceMethodTable];

	public:
		J3ObjectType(J3ClassLoader* loader, const vmkit::Name* name);

		J3InterfaceSlotDescriptor* slotDescriptorAt(uint32_t index) { return &_interfaceSlotDescriptors[index]; }
		void                       prepareInterfaceTable();

		virtual J3Method*          findVirtualMethod(const vmkit::Name* name, const vmkit::Name* sign, bool error=1);
		virtual J3Method*          findStaticMethod(const vmkit::Name* name, const vmkit::Name* sign, bool error=1);

		bool                       isObjectType() { return 1; }

		J3ObjectHandle*            javaClass();

		static J3ObjectType*       nativeClass(J3ObjectHandle* handle);

		void                       dumpInterfaceSlotDescriptors();
	};

	class J3Layout : public J3ObjectType {
		friend class J3Class;

		llvm::Type*       _llvmType;

		size_t            nbFields;
		J3Field*          fields;
		
		size_t            _nbMethods;
		J3Method**        _methods;
	public:
		J3Layout(J3ClassLoader* loader, const vmkit::Name* name);

		virtual bool      isLayout() { return 1; }

		size_t            nbMethods() { return _nbMethods; }
		J3Method**        methods() { return _methods; }

		J3Method*         findMethod(const vmkit::Name* name, const vmkit::Name* sign);
		J3Field*          findField(const vmkit::Name* name, const J3Type* type);
		llvm::Type*       llvmType() { return _llvmType; }
	};

	class J3Class : public J3Layout {
		J3Layout           staticLayout;

		uint16_t           _access;

		size_t             _size;

		size_t             _nbInterfaces;
		J3Class**          _interfaces;
		J3Class*           _super;       /* this for root */

		J3Attributes*      _attributes;

		J3ClassBytes*      _bytes;
		size_t             nbCtp;
		uint8_t*           ctpTypes;
		uint32_t*          ctpValues;
		void**             ctpResolved;

		/* GC Object */
		J3ObjectHandle*    _staticInstance;

		J3Attributes* readAttributes(J3Reader* reader);
		void          readClassBytes(std::vector<llvm::Type*>* virtualBody, J3Field* hiddenFields, uint32_t nbHiddenFields);

		void          check(uint16_t idx, uint32_t id=-1);

		void          fillFields(std::vector<llvm::Type*>* staticBody, std::vector<llvm::Type*>* virtualBody, J3Field** fields, size_t n);

		void          createLLVMTypes();

		void          load();
		void          doResolve(J3Field* hiddenFields, size_t nbHiddenFields);
		void          doInitialise();

		J3Method*     interfaceOrMethodAt(uint16_t idx, uint16_t access);
	public:
		J3Class(J3ClassLoader* loader, const vmkit::Name* name);

		size_t              nbInterfaces() { return _nbInterfaces; }
		J3Class**           interfaces() { return _interfaces; }
		J3Class*            super() { return _super; }
		uint16_t            access() { return _access; }

		void                adjustSize(uint32_t n) { _size += n; }
		size_t              size();
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

		llvm::Type*         llvmType();
		llvm::Type*         staticLLVMType();
		llvm::Type*         virtualLLVMType();
		llvm::GlobalValue*  llvmDescriptor(llvm::Module* module);

		J3ClassBytes*       bytes() { return _bytes; }

		bool                isClass() { return 1; }

		J3Method*           findVirtualMethod(const vmkit::Name* name, const vmkit::Name* sign, bool error=1);
		J3Method*           findStaticMethod(const vmkit::Name* name, const vmkit::Name* sign, bool error=1);
		J3Field*            findVirtualField(const vmkit::Name* name, const J3Type* type, bool error=1);
		J3Field*            findStaticField(const vmkit::Name* name, const J3Type* type, bool error=1);
	};

	class J3ArrayClass : public J3ObjectType {
		llvm::Type*         _llvmType;
		J3Type*             _component;

		void                doResolve(J3Field* hiddenFields, size_t nbHiddenFields);
		void                doInitialise();
	public:
		J3ArrayClass(J3ClassLoader* loader, J3Type* component, const vmkit::Name* name);


		llvm::GlobalValue*  llvmDescriptor(llvm::Module* module);

		J3Type*             component() { return _component; }
		bool                isArrayClass() { return 1; }
		llvm::Type*         llvmType();
	};

	class J3Primitive : public J3Type {
		llvm::Type*  _llvmType;

	public:
		J3Primitive(J3ClassLoader* loader, char id, llvm::Type* type);

		bool        isPrimitive() { return 1; }
		llvm::Type* llvmType() { return _llvmType; }
	};
}

#endif
