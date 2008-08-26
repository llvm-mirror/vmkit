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

#include "JavaAllocator.h"
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

JnjvmBootstrapLoader* JnjvmClassLoader::bootstrapLoader = 0;

extern const char* GNUClasspathGlibj;
extern const char* GNUClasspathLibs;

JnjvmBootstrapLoader* JnjvmBootstrapLoader::createBootstrapLoader() {
  
  JnjvmBootstrapLoader* JCL = gc_new(JnjvmBootstrapLoader)();
  JCL->TheModule = new JnjvmModule("Bootstrap JnJVM");
  JCL->TheModuleProvider = new JnjvmModuleProvider(JCL->TheModule);
  JCL->TheModule->initialise(); 
  
  JCL->allocator = new JavaAllocator();
  
  JCL->hashUTF8 = new UTF8Map(JCL->allocator, JCL->upcalls->ArrayOfChar);
  JCL->classes = allocator_new(allocator, ClassMap)();
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

  return JCL;
}

JnjvmClassLoader::JnjvmClassLoader(JnjvmClassLoader& JCL, JavaObject* loader, Jnjvm* I) {
  TheModule = JCL.TheModule;
  TheModuleProvider = JCL.TheModuleProvider;
  
  allocator = &(isolate->allocator);

  hashUTF8 = JCL.hashUTF8;
  classes = allocator_new(allocator, ClassMap)();
  javaTypes = JCL.javaTypes;
  javaSignatures = JCL.javaSignatures;

  javaLoader = loader;
  isolate = I;

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
    JavaObject* obj = (JavaObject*)
      upcalls->loadInClassLoader->invokeJavaObjectVirtual(isolate, javaLoader,
                                                          str);
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

UserCommonClass* JnjvmClassLoader::lookupClassFromUTF8(const UTF8* utf8, unsigned int start,
                                         unsigned int len,
                                         bool doResolve,
                                         bool doThrow) {
  uint32 origLen = len;
  const UTF8* name = utf8->javaToInternal(hashUTF8, start, len);
  bool doLoop = true;
  UserCommonClass* ret = 0;

  if (len == 0) {
    return 0;
  } else if (name->elements[0] == AssessorDesc::I_TAB) {
    
    while (doLoop) {
      --len;
      if (len == 0) {
        doLoop = false;
      } else {
        ++start;
        if (name->elements[start] != AssessorDesc::I_TAB) {
          if (name->elements[start] == AssessorDesc::I_REF) {
            uint32 size = (uint32)name->size;
            if ((size == (start + 1)) || (size == (start + 2)) || 
                 (name->elements[start + 1] == AssessorDesc::I_TAB) || 
                 (utf8->elements[origLen - 1] != AssessorDesc::I_END_REF)) {
              doLoop = false; 
            } else {
              const UTF8* componentName = utf8->javaToInternal(hashUTF8,
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
            if ((cur == AssessorDesc::I_BOOL || cur == AssessorDesc::I_BYTE ||
                 cur == AssessorDesc::I_CHAR || cur == AssessorDesc::I_SHORT ||
                 cur == AssessorDesc::I_INT || cur == AssessorDesc::I_FLOAT || 
                 cur == AssessorDesc::I_DOUBLE || cur == AssessorDesc::I_LONG)
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
  return lookupClassFromUTF8(str->value, str->offset, str->count,
                             doResolve, doThrow);
}

UserCommonClass* JnjvmClassLoader::lookupClass(const UTF8* utf8) {
  return classes->lookup(utf8);
}

static UserCommonClass* arrayDup(const UTF8*& name, JnjvmClassLoader* loader) {
  UserClassArray* cl = allocator_new(loader->allocator, UserClassArray)(loader, name);
  return cl;
}

UserClassArray* JnjvmClassLoader::constructArray(const UTF8* name) {
  if (javaLoader != 0) {
    JnjvmClassLoader * ld = 
      ClassArray::arrayLoader(name, this, 1, name->size - 1);
    UserClassArray* res = 
      (ClassArray*)ld->classes->lookupOrCreate(name, this, arrayDup);
    return res;
  } else {
    return (UserClassArray*)classes->lookupOrCreate(name, this, arrayDup);
  }
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

Typedef* JnjvmClassLoader::constructType(const UTF8* name) {
  Typedef* res = javaTypes->lookup(name);
  if (res == 0) {
    res = new Typedef(name, this);
    javaTypes->lock->lock();
    Typedef* tmp = javaTypes->lookup(name);
    if (tmp == 0) javaTypes->hash(name, res);
    else res = tmp;
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
    else res = tmp;
    javaSignatures->lock->unlock();
  }
  return res;
}


JnjvmClassLoader* JnjvmClassLoader::getJnjvmLoaderFromJavaObject(JavaObject* loader) {
  
  if (loader == 0)
    return bootstrapLoader;
  
  Classpath* upcalls = bootstrapLoader->upcalls;
  JnjvmClassLoader* JCL = 
    (JnjvmClassLoader*)(upcalls->vmdataClassLoader->getObjectField(loader));
  
  if (!JCL) {
    JCL = gc_new(JnjvmClassLoader)(*bootstrapLoader, loader, JavaThread::get()->isolate);
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
  /*delete hashUTF8;
  delete javaTypes;
  delete javaSignatures;
  delete TheModuleProvider;*/
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
        char* rp = (char*)malloc(PATH_MAX);
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
            free(rp);
          } else {
            ArrayUInt8* bytes =
              Reader::openFile(this, rp);
            free(rp);
            if (bytes) {
              ZipArchive *archive = new ZipArchive(bytes);
              if (archive) {
                bootArchives.push_back(archive);
              }
            }
          }
        } else {
          free(rp);
        }
      }
      cur = cur + top + 1;
      top = 0;
    }
  }
}

