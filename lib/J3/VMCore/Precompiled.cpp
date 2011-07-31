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

void JnjvmClassLoader::insertAllMethodsInVM(Jnjvm* vm) {
  UNIMPLEMENTED();
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


class StaticJ3Frame {
public:
  void* ReturnAddress;
  uint16_t BytecodeIndex;
  uint16_t FrameSize;
  uint16_t NumLiveOffsets;
  int16_t LiveOffsets[1];
};

class StaticJ3Frames {
public:
  void* FunctionAddress;
  uint16_t NumDescriptors;
  StaticJ3Frame* frames() const; 
};

class StaticJ3FrameDecoder {
public:
  StaticJ3Frames* frames ;
  uint32 currentDescriptor;
  StaticJ3Frame* currentFrame;

  StaticJ3FrameDecoder(StaticJ3Frames* frames) {
    this->frames = frames;
    currentDescriptor = 0;
    currentFrame = frames->frames();
  }

  bool hasNext() {
    return currentDescriptor < frames->NumDescriptors;
  }

  void advance() {
    ++currentDescriptor;
    if (!hasNext()) return;
    uintptr_t ptr = reinterpret_cast<uintptr_t>(currentFrame)
      + currentFrame->NumLiveOffsets * sizeof(uint16_t)
      + sizeof(void*)       // ReturnAddress
      + sizeof(uint16_t)    // BytecodeIndex
      + sizeof(uint16_t)    // FrameSize
      + sizeof(uint16_t);   // NumLiveOffsets
    if (ptr & 2) {
      ptr += sizeof(uint16_t);
    }
    currentFrame = reinterpret_cast<StaticJ3Frame*>(ptr);
  }

  StaticJ3Frame* next() {
    assert(hasNext());
    StaticJ3Frame* result = currentFrame;
    advance();
    return result;
  }
};

StaticJ3Frame* StaticJ3Frames::frames() const {
  intptr_t ptr = reinterpret_cast<intptr_t>(this) + sizeof(void*) + sizeof(uint16_t);
  // If the frames structure was not 4-aligned, manually do it here.
  if (ptr & 2) {
    ptr += sizeof(uint16_t);
  }
  return reinterpret_cast<StaticJ3Frame*>(ptr);
}

class JavaStaticMethodInfo : public mvm::MethodInfo {
private:
  StaticJ3Frame* frame;
public:
  virtual void print(void* ip, void* addr);
  virtual bool isHighLevelMethod() {
    return true;
  }

  virtual void scan(uintptr_t closure, void* ip, void* addr) {
    assert(frame != NULL);
    //uintptr_t spaddr = (uintptr_t)addr + CF->FrameSize + sizeof(void*);
    uintptr_t spaddr = ((uintptr_t*)addr)[0];
    for (uint16 i = 0; i < frame->NumLiveOffsets; ++i) {
      mvm::Collector::scanObject((void**)(spaddr + frame->LiveOffsets[i]), closure);
    }
  }

  JavaStaticMethodInfo(StaticJ3Frame* F, JavaMethod* M) {
    frame = F;
    MetaInfo = M;
    Owner = M->classDef->classLoader->getCompiler();
  }
};


void JavaStaticMethodInfo::print(void* ip, void* addr) {
  JavaMethod* meth = (JavaMethod*)MetaInfo;
  CodeLineInfo* info = meth->lookupCodeLineInfo((uintptr_t)ip);
  if (info != NULL) {
    fprintf(stderr, "; %p (%p) in %s.%s (AOT line %d, bytecode %d, code start %p)", ip, addr,
            UTF8Buffer(meth->classDef->name).cString(),
            UTF8Buffer(meth->name).cString(),
            meth->lookupLineNumber((uintptr_t)ip),
            info->bytecodeIndex, meth->code);
  } else {
    fprintf(stderr, "; %p (%p) in %s.%s (native method, code start %p)", ip, addr,
            UTF8Buffer(meth->classDef->name).cString(),
            UTF8Buffer(meth->name).cString(), meth->code);
  }
  fprintf(stderr, "\n");
}


static void ReadFrame(Jnjvm* vm, JnjvmClassLoader* loader, JavaMethod* meth) {
  StaticJ3Frames* frames = reinterpret_cast<StaticJ3Frames*>(meth->codeInfo);
  StaticJ3FrameDecoder decoder(frames);
  mvm::BumpPtrAllocator& allocator = loader->allocator;
  meth->codeInfoLength = frames->NumDescriptors;
  if (frames->NumDescriptors > 0) {
    meth->codeInfo = new(allocator, "CodeLineInfo") CodeLineInfo[frames->NumDescriptors];
  } else {
    meth->codeInfo = NULL;
  }

  int codeInfoIndex = 0;
  while (decoder.hasNext()) {
    StaticJ3Frame* frame = decoder.next();
    mvm::MethodInfo* MI = new(allocator, "JavaStaticMethodInfo") JavaStaticMethodInfo(frame, meth);
    assert(loader->bootstrapLoader == loader && "Can only add frame without lock for the bootstrap loader");
    vm->FunctionsCache.addMethodInfoNoLock(MI, frame->ReturnAddress);
    meth->codeInfo[codeInfoIndex].address = reinterpret_cast<uintptr_t>(frame->ReturnAddress);
    meth->codeInfo[codeInfoIndex++].bytecodeIndex = frame->BytecodeIndex;
  }
}


void Precompiled::ReadFrames(Jnjvm* vm, JnjvmClassLoader* loader) {
  for (ClassMap::iterator i = loader->getClasses()->map.begin(),
       e = loader->getClasses()->map.end(); i != e; ++i) {
    CommonClass* cl = i->second;
    if (cl->isClass()) {
      Class* C = cl->asClass(); 
      for (uint32 i = 0; i < C->nbVirtualMethods; ++i) {
        JavaMethod& meth = C->virtualMethods[i];
        if (meth.code != NULL) {
          ReadFrame(vm, loader, &meth);
        }
      }
      
      for (uint32 i = 0; i < C->nbStaticMethods; ++i) {
        JavaMethod& meth = C->staticMethods[i];
        if (meth.code != NULL) {
          ReadFrame(vm, loader, &meth);
        }
      }
    }
  }
}

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

  ClassArray::SuperArray = javaLangObject;
    
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
  upcalls->OfVoid->classLoader = loader;
  upcalls->OfBool->classLoader = loader;
  upcalls->OfByte->classLoader = loader;
  upcalls->OfChar->classLoader = loader;
  upcalls->OfShort->classLoader = loader;
  upcalls->OfInt->classLoader = loader;
  upcalls->OfFloat->classLoader = loader;
  upcalls->OfLong->classLoader = loader;
  upcalls->OfDouble->classLoader = loader;
  
  J3DenseMap<const UTF8*, CommonClass*>* precompiledClassMap =
    reinterpret_cast<J3DenseMap<const UTF8*, CommonClass*>*>(dlsym(nativeHandle, "ClassMap"));
  loader->classes = new (loader->allocator, "ClassMap") ClassMap(precompiledClassMap);
  for (ClassMap::iterator i = loader->getClasses()->map.begin(),
       e = loader->getClasses()->map.end(); i != e; i++) {
    vmjcAddPreCompiledClass(loader, i->second);
  }
  
  // Get the base object arrays after the init, because init puts arrays
  // in the class loader map.
  upcalls->ArrayOfString = 
    loader->constructArray(loader->asciizConstructUTF8("[Ljava/lang/String;"));

  upcalls->ArrayOfObject = 
    loader->constructArray(loader->asciizConstructUTF8("[Ljava/lang/Object;"));
  
  ClassArray::InterfacesArray = upcalls->ArrayOfObject->interfaces;

  return true;
}

}
