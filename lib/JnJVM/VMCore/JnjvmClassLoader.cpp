//===-- JnjvmClassLoader.cpp - Jnjvm representation of a class loader ------===//
//
//                              Jnjvm
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>

#include "debug.h"

#include "mvm/Allocator.h"

#include "JavaClass.h"
#include "JavaConstantPool.h"
#include "JavaThread.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"
#include "JnjvmClassLoader.h"
#include "JnjvmModule.h"
#include "JnjvmModuleProvider.h"
#include "LockedMap.h"
#include "Reader.h"

using namespace jnjvm;

#ifndef ISOLATE_SHARING
JnjvmBootstrapLoader* JnjvmClassLoader::bootstrapLoader = 0;
UserClass* JnjvmBootstrapLoader::SuperArray = 0;
std::vector<UserClass*> JnjvmBootstrapLoader::InterfacesArray;
#endif


extern const char* GNUClasspathGlibj;
extern const char* GNUClasspathLibs;

JnjvmBootstrapLoader* JnjvmBootstrapLoader::createBootstrapLoader() {
  
  JnjvmBootstrapLoader* JCL = gc_new(JnjvmBootstrapLoader)();
  JnjvmModule::initialise(); 
  JCL->TheModule = new JnjvmModule("Bootstrap JnJVM");
  JCL->TheModuleProvider = new JnjvmModuleProvider(JCL->TheModule);
  
  JCL->allocator = new mvm::Allocator();
  
  JCL->hashUTF8 = new UTF8Map(JCL->allocator, 0);
  JCL->classes = new ClassMap();
  JCL->javaTypes = new TypeMap(); 
  JCL->javaSignatures = new SignMap(); 
  
  JCL->bootClasspathEnv = getenv("JNJVM_BOOTCLASSPATH");
  if (!JCL->bootClasspathEnv) {
    JCL->bootClasspathEnv = GNUClasspathGlibj;
  }
  
  JCL->libClasspathEnv = getenv("JNJVM_LIBCLASSPATH");
  if (!JCL->libClasspathEnv) {
    JCL->libClasspathEnv = GNUClasspathLibs;
  }
  
  JCL->analyseClasspathEnv(JCL->bootClasspathEnv);
  
  JCL->upcalls = new Classpath();
  JCL->bootstrapLoader = JCL;
  return JCL;
}

JnjvmClassLoader::JnjvmClassLoader(JnjvmClassLoader& JCL, JavaObject* loader,
                                   Jnjvm* I) {
  TheModule = new JnjvmModule("Applicative loader");
  TheModuleProvider = new JnjvmModuleProvider(TheModule);
  bootstrapLoader = JCL.bootstrapLoader;
  
  allocator = new mvm::Allocator();

  hashUTF8 = new UTF8Map(allocator, bootstrapLoader->upcalls->ArrayOfChar);
  classes = new ClassMap();
  javaTypes = new TypeMap();
  javaSignatures = new SignMap();

  javaLoader = loader;
  isolate = I;

#ifdef ISOLATE_SHARING
  JavaMethod* meth = bootstrapLoader->upcalls->loadInClassLoader;
  loader->classOf->lookupMethodDontThrow(meth->name, meth->type, false, true,
                                         loadClass);
  assert(loadClass && "Loader does not have a loadClass function");
#endif

}

ArrayUInt8* JnjvmBootstrapLoader::openName(const UTF8* utf8) {
  char* asciiz = utf8->UTF8ToAsciiz();
  uint32 alen = strlen(asciiz);
  ArrayUInt8* res = 0;
  
  for (std::vector<const char*>::iterator i = bootClasspath.begin(),
       e = bootClasspath.end(); i != e; ++i) {
    const char* str = *i;
    unsigned int strLen = strlen(str);
    char* buf = (char*)alloca(strLen + alen + 7);

    sprintf(buf, "%s%s.class", str, asciiz);
    res = Reader::openFile(this, buf);
    if (res) return res;
  }

  for (std::vector<ZipArchive*>::iterator i = bootArchives.begin(),
       e = bootArchives.end(); i != e; ++i) {
    
    ZipArchive* archive = *i;
    char* buf = (char*)alloca(alen + 7);
    sprintf(buf, "%s.class", asciiz);
    res = Reader::openZip(this, archive, buf);
    if (res) return res;
  }

  return 0;
}


