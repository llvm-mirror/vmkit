//===---------- Jnjvm.cpp - Java virtual machine description --------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#define JNJVM_LOAD 0

#include <cfloat>
#include <climits>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "debug.h"

#include "mvm/JIT.h"
#include "mvm/Threads/Thread.h"

#include "ClasspathReflect.h"
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
#include "Zip.h"

using namespace jnjvm;

const char* Jnjvm::dirSeparator = "/";
const char* Jnjvm::envSeparator = ":";
const unsigned int Jnjvm::Magic = 0xcafebabe;

#ifdef ISOLATE
Jnjvm* Jnjvm::RunningIsolates[NR_ISOLATES];
mvm::LockNormal Jnjvm::IsolateLock;
#endif


/// initialiseClass - Java class initialisation. Java specification §2.17.5.

void UserClass::initialiseClass(Jnjvm* vm) {
  
  // Primitives are initialized at boot time, arrays are initialized directly.
  
  // Assumes that the Class object has already been verified and prepared and
  // that the Class object contains state that can indicate one of four
  // situations:
  //
  //  * This Class object is verified and prepared but not initialized.
  //  * This Class object is being initialized by some particular thread T.
  //  * This Class object is fully initialized and ready for use.
  //  * This Class object is in an erroneous state, perhaps because the
  //    verification step failed or because initialization was attempted and
  //    failed.

  assert(isResolved() || getOwnerClass() || isReady() ||
         isErroneous() && "Class in wrong state");
  
  if (getInitializationState() != ready) {
    
    // 1. Synchronize on the Class object that represents the class or 
    //    interface to be initialized. This involves waiting until the
    //    current thread can obtain the lock for that object
    //    (Java specification §8.13).
    acquire();
    JavaThread* self = JavaThread::get();

    if (getInitializationState() == inClinit) {
      // 2. If initialization by some other thread is in progress for the
      //    class or interface, then wait on this Class object (which 
      //    temporarily releases the lock). When the current thread awakens
      //    from the wait, repeat this step.
      if (getOwnerClass() != self) {
        while (getOwnerClass()) {
          waitClass();
        }
      } else {
        // 3. If initialization is in progress for the class or interface by
        //    the current thread, then this must be a recursive request for 
        //    initialization. Release the lock on the Class object and complete
        //    normally.
        release();
        return;
      }
    } 
    
    // 4. If the class or interface has already been initialized, then no 
    //    further action is required. Release the lock on the Class object
    //    and complete normally.
    if (getInitializationState() == ready) {
      release();
      return;
    }
    
    // 5. If the Class object is in an erroneous state, then initialization is
    //    not possible. Release the lock on the Class object and throw a
    //    NoClassDefFoundError.
    if (isErroneous()) {
      release();
      vm->noClassDefFoundError(name);
    }

    // 6. Otherwise, record the fact that initialization of the Class object is
    //    now in progress by the current thread and release the lock on the
    //    Class object.
    setOwnerClass(self);
    setInitializationState(inClinit);
    UserClass* cl = (UserClass*)this;
#if defined(ISOLATE) || defined(ISOLATE_SHARING)
    // Isolate environments allocate the static instance on their own, not when
    // the class is being resolved.
    void* val = cl->allocateStaticInstance(vm);
#else
    // Single environment allocates the static instance during resolution, so
    // that compiled code can access it directly (with an initialization
    // check just before the access)
    void* val = cl->getStaticInstance();
    if (!val) {
      val = cl->allocateStaticInstance(vm);
    }
#endif
    release();
  

    // 7. Next, if the Class object represents a class rather than an interface, 
    //    and the direct superclass of this class has not yet been initialized,
    //    then recursively perform this entire procedure for the uninitialized 
    //    superclass. If the initialization of the direct superclass completes 
    //    abruptly because of a thrown exception, then lock this Class object, 
    //    label it erroneous, notify all waiting threads, release the lock, 
    //    and complete abruptly, throwing the same exception that resulted from 
    //    the initializing the superclass.
    UserClass* super = getSuper();
    if (super) {
      JavaObject *exc = 0;
      try {
        super->initialiseClass(vm);
      } catch(...) {
        exc = self->getJavaException();
        assert(exc && "no exception?");
        self->clearException();
      }
      
      if (exc) {
        acquire();
        setErroneous();
        setOwnerClass(0);
        broadcastClass();
        release();
        self->throwException(exc);
      }
    }
 
#ifdef SERVICE
    JavaObject* exc = 0;
    if (classLoader == classLoader->bootstrapLoader || 
        classLoader->getIsolate() == vm) {
#endif

    // 8. Next, execute either the class variable initializers and static
    //    initializers of the class or the field initializers of the interface,
    //    in textual order, as though they were a single block, except that
    //    final static variables and fields of interfaces whose values are 
    //    compile-time constants are initialized first.
    
    PRINT_DEBUG(JNJVM_LOAD, 0, COLOR_NORMAL, "; ", 0);
    PRINT_DEBUG(JNJVM_LOAD, 0, LIGHT_GREEN, "clinit ", 0);
    PRINT_DEBUG(JNJVM_LOAD, 0, COLOR_NORMAL, "%s\n", printString());


 
    JavaField* fields = cl->getStaticFields();
    for (uint32 i = 0; i < cl->nbStaticFields; ++i) {
      fields[i].initField(val, vm);
    }
  
      
      
    JavaMethod* meth = lookupMethodDontThrow(vm->bootstrapLoader->clinitName,
                                             vm->bootstrapLoader->clinitType,
                                             true, false, 0);

    JavaObject* exc = 0;
    if (meth) {
      try{
        meth->invokeIntStatic(vm, cl);
      } catch(...) {
        exc = self->getJavaException();
        assert(exc && "no exception?");
        self->clearException();
      }
    }
#ifdef SERVICE
    }
#endif

    // 9. If the execution of the initializers completes normally, then lock
    //    this Class object, label it fully initialized, notify all waiting 
    //    threads, release the lock, and complete this procedure normally.
    if (!exc) {
      acquire();
      setInitializationState(ready);
      setOwnerClass(0);
      broadcastClass();
      release();
      return;
    }
    
    // 10. Otherwise, the initializers must have completed abruptly by
    //     throwing some exception E. If the class of E is not Error or one
    //     of its subclasses, then create a new instance of the class 
    //     ExceptionInInitializerError, with E as the argument, and use this
    //     object in place of E in the following step. But if a new instance of
    //     ExceptionInInitializerError cannot be created because an
    //     OutOfMemoryError occurs, then instead use an OutOfMemoryError object
    //     in place of E in the following step.
    if (exc->classOf->isAssignableFrom(vm->upcalls->newException)) {
      Classpath* upcalls = classLoader->bootstrapLoader->upcalls;
      UserClass* clExcp = upcalls->ExceptionInInitializerError;
      Jnjvm* vm = self->getJVM();
      JavaObject* obj = clExcp->doNew(vm);
      if (!obj) {
        fprintf(stderr, "implement me");
        abort();
      }
      JavaMethod* init = upcalls->ErrorWithExcpExceptionInInitializerError;
      init->invokeIntSpecial(vm, clExcp, obj, exc);
      exc = obj;
    } 

    // 11. Lock the Class object, label it erroneous, notify all waiting
    //     threads, release the lock, and complete this procedure abruptly
    //     with reason E or its replacement as determined in the previous step.
    acquire();
    setErroneous();
    setOwnerClass(0);
    broadcastClass();
    release();
    self->throwException(exc);
  }
}
      
