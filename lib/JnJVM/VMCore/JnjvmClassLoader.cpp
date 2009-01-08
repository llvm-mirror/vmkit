//===-- JnjvmClassLoader.cpp - Jnjvm representation of a class loader ------===//
//
//                              Jnjvm
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <climits>
#include <cstdlib>
#include <cstring>

// for dlopen and dlsym
#include <dlfcn.h> 

// for stat, S_IFMT and S_IFDIR
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>



#if defined(__MACH__)
#define SELF_HANDLE RTLD_DEFAULT
#define BOOTLIBNAME "libvmjc.dylib"
#else
#define SELF_HANDLE 0
#define BOOTLIBNAME "libvmjc.so"
#endif

#include "debug.h"

#include "mvm/Allocator.h"

#include "Classpath.h"
#include "ClasspathReflect.h"
#include "JavaClass.h"
#include "JavaConstantPool.h"
#include "JavaString.h"
#include "JavaThread.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"
#include "JnjvmClassLoader.h"
#include "JnjvmModule.h"
#include "JnjvmModuleProvider.h"
#include "LockedMap.h"
#include "Reader.h"
#include "Zip.h"

using namespace jnjvm;

ClassArray ArrayOfBool;
ClassArray ArrayOfByte;
ClassArray ArrayOfChar;
ClassArray ArrayOfShort;
ClassArray ArrayOfInt;
ClassArray ArrayOfFloat;
ClassArray ArrayOfDouble;
ClassArray ArrayOfLong;

typedef void (*static_init_t)(JnjvmClassLoader*);

