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

#define DEF_UTF8(var) \
  const UTF8* Jnjvm::var = 0
  
  DEF_UTF8(NoClassDefFoundError);
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

#ifndef MULTIPLE_VM
/// If we're not in a multi-vm environment, this can be made static.
std::vector<void*> Jnjvm::nativeLibs;
#endif

typedef void (*clinit_t)(Jnjvm* vm);

void UserCommonClass::initialiseClass(Jnjvm* vm) {
  // Primitives are initialized at boot time
  if (isArray()) {
    status = ready;
  } else if (status != ready) {
    acquire();
    if (status == ready) {
      release();
    } else if (status >= resolved && status != clinitParent &&
               status != inClinit) {
      UserClass* cl = (UserClass*)this;
      status = clinitParent;
      release();
      UserCommonClass* super = getSuper();
      if (super) {
        super->initialiseClass(vm);
      }
      
      cl->resolveStaticClass();
      
      status = inClinit;
      JavaMethod* meth = lookupMethodDontThrow(Jnjvm::clinitName,
                                               Jnjvm::clinitType, true,
                                               false);
      
      PRINT_DEBUG(JNJVM_LOAD, 0, COLOR_NORMAL, "; ", 0);
      PRINT_DEBUG(JNJVM_LOAD, 0, LIGHT_GREEN, "clinit ", 0);
      PRINT_DEBUG(JNJVM_LOAD, 0, COLOR_NORMAL, "%s\n", printString());
      
      JavaObject* val = 
        (JavaObject*)vm->allocator.allocateObject(cl->getStaticSize(),
                                                  cl->getStaticVT());
      val->initialise(cl);
      CommonClass::field_map* map = cl->getStaticFields();
      for (CommonClass::field_iterator i = map->begin(), e = map->end(); i!= e;
           ++i) { 
        i->second->initField(val);
      }
  
      cl->setStaticInstance(val);

      if (meth) {
        JavaObject* exc = 0;
        try{
          clinit_t pred = (clinit_t)(intptr_t)meth->compiledPtr();
          pred(vm);
        } catch(...) {
          exc = JavaThread::getJavaException();
          assert(exc && "no exception?");
          JavaThread::clearException();
        }
        if (exc) {
          if (exc->classOf->isAssignableFrom(vm->upcalls->newException)) {
            JavaThread::get()->isolate->initializerError(exc);
          } else {
            JavaThread::throwException(exc);
          }
        }
      }
      
      status = ready;
      broadcastClass();
    } else if (status < resolved) {
      release();
      vm->unknownError("try to clinit a not-read class...");
    } else {
      if (!ownerClass()) {
        while (status < ready) waitClass();
        release();
        initialiseClass(vm);
      } 
      release();
    }
  }
}


void Jnjvm::errorWithExcp(UserClass* cl, JavaMethod* init, const JavaObject* excp) {
  JavaObject* obj = cl->doNew(this);
  init->invokeIntSpecial(this, cl, obj, excp);
  JavaThread::throwException(obj);
}

