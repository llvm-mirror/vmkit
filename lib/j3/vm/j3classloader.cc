#include <map>
#include <dlfcn.h>

#include "llvm/IR/Module.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"

#include "vmkit/allocator.h"

#include "j3/j3classloader.h"
#include "j3/j3.h"
#include "j3/j3reader.h"
#include "j3/j3zip.h"
#include "j3/j3class.h"
#include "j3/j3constants.h"
#include "j3/j3method.h"
#include "j3/j3lib.h"
#include "j3/j3mangler.h"

using namespace j3;

J3ClassLoader::J3InterfaceMethodLess J3ClassLoader::j3InterfaceMethodLess;

J3ClassLoader::J3ClassLoader(J3* v, J3ObjectHandle* javaClassLoader, vmkit::BumpAllocator* allocator) 
	: CompilationUnit(allocator, v, "class-loader"),
		_globalReferences(allocator),
		classes(vmkit::Name::less, allocator),
		types(vmkit::Name::less, allocator),
		interfaces(j3InterfaceMethodLess, allocator),
		methodTypes(vmkit::Name::less, allocator),
		nativeLibraries(allocator) {
	_javaClassLoader = javaClassLoader;

	pthread_mutex_init(&_mutexClasses, 0);
	pthread_mutex_init(&_mutexTypes, 0);
	pthread_mutex_init(&_mutexInterfaces, 0);
	pthread_mutex_init(&_mutexMethodTypes, 0);
}

uint32_t J3ClassLoader::interfaceIndex(J3Method* method) {
	pthread_mutex_lock(&_mutexInterfaces);
	InterfaceMethodRefMap::iterator it = interfaces.find(method);
	uint32_t res;

	if(it == interfaces.end()) {
		res = interfaces.size();
		//		fprintf(stderr, " new interface: %s::%s%s ---> %d\n", method->cl()->name()->cStr(), method->name()->cStr(), method->signature()->name()->cStr(), res);
		interfaces[method] = res;
	} else {
		res = it->second;
	}

	pthread_mutex_unlock(&_mutexInterfaces);

	return res;
}

void* J3ClassLoader::lookupNativeFunctionPointer(J3Method* method, const char* symbol) {
	for(std::vector<void*>::size_type i=0; i!=nativeLibraries.size(); i++) {
		void* fnPtr = dlsym(nativeLibraries[i], symbol);
		if(fnPtr)
			return fnPtr;
	}

	return 0;
}

J3Class* J3ClassLoader::findLoadedClass(const vmkit::Name* name) {
	pthread_mutex_lock(&_mutexClasses);
	std::map<const vmkit::Name*, J3Class*>::iterator it = classes.find(name);
	J3Class* res = it == classes.end() ? 0 : it->second;
	pthread_mutex_unlock(&_mutexClasses);
	return res;
}

J3Class* J3ClassLoader::defineClass(const vmkit::Name* name, J3ClassBytes* bytes) {
	pthread_mutex_lock(&_mutexClasses);
	J3Class* res = classes[name];
	if(!res)
		classes[name] = res = new(allocator()) J3Class(this, name, bytes);
	pthread_mutex_unlock(&_mutexClasses);
	return res;
}

J3Class* J3ClassLoader::loadClass(const vmkit::Name* name) {
	J3::internalError("implement me: loadClass from a Java class loader");
}

void J3ClassLoader::wrongType(J3ObjectType* from, const vmkit::Name* type) {
	J3::classFormatError(from, "wrong type: %s", type->cStr());
}