JnjvmBootstrapLoader::JnjvmBootstrapLoader(bool staticCompilation) {
  
  TheModule = new JnjvmModule("Bootstrap JnJVM", staticCompilation);
  TheModuleProvider = new JnjvmModuleProvider(getModule());
  FunctionPasses = new FunctionPassManager(TheModuleProvider);
  FunctionPasses->add(new TargetData(TheModule));

  hashUTF8 = new(allocator) UTF8Map(allocator, 0);
  classes = new(allocator) ClassMap();
  javaTypes = new(allocator) TypeMap(); 
  javaSignatures = new(allocator) SignMap(); 
  
  bootClasspathEnv = getenv("JNJVM_BOOTCLASSPATH");
  if (!bootClasspathEnv) {
    bootClasspathEnv = GNUClasspathGlibj;
  }
  
  libClasspathEnv = getenv("JNJVM_LIBCLASSPATH");
  if (!libClasspathEnv) {
    libClasspathEnv = GNUClasspathLibs;
  }
  
  analyseClasspathEnv(bootClasspathEnv);
  
  upcalls = new(allocator) Classpath();
  bootstrapLoader = this;
  
  // Allocate interfaces.
  InterfacesArray = 
    (Class**)allocator.Allocate(2 * sizeof(UserClass*));
  
  ClassArray::InterfacesArray = 
    (Class**)allocator.Allocate(2 * sizeof(UserClass*));
  
  // Create the primitive classes.
  upcalls->OfChar = UPCALL_PRIMITIVE_CLASS(this, "char", 2);
  upcalls->OfBool = UPCALL_PRIMITIVE_CLASS(this, "boolean", 1);
  upcalls->OfShort = UPCALL_PRIMITIVE_CLASS(this, "short", 2);
  upcalls->OfInt = UPCALL_PRIMITIVE_CLASS(this, "int", 4);
  upcalls->OfLong = UPCALL_PRIMITIVE_CLASS(this, "long", 8);
  upcalls->OfFloat = UPCALL_PRIMITIVE_CLASS(this, "float", 4);
  upcalls->OfDouble = UPCALL_PRIMITIVE_CLASS(this, "double", 8);
  upcalls->OfVoid = UPCALL_PRIMITIVE_CLASS(this, "void", 0);
  upcalls->OfByte = UPCALL_PRIMITIVE_CLASS(this, "byte", 1);
  
  // Create the primitive arrays.
  upcalls->ArrayOfChar = constructPrimitiveArray(ArrayOfChar,
                                                 asciizConstructUTF8("[C"),
                                                 upcalls->OfChar);

  upcalls->ArrayOfByte = constructPrimitiveArray(ArrayOfByte,
                                                 asciizConstructUTF8("[B"),
                                                 upcalls->OfByte);
  
  upcalls->ArrayOfInt = constructPrimitiveArray(ArrayOfInt,
                                                asciizConstructUTF8("[I"),
                                                upcalls->OfInt);
  
  upcalls->ArrayOfBool = constructPrimitiveArray(ArrayOfBool,
                                                 asciizConstructUTF8("[Z"),
                                                 upcalls->OfBool);
  
  upcalls->ArrayOfLong = constructPrimitiveArray(ArrayOfLong,
                                                 asciizConstructUTF8("[J"),
                                                 upcalls->OfLong);
  
  upcalls->ArrayOfFloat = constructPrimitiveArray(ArrayOfFloat,
                                                  asciizConstructUTF8("[F"),
                                                  upcalls->OfFloat);
  
  upcalls->ArrayOfDouble = constructPrimitiveArray(ArrayOfDouble,
                                                   asciizConstructUTF8("[D"),
                                                   upcalls->OfDouble);
  
  upcalls->ArrayOfShort = constructPrimitiveArray(ArrayOfShort,
                                                  asciizConstructUTF8("[S"),
                                                  upcalls->OfShort);
  
  // Fill the maps.
  primitiveMap[I_VOID] = upcalls->OfVoid;
  primitiveMap[I_BOOL] = upcalls->OfBool;
  primitiveMap[I_BYTE] = upcalls->OfByte;
  primitiveMap[I_CHAR] = upcalls->OfChar;
  primitiveMap[I_SHORT] = upcalls->OfShort;
  primitiveMap[I_INT] = upcalls->OfInt;
  primitiveMap[I_FLOAT] = upcalls->OfFloat;
  primitiveMap[I_LONG] = upcalls->OfLong;
  primitiveMap[I_DOUBLE] = upcalls->OfDouble;

  arrayTable[JavaArray::T_BOOLEAN - 4] = upcalls->ArrayOfBool;
  arrayTable[JavaArray::T_BYTE - 4] = upcalls->ArrayOfByte;
  arrayTable[JavaArray::T_CHAR - 4] = upcalls->ArrayOfChar;
  arrayTable[JavaArray::T_SHORT - 4] = upcalls->ArrayOfShort;
  arrayTable[JavaArray::T_INT - 4] = upcalls->ArrayOfInt;
  arrayTable[JavaArray::T_FLOAT - 4] = upcalls->ArrayOfFloat;
  arrayTable[JavaArray::T_LONG - 4] = upcalls->ArrayOfLong;
  arrayTable[JavaArray::T_DOUBLE - 4] = upcalls->ArrayOfDouble;


  // Now that native types have been loaded, try to find if we have a
  // pre-compiled rt.jar
  nativeHandle = dlopen(BOOTLIBNAME, RTLD_LAZY | RTLD_GLOBAL);
  if (nativeHandle) {
    // Found it!
    SuperArray = (Class*)dlsym(nativeHandle, "java.lang.Object");
    
    if (SuperArray) {
      ClassArray::SuperArray = (Class*)SuperArray->getInternal();
      // We have the java/lang/Object class, execute the static initializer.
      static_init_t init = (static_init_t)(uintptr_t)SuperArray->classLoader;
      assert(init && "Loaded the wrong boot library");
      init(this);
    } 
  }
    
  // We haven't found a pre-compiled rt.jar, load the root class ourself.
  if (!SuperArray) {
    
    SuperArray = loadName(asciizConstructUTF8("java/lang/Object"), false,
                          false);
    ClassArray::SuperArray = (Class*)SuperArray->getInternal();
    
  }
  
  // Set the super of array classes.
#define SET_PARENT(CLASS) \
  CLASS.setSuper(SuperArray); \

  SET_PARENT(ArrayOfBool)
  SET_PARENT(ArrayOfByte)
  SET_PARENT(ArrayOfChar)
  SET_PARENT(ArrayOfShort)
  SET_PARENT(ArrayOfInt)
  SET_PARENT(ArrayOfFloat)
  SET_PARENT(ArrayOfDouble)
  SET_PARENT(ArrayOfLong)
#undef SET_PARENT
  
  // Initialize interfaces of array classes.
  InterfacesArray[0] = loadName(asciizConstructUTF8("java/lang/Cloneable"),
                                false, false);
  
  InterfacesArray[1] = loadName(asciizConstructUTF8("java/io/Serializable"),
                                false, false);
  
  ClassArray::InterfacesArray[0] = (Class*)InterfacesArray[0]->getInternal();
  ClassArray::InterfacesArray[1] = (Class*)(InterfacesArray[1]->getInternal()); 
 
  // Load array classes that JnJVM internally uses.
  upcalls->ArrayOfString = 
    constructArray(asciizConstructUTF8("[Ljava/lang/String;"));
  
  upcalls->ArrayOfObject = 
    constructArray(asciizConstructUTF8("[Ljava/lang/Object;"));
  
 
  Attribut::codeAttribut = asciizConstructUTF8("Code");
  Attribut::exceptionsAttribut = asciizConstructUTF8("Exceptions");
  Attribut::constantAttribut = asciizConstructUTF8("ConstantValue");
  Attribut::lineNumberTableAttribut = asciizConstructUTF8("LineNumberTable");
  Attribut::innerClassesAttribut = asciizConstructUTF8("InnerClasses");
  Attribut::sourceFileAttribut = asciizConstructUTF8("SourceFile");
  
  initName = asciizConstructUTF8("<init>");
  clinitName = asciizConstructUTF8("<clinit>");
  clinitType = asciizConstructUTF8("()V");
  runName = asciizConstructUTF8("run");
  prelib = asciizConstructUTF8("lib");
#if defined(__MACH__)
  postlib = asciizConstructUTF8(".dylib");
#else 
  postlib = asciizConstructUTF8(".so");
#endif
  mathName = asciizConstructUTF8("java/lang/Math");
  stackWalkerName = asciizConstructUTF8("gnu/classpath/VMStackWalker");
  NoClassDefFoundError = asciizConstructUTF8("java/lang/NoClassDefFoundError");

#define DEF_UTF8(var) \
  var = asciizConstructUTF8(#var)
  
  DEF_UTF8(abs);
  DEF_UTF8(sqrt);
  DEF_UTF8(sin);
  DEF_UTF8(cos);
  DEF_UTF8(tan);
  DEF_UTF8(asin);
  DEF_UTF8(acos);
  DEF_UTF8(atan);
  DEF_UTF8(atan2);
  DEF_UTF8(exp);
  DEF_UTF8(log);
  DEF_UTF8(pow);
  DEF_UTF8(ceil);
  DEF_UTF8(floor);
  DEF_UTF8(rint);
  DEF_UTF8(cbrt);
  DEF_UTF8(cosh);
  DEF_UTF8(expm1);
  DEF_UTF8(hypot);
  DEF_UTF8(log10);
  DEF_UTF8(log1p);
  DEF_UTF8(sinh);
  DEF_UTF8(tanh);
  DEF_UTF8(finalize);

#undef DEF_UTF8
  
}

JnjvmClassLoader::JnjvmClassLoader(JnjvmClassLoader& JCL, JavaObject* loader,
                                   Jnjvm* I) {
  TheModule = new JnjvmModule("Applicative loader");
  TheModuleProvider = new JnjvmModuleProvider(getModule());
  bootstrapLoader = JCL.bootstrapLoader;
  FunctionPasses = bootstrapLoader->FunctionPasses;
  
  hashUTF8 = new(allocator) UTF8Map(allocator,
                                    bootstrapLoader->upcalls->ArrayOfChar);
  classes = new(allocator) ClassMap();
  javaTypes = new(allocator) TypeMap();
  javaSignatures = new(allocator) SignMap();

  javaLoader = loader;
  isolate = I;

  JavaMethod* meth = bootstrapLoader->upcalls->loadInClassLoader;
  loader->classOf->asClass()->lookupMethodDontThrow(meth->name, meth->type,
                                                    false, true, &loadClass);
  assert(loadClass && "Loader does not have a loadClass function");

#if defined(SERVICE)
  /// If the appClassLoader is already set in the isolate, then we need
  /// a new one each time a class loader is allocated.
  if (isolate->appClassLoader) {
    isolate = gc_new(Jnjvm)(bootstrapLoader);
    isolate->memoryLimit = 4000000;
    isolate->threadLimit = 10;
    isolate->parent = I->parent;
    isolate->CU = this;
    mvm::Thread* th = mvm::Thread::get();
    mvm::VirtualMachine* OldVM = th->MyVM;
    th->MyVM = isolate;
    th->IsolateID = isolate->IsolateID;
    
    isolate->loadBootstrap();
    
    th->MyVM = OldVM;
    th->IsolateID = OldVM->IsolateID;
  }
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
    const UTF8* javaName = name->internalToJava(isolate, 0, name->size);
    JavaString* str = isolate->UTF8ToStr(javaName);
    Classpath* upcalls = bootstrapLoader->upcalls;
    UserClass* forCtp = loadClass;
    JavaObject* obj = (JavaObject*)
      upcalls->loadInClassLoader->invokeJavaObjectVirtual(isolate, forCtp,
                                                          javaLoader, str);
    cl = (UserCommonClass*)((JavaObjectClass*)obj)->getClass();
  }
  
  if (cl) assert(!cl->isArray());
  return (UserClass*)cl;
}

