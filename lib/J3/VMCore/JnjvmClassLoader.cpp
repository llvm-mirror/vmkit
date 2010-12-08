//===-- JnjvmClassLoader.cpp - Jnjvm representation of a class loader ------===//
//
//                          The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <climits>
#include <cstdlib>

// for strrchr
#include <cstring>

// for dlopen and dlsym
#include <dlfcn.h> 

// for stat, S_IFMT and S_IFDIR
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <string>

#include "debug.h"
#include "mvm/Allocator.h"
#include "mvm/VMKit.h"

#include "Classpath.h"
#include "ClasspathReflect.h"
#include "JavaClass.h"
#include "JavaCompiler.h"
#include "JavaConstantPool.h"
#include "JavaString.h"
#include "JavaThread.h"
#include "JavaTypes.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"
#include "JnjvmClassLoader.h"
#include "LockedMap.h"
#include "Reader.h"
#include "Zip.h"

using namespace j3;

typedef void (*static_init_t)(JnjvmClassLoader*);

JnjvmBootstrapLoader::JnjvmBootstrapLoader(mvm::BumpPtrAllocator& Alloc,
																					 Jnjvm* vm,
                                           JavaCompiler* Comp) : 
	JnjvmClassLoader(Alloc, vm) {
  
	TheCompiler = Comp;
  
  hashUTF8 = new(allocator, "UTF8Map") UTF8Map(allocator);
  classes = new(allocator, "ClassMap") ClassMap();
  javaTypes = new(allocator, "TypeMap") TypeMap(); 
  javaSignatures = new(allocator, "SignMap") SignMap();
  strings = new(allocator, "StringList") StringList();
  
  bootClasspathEnv = getenv("JNJVM_BOOTCLASSPATH");
  if (!bootClasspathEnv) {
    bootClasspathEnv = GNUClasspathGlibj;
  }
  
  libClasspathEnv = getenv("JNJVM_LIBCLASSPATH");
  if (!libClasspathEnv) {
    libClasspathEnv = GNUClasspathLibs;
  }
}

JnjvmClassLoader::JnjvmClassLoader(mvm::BumpPtrAllocator& Alloc,
                                   JavaObject* loader,
                                   VMClassLoader* vmdata,
																	 Jnjvm* v) : allocator(Alloc) {
  llvm_gcroot(loader, 0);
  llvm_gcroot(vmdata, 0);

	vm = v;

  TheCompiler = vm->bootstrapLoader->getCompiler()->Create("Applicative loader");
  
  hashUTF8 = new(allocator, "UTF8Map") UTF8Map(allocator);
  classes = new(allocator, "ClassMap") ClassMap();
  javaTypes = new(allocator, "TypeMap") TypeMap();
  javaSignatures = new(allocator, "SignMap") SignMap();
  strings = new(allocator, "StringList") StringList();

  vmdata->JCL = this;
  javaLoader = loader;

  JavaMethod* meth = vm->upcalls->loadInClassLoader;
  loadClassMethod = 
    JavaObject::getClass(loader)->asClass()->lookupMethodDontThrow(
        meth->name, meth->type, false, true, &loadClass);
  assert(loadClass && "Loader does not have a loadClass function");
}

void JnjvmClassLoader::setCompiler(JavaCompiler* Comp) {
  // Set the new compiler.
  TheCompiler = Comp;
}

ClassBytes* JnjvmBootstrapLoader::openName(const UTF8* utf8) {
  ClassBytes* res = 0;
  mvm::ThreadAllocator threadAllocator;

  char* asciiz = (char*)threadAllocator.Allocate(utf8->size + 1);
  for (sint32 i = 0; i < utf8->size; ++i) 
    asciiz[i] = utf8->elements[i];
  asciiz[utf8->size] = 0;
  
  uint32 alen = utf8->size;
  
  for (std::vector<const char*>::iterator i = bootClasspath.begin(),
       e = bootClasspath.end(); i != e; ++i) {
    const char* str = *i;
    unsigned int strLen = strlen(str);
    char* buf = (char*)threadAllocator.Allocate(strLen + alen + 7);

    sprintf(buf, "%s%s.class", str, asciiz);
    res = Reader::openFile(this, buf);
    if (res) return res;
  }

  for (std::vector<ZipArchive*>::iterator i = bootArchives.begin(),
       e = bootArchives.end(); i != e; ++i) {
    
    ZipArchive* archive = *i;
    char* buf = (char*)threadAllocator.Allocate(alen + 7);
    sprintf(buf, "%s.class", asciiz);
    res = Reader::openZip(this, archive, buf);
    if (res) return res;
  }

  return 0;
}