UserClass* JnjvmBootstrapLoader::internalLoad(const UTF8* name) {
  
  UserCommonClass* cl = lookupClass(name);
  
  if (!cl) {
    ArrayUInt8* bytes = openName(name);
    if (bytes) {
      cl = constructClass(name, bytes);
    }
  }
  
  if (cl) assert(!cl->isArray());
  return (UserClass*)cl;
}

UserClass* JnjvmClassLoader::internalLoad(const UTF8* name) {
  UserCommonClass* cl = lookupClass(name);
  
  if (!cl) {
    const UTF8* javaName = name->internalToJava(hashUTF8, 0, name->size);
    JavaString* str = isolate->UTF8ToStr(javaName);
    Classpath* upcalls = bootstrapLoader->upcalls;
    UserClass* forCtp = 0;
#ifdef ISOLATE_SHARING
    forCtp = loadClass;
#else
    forCtp = upcalls->loadInClassLoader->classDef;
#endif
    JavaObject* obj = (JavaObject*)
      upcalls->loadInClassLoader->invokeJavaObjectVirtual(isolate, forCtp,
                                                          javaLoader, str);
    cl = (UserCommonClass*)(upcalls->vmdataClass->getObjectField(obj));
  }
  
  if (cl) assert(!cl->isArray());
  return (UserClass*)cl;
}

UserClass* JnjvmClassLoader::loadName(const UTF8* name, bool doResolve,
                                        bool doThrow) {
 

  UserClass* cl = internalLoad(name);

  if (!cl && doThrow) {
    if (!(name->equals(Jnjvm::NoClassDefFoundError))) {
      JavaThread::get()->isolate->unknownError("Unable to load NoClassDefFoundError");
    }
    JavaThread::get()->isolate->noClassDefFoundError("unable to load %s", name->printString());
  }

  if (cl && doResolve) cl->resolveClass();

  return cl;
}

UserCommonClass* JnjvmClassLoader::lookupClassFromUTF8(const UTF8* name,
                                                       bool doResolve,
                                                       bool doThrow) {
  uint32 len = name->size;
  uint32 start = 0;
  uint32 origLen = len;
  bool doLoop = true;
  UserCommonClass* ret = 0;

  if (len == 0) {
    return 0;
  } else if (name->elements[0] == I_TAB) {
    
    while (doLoop) {
      --len;
      if (len == 0) {
        doLoop = false;
      } else {
        ++start;
        if (name->elements[start] != I_TAB) {
          if (name->elements[start] == I_REF) {
            uint32 size = (uint32)name->size;
            if ((size == (start + 1)) || (size == (start + 2)) || 
                 (name->elements[start + 1] == I_TAB) || 
                 (name->elements[origLen - 1] != I_END_REF)) {
              doLoop = false; 
            } else {
              const UTF8* componentName = name->javaToInternal(hashUTF8,
                                                               start + 1,
                                                               len - 2);
              if (loadName(componentName, doResolve, doThrow)) {
                ret = constructArray(name);
                if (doResolve) ret->resolveClass();
                doLoop = false;
              } else {
                doLoop = false;
              }
            }
          } else {
            uint16 cur = name->elements[start];
            if ((cur == I_BOOL || cur == I_BYTE ||
                 cur == I_CHAR || cur == I_SHORT ||
                 cur == I_INT || cur == I_FLOAT || 
                 cur == I_DOUBLE || cur == I_LONG)
                && ((uint32)name->size) == start + 1) {

              ret = constructArray(name);
              ret->resolveClass();
              doLoop = false;
            } else {
              doLoop = false;
            }
          }
        }
      }
    }

    return ret;

  } else {
    return loadName(name, doResolve, doThrow);
  }
}

