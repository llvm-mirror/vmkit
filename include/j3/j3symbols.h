#ifndef _J3_SYMBOLS_H_
#define _J3_SYMBOLS_H_

namespace j3 {
	class J3VTSymbol : public vmkit::Symbol {
		const char*     _id;
		J3VirtualTable* _vt;
	public:
		J3VTSymbol() {}

		J3VirtualTable* vt() { return _vt; }
		const char*     id() { return _id; }
		
		void*           getSymbolAddress() { return vt(); }
		
		void            setId(const char* id) { _id = id; }
		void            setVt(J3VirtualTable* vt) { _vt = vt; }
	};

	class J3StaticObjectSymbol : public vmkit::Symbol {
		const char*     _id;
		J3ObjectHandle* _handle;
	public:
		J3StaticObjectSymbol(J3ObjectHandle* handle) { _handle = handle; }

		void*           operator new(size_t n, vmkit::LockedStack<J3StaticObjectSymbol>* stack) { return stack->push(); }

		J3ObjectHandle* handle() { return _handle; }
		const char*     id() { return _id; }

		void*           getSymbolAddress() { return handle(); }
		
		void            setId(const char* id) { _id = id; }
		void            setHandle(J3ObjectHandle* handle) { *_handle = *handle; }
	};

	/* only symbol accessed with a load because unification implies that the string can not be relocated if it is already defined */
	class J3StringSymbol : public vmkit::Symbol {
		J3ObjectHandle*  _handle;
		char*            _id;
	public:
		J3StringSymbol(char* id, J3ObjectHandle* handle) { _id = id; _handle = handle; }

		void* operator new(size_t n, vmkit::BumpAllocator* allocator) { return allocator->allocate(n); }

		char* id() { return _id; }
		J3ObjectHandle* handle() { return _handle; }

		void* getSymbolAddress() { return &_handle; }
	};

}

#endif