J3Type* J3ClassLoader::getTypeInternal(J3ObjectType* from, const vmkit::Name* typeName, uint32_t start, uint32_t* pend, bool unify) {
	J3Type*        res  = 0;
	const char*    type = typeName->cStr();
	uint32_t       len  = typeName->length();
	uint32_t       pos  = start;
	uint32_t       prof = 0;

	while(!res) {
		if(pos >= len)
			wrongType(from, typeName);

		switch(type[pos]) {
			case J3Cst::ID_Array:     prof++; pos++; break;
			case J3Cst::ID_Void:      res = vm()->typeVoid; pos++; break;
			case J3Cst::ID_Byte:      res = vm()->typeByte; pos++; break;
			case J3Cst::ID_Char:      res = vm()->typeChar; pos++; break;
			case J3Cst::ID_Double:    res = vm()->typeDouble; pos++; break;
			case J3Cst::ID_Float:     res = vm()->typeFloat; pos++; break;
			case J3Cst::ID_Integer:   res = vm()->typeInteger; pos++; break;
			case J3Cst::ID_Long:      res = vm()->typeLong; pos++; break;
			case J3Cst::ID_Short:     res = vm()->typeShort; pos++; break;
			case J3Cst::ID_Boolean:   res = vm()->typeBoolean; pos++; break;
			case J3Cst::ID_Classname: 
				if(unify) {
					uint32_t start = ++pos;
					for(; pos < len && type[pos] != J3Cst::ID_End; pos++);
					
					if(type[pos] != J3Cst::ID_End)
						wrongType(from, typeName);

					pos++;
					res = vm()->objectClass;
				} else {
					uint32_t start = ++pos;
					char buf[len + 1 - start], c;
					
					memset(buf, 0, len + 1 - pos);
					
					for(; pos < len && (c = type[pos]) != J3Cst::ID_End; pos++)
						buf[pos - start] = c;
					
					if(type[pos] != J3Cst::ID_End)
						wrongType(from, typeName);
					
					buf[pos++ - start] = 0;

					res = loadClass(vm()->names()->get(buf));
				}
				break;
			case J3Cst::ID_Left:
			case J3Cst::ID_Right:
			default:
				wrongType(from, typeName);
		}
	}

	*pend = pos;
		
	if(prof) {
		if(unify)
			res = vm()->objectClass;
		else
			res = res->getArray(prof, start ? 0 : typeName);
	}

	return res;
}

J3Type* J3ClassLoader::getType(J3ObjectType* from, const vmkit::Name* type) {
	pthread_mutex_lock(&_mutexTypes);
	J3Type* res = types[type];
	pthread_mutex_unlock(&_mutexTypes);

	if(!res) {
		uint32_t end;
		res = getTypeInternal(from, type, 0, &end, 0);

		if(end != type->length())
			wrongType(from, type);

		//printf("Analyse %s => %ls\n", type->cStr(), res->name()->cStr());
		
		pthread_mutex_lock(&_mutexTypes);
		types[type] = res;
		pthread_mutex_unlock(&_mutexTypes);
	}

	return res;
}


J3Signature* J3ClassLoader::getSignature(J3ObjectType* from, const vmkit::Name* signature) {
	pthread_mutex_lock(&_mutexMethodTypes);

	J3Signature* res = methodTypes[signature];
	if(!res)
		methodTypes[signature] = res = new(allocator()) J3Signature(this, signature);

	pthread_mutex_unlock(&_mutexMethodTypes);

	return res;
}

bool J3ClassLoader::J3InterfaceMethodLess::operator()(j3::J3Method const* lhs, j3::J3Method const* rhs) const {
	return lhs->name() < rhs->name()
		|| (lhs->name() == rhs->name()
				&& (lhs->signature() < rhs->signature()));
}

J3InitialClassLoader::J3InitialClassLoader(J3* v, const char* rtjar, vmkit::BumpAllocator* _alloc) 
	: J3ClassLoader(v, 0, _alloc) {
	const char* archives = vm()->options()->rtJar;
	J3ClassBytes* bytes = J3Reader::openFile(allocator(), archives);

	//makeLLVMFunctions_j3();

	if (bytes) {
		archive = new(allocator()) J3ZipArchive(bytes, allocator());
		if(!archive) {
			J3::internalError("unable to find system archive");
		}
	} else 
		J3::internalError("unable to find system archive");

	if(J3Lib::loadSystemLibraries(&nativeLibraries) == -1)
		J3::internalError("unable to find java library");
}

J3Class* J3InitialClassLoader::loadClass(const vmkit::Name* name) {
	J3Class* res = findLoadedClass(name);

	if(res)
		return res;

	char tmp[name->length()+16];
		
	//printf("L: %s\n", name->cStr());
	for(int i=0; i<name->length(); i++) {
		char c = name->cStr()[i] & 0xff;
		tmp[i] = c == '.' ? '/' : c;
	}
	strcpy(tmp + name->length(), ".class");
	J3ZipFile* file = archive->getFile(tmp);

	if(file) {
		J3ClassBytes* bytes = new(allocator(), file->ucsize) J3ClassBytes(file->ucsize);
		
		if(archive->readFile(bytes, file))
			return defineClass(name, bytes);
	}

	return 0;
}

void J3InitialClassLoader::registerCMangling(const char* mangled, const char* demangled) {
	_cmangled[demangled] = mangled;
}