void Jnjvm::errorWithExcp(UserClass* cl, JavaMethod* init,
                          const JavaObject* excp) {
  JavaObject* obj = cl->doNew(this);
  init->invokeIntSpecial(this, cl, obj, excp);
  JavaThread::get()->throwException(obj);
}

JavaObject* Jnjvm::CreateError(UserClass* cl, JavaMethod* init,
                               const char* str) {
  JavaObject* obj = cl->doNew(this);
  init->invokeIntSpecial(this, cl, obj, str ? asciizToStr(str) : 0);
  return obj;
}

void Jnjvm::error(UserClass* cl, JavaMethod* init, const char* fmt, ...) {
  char* tmp = (char*)alloca(4096);
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(tmp, 4096, fmt, ap);
  va_end(ap);
  
  if (cl && !bootstrapLoader->getModule()->isStaticCompiling()) {
    JavaObject* obj = CreateError(cl, init, tmp);
    JavaThread::get()->throwException(obj);
  } else {
    throw std::string(tmp);
  }
}

void Jnjvm::arrayStoreException() {
  error(upcalls->ArrayStoreException,
        upcalls->InitArrayStoreException, "");
}

void Jnjvm::indexOutOfBounds(const JavaObject* obj, sint32 entry) {
  error(upcalls->ArrayIndexOutOfBoundsException,
        upcalls->InitArrayIndexOutOfBoundsException, "%d", entry);
}

void Jnjvm::negativeArraySizeException(sint32 size) {
  error(upcalls->NegativeArraySizeException,
        upcalls->InitNegativeArraySizeException, "%d", size);
}

void Jnjvm::nullPointerException(const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  char* val = va_arg(ap, char*);
  va_end(ap);
  error(upcalls->NullPointerException,
        upcalls->InitNullPointerException, fmt, val);
}

JavaObject* Jnjvm::CreateIndexOutOfBoundsException(sint32 entry) {
  
  char* tmp = (char*)alloca(4096);
  snprintf(tmp, 4096, "%d", entry);

  return CreateError(upcalls->ArrayIndexOutOfBoundsException,
                     upcalls->InitArrayIndexOutOfBoundsException, tmp);
}

JavaObject* Jnjvm::CreateNegativeArraySizeException() {
  return CreateError(upcalls->NegativeArraySizeException,
                     upcalls->InitNegativeArraySizeException, 0);
}

JavaObject* Jnjvm::CreateNullPointerException() {
  return CreateError(upcalls->NullPointerException,
                     upcalls->InitNullPointerException, 0);
}

JavaObject* Jnjvm::CreateOutOfMemoryError() {
  return CreateError(upcalls->OutOfMemoryError,
                     upcalls->InitOutOfMemoryError, 
                     "Java heap space");
}

JavaObject* Jnjvm::CreateClassCastException(JavaObject* obj,
                                            UserCommonClass* cl) {
  return CreateError(upcalls->ClassCastException,
                     upcalls->InitClassCastException, "");
}

void Jnjvm::illegalAccessException(const char* msg) {
  error(upcalls->IllegalAccessException,
        upcalls->InitIllegalAccessException, msg);
}

void Jnjvm::illegalMonitorStateException(const JavaObject* obj) {
  error(upcalls->IllegalMonitorStateException,
        upcalls->InitIllegalMonitorStateException, "");
}

void Jnjvm::interruptedException(const JavaObject* obj) {
  error(upcalls->InterruptedException,
        upcalls->InitInterruptedException, "");
}


void Jnjvm::initializerError(const JavaObject* excp) {
  errorWithExcp(upcalls->ExceptionInInitializerError,
                upcalls->ErrorWithExcpExceptionInInitializerError,
                excp);
}