void Jnjvm::error(UserClass* cl, JavaMethod* init, const char* fmt, ...) {
  char* tmp = (char*)alloca(4096);
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(tmp, 4096, fmt, ap);
  va_end(ap);
  
  JavaObject* obj = cl->doNew(this);
  init->invokeIntSpecial(this, cl, obj, asciizToStr(tmp));
  JavaThread::throwException(obj);
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

void Jnjvm::classCastException(const char* msg) {
  error(upcalls->ClassCastException,
        upcalls->InitClassCastException,
        msg);
}

void Jnjvm::noSuchFieldError(CommonClass* cl, const UTF8* name) {
  error(upcalls->NoSuchFieldError,
        upcalls->InitNoSuchFieldError, 
        "unable to find %s in %s",
        name->printString(), cl->printString());

}

void Jnjvm::noSuchMethodError(CommonClass* cl, const UTF8* name) {
  error(upcalls->NoSuchMethodError,
        upcalls->InitNoSuchMethodError, 
        "unable to find %s in %s",
        name->printString(), cl->printString());

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

void Jnjvm::noClassDefFoundError(const char* fmt, ...) {
  error(upcalls->NoClassDefFoundError,
        upcalls->InitNoClassDefFoundError, 
        fmt);
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

JavaString* Jnjvm::UTF8ToStr(const UTF8* utf8) { 
  JavaString* res = hashStr->lookupOrCreate(utf8, this, JavaString::stringDup);
  return res;
}

JavaString* Jnjvm::asciizToStr(const char* asciiz) {
  // asciizToStr is called by jnjvm code, so utf8s created
  // by this method are stored in the bootstrap class loader
  const UTF8* var = bootstrapLoader->asciizConstructUTF8(asciiz);
  return UTF8ToStr(var);
}

void Jnjvm::addProperty(char* key, char* value) {
  postProperties.push_back(std::make_pair(key, value));
}

JavaObject* UserCommonClass::getClassDelegatee(Jnjvm* vm, JavaObject* pd) {
  acquire();
  if (!(delegatee)) {
    UserClass* cl = vm->upcalls->newClass;
    JavaObject* delegatee = cl->doNew(vm);
    if (!pd) {
      vm->upcalls->initClass->invokeIntSpecial(vm, cl, delegatee, this);
    } else {
      vm->upcalls->initClassWithProtectionDomain->invokeIntSpecial(vm, cl,
                                                                   delegatee,
                                                                   this, pd);
    }
    this->delegatee = delegatee;
  }
  release();
  return delegatee;
}

Jnjvm::~Jnjvm() {
#ifdef MULTIPLE_GC
  GC->destroy();
  delete GC;
#endif
  
  delete hashStr;  
  delete globalRefsLock;
  delete threadSystem;
}

Jnjvm::Jnjvm() {
#ifdef MULTIPLE_GC
  GC = 0;
#endif
  hashStr = 0;
  globalRefsLock = 0;
  threadSystem = 0;
}

#define PATH_MANIFEST "META-INF/MANIFEST.MF"
#define MAIN_CLASS "Main-Class: "
#define PREMAIN_CLASS "Premain-Class: "
#define BOOT_CLASS_PATH "Boot-Class-Path: "
#define CAN_REDEFINE_CLASS_PATH "Can-Redefine-Classes: "

#define LENGTH_MAIN_CLASS 12
#define LENGTH_PREMAIN_CLASS 15
#define LENGTH_BOOT_CLASS_PATH 17

extern "C" struct JNINativeInterface JNI_JNIEnvTable;
extern "C" const struct JNIInvokeInterface JNI_JavaVMTable;

namespace jnjvm {

class ClArgumentsInfo {
public:
  uint32 appArgumentsPos;
  char* className;
  std::vector< std::pair<char*, char*> > agents;

  void readArgs(int argc, char** argv, Jnjvm *vm);
  void extractClassFromJar(Jnjvm* vm, int argc, char** argv, int i);
  void javaAgent(char* cur);

  void printInformation();
  void nyi();
  void printVersion();
};

}

void ClArgumentsInfo::javaAgent(char* cur) {
  assert(0 && "implement me");
}

extern "C" int sys_strnstr(const char *haystack, const char *needle) {
  char * res = strstr(haystack, needle);
  if (res) return res - haystack;
  else return -1; 
}


static char* findInformation(ArrayUInt8* manifest, const char* entry, uint32 len) {
  uint8* ptr = manifest->elements;
  sint32 index = sys_strnstr((char*)ptr, entry);
  if (index != -1) {
    index += len;
    sint32 end = sys_strnstr((char*)&(ptr[index]), "\n");
    if (end == -1) end = manifest->size;
    else end += index;

    sint32 length = end - index - 1;
    char* name = (char*)malloc(length + 1);
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
  char* temp = 
    (char*)malloc(2 + strlen(vm->classpath) + strlen(jarFile));

  sprintf(temp, "%s:%s", vm->classpath, jarFile);
  vm->setClasspath(temp);
  
  ArrayUInt8* bytes = Reader::openFile(JnjvmClassLoader::bootstrapLoader,
                                       jarFile);

  ZipArchive archive(bytes);
  if (archive.getOfscd() != -1) {
    ZipFile* file = archive.getFile(PATH_MANIFEST);
    if (file) {
      UserClassArray* array = vm->bootstrapLoader->upcalls->ArrayOfByte;
      ArrayUInt8* res = ArrayUInt8::acons(file->ucsize, array, &vm->allocator);
      int ok = archive.readFile(res, file);
      if (ok) {
        char* mainClass = findInformation(res, MAIN_CLASS, LENGTH_MAIN_CLASS);
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
    "              load Java programming language agent, see java.lang.instrument\n");
}

void ClArgumentsInfo::readArgs(int argc, char** argv, Jnjvm* vm) {
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
        JnjvmClassLoader::bootstrapLoader->analyseClasspathEnv(path);
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
  buf->write("Java isolate: ");
  buf->write(name);

}

JnjvmClassLoader* Jnjvm::loadAppClassLoader() {
  if (appClassLoader == 0) {
    UserClass* cl = upcalls->newClassLoader;
    JavaObject* loader = 
      upcalls->getSystemClassLoader->invokeJavaObjectStatic(this, cl);
    appClassLoader = JnjvmClassLoader::getJnjvmLoaderFromJavaObject(loader);
  }
  return appClassLoader;
}

void Jnjvm::mapInitialThread() {
  upcalls->mapInitialThread(this);
}

void Jnjvm::loadBootstrap() {
  JnjvmClassLoader* loader = JnjvmClassLoader::bootstrapLoader;
#define LOAD_CLASS(cl) \
  cl->resolveClass(); \
  cl->initialiseClass(this);

  LOAD_CLASS(upcalls->newClass);
  LOAD_CLASS(upcalls->newConstructor);
  LOAD_CLASS(upcalls->newString);
  LOAD_CLASS(upcalls->newMethod);
  LOAD_CLASS(upcalls->newField);
  LOAD_CLASS(upcalls->newStackTraceElement);
  LOAD_CLASS(upcalls->newVMThrowable);
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

  mapInitialThread();
  loadAppClassLoader();
  JavaObject* obj = JavaThread::currentThread();
  JavaObject* javaLoader = appClassLoader->getJavaClassLoader();
  upcalls->setContextClassLoader->invokeIntSpecial(this, upcalls->newThread,
                                                   obj, javaLoader);
  // load and initialise math since it is responsible for dlopen'ing 
  // libjavalang.so and we are optimizing some math operations
  UserCommonClass* math = 
    loader->loadName(loader->asciizConstructUTF8("java/lang/Math"), true, true);
  math->initialiseClass(this);
}

void Jnjvm::executeClass(const char* className, ArrayObject* args) {
  try {
    JavaJIT::invokeOnceVoid(this, appClassLoader, className, "main",
                        "([Ljava/lang/String;)V", ACC_STATIC, args);
  }catch(...) {
  }

  JavaObject* exc = JavaThread::get()->pendingException;
  if (exc) {
    JavaThread::clearException();
    JavaObject* obj = JavaThread::currentThread();
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
  JavaJIT::invokeOnceVoid(this, appClassLoader, className, "premain",
          "(Ljava/lang/String;Ljava/lang/instrument/Instrumentation;)V",
          ACC_STATIC, args, instrumenter);
}

void Jnjvm::waitForExit() { 
  threadSystem->nonDaemonLock->lock();
  --(threadSystem->nonDaemonThreads);
  
  while (threadSystem->nonDaemonThreads) {
    threadSystem->nonDaemonVar->wait(threadSystem->nonDaemonLock);
  }

  threadSystem->nonDaemonLock->unlock();  
  return;
}

void Jnjvm::runMain(int argc, char** argv) {
  ClArgumentsInfo info;

  info.readArgs(argc, argv, this);
  if (info.className) {
    int pos = info.appArgumentsPos;
    //llvm::cl::ParseCommandLineOptions(pos, argv,
    //                                  " JnJVM Java Virtual Machine\n");
    argv = argv + pos - 1;
    argc = argc - pos + 1;
    
    loadBootstrap();
#ifdef SERVICE_VM
    ServiceDomain::initialise((ServiceDomain*)this);
#endif
    
    if (info.agents.size()) {
      assert(0 && "implement me");
      JavaObject* instrumenter = 0;//createInstrumenter();
      for (std::vector< std::pair<char*, char*> >::iterator i = 
                                                  info.agents.begin(),
              e = info.agents.end(); i!= e; ++i) {
        JavaString* args = asciizToStr(i->second);
        executePremain(i->first, args, instrumenter);
      }
    }
    
    UserClassArray* array = bootstrapLoader->upcalls->ArrayOfString;
    ArrayObject* args = ArrayObject::acons(argc - 2, array, &allocator);
    for (int i = 2; i < argc; ++i) {
      args->elements[i - 2] = (JavaObject*)asciizToStr(argv[i]);
    }

    executeClass(info.className, args);
    waitForExit();
  }
}

void Jnjvm::runIsolate(const char* className, ArrayObject* args) {
  Jnjvm *isolate = allocateIsolate();
  isolate->loadBootstrap();
  isolate->executeClass(className, args);
  isolate->waitForExit();
}

Jnjvm* Jnjvm::allocateIsolate() {
  Jnjvm *isolate= gc_new(Jnjvm)();

#ifdef MULTIPLE_GC
  isolate->GC = Collector::allocate();
#endif 
  isolate->classpath = getenv("CLASSPATH");
  if (!(isolate->classpath)) {
    isolate->classpath = ".";
  }
  
  isolate->bootstrapThread = allocator_new(&isolate->allocator, JavaThread)();
  isolate->bootstrapThread->initialise(0, isolate);
  void* baseSP = mvm::Thread::get()->baseSP;
  isolate->bootstrapThread->threadID = (mvm::Thread::self() << 8) & 0x7FFFFF00;
  
#ifdef MULTIPLE_GC
  isolate->bootstrapThread->GC = isolate->GC;
  isolate->GC->inject_my_thread(baseSP);
#endif 
  isolate->bootstrapThread->baseSP = baseSP;
  JavaThread::threadKey->set(isolate->bootstrapThread);
  
  isolate->threadSystem = new ThreadSystem();
  isolate->name = "isolate";
  isolate->appClassLoader = 0;
  isolate->jniEnv = &JNI_JNIEnvTable;
  isolate->javavmEnv = &JNI_JavaVMTable;
  
  isolate->hashStr = new StringMap();
  isolate->globalRefsLock = mvm::Lock::allocNormal();
  isolate->bootstrapLoader = JnjvmClassLoader::bootstrapLoader;
  isolate->upcalls = isolate->bootstrapLoader->upcalls;

#ifdef MULTIPLE_VM
  isolate->throwable = isolate->upcalls->newThrowable;
  isolate->arrayClasses[1] = isolate->upcalls->ArrayOfBool;
  isolate->arrayClasses[2] = isolate->upcalls->ArrayOfByte;
  isolate->arrayClasses[3] = isolate->upcalls->ArrayOfChar;
  isolate->arrayClasses[4] = isolate->upcalls->ArrayOfShort;
  isolate->arrayClasses[5] = isolate->upcalls->ArrayOfInt;
  isolate->arrayClasses[6] = isolate->upcalls->ArrayOfFloat;
  isolate->arrayClasses[7] = isolate->upcalls->ArrayOfLong;
  isolate->arrayClasses[8] = isolate->upcalls->ArrayOfDouble;
#endif

  return isolate;
}