UserCommonClass* JnjvmClassLoader::lookupClassFromJavaString(JavaString* str,
                                              bool doResolve, bool doThrow) {
  
  const UTF8* name = 0;
  
  if (str->value->elements[str->offset] != I_TAB)
    name = str->value->checkedJavaToInternal(hashUTF8, str->offset,
                                             str->count);
  else
    name = str->value->javaToInternal(hashUTF8, str->offset, str->count);

  if (name)
    return lookupClassFromUTF8(name, doResolve, doThrow);

  return 0;
}

UserCommonClass* JnjvmClassLoader::lookupClass(const UTF8* utf8) {
  return classes->lookup(utf8);
}

UserCommonClass* JnjvmClassLoader::loadBaseClass(const UTF8* name,
                                                 uint32 start, uint32 len) {
  
  if (name->elements[start] == I_TAB) {
    UserCommonClass* baseClass = loadBaseClass(name, start + 1, len - 1);
    JnjvmClassLoader* loader = baseClass->classLoader;
    const UTF8* arrayName = name->extract(loader->hashUTF8, start, start + len);
    return loader->constructArray(arrayName);
  } else if (name->elements[start] == I_REF) {
    const UTF8* componentName = name->extract(hashUTF8,
                                              start + 1, start + len - 1);
    UserCommonClass* cl = loadName(componentName, false, true);
    return cl;
  } else {
    Classpath* upcalls = bootstrapLoader->upcalls;
    UserClassPrimitive* prim = 
      UserClassPrimitive::byteIdToPrimitive(name->elements[start], upcalls);
    assert(prim && "No primitive found");
    return prim;
  }
}


UserClassArray* JnjvmClassLoader::constructArray(const UTF8* name) {
  UserCommonClass* cl = loadBaseClass(name, 1, name->size - 1);
  assert(cl && "no base class for an array");
  JnjvmClassLoader* ld = cl->classLoader;
  return ld->constructArray(name, cl);
}

UserClass* JnjvmClassLoader::constructClass(const UTF8* name, ArrayUInt8* bytes) {
  assert(bytes && "constructing a class without bytes");
  classes->lock->lock();
  ClassMap::iterator End = classes->map.end();
  ClassMap::iterator I = classes->map.find(name);
  UserClass* res = 0;
  if (I == End) {
    res = allocator_new(allocator, UserClass)(this, name, bytes);
    classes->map.insert(std::make_pair(name, res));
  } else {
    res = ((UserClass*)(I->second));
  }
  classes->lock->unlock();
  return res;
}

UserClassArray* JnjvmClassLoader::constructArray(const UTF8* name,
                                                 UserCommonClass* baseClass) {
  assert(baseClass && "constructing an array class without a base class");
  assert(baseClass->classLoader == this && 
         "constructing an array with wrong loader");
  classes->lock->lock();
  ClassMap::iterator End = classes->map.end();
  ClassMap::iterator I = classes->map.find(name);
  UserClassArray* res = 0;
  if (I == End) {
    res = allocator_new(allocator, UserClassArray)(this, name, baseClass);
    classes->map.insert(std::make_pair(name, res));
  } else {
    res = ((UserClassArray*)(I->second));
  }
  classes->lock->unlock();
  return res;
}

Typedef* JnjvmClassLoader::constructType(const UTF8* name) {
  Typedef* res = javaTypes->lookup(name);
  if (res == 0) {
    res = Typedef::constructType(name, hashUTF8, isolate);
    javaTypes->lock->lock();
    Typedef* tmp = javaTypes->lookup(name);
    if (tmp == 0) javaTypes->hash(name, res);
    else {
      delete res;
      res = tmp;
    }
    javaTypes->lock->unlock();
  }
  return res;
}