void Jnjvm::invocationTargetException(const JavaObject* excp) {
  errorWithExcp(upcalls->InvocationTargetException,
                upcalls->ErrorWithExcpInvocationTargetException,
                excp);
}

void Jnjvm::outOfMemoryError(sint32 n) {
  error(upcalls->OutOfMemoryError,
        upcalls->InitOutOfMemoryError, "%d", n);
}

void Jnjvm::illegalArgumentExceptionForMethod(JavaMethod* meth, 
                                              UserCommonClass* required,
                                              UserCommonClass* given) {
  error(upcalls->IllegalArgumentException, 
        upcalls->InitIllegalArgumentException, 
        "for method %s", meth->printString());
}

void Jnjvm::illegalArgumentExceptionForField(JavaField* field, 
                                             UserCommonClass* required,
                                             UserCommonClass* given) {
  error(upcalls->IllegalArgumentException, 
        upcalls->InitIllegalArgumentException, 
        "for field %s", field->printString());
}

void Jnjvm::illegalArgumentException(const char* msg) {
  error(upcalls->IllegalArgumentException,
        upcalls->InitIllegalArgumentException,
        msg);
}

void Jnjvm::classCastException(JavaObject* obj, UserCommonClass* cl) {
  error(upcalls->ClassCastException,
        upcalls->InitClassCastException,
        "");
}

void Jnjvm::noSuchFieldError(CommonClass* cl, const UTF8* name) {
  error(upcalls->NoSuchFieldError,
        upcalls->InitNoSuchFieldError, 
        "unable to find %s in %s",
        name->UTF8ToAsciiz(), cl->name->UTF8ToAsciiz());

}

void Jnjvm::noSuchMethodError(CommonClass* cl, const UTF8* name) {
  error(upcalls->NoSuchMethodError,
        upcalls->InitNoSuchMethodError, 
        "unable to find %s in %s",
        name->UTF8ToAsciiz(), cl->name->UTF8ToAsciiz());

}

void Jnjvm::classFormatError(const char* msg, ...) {
  error(upcalls->ClassFormatError,
        upcalls->InitClassFormatError, 
        msg);
}

void Jnjvm::noClassDefFoundError(JavaObject* obj) {
  errorWithExcp(upcalls->NoClassDefFoundError,
        upcalls->ErrorWithExcpNoClassDefFoundError, 
        obj);
}

void Jnjvm::noClassDefFoundError(const UTF8* name) {
  error(upcalls->NoClassDefFoundError,
        upcalls->InitNoClassDefFoundError, 
        "Unable to load %s", name->UTF8ToAsciiz());
}

void Jnjvm::classNotFoundException(JavaString* str) {
  error(upcalls->ClassNotFoundException,
        upcalls->InitClassNotFoundException, 
        "unable to load %s",
        str->strToAsciiz());
}

void Jnjvm::unknownError(const char* fmt, ...) {
  error(upcalls->UnknownError,
        upcalls->InitUnknownError,  
        fmt);
}

JavaString* Jnjvm::internalUTF8ToStr(const UTF8* utf8) {
  JavaString* res = hashStr.lookup(utf8);
  if (!res) {
    uint32 size = utf8->size;
    UTF8* tmp = (UTF8*)upcalls->ArrayOfChar->doNew(size, this);
    uint16* buf = tmp->elements;
  
    for (uint32 i = 0; i < size; i++) {
      buf[i] = utf8->elements[i];
    }
  
    const UTF8* newUTF8 = (const UTF8*)tmp;
    res = hashStr.lookupOrCreate(newUTF8, this, JavaString::stringDup);
  }
  return res;
}

JavaString* Jnjvm::UTF8ToStr(const UTF8* utf8) { 
  JavaString* res = hashStr.lookupOrCreate(utf8, this, JavaString::stringDup);
  return res;
}

JavaString* Jnjvm::asciizToStr(const char* asciiz) {
  const UTF8* var = asciizToUTF8(asciiz);
  return UTF8ToStr(var);
}

void Jnjvm::addProperty(char* key, char* value) {
  postProperties.push_back(std::make_pair(key, value));
}

JavaObject* UserCommonClass::getClassDelegatee(Jnjvm* vm, JavaObject* pd) {
  if (!getDelegatee()) {
    UserClass* cl = vm->upcalls->newClass;
    JavaObject* delegatee = cl->doNew(vm);
    if (!pd) {
      vm->upcalls->initClass->invokeIntSpecial(vm, cl, delegatee, this);
    } else {
      vm->upcalls->initClassWithProtectionDomain->invokeIntSpecial(vm, cl,
                                                                   delegatee,
                                                                   this, pd);
    }
    setDelegatee(delegatee);
  }
  return getDelegatee();
}

#define PATH_MANIFEST "META-INF/MANIFEST.MF"
#define MAIN_CLASS "Main-Class: "
#define MAIN_LOWER_CLASS "Main-class: "
#define PREMAIN_CLASS "Premain-Class: "
#define BOOT_CLASS_PATH "Boot-Class-Path: "
#define CAN_REDEFINE_CLASS_PATH "Can-Redefine-Classes: "

#define LENGTH_MAIN_CLASS 12
#define LENGTH_PREMAIN_CLASS 15
#define LENGTH_BOOT_CLASS_PATH 17

extern "C" struct JNINativeInterface JNI_JNIEnvTable;
extern "C" const struct JNIInvokeInterface JNI_JavaVMTable;

void ClArgumentsInfo::javaAgent(char* cur) {
  assert(0 && "implement me");
}

extern "C" int sys_strnstr(const char *haystack, const char *needle) {
  char * res = strstr(haystack, needle);
  if (res) return res - haystack;
  else return -1; 
}