UserClass* JnjvmBootstrapLoader::internalLoad(const UTF8* name,
                                              bool doResolve,
                                              JavaString* strName) {
  ClassBytes* bytes = NULL;
  llvm_gcroot(strName, 0);

  UserCommonClass* cl = lookupClass(name);
  
  if (!cl) {
    bytes = openName(name);
    if (bytes != NULL) {
      cl = constructClass(name, bytes);
    }
  }
  
  if (cl) {
    assert(!cl->isArray());
    if (doResolve) cl->asClass()->resolveClass();
  }

  return (UserClass*)cl;
}

UserClass* JnjvmClassLoader::internalLoad(const UTF8* name, bool doResolve,
                                          JavaString* strName) {
  JavaObject* obj = 0;
  llvm_gcroot(strName, 0);
  llvm_gcroot(obj, 0);
  
  UserCommonClass* cl = lookupClass(name);
  
  if (!cl) {
    UserClass* forCtp = loadClass;
    if (strName == NULL) {
      strName = JavaString::internalToJava(name, vm);
    }
    obj = loadClassMethod->invokeJavaObjectVirtual(forCtp, javaLoader,
                                                   &strName);
    cl = JavaObjectClass::getClass(((JavaObjectClass*)obj));
  }
  
  if (cl) {
    assert(!cl->isArray());
    if (doResolve) cl->asClass()->resolveClass();
  }

  return (UserClass*)cl;
}

UserClass* JnjvmClassLoader::loadName(const UTF8* name, bool doResolve,
                                      bool doThrow, JavaString* strName) {
  
  llvm_gcroot(strName, 0);

  UserClass* cl = internalLoad(name, doResolve, strName);

  if (!cl && doThrow) {
    if (name->equals(vm->upcalls->NoClassDefFoundErrorName)) {
      fprintf(stderr, "Unable to load NoClassDefFoundError");
      abort();
    }
    if (TheCompiler->isStaticCompiling()) {
      fprintf(stderr, "Could not find %s, needed for static compiling\n",
              UTF8Buffer(name).cString());
      abort();
    }
    vm->noClassDefFoundError(name);
  }

  if (cl && cl->classLoader != this) {
    classes->lock.lock();
    ClassMap::iterator End = classes->map.end();
    ClassMap::iterator I = classes->map.find(cl->name);
    if (I == End)
      classes->map.insert(std::make_pair(cl->name, cl));
    classes->lock.unlock();
  }

  return cl;
}


const UTF8* JnjvmClassLoader::lookupComponentName(const UTF8* name,
                                                  UTF8* holder,
                                                  bool& prim) {
  uint32 len = name->size;
  uint32 start = 0;
  uint32 origLen = len;
  
  while (true) {
    --len;
    if (len == 0) {
      return 0;
    } else {
      ++start;
      if (name->elements[start] != I_TAB) {
        if (name->elements[start] == I_REF) {
          uint32 size = (uint32)name->size;
          if ((size == (start + 1)) || (size == (start + 2)) ||
              (name->elements[start + 1] == I_TAB) ||
              (name->elements[origLen - 1] != I_END_REF)) {
            return 0;
          } else {
            const uint16* buf = &(name->elements[start + 1]);
            uint32 bufLen = len - 2;
            const UTF8* componentName = hashUTF8->lookupReader(buf, bufLen);
            if (!componentName && holder) {
              holder->size = len - 2;
              for (uint32 i = 0; i < len - 2; ++i) {
                holder->elements[i] = name->elements[start + 1 + i];
              }
              componentName = holder;
            }
            return componentName;
          }
        } else {
          uint16 cur = name->elements[start];
          if ((cur == I_BOOL || cur == I_BYTE ||
               cur == I_CHAR || cur == I_SHORT ||
               cur == I_INT || cur == I_FLOAT || 
               cur == I_DOUBLE || cur == I_LONG)
              && ((uint32)name->size) == start + 1) {
            prim = true;
          }
          return 0;
        }
      }
    }
  }

  return 0;
}