UserClass* JnjvmClassLoader::loadName(const UTF8* name, bool doResolve,
                                        bool doThrow) {
 

  UserClass* cl = internalLoad(name);

  if (!cl && doThrow) {
    Jnjvm* vm = JavaThread::get()->getJVM();
    if (name->equals(bootstrapLoader->NoClassDefFoundError)) {
      vm->unknownError("Unable to load NoClassDefFoundError");
    }
    vm->noClassDefFoundError(name);
  }

  if (cl && doResolve) cl->resolveClass();

  return cl;
}

UserCommonClass* JnjvmClassLoader::lookupClassFromUTF8(const UTF8* name,
                                                       Jnjvm* vm,
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
              const UTF8* componentName = name->javaToInternal(vm, start + 1,
                                                               len - 2);
              if (loadName(componentName, doResolve, doThrow)) {
                ret = constructArray(name);
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

UserCommonClass* JnjvmClassLoader::lookupClassOrArray(const UTF8* name) {
  UserCommonClass* temp = lookupClass(name);
  if (temp) return temp;

  if (this != bootstrapLoader) {
    temp = bootstrapLoader->lookupClassOrArray(name);

    if (temp) return temp;
  }

  uint32 len = name->size;
  uint32 start = 0;
  uint32 origLen = len;
  bool doLoop = true;

  if (name->elements[0] == I_TAB) {
    
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
              const UTF8* componentName = name->javaToInternal(isolate,
                                                               start + 1,
                                                               len - 2);
              if (lookupClassOrArray(componentName)) {
                temp = constructArray(name);
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

              temp = constructArray(name);
              doLoop = false;
            } else {
              doLoop = false;
            }
          }
        }
      }
    }
  }

  return temp;
}


