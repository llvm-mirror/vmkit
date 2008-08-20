//===-- JnjvmClassLoader.cpp - Jnjvm representation of a class loader ------===//
//
//                              Jnjvm
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <sys/stat.h>

#include "debug.h"

#include "JavaAllocator.h"
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

#ifdef MULTIPLE_VM
JnjvmClassLoader* JnjvmClassLoader::sharedLoader = 0;
#endif

extern const char* GNUClasspathGlibj;
extern const char* GNUClasspathLibs;

JnjvmBootstrapLoader* JnjvmBootstrapLoader::createBootstrapLoader() {
  
  JnjvmBootstrapLoader* JCL = gc_new(JnjvmBootstrapLoader)();
  JCL->TheModule = new JnjvmModule("Bootstrap JnJVM");
  JCL->TheModuleProvider = new JnjvmModuleProvider(JCL->TheModule);
  JCL->TheModule->initialise(); 
  
  JCL->allocator = new JavaAllocator();
  
  JCL->hashUTF8 = new UTF8Map(JCL->allocator);
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

void JnjvmClassLoader::readParents(Class* cl, Reader& reader) {
  JavaCtpInfo* ctpInfo = cl->ctpInfo;
  unsigned short int superEntry = reader.readU2();
  const UTF8* super = superEntry ? 
        ctpInfo->resolveClassName(superEntry) : 0;

  unsigned short int nbI = reader.readU2();
  cl->superUTF8 = super;
  for (int i = 0; i < nbI; i++)
    cl->interfacesUTF8.push_back(ctpInfo->resolveClassName(reader.readU2()));

}

void JnjvmClassLoader::loadParents(Class* cl) {
  const UTF8* super = cl->superUTF8;
  int nbI = cl->interfacesUTF8.size();
  if (super == 0) {
    cl->depth = 0;
    cl->display = (CommonClass**)malloc(sizeof(CommonClass*));
    cl->display[0] = cl;
    cl->virtualTableSize = VT_SIZE / sizeof(void*);
  } else {
    cl->super = loadName(super, true, false, true);
    int depth = cl->super->depth + 1;
    cl->depth = depth;
    cl->virtualTableSize = cl->super->virtualTableSize;
    cl->display = (CommonClass**)malloc((depth + 1) * sizeof(CommonClass*));
    memcpy(cl->display, cl->super->display, depth * sizeof(CommonClass*));
    cl->display[cl->depth] = cl;
  }

  for (int i = 0; i < nbI; i++)
    cl->interfaces.push_back((Class*)loadName(cl->interfacesUTF8[i],
                                              true, false, true));
}

void JnjvmClassLoader::readAttributs(Class* cl, Reader& reader,
                           std::vector<Attribut*>& attr) {
  JavaCtpInfo* ctpInfo = cl->ctpInfo;
  unsigned short int nba = reader.readU2();
  
  for (int i = 0; i < nba; i++) {
    const UTF8* attName = ctpInfo->UTF8At(reader.readU2());
    uint32 attLen = reader.readU4();
    Attribut* att = new Attribut(attName, attLen, reader.cursor);
    attr.push_back(att);
    reader.seek(attLen, Reader::SeekCur);
  }
}

void JnjvmClassLoader::readFields(Class* cl, Reader& reader) {
  uint16 nbFields = reader.readU2();
  JavaCtpInfo* ctpInfo = cl->ctpInfo;
  uint32 sindex = 0;
  uint32 vindex = 0;
  for (int i = 0; i < nbFields; i++) {
    uint16 access = reader.readU2();
    const UTF8* name = ctpInfo->UTF8At(reader.readU2());
    const UTF8* type = ctpInfo->UTF8At(reader.readU2());
    JavaField* field = cl->constructField(name, type, access);
    isStatic(access) ?
      field->num = sindex++ :
      field->num = vindex++;
    readAttributs(cl, reader, field->attributs);
  }
}

void JnjvmClassLoader::readMethods(Class* cl, Reader& reader) {
  uint16 nbMethods = reader.readU2();
  JavaCtpInfo* ctpInfo = cl->ctpInfo;
  for (int i = 0; i < nbMethods; i++) {
    uint16 access = reader.readU2();
    const UTF8* name = ctpInfo->UTF8At(reader.readU2());
    const UTF8* type = ctpInfo->UTF8At(reader.readU2());
    JavaMethod* meth = cl->constructMethod(name, type, access);
    readAttributs(cl, reader, meth->attributs);
  }
}

void JnjvmClassLoader::readClass(Class* cl) {

  PRINT_DEBUG(JNJVM_LOAD, 0, COLOR_NORMAL, "; ", 0);
  PRINT_DEBUG(JNJVM_LOAD, 0, LIGHT_GREEN, "reading ", 0);
  PRINT_DEBUG(JNJVM_LOAD, 0, COLOR_NORMAL, "%s::%s\n", printString(),
              cl->printString());

  Reader reader(cl->bytes);
  uint32 magic = reader.readU4();
  if (magic != Jnjvm::Magic) {
    JavaThread::get()->isolate->classFormatError("bad magic number %p", magic);
  }
  cl->minor = reader.readU2();
  cl->major = reader.readU2();
  JavaCtpInfo::read(cl, reader);
  JavaCtpInfo* ctpInfo = cl->ctpInfo;
  cl->access = reader.readU2();
  
  const UTF8* thisClassName = 
    ctpInfo->resolveClassName(reader.readU2());
  
  if (!(thisClassName->equals(cl->name))) {
    JavaThread::get()->isolate->classFormatError("try to load %s and found class named %s",
          cl->printString(), thisClassName->printString());
  }

  readParents(cl, reader);
  readFields(cl, reader);
  readMethods(cl, reader);
  readAttributs(cl, reader, cl->attributs);
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


void JnjvmClassLoader::resolveClass(CommonClass* cl, bool doClinit) {
  if (cl->status < resolved) {
    cl->acquire();
    int status = cl->status;
    if (status >= resolved) {
      cl->release();
    } else if (status <  loaded) {
      cl->release();
      JavaThread::get()->isolate->unknownError("try to resolve a not-read class");
    } else if (status == loaded || cl->ownerClass()) {
      if (cl->isArray) {
        ClassArray* arrayCl = (ClassArray*)cl;
        CommonClass* baseClass =  arrayCl->baseClass();
        baseClass->resolveClass(doClinit);
        cl->status = resolved;
      } else {
        readClass((Class*)cl);
        cl->status = classRead;
        cl->release();
        loadParents((Class*)cl);
        cl->acquire(); 
        cl->status = prepared;
        TheModule->resolveVirtualClass((Class*)cl);
        cl->status = resolved;
      }
      cl->release();
      cl->broadcastClass();
    } else {
      while (status < resolved) {
        cl->waitClass();
      }
      cl->release();
    }
  }
  if (doClinit) cl->initialiseClass();
}

CommonClass* JnjvmBootstrapLoader::internalLoad(const UTF8* name) {
  
  CommonClass* cl = lookupClass(name);
  
  if (!cl) {
    ArrayUInt8* bytes = openName(name);
    if (bytes)
      cl = constructClass(name, bytes);
  } else if (cl->status == hashed) {
    ArrayUInt8* bytes = openName(name);
    if (bytes) {
      if (cl->status == hashed) {
        cl->acquire();
        if (cl->status == hashed) {
          cl->status = loaded;
          ((Class*)cl)->bytes = bytes;
        }
        cl->release();
      }
    } else {
      cl = 0;
    }
  }

  return cl;
}

CommonClass* JnjvmClassLoader::internalLoad(const UTF8* name) {
  CommonClass* cl = lookupClass(name);
  
  if (!cl || cl->status == hashed) {
    const UTF8* javaName = name->internalToJava(hashUTF8, 0, name->size);
    JavaString* str = isolate->UTF8ToStr(javaName);
    JavaObject* obj = (JavaObject*)
      Classpath::loadInClassLoader->invokeJavaObjectVirtual(isolate, javaLoader,
                                                          str);
    cl = (CommonClass*)(Classpath::vmdataClass->getVirtualObjectField(obj));
  }
  
  return cl;
}

CommonClass* JnjvmClassLoader::loadName(const UTF8* name, bool doResolve,
                                        bool doClinit, bool doThrow) {
 

  CommonClass* cl = internalLoad(name);

  if (!cl && doThrow) {
    if (!(name->equals(Jnjvm::NoClassDefFoundError))) {
      JavaThread::get()->isolate->unknownError("Unable to load NoClassDefFoundError");
    }
    JavaThread::get()->isolate->noClassDefFoundError("unable to load %s", name->printString());
  }

  if (cl && doResolve) cl->resolveClass(doClinit);

  return cl;
}

CommonClass* JnjvmClassLoader::lookupClassFromUTF8(const UTF8* utf8, unsigned int start,
                                         unsigned int len,
                                         bool doResolve, bool doClinit,
                                         bool doThrow) {
  uint32 origLen = len;
  const UTF8* name = utf8->javaToInternal(hashUTF8, start, len);
  bool doLoop = true;
  CommonClass* ret = 0;

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
              if (loadName(componentName, doResolve, doClinit,
                           doThrow)) {
                ret = constructArray(name);
                if (doResolve) ret->resolveClass(doClinit);
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
              ret->resolveClass(doClinit);
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
    return loadName(name, doResolve, doClinit, doThrow);
  }
}

CommonClass* JnjvmClassLoader::lookupClassFromJavaString(JavaString* str,
                                              bool doResolve, bool doClinit,
                                              bool doThrow) {
  return lookupClassFromUTF8(str->value, str->offset, str->count,
                             doResolve, doClinit, doThrow);
}

CommonClass* JnjvmClassLoader::lookupClass(const UTF8* utf8) {
  return classes->lookup(utf8);
}

static CommonClass* arrayDup(const UTF8*& name, JnjvmClassLoader* loader) {
  ClassArray* cl = allocator_new(loader->allocator, ClassArray)(loader, name);
  return cl;
}

ClassArray* JnjvmClassLoader::constructArray(const UTF8* name) {
  if (javaLoader != 0) {
    JnjvmClassLoader * ld = ClassArray::arrayLoader(name, this, 1, name->size - 1);
    ClassArray* res = (ClassArray*)ld->classes->lookupOrCreate(name, this, arrayDup);
    return res;
  } else {
    return (ClassArray*)classes->lookupOrCreate(name, this, arrayDup);
  }
}


Class* JnjvmClassLoader::constructClass(const UTF8* name, ArrayUInt8* bytes) {
  classes->lock->lock();
  ClassMap::iterator End = classes->map.end();
  ClassMap::iterator I = classes->map.find(name);
  Class* res = 0;
  if (I == End) {
    res = allocator_new(allocator, Class)(this, name);
    if (bytes) {
      res->bytes = bytes;
      res->status = loaded;
    }
    classes->map.insert(std::make_pair(name, res));
  } else {
    res = ((Class*)(I->second));
    if (res->status == hashed && bytes) {
      res->bytes = bytes;
      res->status = loaded;
    }
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

  JnjvmClassLoader* JCL = 
      (JnjvmClassLoader*)(Classpath::vmdataClassLoader->getVirtualObjectField(loader));
  
  if (!JCL) {
    JCL = gc_new(JnjvmClassLoader)(*bootstrapLoader, loader, JavaThread::get()->isolate);
    (Classpath::vmdataClassLoader->setVirtualObjectField(loader, (JavaObject*)JCL));
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