UserCommonClass* JnjvmClassLoader::lookupClassOrArray(const UTF8* name) {
  UserCommonClass* temp = lookupClass(name);
  if (temp) return temp;

  if (this != vm->bootstrapLoader) {
    temp = vm->bootstrapLoader->lookupClassOrArray(name);
    if (temp) return temp;
  }
  
  
  if (name->elements[0] == I_TAB) {
    bool prim = false;
    const UTF8* componentName = lookupComponentName(name, 0, prim);
    if (prim) return constructArray(name);
    if (componentName) {
      UserCommonClass* temp = lookupClass(componentName);
      if (temp) return constructArray(name);
    }
  }

  return 0;
}

UserCommonClass* JnjvmClassLoader::loadClassFromUserUTF8(const UTF8* name,
                                                         bool doResolve,
                                                         bool doThrow,
                                                         JavaString* strName) {
  llvm_gcroot(strName, 0);
  
  if (name->size == 0) {
    return 0;
  } else if (name->elements[0] == I_TAB) {
    mvm::ThreadAllocator threadAllocator;
    bool prim = false;
    UTF8* holder = (UTF8*)threadAllocator.Allocate(
        sizeof(UTF8) + name->size * sizeof(uint16));
    if (!holder) return 0;
    
    const UTF8* componentName = lookupComponentName(name, holder, prim);
    if (prim) return constructArray(name);
    if (componentName) {
      UserCommonClass* temp = loadName(componentName, doResolve, doThrow, NULL);
      if (temp) return constructArray(name);
    }
  } else {
    return loadName(name, doResolve, doThrow, strName);
  }

  return NULL;
}

UserCommonClass* JnjvmClassLoader::loadClassFromAsciiz(const char* asciiz,
                                                       bool doResolve,
                                                       bool doThrow) {
  const UTF8* name = hashUTF8->lookupAsciiz(asciiz);
  mvm::ThreadAllocator threadAllocator;
  UserCommonClass* result = NULL;
  if (!name) name = vm->bootstrapLoader->hashUTF8->lookupAsciiz(asciiz);
  if (!name) {
    uint32 size = strlen(asciiz);
    UTF8* temp = (UTF8*)threadAllocator.Allocate(
        sizeof(UTF8) + size * sizeof(uint16));
    temp->size = size;

    for (uint32 i = 0; i < size; ++i) {
      temp->elements[i] = asciiz[i];
    }
    name = temp;
  }
  
  result = lookupClass(name);
  if ((result == NULL) && (this != vm->bootstrapLoader)) {
    result = vm->bootstrapLoader->lookupClassOrArray(name);
    if (result != NULL) {
      if (result->isClass() && doResolve) {
        result->asClass()->resolveClass();
      }
      return result;
    }
  }
 
  return loadClassFromUserUTF8(name, doResolve, doThrow, NULL);
}


UserCommonClass* 
JnjvmClassLoader::loadClassFromJavaString(JavaString* str, bool doResolve,
                                          bool doThrow) {
  
  llvm_gcroot(str, 0);
  mvm::ThreadAllocator allocator; 
  UTF8* name = (UTF8*)allocator.Allocate(sizeof(UTF8) + str->count * sizeof(uint16));
 
  name->size = str->count;
  if (ArrayUInt16::getElement(JavaString::getValue(str), str->offset) != I_TAB) {
    for (sint32 i = 0; i < str->count; ++i) {
      uint16 cur = ArrayUInt16::getElement(JavaString::getValue(str), str->offset + i);
      if (cur == '.') name->elements[i] = '/';
      else if (cur == '/') {
        return 0;
      }
      else name->elements[i] = cur;
    }
  } else {
    for (sint32 i = 0; i < str->count; ++i) {
      uint16 cur = ArrayUInt16::getElement(JavaString::getValue(str), str->offset + i);
      if (cur == '.') {
        name->elements[i] = '/';
      } else if (cur == '/') {
        return 0;
      } else {
        name->elements[i] = cur;
      }
    }
  }
    
  UserCommonClass* cls = loadClassFromUserUTF8(name, doResolve, doThrow, str);
  return cls;
}

