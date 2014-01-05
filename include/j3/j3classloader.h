#ifndef _CLASSLOADER_H_
#define _CLASSLOADER_H_

#include <map>
#include <vector>

#include "vmkit/allocator.h"
#include "vmkit/names.h"
#include "vmkit/compiler.h"

#include "j3/j3object.h"

namespace vmkit {
	class Symbol;
}

namespace j3 {
	class J3ZipArchive;
	class J3ClassBytes;
	class J3Signature;
	class J3Method;
	class J3Type;
	class J3;
	class J3ObjectType;
	class J3Class;

	class J3ClassLoader : public vmkit::CompilationUnit {
		struct J3InterfaceMethodLess {
			bool operator()(const J3Method* lhs, const J3Method* rhs) const;
		};

		typedef std::map<J3Method*, uint32_t, J3InterfaceMethodLess,
										 vmkit::StdAllocator<std::pair<J3Method*, J3Method*> > > InterfaceMethodRefMap;

		static J3InterfaceMethodLess  j3InterfaceMethodLess;

		J3ObjectHandle*                      _javaClassLoader;
		J3GlobalReferences                   _globalReferences;

		pthread_mutex_t                      _mutexClasses;
		vmkit::NameMap<J3Class*>::map        classes;      /* classes managed by this class loader */

		pthread_mutex_t                      _mutexTypes;
		vmkit::NameMap<J3Type*>::map         types;        /* shortcut to find types */

		pthread_mutex_t                      _mutexInterfaces;
		InterfaceMethodRefMap                interfaces; 

		pthread_mutex_t                      _mutexMethodTypes;
		vmkit::NameMap<J3Signature*>::map   methodTypes;

	protected:
		std::vector<void*, vmkit::StdAllocator<void*> > nativeLibraries;

	public:
		J3ClassLoader(J3ObjectHandle* javaClassLoader, vmkit::BumpAllocator* allocator);

		J3Type*                       getTypeInternal(J3ObjectType* from, const vmkit::Name* type, uint32_t start, uint32_t* end, bool unify);
		J3Type*                       getType(J3ObjectType* from, const vmkit::Name* type);       /* find a type */
		J3Signature*                  getSignature(J3ObjectType* from, const vmkit::Name* signature); /* get a method type */
		void                          wrongType(J3ObjectType* from, const vmkit::Name* type);

		uint32_t                      interfaceIndex(J3Method* signature);

		J3GlobalReferences*           globalReferences() { return &_globalReferences; }
		
		J3ObjectHandle*               javaClassLoader(bool doPush=1);

		J3Class*                      defineClass(const vmkit::Name* name, J3ClassBytes* bytes);
		J3Class*                      findLoadedClass(const vmkit::Name* name);
		virtual J3Class*              loadClass(const vmkit::Name* name);

		void*                         lookupNativeFunctionPointer(J3Method* method, const char* symb);
	};

	struct char_ptr_less : public std::binary_function<char*,char*,bool> {
    bool operator()(const char* lhs, const char* rhs) const;
	};

	class J3InitialClassLoader : public J3ClassLoader {
		J3ZipArchive*                                     archive;
		std::map<const char*, const char*, char_ptr_less> _cmangled;
	public:
		J3InitialClassLoader(vmkit::BumpAllocator* allocator);

		J3Class*    loadClass(const vmkit::Name* name);
		const char* cmangled(const char* demangled) { return _cmangled[demangled]; }
		void        registerCMangling(const char* mangled, const char* demangled);
	};
}

#endif
