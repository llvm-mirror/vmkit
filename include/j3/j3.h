#ifndef _J3_J3_
#define _J3_J3_

#include <pthread.h>
#include <vector>

#include "vmkit/names.h"
#include "vmkit/vmkit.h"
#include "vmkit/allocator.h"

#include "j3/j3options.h"
#include "j3/j3typesdef.h"
#include "j3/j3jni.h"


namespace j3 {
	class J3InitialClassLoader;
	class J3Class;
	class J3ArrayClass;
	class J3Type;
	class J3Object;
	class J3ObjectHandle;
	class J3Primitive;
	class J3Lib;
	class J3Method;

	class J3 : public vmkit::VMKit {
		typedef std::map<J3ObjectHandle*, J3ObjectHandle*, vmkit::T_ptr_less_t<J3ObjectHandle*>, 
										 vmkit::StdAllocator<std::pair<J3ObjectHandle*, J3ObjectHandle*> > > StringMap;

		static vmkit::T_ptr_less_t<J3ObjectHandle*> charArrayLess;

		J3Options                          _options;

		pthread_mutex_t                      stringsMutex;
		vmkit::NameMap<J3ObjectHandle*>::map nameToCharArrays;
		StringMap                            charArrayToStrings;
		vmkit::Names                         _names;

		void                       introspect();

		J3(vmkit::BumpAllocator* allocator);
	public:
		J3InitialClassLoader*               initialClassLoader;

		static J3*  create();

#define defPrimitive(name, ctype, llvmtype)			\
		J3Primitive* type##name;
		onJavaTypes(defPrimitive)
#undef defPrimitive

		void*            interfaceTrampoline;

		J3Type**         arrayInterfaces;
		uint32_t         nbArrayInterfaces;
		J3Class*         objectClass;
		J3ArrayClass*    charArrayClass;

		J3Class*         stringClass;
		J3Method*        stringInit;
		J3Field*         stringValue;

		J3Class*         classClass;
		J3Method*        classInit;
		J3Field*         classVMData;

		J3Field*         threadVMData;
		J3Method*        threadRun;

		const vmkit::Name* codeAttr;
		const vmkit::Name* constantValueAttr;
		const vmkit::Name* initName;
		const vmkit::Name* clinitName;
		const vmkit::Name* clinitSign;

		llvm::Type* typeJNIEnvPtr;
		llvm::Type* typeJ3VirtualTablePtr;
		llvm::Type* typeJ3Type;
		llvm::Type* typeJ3TypePtr;
		llvm::Type* typeJ3Thread;
		llvm::Type* typeJ3ObjectTypePtr;
		llvm::Type* typeJ3Class;
		llvm::Type* typeJ3ClassPtr;
		llvm::Type* typeJ3ArrayClass;
		llvm::Type* typeJ3ArrayClassPtr;
		llvm::Type* typeJ3Method;
		llvm::Type* typeJ3ArrayObject;
		llvm::Type* typeJ3ArrayObjectPtr;
		llvm::Type* typeJ3Object;
		llvm::Type* typeJ3ObjectPtr;
		llvm::Type* typeJ3ObjectHandlePtr;
		llvm::Type* typeGXXException;

		J3Options*                 options() { return &_options; }
		vmkit::Names*              names() { return &_names; }
		J3ObjectHandle*            utfToString(const char* name);
		J3ObjectHandle*            nameToString(const vmkit::Name* name);
		J3ObjectHandle*            arrayToString(J3ObjectHandle* array);

		void                       run();
		void                       start(int argc, char** argv);

		void                       vinternalError(const wchar_t* msg, va_list va) __attribute__((noreturn));

		static JNIEnv* jniEnv();

		static void    noClassDefFoundError(J3Class* cl) __attribute__((noreturn));
		static void    classFormatError(J3Class* cl, const wchar_t* reason, ...) __attribute__((noreturn));
		static void    noSuchMethodError(const wchar_t* msg, 
																		 J3Class* clName, const vmkit::Name* name, const vmkit::Name* sign) __attribute__((noreturn));
		static void    linkageError(J3Method* method) __attribute__((noreturn));

		static void    nullPointerException() __attribute__((noreturn));
		static void    classCastException() __attribute__((noreturn));

		static void    arrayStoreException() __attribute__((noreturn));
		static void    arrayIndexOutOfBoundsException() __attribute__((noreturn));

		static void    printStackTrace();

		void forceSymbolDefinition();
	};
}

#endif
