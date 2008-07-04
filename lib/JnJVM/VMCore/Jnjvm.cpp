//===---------- Jnjvm.cpp - Java virtual machine description --------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#define JNJVM_LOAD 0

#include <float.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "debug.h"

#include "mvm/JIT.h"

#include "JavaArray.h"
#include "JavaClass.h"
#include "JavaConstantPool.h"
#include "JavaJIT.h"
#include "JavaString.h"
#include "JavaThread.h"
#include "JavaTypes.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"
#include "JnjvmModuleProvider.h"
#include "LockedMap.h"
#include "Reader.h"
#ifdef SERVICE_VM
#include "ServiceDomain.h"
#endif
#include "Zip.h"

using namespace jnjvm;

Jnjvm* Jnjvm::bootstrapVM = 0;

#define DEF_UTF8(var) \
  const UTF8* Jnjvm::var = 0
  
  DEF_UTF8(initName);
  DEF_UTF8(clinitName);
  DEF_UTF8(clinitType);
  DEF_UTF8(runName);
  DEF_UTF8(prelib);
  DEF_UTF8(postlib);
  DEF_UTF8(mathName);
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

const char* Jnjvm::dirSeparator = "/";
const char* Jnjvm::envSeparator = ":";
const unsigned int Jnjvm::Magic = 0xcafebabe;

#define DECLARE_EXCEPTION(EXCP) \
  const char* Jnjvm::EXCP = "java/lang/"#EXCP

#define DECLARE_REFLECT_EXCEPTION(EXCP) \
  const char* Jnjvm::EXCP = "java/lang/reflect/"#EXCP

DECLARE_EXCEPTION(ArithmeticException);
DECLARE_REFLECT_EXCEPTION(InvocationTargetException);
DECLARE_EXCEPTION(ArrayStoreException);
DECLARE_EXCEPTION(ClassCastException);
DECLARE_EXCEPTION(IllegalMonitorStateException);
DECLARE_EXCEPTION(IllegalArgumentException);
DECLARE_EXCEPTION(InterruptedException);
DECLARE_EXCEPTION(IndexOutOfBoundsException);
DECLARE_EXCEPTION(ArrayIndexOutOfBoundsException);
DECLARE_EXCEPTION(NegativeArraySizeException);
DECLARE_EXCEPTION(NullPointerException);
DECLARE_EXCEPTION(SecurityException);
DECLARE_EXCEPTION(ClassFormatError);
DECLARE_EXCEPTION(ClassCircularityError);
DECLARE_EXCEPTION(NoClassDefFoundError);
DECLARE_EXCEPTION(UnsupportedClassVersionError);
DECLARE_EXCEPTION(NoSuchFieldError);
DECLARE_EXCEPTION(NoSuchMethodError);
DECLARE_EXCEPTION(InstantiationError);
DECLARE_EXCEPTION(IllegalAccessError);
DECLARE_EXCEPTION(IllegalAccessException);
DECLARE_EXCEPTION(VerifyError);
DECLARE_EXCEPTION(ExceptionInInitializerError);
DECLARE_EXCEPTION(LinkageError);
DECLARE_EXCEPTION(AbstractMethodError);
DECLARE_EXCEPTION(UnsatisfiedLinkError);
DECLARE_EXCEPTION(InternalError);
DECLARE_EXCEPTION(OutOfMemoryError);
DECLARE_EXCEPTION(StackOverflowError);
DECLARE_EXCEPTION(UnknownError);
DECLARE_EXCEPTION(ClassNotFoundException);