UserCommonClass* 
JnjvmClassLoader::lookupClassFromJavaString(JavaString* str, Jnjvm* vm,
                                            bool doResolve, bool doThrow) {
  
  const UTF8* name = 0;
  
  if (str->value->elements[str->offset] != I_TAB)
    name = str->value->checkedJavaToInternal(vm, str->offset, str->count);
  else
    name = str->value->javaToInternal(vm, str->offset, str->count);

  if (name)
    return lookupClassFromUTF8(name, vm, doResolve, doThrow);

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

UserClass* JnjvmClassLoader::constructClass(const UTF8* name,
                                            ArrayUInt8* bytes) {
  assert(bytes && "constructing a class without bytes");
  classes->lock.lock();
  ClassMap::iterator End = classes->map.end();
  ClassMap::iterator I = classes->map.find(name);
  UserClass* res = 0;
  if (I == End) {
    const UTF8* internalName = readerConstructUTF8(name->elements, name->size);
    res = new(allocator) UserClass(this, internalName, bytes);
    classes->map.insert(std::make_pair(internalName, res));
  } else {
    res = ((UserClass*)(I->second));
  }
  classes->lock.unlock();
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
    res = new(allocator) UserClassArray(this, internalName, baseClass);
    classes->map.insert(std::make_pair(internalName, res));
  } else {
    res = ((UserClassArray*)(I->second));
  }
  classes->lock.unlock();
  return res;
}

ClassArray* 
JnjvmBootstrapLoader::constructPrimitiveArray(ClassArray& cl, const UTF8* name,
                                              ClassPrimitive* baseClass) {
    
  cl = ClassArray(this, name, baseClass);
  classes->map.insert(std::make_pair(name, &cl));
  return &cl;
}

