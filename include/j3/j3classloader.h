#ifndef _CLASSLOADER_H_
#define _CLASSLOADER_H_

#include <map>
#include <vector>

#include "llvm/ExecutionEngine/SectionMemoryManager.h"

#include "vmkit/allocator.h"
#include "vmkit/names.h"

#include "j3/j3object.h"

namespace j3 {
	class J3ZipArchive;
	class J3ClassBytes;
	class J3MethodType;
	class J3Method;
	class J3Type;
	class J3;
	class J3Class;
	class J3Symbol;

	class J3ClassLoader : public llvm::SectionMemoryManager {
		struct J3MethodLess : public std::binary_function<wchar_t*,wchar_t*,bool> {
			bool operator()(const J3Method* lhs, const J3Method* rhs) const;
		};

		typedef std::map<J3Method*, J3Method*, J3MethodLess,
										 vmkit::StdAllocator<std::pair<J3Method*, J3Method*> > > MethodRefMap;

		typedef std::map<const char*, J3Symbol*, vmkit::Util::char_less_t,
										 vmkit::StdAllocator<std::pair<const char*, J3Symbol*> > > SymbolMap;

		static J3MethodLess  j3MethodLess;

		SymbolMap                            _symbolTable;
		pthread_mutex_t                      _mutexSymbolTable;

		J3ObjectHandle*                      _javaClassLoader;
		J3FixedPoint                         _fixedPoint;
		pthread_mutex_t                      _mutex;       /* a lock */
		vmkit::BumpAllocator*                _allocator;   /* the local allocator */
		J3*                                  _vm;          /* my vm */
		vmkit::NameMap<J3Class*>::map        classes;      /* classes managed by this class loader */
		vmkit::NameMap<J3Type*>::map         types;        /* shortcut to find types */
		vmkit::NameMap<J3MethodType*>::map   methodTypes;  /* shortcut to find method types - REMOVE */
		MethodRefMap                         methods;      /* all te known method */

		void                          wrongType(J3Class* from, const vmkit::Name* type);
		J3Type*                       getTypeInternal(J3Class* from, const vmkit::Name* type, uint32_t start, uint32_t* end);


		uint64_t                      getSymbolAddress(const std::string &Name);

	protected:
		std::vector<void*, vmkit::StdAllocator<void*> > nativeLibraries;

	public:
		void* operator new(size_t n, vmkit::BumpAllocator* allocator);
		J3ClassLoader(J3* vm, J3ObjectHandle* javaClassLoader, vmkit::BumpAllocator* allocator);

		void                          addSymbol(const char* id, J3Symbol* symbol);

		static void                   destroy(J3ClassLoader* loader);

		J3FixedPoint*                 fixedPoint() { return &_fixedPoint; }

		J3ObjectHandle*               javaClassLoader() { return _javaClassLoader; }

		void                          lock() { pthread_mutex_lock(&_mutex); }
		void                          unlock() { pthread_mutex_unlock(&_mutex); }

		vmkit::BumpAllocator*         allocator() { return _allocator; }
		J3*                           vm() const { return _vm; };

		J3Method*                     method(uint16_t access, J3Class* cl, 
																				 const vmkit::Name* name, const vmkit::Name* sign); /* find a method ref */
		J3Method*                     method(uint16_t access, const vmkit::Name* clName, 
																				 const vmkit::Name* name, const vmkit::Name* sign); 
		J3Method*                     method(uint16_t access, const wchar_t* clName, const wchar_t* name, const wchar_t* sign); 

		J3Class*                      getClass(const vmkit::Name* name);                     /* find a class */
		J3Type*                       getType(J3Class* from, const vmkit::Name* type);       /* find a type */
		J3MethodType*                 getMethodType(J3Class* from, const vmkit::Name* sign); /* get a method type */

		virtual J3ClassBytes*         lookup(const vmkit::Name* name);

		void*                         lookupNativeFunctionPointer(J3Method* method, const char* symb);
	};

	struct char_ptr_less : public std::binary_function<char*,char*,bool> {
    bool operator()(const char* lhs, const char* rhs) const;
	};

	class J3InitialClassLoader : public J3ClassLoader {
		J3ZipArchive*                                     archive;
		std::map<const char*, const char*, char_ptr_less> _cmangled;
	public:
		J3InitialClassLoader(J3* vm, const char* rtjar, vmkit::BumpAllocator* allocator);

		J3ClassBytes* lookup(const vmkit::Name* name);

		const char* cmangled(const char* demangled) { return _cmangled[demangled]; }

		void makeLLVMFunctions_j3();
		void registerCMangling(const char* mangled, const char* demangled);
	};
}

#endif