UserCommonClass* JnjvmClassLoader::lookupClassFromJavaString(JavaString* str) {
  
  const ArrayUInt16* value = NULL;
  llvm_gcroot(str, 0);
  llvm_gcroot(value, 0);
  value = JavaString::getValue(str);
  mvm::ThreadAllocator allocator; 
  
  UTF8* name = (UTF8*)allocator.Allocate(sizeof(UTF8) + str->count * sizeof(uint16));
  name->size = str->count;
  for (sint32 i = 0; i < str->count; ++i) {
    uint16 cur = ArrayUInt16::getElement(value, str->offset + i);
    if (cur == '.') name->elements[i] = '/';
    else name->elements[i] = cur;
  }
  UserCommonClass* cls = lookupClass(name);
  return cls;
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
    return loader->constructArray(arrayName, baseClass);
  } else if (name->elements[start] == I_REF) {
    const UTF8* componentName = name->extract(hashUTF8,
                                              start + 1, start + len - 1);
    UserCommonClass* cl = loadName(componentName, false, true, NULL);
    return cl;
  } else {
    Classpath* upcalls = vm->upcalls;
    UserClassPrimitive* prim = 
      UserClassPrimitive::byteIdToPrimitive(name->elements[start], upcalls);
    assert(prim && "No primitive found");
    return prim;
  }
}


UserClassArray* JnjvmClassLoader::constructArray(const UTF8* name) {
  ClassArray* res = (ClassArray*)lookupClass(name);
  if (res) return res;

  UserCommonClass* cl = loadBaseClass(name, 1, name->size - 1);
  assert(cl && "no base class for an array");
  JnjvmClassLoader* ld = cl->classLoader;
  res = ld->constructArray(name, cl);
  
  if (res && res->classLoader != this) {
    classes->lock.lock();
    ClassMap::iterator End = classes->map.end();
    ClassMap::iterator I = classes->map.find(res->name);
    if (I == End)
      classes->map.insert(std::make_pair(res->name, res));
    classes->lock.unlock();
  }
  return res;
}

UserClass* JnjvmClassLoader::constructClass(const UTF8* name,
                                            ClassBytes* bytes) {
	mvm::gc* excp = NULL;
  llvm_gcroot(excp, 0);
  UserClass* res = NULL;
  lock.lock();
  classes->lock.lock();
  ClassMap::iterator End = classes->map.end();
  ClassMap::iterator I = classes->map.find(name);
  classes->lock.unlock();
  if (I != End) {
    res = ((UserClass*)(I->second));
  } else {
    TRY {
      const UTF8* internalName = readerConstructUTF8(name->elements, name->size);
      res = new(allocator, "Class") UserClass(this, internalName, bytes);
      res->readClass();
      res->makeVT();
      getCompiler()->resolveVirtualClass(res);
      getCompiler()->resolveStaticClass(res);
      classes->lock.lock();
      assert(res->getDelegatee() == NULL);
      assert(res->getStaticInstance() == NULL);
      bool success = classes->map.insert(std::make_pair(internalName, res)).second;
      classes->lock.unlock();
      assert(success && "Could not add class in map");
    } CATCH {
			mvm::Thread* mut = mvm::Thread::get();
      excp = mut->getPendingException();
      mut->clearPendingException();    
    } END_CATCH;
  }
  if (excp != NULL) {
    mvm::Thread::get()->setPendingException(excp)->throwIt();
  }
  lock.unlock();

  if (res->super == NULL) {
    // java.lang.Object just got created, initialise VTs of arrays.
    ClassArray::initialiseVT(res);
  }
  return res;
}

UserClassArray* JnjvmClassLoader::constructArray(const UTF8* name,
                                                 UserCommonClass* baseClass) {
  assert(baseClass && "constructing an array class without a base class");
  assert(baseClass->classLoader == this && 
         "constructing an array with wrong loader");
  classes->lock.lock();
  ClassMap::iterator End = classes->map.end();
  ClassMap::iterator I = classes->map.find(name);
  UserClassArray* res = 0;
  if (I == End) {
    const UTF8* internalName = readerConstructUTF8(name->elements, name->size);
    res = new(allocator, "Array class") UserClassArray(this, internalName,
                                                       baseClass);
    classes->map.insert(std::make_pair(internalName, res));
  } else {
    res = ((UserClassArray*)(I->second));
  }
  classes->lock.unlock();
  return res;
}