static char* findInformation(Jnjvm* vm, ArrayUInt8* manifest, const char* entry,
                             uint32 len) {
  uint8* ptr = manifest->elements;
  sint32 index = sys_strnstr((char*)ptr, entry);
  if (index != -1) {
    index += len;
    sint32 end = sys_strnstr((char*)&(ptr[index]), "\n");
    if (end == -1) end = manifest->size;
    else end += index;

    sint32 length = end - index - 1;
    char* name = (char*)vm->allocator.Allocate(length + 1);
    memcpy(name, &(ptr[index]), length);
    name[length] = 0;
    return name;
  } else {
    return 0;
  }
}

void ClArgumentsInfo::extractClassFromJar(Jnjvm* vm, int argc, char** argv, 
                                          int i) {
  char* jarFile = argv[i];
  uint32 size = 2 + strlen(vm->classpath) + strlen(jarFile);
  char* temp = (char*)vm->allocator.Allocate(size);

  sprintf(temp, "%s:%s", vm->classpath, jarFile);
  vm->setClasspath(temp);
  
  ArrayUInt8* bytes = Reader::openFile(vm->bootstrapLoader, jarFile);

  ZipArchive archive(bytes, vm->allocator);
  if (archive.getOfscd() != -1) {
    ZipFile* file = archive.getFile(PATH_MANIFEST);
    if (file) {
      UserClassArray* array = vm->bootstrapLoader->upcalls->ArrayOfByte;
      ArrayUInt8* res = (ArrayUInt8*)array->doNew(file->ucsize, vm);
      int ok = archive.readFile(res, file);
      if (ok) {
        char* mainClass = findInformation(vm, res, MAIN_CLASS,
                                          LENGTH_MAIN_CLASS);
        if (!mainClass)
          mainClass = findInformation(vm, res, MAIN_LOWER_CLASS,
                                      LENGTH_MAIN_CLASS);
        if (mainClass) {
          className = mainClass;
        } else {
          printf("No Main-Class:  in Manifest of archive %s.\n", jarFile);
        }
      } else {
        printf("Can't extract Manifest file from archive %s\n", jarFile);
      }
    } else {
      printf("Can't find Manifest file in archive %s\n", jarFile);
    }
  } else {
    printf("Can't find archive %s\n", jarFile);
  }
}

void ClArgumentsInfo::nyi() {
  fprintf(stdout, "Not yet implemented\n");
}

void ClArgumentsInfo::printVersion() {
  fprintf(stdout, "JnJVM for Java 1.1 -- 1.5\n");
}

void ClArgumentsInfo::printInformation() {
  fprintf(stdout, 
  "Usage: java [-options] class [args...] (to execute a class)\n"
   "or  java [-options] -jar jarfile [args...]\n"
           "(to execute a jar file) where options include:\n"
    "-client       to select the \"client\" VM\n"
    "-server       to select the \"server\" VM\n"
    "-hotspot      is a synonym for the \"client\" VM  [deprecated]\n"
    "              The default VM is client.\n"
    "\n"
    "-cp <class search path of directories and zip/jar files>\n"
    "-classpath <class search path of directories and zip/jar files>\n"
    "              A : separated list of directories, JAR archives,\n"
    "              and ZIP archives to search for class files.\n"
    "-D<name>=<value>\n"
    "              set a system property\n"
    "-verbose[:class|gc|jni]\n"
    "              enable verbose output\n"
    "-version      print product version and exit\n"
    "-version:<value>\n"
    "              require the specified version to run\n"
    "-showversion  print product version and continue\n"
    "-jre-restrict-search | -jre-no-restrict-search\n"
    "              include/exclude user private JREs in the version search\n"
    "-? -help      print this help message\n"
    "-X            print help on non-standard options\n"
    "-ea[:<packagename>...|:<classname>]\n"
    "-enableassertions[:<packagename>...|:<classname>]\n"
    "              enable assertions\n"
    "-da[:<packagename>...|:<classname>]\n"
    "-disableassertions[:<packagename>...|:<classname>]\n"
    "              disable assertions\n"
    "-esa | -enablesystemassertions\n"
    "              enable system assertions\n"
    "-dsa | -disablesystemassertions\n"
    "              disable system assertions\n"
    "-agentlib:<libname>[=<options>]\n"
    "              load native agent library <libname>, e.g. -agentlib:hprof\n"
    "                see also, -agentlib:jdwp=help and -agentlib:hprof=help\n"
    "-agentpath:<pathname>[=<options>]\n"
    "              load native agent library by full pathname\n"
    "-javaagent:<jarpath>[=<options>]\n"
    "       load Java programming language agent, see java.lang.instrument\n");
}

