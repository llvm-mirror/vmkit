//===-------- Precompiled.cpp - Support for precompiled code --------------===//
//
//                          The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// for dlopen and dlsym
#include <dlfcn.h> 

#if defined(__MACH__)
#define SELF_HANDLE RTLD_DEFAULT
#define DYLD_EXTENSION ".dylib"
#else
#define SELF_HANDLE 0
#define DYLD_EXTENSION ".so"
#endif

#include "mvm/MethodInfo.h"

#include "JavaClass.h"
#include "JavaUpcalls.h"
#include "JnjvmClassLoader.h"
#include "Jnjvm.h"
#include "LockedMap.h"

namespace j3 {

typedef void (*static_init_t)(JnjvmClassLoader*);

bool Precompiled::Init(JnjvmBootstrapLoader* loader) {
  Class* javaLangObject = (Class*)dlsym(SELF_HANDLE, "java_lang_Object");
  void* nativeHandle = SELF_HANDLE;
  if (javaLangObject == NULL) {
    void* handle = dlopen("libvmjc"DYLD_EXTENSION, RTLD_LAZY | RTLD_GLOBAL);
    if (handle != NULL) {
      nativeHandle = handle;
      javaLangObject = (Class*)dlsym(nativeHandle, "java_lang_Object");
    }
  }

  if (javaLangObject == NULL) {
    return false;
  }
    
  // Get the native classes.
  Classpath* upcalls = loader->upcalls;
  upcalls->OfVoid = (ClassPrimitive*)dlsym(nativeHandle, "void");
  upcalls->OfBool = (ClassPrimitive*)dlsym(nativeHandle, "boolean");
  upcalls->OfByte = (ClassPrimitive*)dlsym(nativeHandle, "byte");
  upcalls->OfChar = (ClassPrimitive*)dlsym(nativeHandle, "char");
  upcalls->OfShort = (ClassPrimitive*)dlsym(nativeHandle, "short");
  upcalls->OfInt = (ClassPrimitive*)dlsym(nativeHandle, "int");
  upcalls->OfFloat = (ClassPrimitive*)dlsym(nativeHandle, "float");
  upcalls->OfLong = (ClassPrimitive*)dlsym(nativeHandle, "long");
  upcalls->OfDouble = (ClassPrimitive*)dlsym(nativeHandle, "double");
   
  // We have the java/lang/Object class, execute the static initializer.
  static_init_t init = (static_init_t)(uintptr_t)javaLangObject->classLoader;
  assert(init && "Loaded the wrong boot library");
  init(loader);
  
  // Get the base object arrays after the init, because init puts arrays
  // in the class loader map.
  upcalls->ArrayOfString = 
    loader->constructArray(loader->asciizConstructUTF8("[Ljava/lang/String;"));

  upcalls->ArrayOfObject = 
    loader->constructArray(loader->asciizConstructUTF8("[Ljava/lang/Object;"));
  
  ClassArray::InterfacesArray = upcalls->ArrayOfObject->interfaces;

  return true;
}

class JavaStaticMethodInfo : public mvm::CamlMethodInfo {
public:
  virtual void print(void* ip, void* addr);
  virtual bool isHighLevelMethod() {
    return true;
  }
  