Signdef* JnjvmClassLoader::constructSign(const UTF8* name) {
  Signdef* res = javaSignatures->lookup(name);
  if (res == 0) {
    res = new Signdef(name, this);
    javaSignatures->lock->lock();
    Signdef* tmp = javaSignatures->lookup(name);
    if (tmp == 0) javaSignatures->hash(name, res);
    else {
      delete res;
      res = tmp;
    }
    javaSignatures->lock->unlock();
  }
  return res;
}


JnjvmClassLoader* JnjvmClassLoader::getJnjvmLoaderFromJavaObject(JavaObject* loader, Jnjvm* vm) {
  
  if (loader == 0)
    return vm->bootstrapLoader;
  
  Classpath* upcalls = vm->bootstrapLoader->upcalls;
  JnjvmClassLoader* JCL = 
    (JnjvmClassLoader*)(upcalls->vmdataClassLoader->getObjectField(loader));
  
  if (!JCL) {
    JCL = gc_new(JnjvmClassLoader)(*vm->bootstrapLoader, loader, JavaThread::get()->isolate);
    (upcalls->vmdataClassLoader->setObjectField(loader, (JavaObject*)JCL));
  }

  return JCL;
}

const UTF8* JnjvmClassLoader::asciizConstructUTF8(const char* asciiz) {
  return hashUTF8->lookupOrCreateAsciiz(asciiz);
}

const UTF8* JnjvmClassLoader::readerConstructUTF8(const uint16* buf, uint32 size) {
  return hashUTF8->lookupOrCreateReader(buf, size);
}

JnjvmClassLoader::~JnjvmClassLoader() {
  delete classes;
  delete hashUTF8;
  delete javaTypes;
  delete javaSignatures;
  delete TheModuleProvider;
}

void JnjvmBootstrapLoader::analyseClasspathEnv(const char* str) {
  if (str != 0) {
    unsigned int len = strlen(str);
    char* buf = (char*)alloca(len + 1);
    const char* cur = str;
    int top = 0;
    char c = 1;
    while (c != 0) {
      while (((c = cur[top]) != 0) && c != Jnjvm::envSeparator[0]) {
        top++;
      }
      if (top != 0) {
        memcpy(buf, cur, top);
        buf[top] = 0;
        char* rp = (char*)alloca(PATH_MAX);
        memset(rp, 0, PATH_MAX);
        rp = realpath(buf, rp);
        if (rp[PATH_MAX - 1] == 0 && strlen(rp) != 0) {
          struct stat st;
          stat(rp, &st);
          if ((st.st_mode & S_IFMT) == S_IFDIR) {
            unsigned int len = strlen(rp);
            char* temp = (char*)malloc(len + 2);
            memcpy(temp, rp, len);
            temp[len] = Jnjvm::dirSeparator[0];
            temp[len + 1] = 0;
            bootClasspath.push_back(temp);
          } else {
            ArrayUInt8* bytes =
              Reader::openFile(this, rp);
            if (bytes) {
              ZipArchive *archive = new(allocator) ZipArchive(bytes, allocator);
              if (archive) {
                bootArchives.push_back(archive);
              }
            }
          }
        } 
      }
      cur = cur + top + 1;
      top = 0;
    }
  }
}

const UTF8* JnjvmClassLoader::constructArrayName(uint32 steps,
                                                 const UTF8* className) {
  uint32 len = className->size;
  uint32 pos = steps;
  bool isTab = (className->elements[0] == I_TAB ? true : false);
  uint32 n = steps + len + (isTab ? 0 : 2);
  uint16* buf = (uint16*)alloca(n * sizeof(uint16));
    
  for (uint32 i = 0; i < steps; i++) {
    buf[i] = I_TAB;
  }

  if (!isTab) {
    ++pos;
    buf[steps] = I_REF;
  }

  for (uint32 i = 0; i < len; i++) {
    buf[pos + i] = className->elements[i];
  }

  if (!isTab) {
    buf[n - 1] = I_END_REF;
  }

  return readerConstructUTF8(buf, n);
}