void ClArgumentsInfo::readArgs(Jnjvm* vm) {
  className = 0;
  appArgumentsPos = 0;
  sint32 i = 1;
  if (i == argc) printInformation();
  while (i < argc) {
    char* cur = argv[i];
    if (!(strcmp(cur, "-client"))) {
      nyi();
    } else if (!(strcmp(cur, "-server"))) {
      nyi();
    } else if (!(strcmp(cur, "-classpath"))) {
      ++i;
      if (i == argc) printInformation();
      else vm->setClasspath(argv[i]);
    } else if (!(strcmp(cur, "-cp"))) {
      ++i;
      if (i == argc) printInformation();
      else vm->setClasspath(argv[i]);
    } else if (!(strcmp(cur, "-debug"))) {
      nyi();
    } else if (!(strncmp(cur, "-D", 2))) {
      uint32 len = strlen(cur);
      if (len == 2) {
        printInformation();
      } else {
        char* key = &cur[2];
        char* value = strchr(key, '=');
        if (!value) {
          printInformation();
          return;
        } else {
          value[0] = 0;
          vm->addProperty(key, &value[1]);
        }
      }
    } else if (!(strncmp(cur, "-Xbootclasspath:", 16))) {
      uint32 len = strlen(cur);
      if (len == 16) {
        printInformation();
      } else {
        char* path = &cur[16];
        vm->bootstrapLoader->analyseClasspathEnv(path);
      }
    } else if (!(strcmp(cur, "-enableassertions"))) {
      nyi();
    } else if (!(strcmp(cur, "-ea"))) {
      nyi();
    } else if (!(strcmp(cur, "-disableassertions"))) {
      nyi();
    } else if (!(strcmp(cur, "-da"))) {
      nyi();
    } else if (!(strcmp(cur, "-enablesystemassertions"))) {
      nyi();
    } else if (!(strcmp(cur, "-esa"))) {
      nyi();
    } else if (!(strcmp(cur, "-disablesystemassertions"))) {
      nyi();
    } else if (!(strcmp(cur, "-dsa"))) {
      nyi();
    } else if (!(strcmp(cur, "-jar"))) {
      ++i;
      if (i == argc) {
        printInformation();
      } else {
        extractClassFromJar(vm, argc, argv, i);
        appArgumentsPos = i;
        return;
      }
    } else if (!(strcmp(cur, "-jre-restrict-research"))) {
      nyi();
    } else if (!(strcmp(cur, "-jre-no-restrict-research"))) {
      nyi();
    } else if (!(strcmp(cur, "-noclassgc"))) {
      nyi();
    } else if (!(strcmp(cur, "-ms"))) {
      nyi();
    } else if (!(strcmp(cur, "-mx"))) {
      nyi();
    } else if (!(strcmp(cur, "-ss"))) {
      nyi();
    } else if (!(strcmp(cur, "-verbose"))) {
      nyi();
    } else if (!(strcmp(cur, "-verbose:class"))) {
      nyi();
    } else if (!(strcmp(cur, "-verbosegc"))) {
      nyi();
    } else if (!(strcmp(cur, "-verbose:gc"))) {
      nyi();
    } else if (!(strcmp(cur, "-verbose:jni"))) {
      nyi();
    } else if (!(strcmp(cur, "-version"))) {
      printVersion();
    } else if (!(strcmp(cur, "-showversion"))) {
      nyi();
    } else if (!(strcmp(cur, "-?"))) {
      printInformation();
    } else if (!(strcmp(cur, "-help"))) {
      printInformation();
    } else if (!(strcmp(cur, "-X"))) {
      nyi();
    } else if (!(strcmp(cur, "-agentlib"))) {
      nyi();
    } else if (!(strcmp(cur, "-agentpath"))) {
      nyi();
    } else if (cur[0] == '-') {
    } else if (!(strcmp(cur, "-javaagent"))) {
      javaAgent(cur);
    } else {
      className = cur;
      appArgumentsPos = i;
      return;
    }
    ++i;
  }
}


void Jnjvm::print(mvm::PrintBuffer* buf) const {
  buf->write("Java isolate");
}

JnjvmClassLoader* Jnjvm::loadAppClassLoader() {
  if (appClassLoader == 0) {
    UserClass* cl = upcalls->newClassLoader;
    JavaObject* loader = 
      upcalls->getSystemClassLoader->invokeJavaObjectStatic(this, cl);
    appClassLoader = JnjvmClassLoader::getJnjvmLoaderFromJavaObject(loader,
                                                                    this);
  }
  return appClassLoader;
}

void Jnjvm::mapInitialThread() {
  upcalls->mapInitialThread(this);
}