Typedef* JnjvmClassLoader::internalConstructType(const UTF8* name) {
  short int cur = name->elements[0];
  Typedef* res = 0;
  switch (cur) {
    case I_TAB :
      res = new(allocator, "ArrayTypedef") ArrayTypedef(name);
      break;
    case I_REF :
      res = new(allocator, "ObjectTypedef") ObjectTypedef(name, hashUTF8);
      break;
    default :
      UserClassPrimitive* cl = 
        vm->upcalls->getPrimitiveClass((char)name->elements[0]);
      assert(cl && "No primitive");
      bool unsign = (cl == vm->upcalls->OfChar || 
                     cl == vm->upcalls->OfBool);
      res = new(allocator, "PrimitiveTypedef") PrimitiveTypedef(name, cl,
                                                                unsign, cur);
  }
  return res;
}


Typedef* JnjvmClassLoader::constructType(const UTF8* name) {
  javaTypes->lock.lock();
  Typedef* res = javaTypes->lookup(name);
  if (res == 0) {
    res = internalConstructType(name);
    javaTypes->hash(name, res);
  }
  javaTypes->lock.unlock();
  return res;
}

static void typeError(const UTF8* name, short int l) {
  if (l != 0) {
    fprintf(stderr, "wrong type %d in %s", l, UTF8Buffer(name).cString());
  } else {
    fprintf(stderr, "wrong type %s", UTF8Buffer(name).cString());
  }
  abort();
}


static bool analyseIntern(const UTF8* name, uint32 pos, uint32 meth,
                          uint32& ret) {
  short int cur = name->elements[pos];
  switch (cur) {
    case I_PARD :
      ret = pos + 1;
      return true;
    case I_BOOL :
      ret = pos + 1;
      return false;
    case I_BYTE :
      ret = pos + 1;
      return false;
    case I_CHAR :
      ret = pos + 1;
      return false;
    case I_SHORT :
      ret = pos + 1;
      return false;
    case I_INT :
      ret = pos + 1;
      return false;
    case I_FLOAT :
      ret = pos + 1;
      return false;
    case I_DOUBLE :
      ret = pos + 1;
      return false;
    case I_LONG :
      ret = pos + 1;
      return false;
    case I_VOID :
      ret = pos + 1;
      return false;
    case I_TAB :
      if (meth == 1) {
        pos++;
      } else {
        while (name->elements[++pos] == I_TAB) {}
        analyseIntern(name, pos, 1, pos);
      }
      ret = pos;
      return false;
    case I_REF :
      if (meth != 2) {
        while (name->elements[++pos] != I_END_REF) {}
      }
      ret = pos + 1;
      return false;
    default :
      typeError(name, cur);
  }
  return false;
}

Signdef* JnjvmClassLoader::constructSign(const UTF8* name) {
  javaSignatures->lock.lock();
  Signdef* res = javaSignatures->lookup(name);
  if (res == 0) {
    std::vector<Typedef*> buf;
    uint32 len = (uint32)name->size;
    uint32 pos = 1;
    uint32 pred = 0;

    while (pos < len) {
      pred = pos;
      bool end = analyseIntern(name, pos, 0, pos);
      if (end) break;
      else {
        buf.push_back(constructType(name->extract(hashUTF8, pred, pos)));
      } 
    }
  
    if (pos == len) {
      typeError(name, 0);
    }
  
    analyseIntern(name, pos, 0, pred);

    if (pred != len) {
      typeError(name, 0);
    }
    
    Typedef* ret = constructType(name->extract(hashUTF8, pos, pred));
    
    res = new(allocator, buf.size()) Signdef(name, this, buf, ret);

    javaSignatures->hash(name, res);
  }
  javaSignatures->lock.unlock();
  return res;
}


