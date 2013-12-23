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

J3ClassLoader::J3MethodLess J3ClassLoader::j3MethodLess;

J3ClassLoader::J3ClassLoader(J3* v, J3ObjectHandle* javaClassLoader, vmkit::BumpAllocator* allocator) 
	: CompilationUnit(allocator, v, "class-loader"),
		_globalReferences(allocator),
		classes(vmkit::Name::less, allocator),
		types(vmkit::Name::less, allocator),
		methodTypes(vmkit::Name::less, allocator), 
		interfaceSignatures(vmkit::Name::less, allocator),
		methods(j3MethodLess, allocator),
		nativeLibraries(allocator) {
	_javaClassLoader = javaClassLoader;

	pthread_mutex_init(&_mutexClasses, 0);
	pthread_mutex_init(&_mutexTypes, 0);
	pthread_mutex_init(&_mutexMethodTypes, 0);
	pthread_mutex_init(&_mutexInterfaceSignatures, 0);
	pthread_mutex_init(&_mutexMethods, 0);
}

uint32_t J3ClassLoader::interfaceIndex(const vmkit::Name* sign) {
	pthread_mutex_lock(&_mutexInterfaceSignatures);
	std::map<const vmkit::Name*, uint32_t>::iterator it = interfaceSignatures.find(sign);
	uint32_t res;

	if(it == interfaceSignatures.end()) {
		res = interfaceSignatures.size();
		interfaceSignatures[sign] = res;
	} else {
		res = it->second;
	}

	pthread_mutex_unlock(&_mutexInterfaceSignatures);

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

J3Class* J3ClassLoader::getClass(const vmkit::Name* name) {
	pthread_mutex_lock(&_mutexClasses);
	J3Class* res = classes[name];
	if(!res)
		classes[name] = res = new(allocator()) J3Class(this, name);
	pthread_mutex_unlock(&_mutexClasses);
	return res;
}

J3ClassBytes* J3ClassLoader::lookup(const vmkit::Name* name) { 
	J3::internalError(L"should not happen");
}

void J3ClassLoader::wrongType(J3Class* from, const vmkit::Name* type) {
	J3::classFormatError(from, L"wrong type: %ls", type->cStr());
}

J3Type* J3ClassLoader::getTypeInternal(J3Class* from, const vmkit::Name* typeName, uint32_t start, uint32_t* pend) {
	J3Type*        res  = 0;
	const wchar_t* type = typeName->cStr();
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
				{ 
					uint32_t start = ++pos;
					wchar_t buf[len + 1 - start], c;
					
					memset(buf, 0, len + 1 - pos);
					
					for(; pos < len && (c = type[pos]) != J3Cst::ID_End; pos++)
						buf[pos - start] = c;
					
					if(type[pos] != J3Cst::ID_End)
						wrongType(from, typeName);
					
					buf[pos++ - start] = 0;

					res = getClass(vm()->names()->get(buf));
				}
				break;
			case J3Cst::ID_Left:
			case J3Cst::ID_Right:
			default:
				wrongType(from, typeName);
		}
	}

	*pend = pos;
		
	if(prof)
		res = res->getArray(prof, start ? 0 : typeName);

	return res;
}

J3Type* J3ClassLoader::getType(J3Class* from, const vmkit::Name* type) {
	pthread_mutex_lock(&_mutexTypes);
	J3Type* res = types[type];
	pthread_mutex_unlock(&_mutexTypes);

	if(!res) {
		uint32_t end;
		res = getTypeInternal(from, type, 0, &end);

		if(end != type->length())
			wrongType(from, type);

		//printf("Analyse %ls => %ls\n", type->cStr(), res->name()->cStr());
		
		pthread_mutex_lock(&_mutexTypes);
		types[type] = res;
		pthread_mutex_unlock(&_mutexTypes);
	}

	return res;
}