void Jnjvm::loadBootstrap() {
  JnjvmClassLoader* loader = bootstrapLoader;
  
  // Initialise the bootstrap class loader if it's not
  // done already.
  if (!bootstrapLoader->upcalls->newString)
    bootstrapLoader->upcalls->initialiseClasspath(bootstrapLoader);
  
#define LOAD_CLASS(cl) \
  cl->resolveClass(); \
  cl->initialiseClass(this);
  
  // If a string belongs to the vm hashmap, we must remove it when
  // it's destroyed. So we define a new VT for strings that will be
  // placed in the hashmap. This VT will have its destructor set so
  // that the string is removed when deallocated.
  upcalls->newString->resolveClass();
  void* stringVT = ((void*)upcalls->newString->getVirtualVT());
  uint32 size = upcalls->newString->virtualTableSize * sizeof(void*);
  if (!JavaString::internStringVT) {
    JavaString::internStringVT = bootstrapLoader->allocator.Allocate(size);
    memcpy(JavaString::internStringVT, stringVT, size);
    ((void**)(JavaString::internStringVT))[VT_DESTRUCTOR_OFFSET] = 
      (void*)(uintptr_t)JavaString::stringDestructor;
  }
  upcalls->newString->initialiseClass(this);

  // To make classes non GC-allocated, we have to bypass the tracer
  // functions of java.lang.Class, java.lang.reflect.Field,
  // java.lang.reflect.Method and java.lang.reflect.constructor. The new
  // tracer functions trace the classloader instead of the class/field/method.
  LOAD_CLASS(upcalls->newClass);
  uintptr_t* ptr = ((uintptr_t*)upcalls->newClass->getVirtualVT());
  ptr[VT_TRACER_OFFSET] = (uintptr_t)JavaObjectClass::staticTracer;

  LOAD_CLASS(upcalls->newConstructor);
  ptr = ((uintptr_t*)upcalls->newConstructor->getVirtualVT());
  ptr[VT_TRACER_OFFSET] = (uintptr_t)JavaObjectConstructor::staticTracer;

  
  LOAD_CLASS(upcalls->newMethod);
  ptr = ((uintptr_t*)upcalls->newMethod->getVirtualVT());
  ptr[VT_TRACER_OFFSET] = (uintptr_t)JavaObjectMethod::staticTracer;
  
  LOAD_CLASS(upcalls->newField);
  ptr = ((uintptr_t*)upcalls->newField->getVirtualVT());
  ptr[VT_TRACER_OFFSET] = (uintptr_t)JavaObjectField::staticTracer;
  
  LOAD_CLASS(upcalls->newVMThread);
  ptr = ((uintptr_t*)upcalls->newVMThread->getVirtualVT());
  ptr[VT_DESTRUCTOR_OFFSET] = (uintptr_t)JavaObjectVMThread::staticDestructor;
  
  LOAD_CLASS(upcalls->newStackTraceElement);
  LOAD_CLASS(upcalls->newVMThrowable);
  LOAD_CLASS(upcalls->boolClass);
  LOAD_CLASS(upcalls->byteClass);
  LOAD_CLASS(upcalls->charClass);
  LOAD_CLASS(upcalls->shortClass);
  LOAD_CLASS(upcalls->intClass);
  LOAD_CLASS(upcalls->longClass);
  LOAD_CLASS(upcalls->floatClass);
  LOAD_CLASS(upcalls->doubleClass);
  LOAD_CLASS(upcalls->InvocationTargetException);
  LOAD_CLASS(upcalls->ArrayStoreException);
  LOAD_CLASS(upcalls->ClassCastException);
  LOAD_CLASS(upcalls->IllegalMonitorStateException);
  LOAD_CLASS(upcalls->IllegalArgumentException);
  LOAD_CLASS(upcalls->InterruptedException);
  LOAD_CLASS(upcalls->IndexOutOfBoundsException);
  LOAD_CLASS(upcalls->ArrayIndexOutOfBoundsException);
  LOAD_CLASS(upcalls->NegativeArraySizeException);
  LOAD_CLASS(upcalls->NullPointerException);
  LOAD_CLASS(upcalls->SecurityException);
  LOAD_CLASS(upcalls->ClassFormatError);
  LOAD_CLASS(upcalls->ClassCircularityError);
  LOAD_CLASS(upcalls->NoClassDefFoundError);
  LOAD_CLASS(upcalls->UnsupportedClassVersionError);
  LOAD_CLASS(upcalls->NoSuchFieldError);
  LOAD_CLASS(upcalls->NoSuchMethodError);
  LOAD_CLASS(upcalls->InstantiationError);
  LOAD_CLASS(upcalls->IllegalAccessError);
  LOAD_CLASS(upcalls->IllegalAccessException);
  LOAD_CLASS(upcalls->VerifyError);
  LOAD_CLASS(upcalls->ExceptionInInitializerError);
  LOAD_CLASS(upcalls->LinkageError);
  LOAD_CLASS(upcalls->AbstractMethodError);
  LOAD_CLASS(upcalls->UnsatisfiedLinkError);
  LOAD_CLASS(upcalls->InternalError);
  LOAD_CLASS(upcalls->OutOfMemoryError);
  LOAD_CLASS(upcalls->StackOverflowError);
  LOAD_CLASS(upcalls->UnknownError);
  LOAD_CLASS(upcalls->ClassNotFoundException); 
#undef LOAD_CLASS

#ifdef SERVICE
  if (!IsolateID)
#endif
  mapInitialThread();
  loadAppClassLoader();
  JavaObject* obj = JavaThread::get()->currentThread();
  JavaObject* javaLoader = appClassLoader->getJavaClassLoader();
#ifdef SERVICE
  if (!IsolateID)
#endif
  upcalls->setContextClassLoader->invokeIntSpecial(this, upcalls->newThread,
                                                   obj, javaLoader);
  // load and initialise math since it is responsible for dlopen'ing 
  // libjavalang.so and we are optimizing some math operations
  UserCommonClass* math = 
    loader->loadName(loader->asciizConstructUTF8("java/lang/Math"), true, true);
  math->asClass()->initialiseClass(this);
}

void Jnjvm::executeClass(const char* className, ArrayObject* args) {
  const UTF8* name = appClassLoader->asciizConstructUTF8(className);
  UserClass* cl = (UserClass*)appClassLoader->loadName(name, true, true);
  cl->initialiseClass(this);
  
  const UTF8* funcSign = 
    appClassLoader->asciizConstructUTF8("([Ljava/lang/String;)V");
  const UTF8* funcName = appClassLoader->asciizConstructUTF8("main");
  JavaMethod* method = cl->lookupMethod(funcName, funcSign, true, true, 0);
  
  try {
    method->invokeIntStatic(this, method->classDef, args);
  }catch(...) {
  }

  JavaObject* exc = JavaThread::get()->pendingException;
  if (exc) {
    JavaThread* th = JavaThread::get();
    th->clearException();
    JavaObject* obj = th->currentThread();
    JavaObject* group = 
      upcalls->group->getObjectField(obj);
    try{
      upcalls->uncaughtException->invokeIntSpecial(this, upcalls->threadGroup,
                                                   group, obj, exc);
    }catch(...) {
      printf("Even uncaught exception throwed an exception!\n");
      assert(0);
    }
  }
}

