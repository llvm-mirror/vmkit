#ifndef _J3_SYMBOLS_H_
#define _J3_SYMBOLS_H_

namespace j3 {
	class J3StringSymbol : public vmkit::Symbol {
		char*           _id;
		J3ObjectHandle* _handle;
	public:
		J3StringSymbol(char* id, J3ObjectHandle* handle) { _id = id; _handle = handle; }

		char* id() { return _id; }
		void* getSymbolAddress() { return _handle; }
	};

	class J3VTSymbol : public vmkit::Symbol {
		char*           _id;
		J3VirtualTable* _vt;
	public:
		J3VTSymbol() {}

		J3VirtualTable* vt() { return _vt; }
		
		char* id() { return _id; }
		void* getSymbolAddress() { return vt(); }
		
		void  setId(char* id) { _id = id; }
		void  setVt(J3VirtualTable* vt) { _vt = vt; }
	};

	class J3StaticObjectSymbol : public vmkit::Symbol {
		char* volatile _id;
		J3ObjectHandle _handle;
	public:
		J3StaticObjectSymbol() {}

		void* operator new(size_t n, vmkit::LockedStack<J3StaticObjectSymbol>* stack) { return stack->push(); }

		J3ObjectHandle* handle() { return &_handle; }

		char* id() { return _id; }
		void* getSymbolAddress() { return handle(); }
		
		void  setId(char* id) { _id = id; }
		void  setHandle(J3ObjectHandle* handle) { _handle = *handle; }
	};
}

#endif