J3MethodType* J3ClassLoader::getMethodType(J3Class* from, const vmkit::Name* sign) {
	pthread_mutex_lock(&_mutexMethodTypes);
	J3MethodType* res = methodTypes[sign];
	pthread_mutex_unlock(&_mutexMethodTypes);

	if(!res) {
		J3Type*  args[sign->length()];
		uint32_t nbArgs = 0;
		uint32_t cur = 1;

		if(sign->cStr()[0] != J3Cst::ID_Left)
			wrongType(from, sign);

		while(sign->cStr()[cur] != J3Cst::ID_Right) {
			args[nbArgs++] = getTypeInternal(from, sign, cur, &cur);
		}
		args[nbArgs++] = getTypeInternal(from, sign, cur+1, &cur);
		if(cur != sign->length())
			wrongType(from, sign);

		pthread_mutex_lock(&_mutexMethodTypes);
		J3MethodType* tmp = methodTypes[sign];
		if(tmp)
			res = tmp;
		else
			res = new(allocator(), nbArgs - 1) J3MethodType(args, nbArgs);
		pthread_mutex_unlock(&_mutexMethodTypes);
	}

	return res;
}

J3Method* J3ClassLoader::method(uint16_t access, J3Class* cl, const vmkit::Name* name, const vmkit::Name* sign) {
	J3Method method(access, cl, name, sign), *res;

	pthread_mutex_lock(&_mutexMethods);
	std::map<J3Method*, J3Method*>::iterator it = methods.find(&method);

	if(it == methods.end()) {
		res = new(allocator()) J3Method(access, cl, name, sign);
		methods[res] = res;
	} else {
		res = it->second;
	}
	pthread_mutex_unlock(&_mutexMethods);

	return res;
}

J3Method* J3ClassLoader::method(uint16_t access, const vmkit::Name* clName, const vmkit::Name* name, const vmkit::Name* sign) {
	return method(access, getClass(clName), name, sign);
}

J3Method* J3ClassLoader::method(uint16_t access, const wchar_t* clName, const wchar_t* name, const wchar_t* sign) {
	vmkit::Names* names = vm()->names();
	return method(access, names->get(clName), names->get(name), names->get(sign));
}

bool J3ClassLoader::J3MethodLess::operator()(j3::J3Method const* lhs, j3::J3Method const* rhs) const {
	return lhs->name() < rhs->name()
		|| (lhs->name() == rhs->name()
				&& (lhs->sign() < rhs->sign()
						|| (lhs->sign() == rhs->sign()
								&& (lhs->cl() < rhs->cl()
										|| (lhs->cl() == rhs->cl()
												&& ((lhs->access() & J3Cst::ACC_STATIC) < (rhs->access() & J3Cst::ACC_STATIC)))))));
}

J3InitialClassLoader::J3InitialClassLoader(J3* v, const char* rtjar, vmkit::BumpAllocator* _alloc) 
	: J3ClassLoader(v, 0, _alloc) {
	const char* archives = vm()->options()->rtJar;
	J3ClassBytes* bytes = J3Reader::openFile(allocator(), archives);

	//makeLLVMFunctions_j3();

	if (bytes) {
		archive = new(allocator()) J3ZipArchive(bytes, allocator());
		if(!archive) {
			J3::internalError(L"unable to find system archive");
		}
	} else 
		J3::internalError(L"unable to find system archive");

	if(J3Lib::loadSystemLibraries(&nativeLibraries) == -1)
		J3::internalError(L"unable to find java library");
}

J3ClassBytes* J3InitialClassLoader::lookup(const vmkit::Name* name) {
	char tmp[name->length()+16];

	//printf("L: %ls\n", name->cStr());
	for(int i=0; i<name->length(); i++) {
		char c = name->cStr()[i] & 0xff;
		tmp[i] = c == '.' ? '/' : c;
	}
	strcpy(tmp + name->length(), ".class");
	J3ZipFile* file = archive->getFile(tmp);

	if(file) {
		J3ClassBytes* res = new(allocator(), file->ucsize) J3ClassBytes(file->ucsize);

		if(archive->readFile(res, file))
			return res;
	}

	return 0;
}

bool char_ptr_less::operator()(const char* lhs, const char* rhs) const {
	//printf("Compare: %ls - %ls - %d\n", lhs, rhs, wcscmp(lhs, rhs));
	return strcmp(lhs, rhs) < 0;
}

void J3InitialClassLoader::registerCMangling(const char* mangled, const char* demangled) {
	_cmangled[demangled] = mangled;
}