void Jnjvm::executePremain(const char* className, JavaString* args,
                             JavaObject* instrumenter) {
  const UTF8* name = appClassLoader->asciizConstructUTF8(className);
  UserClass* cl = (UserClass*)appClassLoader->loadName(name, true, true);
  cl->initialiseClass(this);
  
  const UTF8* funcSign = appClassLoader->asciizConstructUTF8(
      "(Ljava/lang/String;Ljava/lang/instrument/Instrumentation;)V");
  const UTF8* funcName = appClassLoader->asciizConstructUTF8("premain");
  JavaMethod* method = cl->lookupMethod(funcName, funcSign, true, true, 0);
  
  method->invokeIntStatic(this, method->classDef, args, instrumenter);
}

void Jnjvm::waitForExit() { 
  
  threadSystem.nonDaemonLock.lock();
  
  while (threadSystem.nonDaemonThreads) {
    threadSystem.nonDaemonVar.wait(&threadSystem.nonDaemonLock);
  }
  
  threadSystem.nonDaemonLock.unlock();

  return;
}

void Jnjvm::mainJavaStart(JavaThread* thread) {
  Jnjvm* vm = thread->getJVM();
  vm->bootstrapThread = thread;

  vm->loadBootstrap();

#ifdef SERVICE
  thread->ServiceException = vm->upcalls->newThrowable->doNew(vm);
#endif

  ClArgumentsInfo& info = vm->argumentsInfo;
  
  if (info.agents.size()) {
    assert(0 && "implement me");
    JavaObject* instrumenter = 0;//createInstrumenter();
    for (std::vector< std::pair<char*, char*> >::iterator i = 
                                                info.agents.begin(),
            e = info.agents.end(); i!= e; ++i) {
      JavaString* args = vm->asciizToStr(i->second);
      vm->executePremain(i->first, args, instrumenter);
    }
  }
    
  UserClassArray* array = vm->bootstrapLoader->upcalls->ArrayOfString;
  ArrayObject* args = (ArrayObject*)array->doNew(info.argc - 2, vm);
  for (int i = 2; i < info.argc; ++i) {
    args->elements[i - 2] = (JavaObject*)vm->asciizToStr(info.argv[i]);
  }

  vm->executeClass(info.className, args);
  
  vm->threadSystem.nonDaemonLock.lock();
  --(vm->threadSystem.nonDaemonThreads);
  if (vm->threadSystem.nonDaemonThreads == 0)
      vm->threadSystem.nonDaemonVar.signal();
  vm->threadSystem.nonDaemonLock.unlock();  
}

#ifdef SERVICE

#include <signal.h>

extern void terminationHandler(int);

static void serviceCPUMonitor(mvm::Thread* th) {
  while (true) {
    sleep(1);
    for(mvm::Thread* cur = (mvm::Thread*)th->next(); cur != th;
        cur = (mvm::Thread*)cur->next()) {
      if (JavaThread::isJavaThread(cur)) {
        JavaThread* th = (JavaThread*)cur;
        if (!(th->StateWaiting)) {
          mvm::VirtualMachine* executingVM = cur->MyVM;
          assert(executingVM && "Thread with no VM!");
          ++executingVM->executionTime;
        }
      }
    }
  }
}
#endif

void Jnjvm::runApplication(int argc, char** argv) {
  argumentsInfo.argc = argc;
  argumentsInfo.argv = argv;
  argumentsInfo.readArgs(this);
  if (argumentsInfo.className) {
    int pos = argumentsInfo.appArgumentsPos;
    
    argumentsInfo.argv = argumentsInfo.argv + pos - 1;
    argumentsInfo.argc = argumentsInfo.argc - pos + 1;
#ifdef SERVICE
    struct sigaction sa;
    sigset_t mask;
    sigfillset(&mask);
    sigaction(SIGUSR1, 0, &sa);
    sa.sa_mask = mask;
    sa.sa_handler = terminationHandler;
    sa.sa_flags |= SA_RESTART;
    sigaction(SIGUSR1, &sa, NULL);

    mvm::Thread* th = new JavaThread(0, 0, this);
    th->start(serviceCPUMonitor);
#endif
    
    bootstrapThread = new JavaThread(0, 0, this);
    bootstrapThread->start((void (*)(mvm::Thread*))mainJavaStart);
  } else {
    threadSystem.nonDaemonThreads = 0;
  }
}

Jnjvm::Jnjvm(JnjvmBootstrapLoader* loader) : VirtualMachine() {

  classpath = getenv("CLASSPATH");
  if (!classpath) classpath = ".";
  
  appClassLoader = 0;
  jniEnv = &JNI_JNIEnvTable;
  javavmEnv = &JNI_JavaVMTable;
  

  bootstrapLoader = loader;
  upcalls = bootstrapLoader->upcalls;

  throwable = upcalls->newThrowable;
 
#ifdef ISOLATE
  IsolateLock.lock();
  for (uint32 i = 0; i < NR_ISOLATES; ++i) {
    if (RunningIsolates[i] == 0) {
      RunningIsolates[i] = this;
      IsolateID = i;
      break;
    }
  }
  IsolateLock.unlock();
#endif

}

Jnjvm::~Jnjvm() {
#ifdef ISOLATE
  RunningIsolates[IsolateID] = 0;
#endif
}

const UTF8* Jnjvm::asciizToInternalUTF8(const char* asciiz) {
  uint32 size = strlen(asciiz);
  UTF8* tmp = (UTF8*)upcalls->ArrayOfChar->doNew(size, this);
  uint16* buf = tmp->elements;
  
  for (uint32 i = 0; i < size; i++) {
    if (asciiz[i] == '.') buf[i] = '/';
    else buf[i] = asciiz[i];
  }
  return (const UTF8*)tmp;

}
  