Typedef* JnjvmClassLoader::internalConstructType(const UTF8* name) {
  short int cur = name->elements[0];
  Typedef* res = 0;
  switch (cur) {
    case I_TAB :
      res = new(allocator) ArrayTypedef(name);
      break;
    case I_REF :
      res = new(allocator) ObjectTypedef(name, hashUTF8);
      break;
    default :
      UserClassPrimitive* cl = 
        bootstrapLoader->getPrimitiveClass((char)name->elements[0]);
      assert(cl && "No primitive");
      bool unsign = (cl == bootstrapLoader->upcalls->OfChar || 
                     cl == bootstrapLoader->upcalls->OfBool);
      res = new(allocator) PrimitiveTypedef(name, cl, unsign, cur);
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
    JavaThread::get()->getJVM()->
      unknownError("wrong type %d in %s", l, name->printString());
  } else {
    JavaThread::get()->getJVM()->
      unknownError("wrong type %s", name->printString());
  }
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
JnjvmClassLoader::getJnjvmLoaderFromJavaObject(JavaObject* loader, Jnjvm* vm) {
  
  if (loader == 0)
    return vm->bootstrapLoader;
  
  Classpath* upcalls = vm->bootstrapLoader->upcalls;
  JnjvmClassLoader* JCL = 
    (JnjvmClassLoader*)(upcalls->vmdataClassLoader->getObjectField(loader));
  
  if (!JCL) {
    JCL = gc_new(JnjvmClassLoader)(*vm->bootstrapLoader, loader, vm);
    (upcalls->vmdataClassLoader->setObjectField(loader, (JavaObject*)JCL));
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
  
  if (isolate)
    isolate->removeMethodsInFunctionMap(this);

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

  delete TheModuleProvider;
}


JnjvmBootstrapLoader::~JnjvmBootstrapLoader() {
  delete FunctionPasses;
}

JavaString* JnjvmClassLoader::UTF8ToStr(const UTF8* val) {
  JavaString* res = isolate->internalUTF8ToStr(val);
  strings.push_back(res);
  return res;
}

JavaString* JnjvmBootstrapLoader::UTF8ToStr(const UTF8* val) {
  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaString* res = vm->internalUTF8ToStr(val);
  strings.push_back(res);
  return res;
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
            char* temp = (char*)allocator.Allocate(len + 2);
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

// constructArrayName can allocate the UTF8 directly in the classloader
// memory because it is called by safe places, ie only valid names are
// created.
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

intptr_t JnjvmClassLoader::loadInLib(const char* buf, bool& jnjvm) {
  uintptr_t res = (uintptr_t)dlsym(SELF_HANDLE, buf);
  
  if (!res) {
    for (std::vector<void*>::iterator i = nativeLibs.begin(),
              e = nativeLibs.end(); i!= e; ++i) {
      res = (uintptr_t)dlsym((*i), buf);
      if (res) break;
    }
  } else {
    jnjvm = true;
  }
  
  if (!res && this != bootstrapLoader)
    res = bootstrapLoader->loadInLib(buf, jnjvm);

  return (intptr_t)res;
}

void* JnjvmClassLoader::loadLib(const char* buf) {
  void* handle = dlopen(buf, RTLD_LAZY | RTLD_LOCAL);
  if (handle) nativeLibs.push_back(handle);
  return handle;
}

intptr_t JnjvmClassLoader::loadInLib(const char* name, void* handle) {
  return (intptr_t)dlsym(handle, name);
}

intptr_t JnjvmClassLoader::nativeLookup(JavaMethod* meth, bool& jnjvm,
                                        char* buf) {

  meth->jniConsFromMeth(buf);
  intptr_t res = loadInLib(buf, jnjvm);
  if (!res) {
    meth->jniConsFromMethOverloaded(buf);
    res = loadInLib(buf, jnjvm);
  }
  return res;
}


// Extern "C" functions called by the vmjc static intializer.
extern "C" void vmjcAddPreCompiledClass(JnjvmClassLoader* JCL,
                                        CommonClass* cl) {
  JCL->getClasses()->map.insert(std::make_pair(cl->name, cl));
  cl->classLoader = JCL;
}

extern "C" void vmjcGetClassArray(JnjvmClassLoader* JCL, ClassArray** ptr,
                              const UTF8* name) {
  *ptr = JCL->constructArray(name);
}

extern "C" void vmjcLoadClass(JnjvmClassLoader* JCL, const UTF8* name) {
  JCL->loadName(name, true, true);
}

extern "C" void vmjcAddUTF8(JnjvmClassLoader* JCL, const UTF8* val) {
  JCL->hashUTF8->insert(val);
}

extern "C" void vmjcAddString(JnjvmClassLoader* JCL, JavaString* val) {
  JCL->strings.push_back(val);
}

extern "C" intptr_t vmjcNativeLoader(UserClass* cl, const char* name) {
  bool jnjvm = false;
  intptr_t res = cl->classLoader->loadInLib(name, jnjvm);
  assert(res && "Could not find required native method");
  return res;
}