void Jnjvm::analyseClasspathEnv(const char* str) {
  if (str != 0) {
    unsigned int len = strlen(str);
    char* buf = (char*)alloca(len + 1);
    const char* cur = str;
    int top = 0;
    char c = 1;
    while (c != 0) {
      while (((c = cur[top]) != 0) && c != envSeparator[0]) {
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
            temp[len] = dirSeparator[0];
            temp[len + 1] = 0;
            bootClasspath.push_back(temp);
            free(rp);
          } else {
            ArrayUInt8* bytes = Reader::openFile(this, rp);
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

void Jnjvm::readParents(Class* cl, Reader& reader) {
  JavaCtpInfo* ctpInfo = cl->ctpInfo;
  unsigned short int superEntry = reader.readU2();
  const UTF8* super = superEntry ? 
        ctpInfo->resolveClassName(superEntry) : 0;

  unsigned short int nbI = reader.readU2();
  cl->superUTF8 = super;
  for (int i = 0; i < nbI; i++)
    cl->interfacesUTF8.push_back(ctpInfo->resolveClassName(reader.readU2()));

}

void Jnjvm::loadParents(Class* cl) {
  const UTF8* super = cl->superUTF8;
  int nbI = cl->interfacesUTF8.size();
  JavaObject* classLoader = cl->classLoader;
  if (super == 0) {
    cl->depth = 0;
    cl->display = (CommonClass**)malloc(sizeof(CommonClass*));
    cl->display[0] = cl;
    cl->virtualTableSize = VT_SIZE / sizeof(void*);
  } else {
    cl->super = loadName(super, classLoader, true, false, true);
    int depth = cl->super->depth + 1;
    cl->depth = depth;
    cl->virtualTableSize = cl->super->virtualTableSize;
    cl->display = (CommonClass**)malloc((depth + 1) * sizeof(CommonClass*));
    memcpy(cl->display, cl->super->display, depth * sizeof(CommonClass*));
    cl->display[cl->depth] = cl;
  }

  for (int i = 0; i < nbI; i++)
    cl->interfaces.push_back((Class*)loadName(cl->interfacesUTF8[i],
                                              classLoader, true, false, true));
}

void Jnjvm::readAttributs(Class* cl, Reader& reader,
                           std::vector<Attribut*>& attr) {
  JavaCtpInfo* ctpInfo = cl->ctpInfo;
  unsigned short int nba = reader.readU2();
  
  for (int i = 0; i < nba; i++) {
    const UTF8* attName = ctpInfo->UTF8At(reader.readU2());
    uint32 attLen = reader.readU4();
    Attribut* att = new Attribut(attName, attLen, reader);
    attr.push_back(att);
    reader.seek(attLen, Reader::SeekCur);
  }
}

void Jnjvm::readFields(Class* cl, Reader& reader) {
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

void Jnjvm::readMethods(Class* cl, Reader& reader) {
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

void Jnjvm::readClass(Class* cl) {

  PRINT_DEBUG(JNJVM_LOAD, 0, COLOR_NORMAL, "; ", 0);
  PRINT_DEBUG(JNJVM_LOAD, 0, LIGHT_GREEN, "reading ", 0);
  PRINT_DEBUG(JNJVM_LOAD, 0, COLOR_NORMAL, "%s::%s\n", printString(),
              cl->printString());

  Reader reader(cl->bytes);
  uint32 magic = reader.readU4();
  if (magic != Jnjvm::Magic) {
    Jnjvm::error(ClassFormatError, "bad magic number %p", magic);
  }
  cl->minor = reader.readU2();
  cl->major = reader.readU2();
  JavaCtpInfo::read(this, cl, reader);
  JavaCtpInfo* ctpInfo = cl->ctpInfo;
  cl->access = reader.readU2();
  
  const UTF8* thisClassName = 
    ctpInfo->resolveClassName(reader.readU2());
  
  if (!(thisClassName->equals(cl->name))) {
    error(ClassFormatError, "try to load %s and found class named %s",
          cl->printString(), thisClassName->printString());
  }

  readParents(cl, reader);
  readFields(cl, reader);
  readMethods(cl, reader);
  readAttributs(cl, reader, cl->attributs);
}

ArrayUInt8* Jnjvm::openName(const UTF8* utf8) {
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


typedef void (*clinit_t)(Jnjvm* vm);

void Jnjvm::initialiseClass(CommonClass* cl) {
  JavaState* status = cl->getStatus();
  if (cl->isArray || cl->isPrimitive) {
    *status = ready;
  } else if (!(*status == ready)) {
    cl->acquire();
    JavaState* status = cl->getStatus();
    if (*status == ready) {
      cl->release();
    } else if (*status >= resolved && *status != clinitParent &&
               *status != inClinit) {
      *status = clinitParent;
      cl->release();
      if (cl->super) {
        cl->super->initialiseClass();
      }

      TheModule->resolveStaticClass((Class*)cl);
      
      *status = inClinit;
      JavaMethod* meth = cl->lookupMethodDontThrow(clinitName, clinitType, true,
                                                   false);
      
      PRINT_DEBUG(JNJVM_LOAD, 0, COLOR_NORMAL, "; ", 0);
      PRINT_DEBUG(JNJVM_LOAD, 0, LIGHT_GREEN, "clinit ", 0);
      PRINT_DEBUG(JNJVM_LOAD, 0, COLOR_NORMAL, "%s::%s\n", printString(),
                  cl->printString());
      
      ((Class*)cl)->createStaticInstance();
      
      if (meth) {
        JavaObject* exc = 0;
        try{
          clinit_t pred = (clinit_t)(intptr_t)meth->compiledPtr();
          pred(JavaThread::get()->isolate);
        } catch(...) {
          exc = JavaThread::getJavaException();
          assert(exc && "no exception?");
          JavaThread::clearException();
        }
        if (exc) {
          if (exc->classOf->isAssignableFrom(Classpath::newException)) {
            JavaThread::get()->isolate->initializerError(exc);
          } else {
            JavaThread::throwException(exc);
          }
        }
      }
      
      *status = ready;
      cl->broadcastClass();
    } else if (*status < resolved) {
      cl->release();
      unknownError("try to clinit a not-readed class...");
    } else {
      if (!cl->ownerClass()) {
        while (*status < ready) cl->waitClass();
        cl->release();
        initialiseClass(cl);
      } 
      cl->release();
    }
  }
}

void Jnjvm::resolveClass(CommonClass* cl, bool doClinit) {
  if (cl->status < resolved) {
    cl->acquire();
    int status = cl->status;
    if (status >= resolved) {
      cl->release();
    } else if (status <  loaded) {
      cl->release();
      unknownError("try to resolve a not-readed class");
    } else if (status == loaded || cl->ownerClass()) {
      if (cl->isArray) {
        ClassArray* arrayCl = (ClassArray*)cl;
        CommonClass* baseClass =  arrayCl->baseClass();
        baseClass->resolveClass(doClinit);
        cl->status = resolved;
      } else {
        readClass((Class*)cl);
        cl->status = readed;
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

CommonClass* Jnjvm::loadName(const UTF8* name, JavaObject* loader,
                              bool doResolve, bool doClinit, bool doThrow) {
 

  CommonClass* cl = lookupClass(name, loader);
  const UTF8* bootstrapName = name;
  
  if (!cl || cl->status == hashed) {
    if (!loader) { // I have to load it
      ArrayUInt8* bytes = openName(name);
      if (bytes) {
        if (!cl) cl = bootstrapVM->constructClass(bootstrapName, loader);
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
    } else {
      cl = loadInClassLoader(name->internalToJava(this, 0, name->size), loader);
    }
  }

  if (!cl && doThrow) {
    if (!memcmp(name->UTF8ToAsciiz(), NoClassDefFoundError, 
                    strlen(NoClassDefFoundError))) {
      unknownError("Unable to load NoClassDefFoundError");
    }
    Jnjvm::error(NoClassDefFoundError, "unable to load %s", name->printString());
  }

  if (cl && doResolve) cl->resolveClass(doClinit);

  return cl;
}

void Jnjvm::errorWithExcp(const char* className, const JavaObject* excp) {
  Class* cl = (Class*) this->loadName(this->asciizConstructUTF8(className),
                                      CommonClass::jnjvmClassLoader,
                                      true, true, true);
  JavaObject* obj = (*cl)(this);
  JavaJIT::invokeOnceVoid(this, CommonClass::jnjvmClassLoader, className, "<init>",
                          "(Ljava/lang/Throwable;)V", ACC_VIRTUAL, obj, excp);
  JavaThread::throwException(obj);
}

void Jnjvm::error(const char* className, const char* fmt, ...) {
  char* tmp = (char*)alloca(4096);
  Class* cl = (Class*) this->loadName(this->asciizConstructUTF8(className),
                                      CommonClass::jnjvmClassLoader,
                                      true, true, true);
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(tmp, 4096, fmt, ap);
  va_end(ap);

  JavaObject* obj = (*cl)(this);
  JavaJIT::invokeOnceVoid(this, CommonClass::jnjvmClassLoader, className, "<init>",
                          "(Ljava/lang/String;)V", ACC_VIRTUAL, obj, 
                          this->asciizToStr(tmp));
  JavaThread::throwException(obj);
}


void Jnjvm::verror(const char* className, const char* fmt, va_list ap) {
  char* tmp = (char*)alloca(4096);
  Class* cl = (Class*) this->loadName(this->asciizConstructUTF8(className),
                                      CommonClass::jnjvmClassLoader,
                                      true, true, true);
  vsnprintf(tmp, 4096, fmt, ap);
  va_end(ap);
  JavaObject* obj = (*cl)(this);
  JavaJIT::invokeOnceVoid(this, CommonClass::jnjvmClassLoader, className, "<init>",
                          "(Ljava/lang/String;)V", ACC_VIRTUAL, obj, 
                          this->asciizToStr(tmp));

  JavaThread::throwException(obj);
}

void Jnjvm::arrayStoreException() {
  error(ArrayStoreException, "");
}

void Jnjvm::indexOutOfBounds(const JavaObject* obj, sint32 entry) {
  error(ArrayIndexOutOfBoundsException, "%d", entry);
}

void Jnjvm::negativeArraySizeException(sint32 size) {
  error(NegativeArraySizeException, "%d", size);
}

void Jnjvm::nullPointerException(const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  char* val = va_arg(ap, char*);
  va_end(ap);
  error(NullPointerException, fmt, val);
}

void Jnjvm::illegalAccessException(const char* msg) {
  error(IllegalAccessException, msg);
}

void Jnjvm::illegalMonitorStateException(const JavaObject* obj) {
  error(IllegalMonitorStateException, "");
}

void Jnjvm::interruptedException(const JavaObject* obj) {
  error(InterruptedException, "");
}


void Jnjvm::initializerError(const JavaObject* excp) {
  errorWithExcp(ExceptionInInitializerError, excp);
}

void Jnjvm::invocationTargetException(const JavaObject* excp) {
  errorWithExcp(InvocationTargetException, excp);
}

void Jnjvm::outOfMemoryError(sint32 n) {
  error(OutOfMemoryError, "");
}

void Jnjvm::illegalArgumentExceptionForMethod(JavaMethod* meth, 
                                               CommonClass* required,
                                               CommonClass* given) {
  error(IllegalArgumentException, "for method %s", meth->printString());
}

void Jnjvm::illegalArgumentExceptionForField(JavaField* field, 
                                              CommonClass* required,
                                              CommonClass* given) {
  error(IllegalArgumentException, "for field %s", field->printString());
}

void Jnjvm::illegalArgumentException(const char* msg) {
  error(IllegalArgumentException, msg);
}

void Jnjvm::classCastException(const char* msg) {
  error(ClassCastException, msg);
}

void Jnjvm::unknownError(const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  verror(UnknownError, fmt, ap);
}

CommonClass* Jnjvm::lookupClassFromUTF8(const UTF8* utf8, unsigned int start,
                                         unsigned int len, JavaObject* loader,
                                         bool doResolve, bool doClinit,
                                         bool doThrow) {
  uint32 origLen = len;
  const UTF8* name = utf8->javaToInternal(this, start, len);
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
              const UTF8* componentName = utf8->javaToInternal(this, start + 1,
                                                               len - 2);
              if (loadName(componentName, loader, doResolve, doClinit,
                           doThrow)) {
                ret = constructArray(name, loader);
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

              ret = constructArray(name, loader);
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
    return loadName(name, loader, doResolve, doClinit, doThrow);
  }
}

CommonClass* Jnjvm::lookupClassFromJavaString(JavaString* str,
                                              JavaObject* loader,
                                              bool doResolve, bool doClinit,
                                              bool doThrow) {
  return lookupClassFromUTF8(str->value, str->offset, str->count, loader,
                             doResolve, doClinit, doThrow);
}

CommonClass* Jnjvm::lookupClass(const UTF8* utf8, JavaObject* loader) {
  if (loader) {
#ifndef SERVICE_VM
    ClassMap* map = 
      (ClassMap*)(Classpath::vmdataClassLoader->getVirtualObjectField(loader));
    if (!map) {
      map = vm_new(this, ClassMap)();
      Classpath::vmdataClassLoader->setVirtualObjectField(loader, (JavaObject*)map);
    }
#else
    ClassMap* map = 0;
    ServiceDomain* vm = 
      (ServiceDomain*)(Classpath::vmdataClassLoader->getVirtualObjectField(loader));
    if (!vm) {
      vm = ServiceDomain::allocateService((JavaIsolate*)Jnjvm::bootstrapVM);
      Classpath::vmdataClassLoader->setVirtualObjectField(loader, (JavaObject*)vm);
    }
    map = vm->classes;
#endif
    return map->lookup(utf8);
  } else {
    return bootstrapClasses->lookup(utf8);
  }
}

static CommonClass* arrayDup(const UTF8*& name, Jnjvm *vm) {
  ClassArray* cl = vm_new(vm, ClassArray)(vm, name);
  return cl;
}

ClassArray* Jnjvm::constructArray(const UTF8* name, JavaObject* loader) {
  if (loader != 0)
    loader = ClassArray::arrayLoader(this, name, loader, 1, name->size - 1);

  if (loader) {
#ifndef SERVICE_VM
    ClassMap* map = 
      (ClassMap*)(Classpath::vmdataClassLoader->getVirtualObjectField(loader));
    if (!map) {
      map = vm_new(this, ClassMap)();
      Classpath::vmdataClassLoader->setVirtualObjectField(loader, (JavaObject*)map);
    }
    ClassArray* res = (ClassArray*)map->lookupOrCreate(name, this, arrayDup);
#else
    ClassMap* map = 0;
    ServiceDomain* vm = 
      (ServiceDomain*)(Classpath::vmdataClassLoader->getVirtualObjectField(loader));
    if (!vm) {
      vm = ServiceDomain::allocateService((JavaIsolate*)Jnjvm::bootstrapVM);
      Classpath::vmdataClassLoader->getVirtualObjectField(loader, (JavaObject*)vm);
    }
    map = vm->classes;
    ClassArray* res = (ClassArray*)map->lookupOrCreate(name, vm, arrayDup);
#endif
    if (!res->classLoader) res->classLoader = loader;
    return res;
  } else {
    return (ClassArray*)bootstrapClasses->lookupOrCreate(name, this, arrayDup);
  }
}


static CommonClass* classDup(const UTF8*& name, Jnjvm *vm) {
  Class* cl = vm_new(vm, Class)(vm, name);
  return cl;
}

Class* Jnjvm::constructClass(const UTF8* name, JavaObject* loader) {
  if (loader) {
#ifndef SERVICE_VM
    ClassMap* map = 
      (ClassMap*)(Classpath::vmdataClassLoader->getVirtualObjectField(loader));
    if (!map) {
      map = vm_new(this, ClassMap)();
      (Classpath::vmdataClassLoader->setVirtualObjectField(loader, (JavaObject*)map));
    }
    Class* res = (Class*)map->lookupOrCreate(name, this, classDup);
#else
    ClassMap* map = 0;
    ServiceDomain* vm = 
      (ServiceDomain*)(Classpath::vmdataClassLoader->getVirtualObjectField(loader));
    if (!vm) {
      vm = ServiceDomain::allocateService((JavaIsolate*)Jnjvm::bootstrapVM);
      Classpath::vmdataClassLoader->setVirtualObjectField(loader, (JavaObject*)vm);
    }
    map = vm->classes;
    Class* res = (Class*)map->lookupOrCreate(name, vm, classDup);
#endif
    if (!res->classLoader) res->classLoader = loader;
    return res;
  } else {
    return (Class*)bootstrapClasses->lookupOrCreate(name, this, classDup);
  }
}

const UTF8* Jnjvm::asciizConstructUTF8(const char* asciiz) {
  return hashUTF8->lookupOrCreateAsciiz(this, asciiz);
}

const UTF8* Jnjvm::readerConstructUTF8(const uint16* buf, uint32 size) {
  return hashUTF8->lookupOrCreateReader(this, buf, size);
}

Typedef* Jnjvm::constructType(const UTF8* name) {
  Typedef* res = javaTypes->lookup(name);
  if (res == 0) {
    res = Typedef::typeDup(name, this);
    javaTypes->lock->lock();
    Typedef* tmp = javaTypes->lookup(name);
    if (tmp == 0) javaTypes->hash(name, res);
    else res = tmp;
    javaTypes->lock->unlock();
  }
  return res;
}

CommonClass* Jnjvm::loadInClassLoader(const UTF8* name, JavaObject* loader) {
  JavaString* str = this->UTF8ToStr(name);
  JavaObject* obj = (JavaObject*)
    Classpath::loadInClassLoader->invokeJavaObjectVirtual(this, loader, str);
  return (CommonClass*)(Classpath::vmdataClass->getVirtualObjectField(obj));
}

JavaString* Jnjvm::UTF8ToStr(const UTF8* utf8) { 
  JavaString* res = hashStr->lookupOrCreate(utf8, this, JavaString::stringDup);
  return res;
}

JavaString* Jnjvm::asciizToStr(const char* asciiz) {
  const UTF8* var = asciizConstructUTF8(asciiz);
  return UTF8ToStr(var);
}

void Jnjvm::addProperty(char* key, char* value) {
  postProperties.push_back(std::make_pair(key, value));
}

#ifndef MULTIPLE_VM
JavaObject* Jnjvm::getClassDelegatee(CommonClass* cl) {
  cl->acquire();
  if (!(cl->delegatee)) {
    JavaObject* delegatee = (*Classpath::newClass)(this);
    cl->delegatee = delegatee;
    Classpath::initClass->invokeIntSpecial(this, delegatee, cl);
  } else if (cl->delegatee->classOf != Classpath::newClass) {
    JavaObject* pd = cl->delegatee;
    JavaObject* delegatee = (*Classpath::newClass)(this);
    cl->delegatee = delegatee;;
    Classpath::initClassWithProtectionDomain->invokeIntSpecial(this, delegatee,
                                                               cl, pd);
  }
  cl->release();
  return cl->delegatee;
}
#else
JavaObject* Jnjvm::getClassDelegatee(CommonClass* cl) {
  cl->acquire();
  JavaObject* val = delegatees->lookup(cl);
  if (!val) {
    val = (*Classpath::newClass)(this);
    delegatees->hash(cl, val);
    Classpath::initClass->invokeIntSpecial(this, val, cl);
  } else if (val->classOf != Classpath::newClass) {
    JavaObject* pd = val;
    val = (*Classpath::newClass)(this);
    delegatees->remove(cl);
    delegatees->hash(cl, val);
    Classpath::initClassWithProtectionDomain->invokeIntSpecial(this, val, cl,
                                                               pd);
  }
  cl->release();
  return val;
}
#endif

Jnjvm::~Jnjvm() {
#ifdef MULTIPLE_GC
  GC->destroy();
  delete GC;
#endif
  
  delete hashUTF8;
  delete hashStr;
  delete javaTypes;
  
  delete globalRefsLock;
  delete TheModuleProvider;
}

Jnjvm::Jnjvm() {
#ifdef MULTIPLE_GC
  GC = 0;
#endif
  hashUTF8 = 0;
  hashStr = 0;
  javaTypes = 0;
  globalRefsLock = 0;
  TheModuleProvider = 0;
}