  JavaStaticMethodInfo(mvm::CamlMethodInfo* super, void* ip, JavaMethod* M) :
    mvm::CamlMethodInfo(super->CF) {
    MetaInfo = M;
    Owner = M->classDef->classLoader->getCompiler();
  }
};

void JavaStaticMethodInfo::print(void* ip, void* addr) {
  void* new_ip = NULL;
  if (ip) new_ip = mvm::MethodInfo::isStub(ip, addr);
  JavaMethod* meth = (JavaMethod*)MetaInfo;
  fprintf(stderr, "; %p in %s.%s", new_ip,
          UTF8Buffer(meth->classDef->name).cString(),
          UTF8Buffer(meth->name).cString());
  if (ip != new_ip) fprintf(stderr, " (from stub)");
  fprintf(stderr, "\n");
}

void JnjvmClassLoader::insertAllMethodsInVM(Jnjvm* vm) {
  for (ClassMap::iterator i = classes->map.begin(), e = classes->map.end();
       i != e; ++i) {
    CommonClass* cl = i->second;
    if (cl->isClass()) {
      Class* C = cl->asClass();
      
      for (uint32 i = 0; i < C->nbVirtualMethods; ++i) {
        JavaMethod& meth = C->virtualMethods[i];
        if (!isAbstract(meth.access) && meth.code) {
          JavaStaticMethodInfo* MI = new (allocator, "JavaStaticMethodInfo")
            JavaStaticMethodInfo(0, meth.code, &meth);
          vm->FunctionsCache.addMethodInfo(MI, meth.code);
        }
      }
      
      for (uint32 i = 0; i < C->nbStaticMethods; ++i) {
        JavaMethod& meth = C->staticMethods[i];
        if (!isAbstract(meth.access) && meth.code) {
          JavaStaticMethodInfo* MI = new (allocator, "JavaStaticMethodInfo")
            JavaStaticMethodInfo(0, meth.code, &meth);
          vm->FunctionsCache.addMethodInfo(MI, meth.code);
        }
      }
    }
  }
}

void JnjvmClassLoader::loadLibFromJar(Jnjvm* vm, const char* name,
                                      const char* file) {

  mvm::ThreadAllocator threadAllocator;
  char* soName = (char*)threadAllocator.Allocate(
      strlen(name) + strlen(DYLD_EXTENSION));
  const char* ptr = strrchr(name, '/');
  sprintf(soName, "%s%s", ptr ? ptr + 1 : name, DYLD_EXTENSION);
  void* handle = dlopen(soName, RTLD_LAZY | RTLD_LOCAL);
  if (handle) {
    Class* cl = (Class*)dlsym(handle, file);
    if (cl) {
      static_init_t init = (static_init_t)(uintptr_t)cl->classLoader;
      assert(init && "Loaded the wrong library");
      init(this);
      insertAllMethodsInVM(vm);
    }
  }
}

void JnjvmClassLoader::loadLibFromFile(Jnjvm* vm, const char* name) {
  mvm::ThreadAllocator threadAllocator;
  assert(classes->map.size() == 0);
  char* soName = (char*)threadAllocator.Allocate(
      strlen(name) + strlen(DYLD_EXTENSION));
  sprintf(soName, "%s%s", name, DYLD_EXTENSION);
  void* handle = dlopen(soName, RTLD_LAZY | RTLD_LOCAL);
  if (handle) {
    Class* cl = (Class*)dlsym(handle, name);
    if (cl) {
      static_init_t init = (static_init_t)(uintptr_t)cl->classLoader;
      init(this);
      insertAllMethodsInVM(vm);
    }
  }
}

Class* JnjvmClassLoader::loadClassFromSelf(Jnjvm* vm, const char* name) {
  assert(classes->map.size() == 0);
  Class* cl = (Class*)dlsym(SELF_HANDLE, name);
  if (cl) {
    static_init_t init = (static_init_t)(uintptr_t)cl->classLoader;
    init(this);
    insertAllMethodsInVM(vm);
  }
  return cl;
}


// Extern "C" functions called by the vmjc static intializer.
extern "C" void vmjcAddPreCompiledClass(JnjvmClassLoader* JCL,
                                        CommonClass* cl) {
  cl->classLoader = JCL;
  
  JCL->hashUTF8->insert(cl->name);

  if (cl->isClass()) {
    Class* realCl = cl->asClass();
		// To avoid data alignment in the llvm assembly emitter, we set the
  	// staticMethods and staticFields fields here.
    realCl->staticMethods = realCl->virtualMethods + realCl->nbVirtualMethods;
    realCl->staticFields = realCl->virtualFields + realCl->nbVirtualFields;
  	cl->virtualVT->setNativeTracer(cl->virtualVT->tracer, "");

    for (uint32 i = 0; i< realCl->nbStaticMethods; ++i) {
      JavaMethod& meth = realCl->staticMethods[i];
      JCL->hashUTF8->insert(meth.name);
      JCL->hashUTF8->insert(meth.type);
    }
    
    for (uint32 i = 0; i< realCl->nbVirtualMethods; ++i) {
      JavaMethod& meth = realCl->virtualMethods[i];
      JCL->hashUTF8->insert(meth.name);
      JCL->hashUTF8->insert(meth.type);
    }
    
    for (uint32 i = 0; i< realCl->nbStaticFields; ++i) {
      JavaField& field = realCl->staticFields[i];
      JCL->hashUTF8->insert(field.name);
      JCL->hashUTF8->insert(field.type);
    }
    
    for (uint32 i = 0; i< realCl->nbVirtualFields; ++i) {
      JavaField& field = realCl->virtualFields[i];
      JCL->hashUTF8->insert(field.name);
      JCL->hashUTF8->insert(field.type);
    }
  }

  if (!cl->isPrimitive()) {
    JCL->getClasses()->map.insert(std::make_pair(cl->name, cl));
  }
}

extern "C" void vmjcGetClassArray(JnjvmClassLoader* JCL, ClassArray** ptr,
                                  const UTF8* name) {
  JCL->hashUTF8->insert(name);
  *ptr = JCL->constructArray(name);
}

extern "C" void vmjcAddUTF8(JnjvmClassLoader* JCL, const UTF8* val) {
  JCL->hashUTF8->insert(val);
}

extern "C" void vmjcAddString(JnjvmClassLoader* JCL, JavaString* val) {
  JCL->strings->addString(JCL, val);
}

extern "C" intptr_t vmjcNativeLoader(JavaMethod* meth) {
  bool j3 = false;
  const UTF8* jniConsClName = meth->classDef->name;
  const UTF8* jniConsName = meth->name;
  const UTF8* jniConsType = meth->type;
  sint32 clen = jniConsClName->size;
  sint32 mnlen = jniConsName->size;
  sint32 mtlen = jniConsType->size;

  mvm::ThreadAllocator threadAllocator;
  char* buf = (char*)threadAllocator.Allocate(
      3 + JNI_NAME_PRE_LEN + 1 + ((mnlen + clen + mtlen) << 3));
  intptr_t res = meth->classDef->classLoader->nativeLookup(meth, j3, buf);
  assert(res && "Could not find required native method");
  return res;
}

extern "C" void staticCallback() {
  fprintf(stderr, "Implement me");
  abort();
}

}