JnjvmClassLoader*
JnjvmClassLoader::getJnjvmLoaderFromJavaObject(JavaObject* jloader, Jnjvm* vm) {
  
  VMClassLoader* vmdata = 0;
  
  llvm_gcroot(jloader, 0);
  llvm_gcroot(vmdata, 0);
  
  if (jloader == NULL) return vm->bootstrapLoader;
 
  JnjvmClassLoader* JCL = 0;
  Classpath* upcalls = vm->upcalls;
  vmdata = 
    (VMClassLoader*)(upcalls->vmdataClassLoader->getInstanceObjectField(jloader));

  if (vmdata == NULL) {
    JavaObject::acquire(jloader);
    vmdata = 
      (VMClassLoader*)(upcalls->vmdataClassLoader->getInstanceObjectField(jloader));
    if (!vmdata) {
      vmdata = VMClassLoader::allocate(vm);
      mvm::BumpPtrAllocator* A = new mvm::BumpPtrAllocator();
      JCL = new(*A, "Class loader") JnjvmClassLoader(*A, jloader, vmdata, vm);
      upcalls->vmdataClassLoader->setInstanceObjectField(jloader, (JavaObject*)vmdata);
    }
    JavaObject::release(jloader);
  } else {
    JCL = vmdata->getClassLoader();
    assert(JCL->javaLoader == jloader);
  }

  return JCL;
}

const UTF8* JnjvmClassLoader::asciizConstructUTF8(const char* asciiz) {
  return hashUTF8->lookupOrCreateAsciiz(asciiz);
}

const UTF8* JnjvmClassLoader::readerConstructUTF8(const uint16* buf,
                                                  uint32 size) {
  return hashUTF8->lookupOrCreateReader(buf, size);
}

JnjvmClassLoader::~JnjvmClassLoader() {

	mvm::Thread::get()->vmkit->removeMethodInfos(TheCompiler);

  if (classes) {
    classes->~ClassMap();
    allocator.Deallocate(classes);
  }

  if (hashUTF8) {
    hashUTF8->~UTF8Map();
    allocator.Deallocate(hashUTF8);
  }

  if (javaTypes) {
    javaTypes->~TypeMap();
    allocator.Deallocate(javaTypes);
  }

  if (javaSignatures) {
    javaSignatures->~SignMap();
    allocator.Deallocate(javaSignatures);
  }

  for (std::vector<void*>::iterator i = nativeLibs.begin(); 
       i < nativeLibs.end(); ++i) {
    dlclose(*i);
  }

  delete TheCompiler;

  // Don't delete the allocator. The caller of this method must
  // delete it after the current object is deleted.
}


JnjvmBootstrapLoader::~JnjvmBootstrapLoader() {
}

JavaString** JnjvmClassLoader::UTF8ToStr(const UTF8* val) {
  JavaString* res = NULL;
  llvm_gcroot(res, 0);
  res = vm->internalUTF8ToStr(val);
  return strings->addString(this, res);
}