const UTF8* Jnjvm::asciizToUTF8(const char* asciiz) {
  uint32 size = strlen(asciiz);
  UTF8* tmp = (UTF8*)upcalls->ArrayOfChar->doNew(size, this);
  uint16* buf = tmp->elements;
  
  for (uint32 i = 0; i < size; i++) {
    buf[i] = asciiz[i];
  }
  return (const UTF8*)tmp;
}


static void compileClass(Class* cl) {

  for (uint32 i = 0; i < cl->nbVirtualMethods; ++i) {
    JavaMethod& meth = cl->virtualMethods[i];
    if (!isAbstract(meth.access))
      cl->classLoader->getModuleProvider()->parseFunction(&meth);
  }
  
  for (uint32 i = 0; i < cl->nbStaticMethods; ++i) {
    JavaMethod& meth = cl->staticMethods[i];
    if (!isAbstract(meth.access))
      cl->classLoader->getModuleProvider()->parseFunction(&meth);
  }
}

static const char* name;

void Jnjvm::mainCompilerStart(JavaThread* th) {
  
  Jnjvm* vm = th->getJVM();
  try {
    JnjvmBootstrapLoader* bootstrapLoader = vm->bootstrapLoader;

    bootstrapLoader->analyseClasspathEnv(vm->classpath);
    bootstrapLoader->upcalls->initialiseClasspath(bootstrapLoader);
  
    uint32 size = strlen(name);
    if (size > 4 && 
       (!strcmp(&name[size - 4], ".jar") || !strcmp(&name[size - 4], ".zip"))) {
  

      std::vector<Class*> classes;

      ArrayUInt8* bytes = Reader::openFile(bootstrapLoader, name);
      if (!bytes) vm->unknownError("Can't find zip file.");
      ZipArchive archive(bytes, bootstrapLoader->allocator);
    
      char* realName = (char*)alloca(4096);
      for (ZipArchive::table_iterator i = archive.filetable.begin(), 
           e = archive.filetable.end(); i != e; ++i) {
        ZipFile* file = i->second;
      
        size = strlen(file->filename);
        if (size > 6 && !strcmp(&(file->filename[size - 6]), ".class")) {
          UserClassArray* array = bootstrapLoader->upcalls->ArrayOfByte;
          ArrayUInt8* res = 
            (ArrayUInt8*)array->doNew(file->ucsize, bootstrapLoader->allocator);
          int ok = archive.readFile(res, file);
          if (!ok) vm->unknownError("Wrong zip file.");
      
        
          memcpy(realName, file->filename, size);
          realName[size - 6] = 0;
          const UTF8* utf8 = bootstrapLoader->asciizConstructUTF8(realName);
          Class* cl = bootstrapLoader->constructClass(utf8, res);
          classes.push_back(cl);
        }
      }

      // First resolve everyone so that there can not be unknown references in
      // constant pools.
      for (std::vector<Class*>::iterator i = classes.begin(),
           e = classes.end(); i != e; ++i) {
        Class* cl = *i;
        cl->resolveClass();
      }
      
      for (std::vector<Class*>::iterator i = classes.begin(), e = classes.end();
           i != e; ++i) {
        Class* cl = *i;
        if (!cl->isInterface()) compileClass(cl);
      }

    } else {

      const UTF8* utf8 = bootstrapLoader->asciizConstructUTF8(name);
      UserClass* cl = bootstrapLoader->loadName(utf8, true, true);
      compileClass(cl);
    }
   
    // Set the linkage to External, so that the printer does not complain.
    llvm::Module* M = bootstrapLoader->getModule();
    for (Module::iterator i = M->begin(), e = M->end(); i != e; ++i) {
      i->setLinkage(llvm::GlobalValue::ExternalLinkage);
    }

    // Print stats before quitting.
    bootstrapLoader->getModule()->printStats();
    bootstrapLoader->getModuleProvider()->printStats();

  } catch(std::string str) {
    fprintf(stderr, "Error : %s\n", str.c_str());
  }

  vm->threadSystem.nonDaemonLock.lock();
  --(vm->threadSystem.nonDaemonThreads);
  if (vm->threadSystem.nonDaemonThreads == 0)
      vm->threadSystem.nonDaemonVar.signal();
  vm->threadSystem.nonDaemonLock.unlock();  
  
}

void Jnjvm::compile(const char* n) {
  name = n;
  bootstrapThread = new JavaThread(0, 0, this);
  bootstrapThread->start((void (*)(mvm::Thread*))mainCompilerStart);
}



void Jnjvm::addMethodInFunctionMap(JavaMethod* meth, void* addr) {
  FunctionMapLock.acquire();
  JavaFunctionMap.insert(std::make_pair(addr, meth));
  FunctionMapLock.release();
}

void Jnjvm::removeMethodsInFunctionMap(JnjvmClassLoader* loader) {
  // Loop over all methods in the map to find which ones belong
  // to this class loader.
  FunctionMapLock.acquire();
  std::map<void*, JavaMethod*>::iterator temp;
  for (std::map<void*, JavaMethod*>::iterator i = JavaFunctionMap.begin(), 
       e = JavaFunctionMap.end(); i != e;) {
    if (i->second->classDef->classLoader == loader) {
      temp = i;
      ++i;
      JavaFunctionMap.erase(temp);
    } else {
      ++i;
    }
  }
  FunctionMapLock.release();
}

JavaMethod* Jnjvm::IPToJavaMethod(void* Addr) {
  FunctionMapLock.acquire();
  std::map<void*, JavaMethod*>::iterator I = JavaFunctionMap.upper_bound(Addr);
  assert(I != JavaFunctionMap.begin() && "Wrong value in function map");
  FunctionMapLock.release();

  // Decrement because we had the "greater than" value.
  I--;
  return I->second;

}