void JnjvmBootstrapLoader::analyseClasspathEnv(const char* str) {
  ClassBytes* bytes = NULL;
  mvm::ThreadAllocator threadAllocator;
  if (str != 0) {
    unsigned int len = strlen(str);
    char* buf = (char*)threadAllocator.Allocate((len + 1) * sizeof(char));
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
        char* rp = (char*)threadAllocator.Allocate(PATH_MAX);
        memset(rp, 0, PATH_MAX);
        rp = realpath(buf, rp);
        if (rp && rp[PATH_MAX - 1] == 0 && strlen(rp) != 0) {
          struct stat st;
          stat(rp, &st);
          if ((st.st_mode & S_IFMT) == S_IFDIR) {
            unsigned int len = strlen(rp);
            char* temp = (char*)allocator.Allocate(len + 2, "Boot classpath");
            memcpy(temp, rp, len);
            temp[len] = Jnjvm::dirSeparator[0];
            temp[len + 1] = 0;
            bootClasspath.push_back(temp);
          } else {
            bytes = Reader::openFile(this, rp);
            if (bytes) {
              ZipArchive *archive = new(allocator, "ZipArchive")
                ZipArchive(bytes, allocator);
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

// constructArrayName can allocate the UTF8 directly in the classloader
// memory because it is called by safe places, ie only valid names are
// created.
const UTF8* JnjvmClassLoader::constructArrayName(uint32 steps,
                                                 const UTF8* className) {
  uint32 len = className->size;
  uint32 pos = steps;
  bool isTab = (className->elements[0] == I_TAB ? true : false);
  uint32 n = steps + len + (isTab ? 0 : 2);
  mvm::ThreadAllocator allocator;
  uint16* buf = (uint16*)allocator.Allocate(n * sizeof(uint16));
    
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

  const UTF8* res = readerConstructUTF8(buf, n);
  return res;
}

intptr_t JnjvmClassLoader::loadInLib(const char* buf, bool& j3) {
  uintptr_t res = (uintptr_t)TheCompiler->loadMethod(SELF_HANDLE, buf);
  
  if (!res) {
    for (std::vector<void*>::iterator i = nativeLibs.begin(),
              e = nativeLibs.end(); i!= e; ++i) {
      res = (uintptr_t)TheCompiler->loadMethod((*i), buf);
      if (res) break;
    }
  } else {
    j3 = true;
  }
  
  if (!res && this != vm->bootstrapLoader)
    res = vm->bootstrapLoader->loadInLib(buf, j3);

  return (intptr_t)res;
}

void* JnjvmClassLoader::loadLib(const char* buf) {
  void* handle = dlopen(buf, RTLD_LAZY | RTLD_LOCAL);
  if (handle) nativeLibs.push_back(handle);
  return handle;
}

intptr_t JnjvmClassLoader::loadInLib(const char* name, void* handle) {
  return (intptr_t)TheCompiler->loadMethod(handle, name);
}

intptr_t JnjvmClassLoader::nativeLookup(JavaMethod* meth, bool& j3,
                                        char* buf) {

  meth->jniConsFromMeth(buf);
  intptr_t res = loadInLib(buf, j3);
  if (!res) {
    meth->jniConsFromMethOverloaded(buf);
    res = loadInLib(buf, j3);
  }
  return res;
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

void JnjvmClassLoader::insertAllMethodsInVM() {
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
          vm->vmkit->FunctionsCache.addMethodInfo(MI, meth.code);
        }
      }
      
      for (uint32 i = 0; i < C->nbStaticMethods; ++i) {
        JavaMethod& meth = C->staticMethods[i];
        if (!isAbstract(meth.access) && meth.code) {
          JavaStaticMethodInfo* MI = new (allocator, "JavaStaticMethodInfo")
            JavaStaticMethodInfo(0, meth.code, &meth);
          vm->vmkit->FunctionsCache.addMethodInfo(MI, meth.code);
        }
      }
    }
  }
}

void JnjvmClassLoader::loadLibFromJar(const char* name,
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
      insertAllMethodsInVM();
    }
  }
}

void JnjvmClassLoader::loadLibFromFile(const char* name) {
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
      insertAllMethodsInVM();
    }
  }
}

Class* JnjvmClassLoader::loadClassFromSelf(const char* name) {
  assert(classes->map.size() == 0);
  Class* cl = (Class*)dlsym(SELF_HANDLE, name);
  if (cl) {
    static_init_t init = (static_init_t)(uintptr_t)cl->classLoader;
    init(this);
    insertAllMethodsInVM();
  }
  return cl;
}


// Extern "C" functions called by the vmjc static intializer.
extern "C" void vmjcAddPreCompiledClass(JnjvmClassLoader* JCL,
                                        CommonClass* cl) {
  cl->classLoader = JCL;
	cl->virtualVT->vm = JCL->vm;
  
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

	if (!cl->isPrimitive())
	  JCL->getClasses()->map.insert(std::make_pair(cl->name, cl));

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

VMClassLoader* VMClassLoader::allocate(Jnjvm *vm) {
	VMClassLoader* res = 0;
	llvm_gcroot(res, 0);
	res = (VMClassLoader*)gc::operator new(sizeof(VMClassLoader), vm->VMClassLoader__VT);
	return res;
}

bool VMClassLoader::isVMClassLoader(Jnjvm *vm, JavaObject* obj) {
	llvm_gcroot(obj, 0);
	// not safe: must verify that obj belongs to a jvm
	return obj->getVirtualTable() == vm->VMClassLoader__VT;
}

extern "C" void staticCallback() {
  fprintf(stderr, "Implement me");
  abort();
}
