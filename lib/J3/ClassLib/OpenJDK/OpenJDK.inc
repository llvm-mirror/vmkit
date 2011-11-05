//===---------- OpenJDK.inc - OpenJDK JVM Interface Definition ------------===//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "jvm.h"

#include "ArrayCopy.inc"
#include "JavaConstantPool.h"
#include "Reader.h"
#include "SetProperties.inc"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#define NYI() \
  do{ assert(0 && "Not yet implemented!"); abort(); } while(0)

JavaObject* internalFillInStackTrace(JavaObject* throwable) {

  ArrayPtr* result = 0;
  llvm_gcroot(throwable, 0);
  llvm_gcroot(result, 0);

  JavaThread* th = JavaThread::get();
  Jnjvm* vm = th->getJVM();
  assert(th);
  assert(vm);

  uint32 length = th->getFrameContextLength();

#ifndef ARCH_64
    ClassArray* cl = vm->upcalls->ArrayOfInt;
    result = (ArrayPtr*) cl->doNew(length, vm);
#else
    ClassArray* cl = vm->upcalls->ArrayOfLong;
    result = (ArrayPtr*) cl->doNew(length, vm);
#endif

  // Don't call th->getFrameContext because it is not GC-safe.
  mvm::StackWalker Walker(th);
  uint32_t i = 0;

  while (intptr_t ip = *Walker) {
    ArrayPtr::setElement(result, ip, i);
    ++i;
    ++Walker;
  }

  return result;
}

JavaObject* consStackElement(mvm::FrameInfo* FI, intptr_t ip) {

  JavaString* methodName = 0;
  JavaString* className = 0;
  JavaString* sourceName = 0;
  JavaObject* res = 0;
  llvm_gcroot(methodName, 0);
  llvm_gcroot(className, 0);
  llvm_gcroot(sourceName, 0);
  llvm_gcroot(res, 0);

  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaMethod* meth = (JavaMethod*)FI->Metadata;
  methodName = vm->internalUTF8ToStr(meth->name);
  Class* cl = meth->classDef;
  className = JavaString::internalToJava(cl->name, vm);

  Attribut* sourceAtt = cl->lookupAttribut(Attribut::sourceFileAttribut);

  if (sourceAtt) {
    Reader reader(sourceAtt, cl->bytes);
    uint16 index = reader.readU2();
    sourceName = vm->internalUTF8ToStr(cl->getConstantPool()->UTF8At(index));
  }

  uint16 lineNumber = meth->lookupLineNumber(FI);

  UserClass* newS = vm->upcalls->newStackTraceElement;
  res = newS->doNew(vm);
  vm->upcalls->initStackTraceElement->invokeIntSpecial(vm, newS, res,
                                                       &className,
                                                       &methodName,
                                                       &sourceName,
                                                       lineNumber);
  return res;
}

JNIEXPORT jint JNICALL
JVM_GetInterfaceVersion(void) {
  return JVM_INTERFACE_VERSION;
}

jint JVM_CreateJavaVM(JavaVM **pvm, void **penv, void *args) {
  NYI();
}

JNIEXPORT jint JNICALL
JVM_IHashCode(JNIEnv *env, jobject _obj) {
  JavaObject * obj = *(JavaObject**)_obj;
  llvm_gcroot(obj, 0);

  jint hash = (obj == NULL) ? 0 : JavaObject::hashCode(obj);
  return hash;
}

JNIEXPORT void JNICALL
JVM_MonitorWait(JNIEnv *env, jobject obj, jlong ms) {
  JavaObject * o = 0;
  llvm_gcroot(o, 0);

  BEGIN_JNI_EXCEPTION
  Jnjvm* vm = JavaThread::get()->getJVM();

  o = *(JavaObject**)obj;

  if (ms < 0) {
    vm->illegalArgumentException("Negative wait time specified");
  }
  if (ms) {
    struct timeval info;
    uint64_t usec = ms * 1000LL;
    info.tv_usec = usec % 1000000LL;
    info.tv_sec = usec / 1000000LL;
    JavaObject::timedWait(o, info);
  } else {
    JavaObject::wait(o);
  }
  RETURN_VOID_FROM_JNI
  END_JNI_EXCEPTION
}

JNIEXPORT void JNICALL
JVM_MonitorNotify(JNIEnv *env, jobject obj) {
  BEGIN_JNI_EXCEPTION
  JavaObject * o = *(JavaObject**)obj;
  JavaObject::notify(o);
  RETURN_VOID_FROM_JNI
  END_JNI_EXCEPTION
}

JNIEXPORT void JNICALL
JVM_MonitorNotifyAll(JNIEnv *env, jobject obj) {
  BEGIN_JNI_EXCEPTION
  JavaObject * o = *(JavaObject**)obj;
  JavaObject::notifyAll(o);
  RETURN_VOID_FROM_JNI
  END_JNI_EXCEPTION
}

JNIEXPORT jobject JNICALL
JVM_Clone(JNIEnv *env, jobject obj) {
  NYI();
}

/*
 * java.lang.String
 */
JNIEXPORT jstring JNICALL
JVM_InternString(JNIEnv *env, jstring _str) {
  JavaString * str = *(JavaString**)_str;
  const ArrayUInt16* array = 0;
  JavaString * res = 0;
  llvm_gcroot(str, 0);
  llvm_gcroot(array, 0);
  llvm_gcroot(res, 0);

  // If already intern'd, just return.
  if(str->getVirtualTable() == JavaString::internStringVT)
    return _str;

  BEGIN_JNI_EXCEPTION

  Jnjvm* vm = JavaThread::get()->getJVM();
  array = JavaString::strToArray(str, vm);
  res = vm->constructString(array);

  RETURN_FROM_JNI((jstring)th->pushJNIRef(res))

  END_JNI_EXCEPTION

  return 0;
}

/*
 * java.lang.System
 */
JNIEXPORT jlong JNICALL
JVM_CurrentTimeMillis(JNIEnv *env, jclass ignored) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  jlong time = (tv.tv_sec * 1000LL) + (tv.tv_usec / 1000LL);
  return time;
}

JNIEXPORT jlong JNICALL
JVM_NanoTime(JNIEnv *env, jclass ignored) {
  // TODO: Impl with more accurate clock?
  struct timeval tv;

  gettimeofday(&tv, NULL);

  jlong time = (tv.tv_sec * 1000000LL) + (tv.tv_usec / 1000000LL);

  return time;
}

JNIEXPORT void JNICALL
JVM_ArrayCopy(JNIEnv *env, jclass ignored, jobject jsrc, jint sstart,
              jobject jdst, jint dstart, jint len) {
  JavaObject *src = 0;
  JavaObject *dst = 0;
  JavaObject* cur = 0;
  llvm_gcroot(src, 0);
  llvm_gcroot(dst, 0);
  llvm_gcroot(cur, 0);

  BEGIN_JNI_EXCEPTION

  src = *(JavaObject**)jsrc;
  dst = *(JavaObject**)jdst;

  ArrayCopy(src, sstart, dst, dstart, len);

  RETURN_VOID_FROM_JNI
  END_JNI_EXCEPTION
}

JNIEXPORT jobject JNICALL
JVM_InitProperties(JNIEnv *env, jobject p) {
  BEGIN_JNI_EXCEPTION
  JavaObject * prop = *(JavaObject**)p;
  llvm_gcroot(prop, 0);
  setProperties(prop);
  setCommandLineProperties(prop);

  RETURN_FROM_JNI(p);

  END_JNI_EXCEPTION

  return 0;
}

/*
 * java.io.File
 */
JNIEXPORT void JNICALL
JVM_OnExit(void (*func)(void)) {
  NYI();
}

/*
 * java.lang.Runtime
 */
JNIEXPORT void JNICALL
JVM_Exit(jint code) {
  mvm::System::Exit(code);
}

JNIEXPORT void JNICALL
JVM_Halt(jint code) {
  mvm::System::Exit(code);
}

JNIEXPORT void JNICALL
JVM_GC(void) {
  BEGIN_JNI_EXCEPTION
  mvm::Collector::collect();
  RETURN_VOID_FROM_JNI;
  END_JNI_EXCEPTION
}

/* Returns the number of real-time milliseconds that have elapsed since the
 * least-recently-inspected heap object was last inspected by the garbage
 * collector.
 *
 * For simple stop-the-world collectors this value is just the time
 * since the most recent collection.  For generational collectors it is the
 * time since the oldest generation was most recently collected.  Other
 * collectors are free to return a pessimistic estimate of the elapsed time, or
 * simply the time since the last full collection was performed.
 *
 * Note that in the presence of reference objects, a given object that is no
 * longer strongly reachable may have to be inspected multiple times before it
 * can be reclaimed.
 */
JNIEXPORT jlong JNICALL
JVM_MaxObjectInspectionAge(void) {
  NYI();
}

JNIEXPORT void JNICALL
JVM_TraceInstructions(jboolean on) {
  NYI();
}

JNIEXPORT void JNICALL
JVM_TraceMethodCalls(jboolean on) {
  NYI();
}

JNIEXPORT jlong JNICALL
JVM_TotalMemory(void) {
  return (jlong)mvm::Collector::getTotalMemory();
}

JNIEXPORT jlong JNICALL
JVM_FreeMemory(void) {
  return (jlong)mvm::Collector::getFreeMemory();
}

JNIEXPORT jlong JNICALL
JVM_MaxMemory(void) {
  return (jlong)mvm::Collector::getMaxMemory();
}

JNIEXPORT jint JNICALL
JVM_ActiveProcessorCount(void) {
  return mvm::System::GetNumberOfProcessors();
}

JNIEXPORT void * JNICALL
JVM_LoadLibrary(const char *name) {
  BEGIN_JNI_EXCEPTION
  Jnjvm* vm = JavaThread::get()->getJVM();
  RETURN_FROM_JNI(vm->bootstrapLoader->loadLib(name));

  END_JNI_EXCEPTION

  return 0;
}

JNIEXPORT void JNICALL
JVM_UnloadLibrary(void * handle) {
  NYI();
}

JNIEXPORT void * JNICALL
JVM_FindLibraryEntry(void *handle, const char *name) {
  BEGIN_JNI_EXCEPTION
  Jnjvm* vm = JavaThread::get()->getJVM();
  bool j3 = false;
  RETURN_FROM_JNI((void *)vm->bootstrapLoader->loadInLib(name, j3));

  END_JNI_EXCEPTION

  return 0;
}

JNIEXPORT jboolean JNICALL
JVM_IsSupportedJNIVersion(jint version) {
  NYI();
}

/*
 * java.lang.Float and java.lang.Double
 */
JNIEXPORT jboolean JNICALL
JVM_IsNaN(jdouble d) {
  NYI();
}

/*
 * java.lang.Throwable
 */
JNIEXPORT void JNICALL
JVM_FillInStackTrace(JNIEnv *env, jobject throwable) {
  JavaObjectThrowable * T = 0;
  llvm_gcroot(T, 0);

  BEGIN_JNI_EXCEPTION

  T = *(JavaObjectThrowable**)throwable;
  JavaObjectThrowable::fillInStackTrace(T);

  RETURN_VOID_FROM_JNI;

  END_JNI_EXCEPTION
}

JNIEXPORT void JNICALL
JVM_PrintStackTrace(JNIEnv *env, jobject throwable, jobject printable) {
  NYI();
}

JNIEXPORT jint JNICALL
JVM_GetStackTraceDepth(JNIEnv *env, jobject throwable) {
  JavaObjectThrowable * T = 0;
  llvm_gcroot(T, 0);

  jint res = -1;

  BEGIN_JNI_EXCEPTION

  T = *(JavaObjectThrowable**)throwable;
  res = JavaObjectThrowable::getStackTraceDepth(T);

  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION

  return 0;
}

JNIEXPORT jobject JNICALL
JVM_GetStackTraceElement(JNIEnv *env, jobject throwable, jint index) {
  JavaObjectThrowable * T = 0;
  JavaObject * result = 0;
  JavaObject * stack = 0;
  llvm_gcroot(T, 0);
  llvm_gcroot(result, 0);
  llvm_gcroot(stack, 0);

  BEGIN_JNI_EXCEPTION

  T = *(JavaObjectThrowable**)throwable;
  Jnjvm* vm = JavaThread::get()->getJVM();
  stack = vm->upcalls->backtrace->getInstanceObjectField(T);
  verifyNull(stack);

  sint32 base = JavaObjectThrowable::getStackTraceBase(T);

  sint32 cur = 0;
  for (sint32 i = base; i < JavaArray::getSize(stack); ++i) {
    intptr_t ip = ArrayPtr::getElement((ArrayPtr*)stack, i);
    mvm::FrameInfo* FI = vm->IPToFrameInfo(ip);
    if (FI->Metadata != NULL) {
      if (cur == index) {
        result = consStackElement(FI, ip);
        break;
      }
      cur++;
    }
  }

  assert(result && "No stack element found");
  RETURN_FROM_JNI((jobject)th->pushJNIRef(result));

  END_JNI_EXCEPTION

  return 0;
}

/*
 * java.lang.Compiler
 */
JNIEXPORT void JNICALL
JVM_InitializeCompiler (JNIEnv *env, jclass compCls) {
  NYI();
}

JNIEXPORT jboolean JNICALL
JVM_IsSilentCompiler(JNIEnv *env, jclass compCls) {
  NYI();
}

JNIEXPORT jboolean JNICALL
JVM_CompileClass(JNIEnv *env, jclass compCls, jclass cls) {
  NYI();
}

JNIEXPORT jboolean JNICALL
JVM_CompileClasses(JNIEnv *env, jclass cls, jstring jname) {
  NYI();
}

JNIEXPORT jobject JNICALL
JVM_CompilerCommand(JNIEnv *env, jclass compCls, jobject arg) {
  NYI();
}

JNIEXPORT void JNICALL
JVM_EnableCompiler(JNIEnv *env, jclass compCls) {
  NYI();
}

JNIEXPORT void JNICALL
JVM_DisableCompiler(JNIEnv *env, jclass compCls) {
  NYI();
}

void start(JavaThread* thread) {

  JavaObject* javaThread = NULL;
  llvm_gcroot(javaThread, 0);

  Jnjvm* vm = thread->getJVM();

  // Wait some time to let the creator initialise this field
  while (thread->javaThread == NULL) {
    mvm::Thread::yield();
  }

  javaThread = thread->javaThread;
  assert(javaThread && "Didn't fix the javaThread of a j3 thread");

  // Ok, now that the thread is created we can set the the value of eetop
  // which points from the javaobject to our internal JavaThread
  vm->upcalls->eetop->setInstanceLongField(javaThread, (long)thread);

  // If the thread is not a daemon, it is added to the list of threads to
  // wait until exit.
  bool isDaemon = vm->upcalls->daemon->getInstanceInt8Field(javaThread);

  if (!isDaemon) {
    vm->threadSystem.enter();
  }

  assert(javaThread->getVirtualTable());
  // Run the VMThread::run function
  vm->upcalls->runThread->invokeIntSpecial(vm, vm->upcalls->newThread, javaThread);

  // Remove the thread from the list.
  if (!isDaemon) {
    vm->threadSystem.leave();
  }
}
/*
 * java.lang.Thread
 */
JNIEXPORT void JNICALL
JVM_StartThread(JNIEnv *env, jobject _thread) {
  JavaObject * thread = 0;
  JavaThread * javaThread = 0;
  llvm_gcroot(thread, 0);
  llvm_gcroot(javaThread, 0);
  BEGIN_JNI_EXCEPTION

  thread = *(JavaObject**)_thread;
  assert(thread);
  assert(thread->getVirtualTable());
  Jnjvm* vm = JavaThread::get()->getJVM();

  JavaThread * newTh = new JavaThread(vm);
  if (!newTh) vm->outOfMemoryError();
  newTh->start((void (*)(mvm::Thread*))start);

  newTh->initialise(thread, 0);

  RETURN_VOID_FROM_JNI

  END_JNI_EXCEPTION
}

JNIEXPORT void JNICALL
JVM_StopThread(JNIEnv *env, jobject thread, jobject exception) {
  NYI();
}

JNIEXPORT jboolean JNICALL
JVM_IsThreadAlive(JNIEnv *env, jobject _thread) {
  BEGIN_JNI_EXCEPTION
  JavaObject * thread = *(JavaObject**)_thread;
  llvm_gcroot(thread, 0);
  assert(thread);

  Jnjvm* vm = JavaThread::get()->getJVM();
  jint status = vm->upcalls->threadStatus->getInstanceInt32Field(thread);

  // TODO: We don't actually maintain 'status'...
  RETURN_FROM_JNI(status != 0);

  END_JNI_EXCEPTION

  return 0;
}

JNIEXPORT void JNICALL
JVM_SuspendThread(JNIEnv *env, jobject thread) {
  NYI();
}

JNIEXPORT void JNICALL
JVM_ResumeThread(JNIEnv *env, jobject thread) {
  NYI();
}

JNIEXPORT void JNICALL
JVM_SetThreadPriority(JNIEnv *env, jobject thread, jint prio) {
  JavaObject * T;
  llvm_gcroot(T, 0);

  BEGIN_JNI_EXCEPTION

  Jnjvm * vm = th->getJVM();
  T = *(JavaObject**)thread;

  vm->upcalls->priority->setInstanceInt32Field(T, prio);

  RETURN_VOID_FROM_JNI
  END_JNI_EXCEPTION
}

JNIEXPORT void JNICALL
JVM_Yield(JNIEnv *env, jclass threadClass) {
  mvm::Thread::yield();
}

JNIEXPORT void JNICALL
JVM_Sleep(JNIEnv *env, jclass threadClass, jlong millis) {
  NYI();
}

JNIEXPORT jobject JNICALL
JVM_CurrentThread(JNIEnv *env, jclass threadClass) {
  BEGIN_JNI_EXCEPTION
  JavaThread* th = JavaThread::get();
  RETURN_FROM_JNI((jobject)th->pushJNIRef(th->currentThread()));
  END_JNI_EXCEPTION

  return 0;
}

JNIEXPORT jint JNICALL
JVM_CountStackFrames(JNIEnv *env, jobject thread) {
  NYI();
}

JNIEXPORT void JNICALL
JVM_Interrupt(JNIEnv *env, jobject thread) {
  NYI();
}

JNIEXPORT jboolean JNICALL
JVM_IsInterrupted(JNIEnv *env, jobject thread, jboolean clearInterrupted) {
  NYI();
}

JNIEXPORT jboolean JNICALL
JVM_HoldsLock(JNIEnv *env, jclass threadClass, jobject obj) {
  NYI();
}

JNIEXPORT void JNICALL
JVM_DumpAllStacks(JNIEnv *env, jclass unused) {
  NYI();
}

JNIEXPORT jobjectArray JNICALL
JVM_GetAllThreads(JNIEnv *env, jclass dummy) {
  NYI();
}

/* getStackTrace() and getAllStackTraces() method */
JNIEXPORT jobjectArray JNICALL
JVM_DumpThreads(JNIEnv *env, jclass threadClass, jobjectArray threads) {
  NYI();
}

/*
 * java.lang.SecurityManager
 */
JNIEXPORT jclass JNICALL
JVM_CurrentLoadedClass(JNIEnv *env) {
  NYI();
}

JNIEXPORT jobject JNICALL
JVM_CurrentClassLoader(JNIEnv *env) {
  NYI();
}

JNIEXPORT jobjectArray JNICALL
JVM_GetClassContext(JNIEnv *env) {
  NYI();
}

JNIEXPORT jint JNICALL
JVM_ClassDepth(JNIEnv *env, jstring name) {
  NYI();
}

JNIEXPORT jint JNICALL
JVM_ClassLoaderDepth(JNIEnv *env) {
  NYI();
}

/*
 * java.lang.Package
 */
JNIEXPORT jstring JNICALL
JVM_GetSystemPackage(JNIEnv *env, jstring name) {
  NYI();
}

JNIEXPORT jobjectArray JNICALL
JVM_GetSystemPackages(JNIEnv *env) {
  NYI();
}

/*
 * java.io.ObjectInputStream
 */
JNIEXPORT jobject JNICALL
JVM_AllocateNewObject(JNIEnv *env, jobject obj, jclass currClass,
                      jclass initClass) {
  NYI();
}

JNIEXPORT jobject JNICALL
JVM_AllocateNewArray(JNIEnv *env, jobject obj, jclass currClass,
                     jint length) {
  NYI();
}

JNIEXPORT jobject JNICALL
JVM_LatestUserDefinedLoader(JNIEnv *env) {
  NYI();
}

/*
 * This function has been deprecated and should not be considered
 * part of the specified JVM interface.
 */
JNIEXPORT jclass JNICALL
JVM_LoadClass0(JNIEnv *env, jobject obj, jclass currClass,
               jstring currClassName) {
  NYI();
}

/*
 * java.lang.reflect.Array
 */
JNIEXPORT jint JNICALL
JVM_GetArrayLength(JNIEnv *env, jobject arr) {
  NYI();
}

JNIEXPORT jobject JNICALL
JVM_GetArrayElement(JNIEnv *env, jobject arr, jint index) {
  NYI();
}

JNIEXPORT jvalue JNICALL
JVM_GetPrimitiveArrayElement(JNIEnv *env, jobject arr, jint index, jint wCode) {
  NYI();
}

JNIEXPORT void JNICALL
JVM_SetArrayElement(JNIEnv *env, jobject arr, jint index, jobject val) {
  NYI();
}

JNIEXPORT void JNICALL
JVM_SetPrimitiveArrayElement(JNIEnv *env, jobject arr, jint index, jvalue v,
                             unsigned char vCode) {
  NYI();
}

JNIEXPORT jobject JNICALL
JVM_NewArray(JNIEnv *env, jclass eltClass, jint length) {
  NYI();
}

JNIEXPORT jobject JNICALL
JVM_NewMultiArray(JNIEnv *env, jclass eltClass, jintArray dim) {
  NYI();
}

/*
 * java.lang.Class and java.lang.ClassLoader
 */
/*
 * Returns the class in which the code invoking the native method
 * belongs.
 *
 * Note that in JDK 1.1, native methods did not create a frame.
 * In 1.2, they do. Therefore native methods like Class.forName
 * can no longer look at the current frame for the caller class.
 */
JNIEXPORT jclass JNICALL
JVM_GetCallerClass(JNIEnv *env, int n) {
  BEGIN_JNI_EXCEPTION

  JavaThread* th = JavaThread::get();

  Jnjvm* vm = th->getJVM();
  UserClass* cl = th->getCallingClassLevel(n);
  assert(cl);
  JavaObject * const * res = cl->getClassDelegateePtr(vm);
  RETURN_FROM_JNI((jclass)res);

  END_JNI_EXCEPTION

  return 0;
}

/*
 * Find primitive classes
 * utf: class name
 */


const struct {
  const char * name;
  char id;
} static PrimMap[] = {
  { "boolean", 'Z' },
  { "byte"   , 'B' },
  { "char"   , 'C' },
  { "short"  , 'S' },
  { "int"    , 'I' },
  { "long"   , 'J' },
  { "float"  , 'F' },
  { "double" , 'D' },
  { "void"   , 'V' }
};
const static unsigned PrimMapSize = sizeof(PrimMap)/sizeof(PrimMap[0]);

JNIEXPORT jclass JNICALL
JVM_FindPrimitiveClass(JNIEnv *env, const char *utf) {
  JavaObject* res = 0;
  llvm_gcroot(res, 0);

  BEGIN_JNI_EXCEPTION

  Jnjvm* vm = JavaThread::get()->getJVM();

  UserClassPrimitive * prim =  0;
  for(unsigned i = 0; i < PrimMapSize; ++i) {
    if(!strcmp(PrimMap[i].name, utf)) {
      prim = UserClassPrimitive::byteIdToPrimitive(PrimMap[i].id, vm->upcalls);
      break;
    }
  }
  if (!prim) {
    fprintf(stderr, "Unsupported primitive \"%s\"!  Missing table entry?\n", utf);
  }
  assert(prim && "Invalid Primitive in JVM_FindPrimitiveClass");

  res = (JavaObject*)prim->getClassDelegateePtr(vm);
  RETURN_FROM_JNI((jclass)res);

  END_JNI_EXCEPTION

  return 0;
}

/*
 * Link the class
 */
JNIEXPORT void JNICALL
JVM_ResolveClass(JNIEnv *env, jclass cls) {
  NYI();
}

/*
 * Find a class from a given class loader. Throw ClassNotFoundException
 * or NoClassDefFoundError depending on the value of the last
 * argument.
 */
JNIEXPORT jclass JNICALL
JVM_FindClassFromClassLoader(JNIEnv *env, const char *name, jboolean init,
                             jobject _loader, jboolean throwError) {
  BEGIN_JNI_EXCEPTION

  JavaObject * loader = _loader ? *(JavaObject**)_loader : 0;
  llvm_gcroot(loader, 0);

  jclass res;

  Jnjvm* vm = JavaThread::get()->getJVM();
  JnjvmClassLoader* JCL =
    JnjvmClassLoader::getJnjvmLoaderFromJavaObject(loader, vm);

  CommonClass * Cl = JCL->loadClassFromAsciiz(name, true, throwError);
  if (Cl) {
    if (init && Cl->asClass())
      Cl->asClass()->initialiseClass(vm);
    res = (jclass)Cl->getClassDelegateePtr(vm);
  }
  else {
    vm->classNotFoundException(vm->asciizToStr(name));
  }

  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION

  return 0;
}

JNIEXPORT jclass JNICALL
JVM_FindClassFromBootLoader(JNIEnv *env, const char *name) {
  BEGIN_JNI_EXCEPTION

  jclass res = 0;

  Jnjvm* vm = JavaThread::get()->getJVM();
  JnjvmClassLoader* JCL = vm->bootstrapLoader;

  CommonClass * Cl = JCL->loadClassFromAsciiz(name, true, false);
  if (Cl) {
    res = (jclass)Cl->getClassDelegateePtr(vm);
  }
  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION

  return 0;
}

/*
 * Find a class from a given class.
 */
JNIEXPORT jclass JNICALL
JVM_FindClassFromClass(JNIEnv *env, const char *name, jboolean init,
                             jclass from) {
  NYI();
}

/* Find a loaded class cached by the VM */
JNIEXPORT jclass JNICALL
JVM_FindLoadedClass(JNIEnv *env, jobject _loader, jstring name) {
  JavaObject * loader = 0;
  llvm_gcroot(loader, 0);
  BEGIN_JNI_EXCEPTION

  Jnjvm* vm = JavaThread::get()->getJVM();
  JnjvmClassLoader* JCL = NULL;

  JavaString * str = *(JavaString**)name;

  loader = *(JavaObject**)_loader;

  CommonClass * Cl = 0;

  JCL = JnjvmClassLoader::getJnjvmLoaderFromJavaObject(loader, vm);
  if (JCL)
   Cl = JCL->lookupClassFromJavaString(str);
  jclass res = Cl ? (jclass)Cl->getClassDelegateePtr(vm) : 0;
  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION

}

/* Define a class */
JNIEXPORT jclass JNICALL
JVM_DefineClass(JNIEnv *env, const char *name, jobject _loader, const jbyte *buf,
                jsize len, jobject pd) {
  NYI();
}

/* Define a class with a source (added in JDK1.5) */
JNIEXPORT jclass JNICALL
JVM_DefineClassWithSource(JNIEnv *env, const char *name, jobject loader,
                          const jbyte *buf, jsize len, jobject pd,
                          const char *source) {
  NYI();
}

/* Define a class with a source with conditional verification (added HSX 14)
 * -Xverify:all will verify anyway, -Xverify:none will not verify,
 * -Xverify:remote (default) will obey this conditional
 * i.e. true = should_verify_class
 */
JNIEXPORT jclass JNICALL
JVM_DefineClassWithSourceCond(JNIEnv *env, const char *name, jobject loader,
                              const jbyte *buf, jsize len, jobject pd,
                              const char *source, jboolean verify) {
  NYI();
}

/*
 * Reflection support functions
 */

JNIEXPORT jstring JNICALL
JVM_GetClassName(JNIEnv *env, jclass cls) {
  NYI();
}

JNIEXPORT jobjectArray JNICALL
JVM_GetClassInterfaces(JNIEnv *env, jclass cls) {
  NYI();
}

JNIEXPORT jobject JNICALL
JVM_GetClassLoader(JNIEnv *env, jclass cls) {
  JavaObject * Cl = *(JavaObject**)cls;
  JavaObject * res = 0;
  llvm_gcroot(Cl, 0);
  llvm_gcroot(res, 0);
  BEGIN_JNI_EXCEPTION

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserCommonClass* cl = UserCommonClass::resolvedImplClass(vm, Cl, false);
  res = cl->classLoader->getJavaClassLoader();
  RETURN_FROM_JNI((jobject)th->pushJNIRef(res));

  END_JNI_EXCEPTION

  return 0;
}

JNIEXPORT jboolean JNICALL
JVM_IsInterface(JNIEnv *env, jclass cls) {
  jboolean res = JNI_FALSE;

  JavaObject * Cl = 0;
  llvm_gcroot(Cl, 0);
  BEGIN_JNI_EXCEPTION

  Cl = *(JavaObject**)cls;

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserCommonClass* cl = UserCommonClass::resolvedImplClass(vm, Cl, false);

  res = cl->isInterface();

  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION

  return JNI_FALSE;
}

JNIEXPORT jobjectArray JNICALL
JVM_GetClassSigners(JNIEnv *env, jclass cls) {
  NYI();
}

JNIEXPORT void JNICALL
JVM_SetClassSigners(JNIEnv *env, jclass cls, jobjectArray signers) {
  NYI();
}

JNIEXPORT jobject JNICALL
JVM_GetProtectionDomain(JNIEnv *env, jclass cls) {
  JavaObjectClass * JOC = 0;
  JavaObject * pd = 0;
  llvm_gcroot(JOC, 0);
  llvm_gcroot(pd, 0);
  BEGIN_JNI_EXCEPTION

  JOC = *(JavaObjectClass**)cls;
  pd = JavaObjectClass::getProtectionDomain(JOC);

  RETURN_FROM_JNI((jobject)th->pushJNIRef(pd));

  END_JNI_EXCEPTION

  return 0;
}

JNIEXPORT void JNICALL
JVM_SetProtectionDomain(JNIEnv *env, jclass cls, jobject protection_domain) {
  NYI();
}

JNIEXPORT jboolean JNICALL
JVM_IsArrayClass(JNIEnv *env, jclass cls) {
  JavaObject * Cl = 0;
  llvm_gcroot(Cl, 0);
  BEGIN_JNI_EXCEPTION

  Cl = *(JavaObject**)cls;

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserCommonClass* cl = UserCommonClass::resolvedImplClass(vm, Cl, false);

  RETURN_FROM_JNI(cl->isArray());

  END_JNI_EXCEPTION

  return JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
JVM_IsPrimitiveClass(JNIEnv *env, jclass cls) {
  JavaObject * Cl = 0;
  llvm_gcroot(Cl, 0);
  BEGIN_JNI_EXCEPTION

  Cl = *(JavaObject**)cls;

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserCommonClass* cl = UserCommonClass::resolvedImplClass(vm, Cl, false);

  RETURN_FROM_JNI(cl->isPrimitive());

  END_JNI_EXCEPTION

  return JNI_FALSE;
}

JNIEXPORT jclass JNICALL
JVM_GetComponentType(JNIEnv *env, jclass cls) {
  JavaObject * Cl = 0;
  JavaObject* res = 0;
  llvm_gcroot(Cl, 0);
  llvm_gcroot(res, 0);
  BEGIN_JNI_EXCEPTION

  Cl = *(JavaObject**)cls;
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserCommonClass* cl = UserCommonClass::resolvedImplClass(vm, Cl, false);

  if (cl->isArray()) {
    UserCommonClass* bc = cl->asArrayClass()->baseClass();
    res = (JavaObject*)bc->getClassDelegateePtr(vm);
  } else {
    res = 0;
  }

  RETURN_FROM_JNI((jclass)res);

  END_JNI_EXCEPTION

  return 0;
}

JNIEXPORT jint JNICALL
JVM_GetClassModifiers(JNIEnv *env, jclass cls) {
  JavaObject * Cl = 0;
  llvm_gcroot(Cl, 0);
  BEGIN_JNI_EXCEPTION

  Cl = *(JavaObject**)cls;

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClass* cl = UserCommonClass::resolvedImplClass(vm, Cl, false)->asClass();;

  jint res = cl->getAccess();

  if (isEnum(res) && cl->getSuper() != vm->upcalls->EnumClass) {
    // javac may put that flag to inner classes of enum classes.
    res &= ~ACC_ENUM;
  }

  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION

  return 0;
}

JNIEXPORT jobjectArray JNICALL
JVM_GetDeclaredClasses(JNIEnv *env, jclass ofClass) {
  NYI();
}

JNIEXPORT jclass JNICALL
JVM_GetDeclaringClass(JNIEnv *env, jclass ofClass) {
  NYI();
}

/* Generics support (JDK 1.5) */
JNIEXPORT jstring JNICALL
JVM_GetClassSignature(JNIEnv *env, jclass cls) {
  NYI();
}

/* Annotations support (JDK 1.5) */
JNIEXPORT jbyteArray JNICALL
JVM_GetClassAnnotations(JNIEnv *env, jclass cls) {
  NYI();
}

/*
 * New (JDK 1.4) reflection implementation
 */

JNIEXPORT jobjectArray JNICALL
JVM_GetClassDeclaredMethods(JNIEnv *env, jclass ofClass, jboolean publicOnly) {
  ArrayObject* ret = 0;
  JavaObject* tmp = 0;
  JavaString* str = 0;
  JavaObject * Cl = 0;
  llvm_gcroot(Cl, 0);
  llvm_gcroot(ret, 0);
  llvm_gcroot(tmp, 0);
  llvm_gcroot(str, 0);

  BEGIN_JNI_EXCEPTION

  Cl = *(JavaObject**)ofClass;

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserCommonClass* cl = UserCommonClass::resolvedImplClass(vm, Cl, false);
  Classpath* upcalls = vm->upcalls;

  if (cl->isArray() || cl->isPrimitive()) {
    ret = (ArrayObject*)upcalls->methodArrayClass->doNew(0, vm);
  } else {
    UserClass* realCl = cl->asClass();
    JnjvmClassLoader* classLoader = cl->classLoader;
    uint32 size = 0;

    for (uint32 i = 0; i < realCl->nbVirtualMethods + realCl->nbStaticMethods;
         ++i) {
      JavaMethod* meth = &realCl->virtualMethods[i];
      bool pub = isPublic(meth->access);
      if (!(meth->name->equals(classLoader->bootstrapLoader->initName)) &&
          (!publicOnly || pub)) {
        ++size;
      }
    }


    ret = (ArrayObject*)upcalls->methodArrayClass->doNew(size, vm);

    sint32 index = 0;
    for (uint32 i = 0; i < realCl->nbVirtualMethods + realCl->nbStaticMethods;
         ++i) {
      JavaMethod* meth = &realCl->virtualMethods[i];
      bool pub = isPublic(meth->access);
      if (!(meth->name->equals(classLoader->bootstrapLoader->initName)) &&
          (!publicOnly || pub)) {
        // TODO: check parameter types
        UserClass* Meth = vm->upcalls->newMethod;
        tmp = Meth->doNew(vm);
        str = vm->internalUTF8ToStr(meth->name);
        JavaObject * pArr = meth->getParameterTypes(classLoader);
        JavaObject * eArr = meth->getExceptionTypes(classLoader);
        JavaObject * retTy = meth->getReturnType(classLoader);
        upcalls->initMethod->invokeIntSpecial(vm, Meth, tmp,
          &Cl,          /* declaring class */
          &str,         /* name */
          &pArr,        /* parameter types */
          &retTy,       /* return type */
          &eArr,        /* exceptions */
          meth->access, /* modifiers */
          i,            /* slot */
          NULL,         /* signature */
          NULL,         /* annotations */
          NULL,         /* parameter annotations */
          NULL,         /* default annotations */
          i);
        ArrayObject::setElement(ret, tmp, index);
        index++;
      }
    }
  }

  RETURN_FROM_JNI((jobjectArray)th->pushJNIRef(ret));

  END_JNI_EXCEPTION

  return 0;
}

JNIEXPORT jobjectArray JNICALL
JVM_GetClassDeclaredFields(JNIEnv *env, jclass ofClass, jboolean publicOnly) {
  ArrayObject* ret = 0;
  JavaObject* tmp = 0;
  JavaString* name = 0;
  JavaObject*const*  type = 0;
  JavaString* sign = 0;
  JavaObject* annArray = 0;
  llvm_gcroot(ret, 0);
  llvm_gcroot(tmp, 0);
  llvm_gcroot(name, 0);
  llvm_gcroot(type, 0);
  llvm_gcroot(sign, 0);
  llvm_gcroot(annArray, 0);

  BEGIN_JNI_EXCEPTION

  JavaObject * Cl = *(JavaObject**)ofClass;
  llvm_gcroot(Cl, 0);

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserCommonClass* cl = UserCommonClass::resolvedImplClass(vm, Cl, false);

  if (!cl->isClass()) {
    ret = (ArrayObject*)vm->upcalls->fieldArrayClass->doNew(0, vm);
  } else {
    UserClass* realCl = cl->asClass();
    uint32 size = 0;
    for (uint32 i = 0; i < realCl->nbVirtualFields + realCl->nbStaticFields;
         ++i) {
      JavaField* field = &realCl->virtualFields[i];
      if (!publicOnly || isPublic(field->access)) {
        ++size;
      }
    }


    ret = (ArrayObject*)vm->upcalls->fieldArrayClass->doNew(size, vm);

    sint32 index = 0;
    for (uint32 i = 0; i < realCl->nbVirtualFields + realCl->nbStaticFields;
         ++i) {
      JavaField* field = &realCl->virtualFields[i];
      if (!publicOnly || isPublic(field->access)) {
        // TODO: check parameter types
        UserClass* Field = vm->upcalls->newField;
        tmp = Field->doNew(vm);
        name = vm->internalUTF8ToStr(field->name);

        //type->Class
        UserCommonClass * fieldCl = field->getSignature()->assocClass(cl->classLoader);
        assert(fieldCl);
        type = fieldCl->getClassDelegateePtr(vm);

        // TODO:Implement these!
        sign = NULL;
        annArray = NULL;

        //Convert to string

        /* java.reflect.Field(
         *   Class declaringClass,
         *   String name,
         *   Class type,
         *   int modifiers,
         *   int slot,
         *   String signature,
         *   byte[] annotations)
         */
        vm->upcalls->initField->invokeIntSpecial(vm, Field, tmp,
          &Cl,
          &name,
          type,
          field->access,
          i,
          sign,
          annArray);
        ArrayObject::setElement(ret, tmp, index);
        index++;
      }
    }
  }
  RETURN_FROM_JNI((jobjectArray)th->pushJNIRef(ret));

  END_JNI_EXCEPTION

  return 0;
}

JNIEXPORT jobjectArray JNICALL
JVM_GetClassDeclaredConstructors(JNIEnv *env, jclass ofClass, jboolean publicOnly) {
  ArrayObject* ret = 0;
  JavaObject* tmp = 0;
  JavaObject* Cl = 0;
  llvm_gcroot(Cl, 0);
  llvm_gcroot(ret, 0);
  llvm_gcroot(tmp, 0);

  BEGIN_JNI_EXCEPTION

  Cl = *(JavaObject**)ofClass;

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserCommonClass* cl = UserCommonClass::resolvedImplClass(vm, Cl, false);

  if (cl->isArray() || cl->isInterface() || cl->isPrimitive()) {
    ret = (ArrayObject*)vm->upcalls->constructorArrayClass->doNew(0, vm);
  } else {
    UserClass* realCl = cl->asClass();;
    JnjvmClassLoader* classLoader = cl->classLoader;
    uint32 size = 0;

    for (uint32 i = 0; i < realCl->nbVirtualMethods; ++i) {
      JavaMethod* meth = &realCl->virtualMethods[i];
      bool pub = isPublic(meth->access);
      if (meth->name->equals(classLoader->bootstrapLoader->initName) &&
          (!publicOnly || pub)) {
        ++size;
      }
    }

    ret = (ArrayObject*)vm->upcalls->constructorArrayClass->doNew(size, vm);

    sint32 index = 0;
    for (uint32 i = 0; i < realCl->nbVirtualMethods; ++i) {
      JavaMethod* meth = &realCl->virtualMethods[i];
      bool pub = isPublic(meth->access);
      if (meth->name->equals(classLoader->bootstrapLoader->initName) &&
          (!publicOnly || pub)) {
        UserClass* Cons = vm->upcalls->newConstructor;
        JavaObject * pArr = meth->getParameterTypes(classLoader);
        JavaObject * eArr = meth->getExceptionTypes(classLoader);
        tmp = Cons->doNew(vm);
        vm->upcalls->initConstructor->invokeIntSpecial(vm, Cons, tmp,
          &Cl,          /* declaringClass */
          &pArr,        /* parameterTypes */
          &eArr,        /* checkedExceptions */
          meth->access, /* modifiers */
          i,            /* slot */
          NULL,         /* String signature */
          NULL,         /* annotations */
          NULL          /* parameterAnnotations */
        );
        ArrayObject::setElement(ret, tmp, index);
        index++;
      }
    }
  }

  RETURN_FROM_JNI((jobjectArray)th->pushJNIRef(ret));

  END_JNI_EXCEPTION

  return 0;
}

/* Differs from JVM_GetClassModifiers in treatment of inner classes.
   This returns the access flags for the class as specified in the
   class file rather than searching the InnerClasses attribute (if
   present) to find the source-level access flags. Only the values of
   the low 13 bits (i.e., a mask of 0x1FFF) are guaranteed to be
   valid. */
JNIEXPORT jint JNICALL
JVM_GetClassAccessFlags(JNIEnv *env, jclass cls) {
  JavaObject * Cl = 0;
  llvm_gcroot(Cl, 0);
  BEGIN_JNI_EXCEPTION

  Cl = *(JavaObject**)cls;

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserCommonClass* cl = UserCommonClass::resolvedImplClass(vm, Cl, false);
  assert(cl->isClass());

  // TODO: Verify this is doing what this function is supposed to do
  // (In particular regarding the comment block above)
  RETURN_FROM_JNI(cl->asClass()->access);

  END_JNI_EXCEPTION

  return 0;
}

/* The following two reflection routines are still needed due to startup time issues */
/*
 * java.lang.reflect.Method
 */
JNIEXPORT jobject JNICALL
JVM_InvokeMethod(JNIEnv *env, jobject method, jobject _obj, jobjectArray args0) {
  NYI();
}

/*
 * java.lang.reflect.Constructor
 */
JNIEXPORT jobject JNICALL
JVM_NewInstanceFromConstructor(JNIEnv *env, jobject _c, jobjectArray args0) {
  NYI();
}

/*
 * Constant pool access; currently used to implement reflective access to annotations (JDK 1.5
 */

JNIEXPORT jobject JNICALL
JVM_GetClassConstantPool(JNIEnv *env, jclass cls) {
  NYI();
}

JNIEXPORT jint JNICALL JVM_ConstantPoolGetSize
(JNIEnv *env, jobject unused, jobject jcpool) {
  NYI();
}

JNIEXPORT jclass JNICALL JVM_ConstantPoolGetClassAt
(JNIEnv *env, jobject unused, jobject jcpool, jint index) {
  NYI();
}

JNIEXPORT jclass JNICALL JVM_ConstantPoolGetClassAtIfLoaded
(JNIEnv *env, jobject unused, jobject jcpool, jint index) {
  NYI();
}

JNIEXPORT jobject JNICALL JVM_ConstantPoolGetMethodAt
(JNIEnv *env, jobject unused, jobject jcpool, jint index) {
  NYI();
}

JNIEXPORT jobject JNICALL JVM_ConstantPoolGetMethodAtIfLoaded
(JNIEnv *env, jobject unused, jobject jcpool, jint index) {
  NYI();
}

JNIEXPORT jobject JNICALL JVM_ConstantPoolGetFieldAt
(JNIEnv *env, jobject unused, jobject jcpool, jint index) {
  NYI();
}

JNIEXPORT jobject JNICALL JVM_ConstantPoolGetFieldAtIfLoaded
(JNIEnv *env, jobject unused, jobject jcpool, jint index) {
  NYI();
}

JNIEXPORT jobjectArray JNICALL JVM_ConstantPoolGetMemberRefInfoAt
(JNIEnv *env, jobject unused, jobject jcpool, jint index) {
  NYI();
}

JNIEXPORT jint JNICALL JVM_ConstantPoolGetIntAt
(JNIEnv *env, jobject unused, jobject jcpool, jint index) {
  NYI();
}

JNIEXPORT jlong JNICALL JVM_ConstantPoolGetLongAt
(JNIEnv *env, jobject unused, jobject jcpool, jint index) {
  NYI();
}

JNIEXPORT jfloat JNICALL JVM_ConstantPoolGetFloatAt
(JNIEnv *env, jobject unused, jobject jcpool, jint index) {
  NYI();
}

JNIEXPORT jdouble JNICALL JVM_ConstantPoolGetDoubleAt
(JNIEnv *env, jobject unused, jobject jcpool, jint index) {
  NYI();
}

JNIEXPORT jstring JNICALL JVM_ConstantPoolGetStringAt
(JNIEnv *env, jobject unused, jobject jcpool, jint index) {
  NYI();
}

JNIEXPORT jstring JNICALL JVM_ConstantPoolGetUTF8At
(JNIEnv *env, jobject unused, jobject jcpool, jint index) {
  NYI();
}

/*
 * java.security.*
 */

JNIEXPORT jobject JNICALL
JVM_DoPrivileged(JNIEnv *env, jclass cls,
                 jobject action, jobject context, jboolean wrapException) {
  JavaObject * obj = 0;
  JavaObject * res = 0;
  llvm_gcroot(obj, 0);
  llvm_gcroot(res, 0);
  BEGIN_JNI_EXCEPTION

  verifyNull(action);

  obj = *(JavaObject**)action;

  Jnjvm* vm = th->getJVM();

  // For now, we don't do anything special,
  // just call the requested 'run()' method...
  UserClass * cl = JavaObject::getClass(obj)->asClass();
  const UTF8 * runName = cl->classLoader->hashUTF8->lookupAsciiz("run");
  const UTF8 * runType = cl->classLoader->hashUTF8->lookupAsciiz("()Ljava/lang/Object;");
  assert(cl); assert(runName); assert(runType);

  JavaMethod * meth = cl->lookupMethodDontThrow(runName, runType, false, true, 0);
  assert(meth);

  res = meth->invokeJavaObjectVirtual(vm, cl, obj, obj);

  RETURN_FROM_JNI((jobject)th->pushJNIRef(res));

  END_JNI_EXCEPTION

  return NULL;
}

JNIEXPORT jobject JNICALL
JVM_GetInheritedAccessControlContext(JNIEnv *env, jclass cls) {
  // No AccessControlContext supported yet
  return NULL;
}


JNIEXPORT jobject JNICALL
JVM_GetStackAccessControlContext(JNIEnv *env, jclass cls) {
  // No AccessControlContext supported yet
  return NULL;
}

/*
 * Signal support, used to implement the shutdown sequence.  Every VM must
 * support JVM_SIGINT and JVM_SIGTERM, raising the former for user interrupts
 * (^C) and the latter for external termination (kill, system shutdown, etc.).
 * Other platform-dependent signal values may also be supported.
 */

JNIEXPORT void * JNICALL
JVM_RegisterSignal(jint sig, void *handler) {
  NYI();
}

JNIEXPORT jboolean JNICALL
JVM_RaiseSignal(jint sig) {
  NYI();
}

JNIEXPORT jint JNICALL
JVM_FindSignal(const char *name) {
  NYI();
}

/*
 * Retrieve the assertion directives for the specified class.
 */
JNIEXPORT jboolean JNICALL
JVM_DesiredAssertionStatus(JNIEnv *env, jclass unused, jclass cls) {
  // TODO: Actually implement?
  return JNI_FALSE;
}

/*
 * Retrieve the assertion directives from the VM.
 */
JNIEXPORT jobject JNICALL
JVM_AssertionStatusDirectives(JNIEnv *env, jclass unused) {
  NYI();
}

/*
 * java.util.concurrent.AtomicLong
 */
JNIEXPORT jboolean JNICALL
JVM_SupportsCX8(void) {
  NYI();
}

/*************************************************************************
 PART 2: Support for the Verifier and Class File Format Checker
 ************************************************************************/
/*
 * Return the class name in UTF format. The result is valid
 * until JVM_ReleaseUTf is called.
 *
 * The caller must treat the string as a constant and not modify it
 * in any way.
 */
JNIEXPORT const char * JNICALL
JVM_GetClassNameUTF(JNIEnv *env, jclass cb) {
  NYI();
}

/*
 * Returns the constant pool types in the buffer provided by "types."
 */
JNIEXPORT void JNICALL
JVM_GetClassCPTypes(JNIEnv *env, jclass cb, unsigned char *types) {
  NYI();
}

/*
 * Returns the number of Constant Pool entries.
 */
JNIEXPORT jint JNICALL
JVM_GetClassCPEntriesCount(JNIEnv *env, jclass cb) {
  NYI();
}

/*
 * Returns the number of *declared* fields or methods.
 */
JNIEXPORT jint JNICALL
JVM_GetClassFieldsCount(JNIEnv *env, jclass cb) {
  NYI();
}

JNIEXPORT jint JNICALL
JVM_GetClassMethodsCount(JNIEnv *env, jclass cb) {
  NYI();
}

/*
 * Returns the CP indexes of exceptions raised by a given method.
 * Places the result in the given buffer.
 *
 * The method is identified by method_index.
 */
JNIEXPORT void JNICALL
JVM_GetMethodIxExceptionIndexes(JNIEnv *env, jclass cb, jint method_index,
                                unsigned short *exceptions) {
  NYI();
}

/*
 * Returns the number of exceptions raised by a given method.
 * The method is identified by method_index.
 */
JNIEXPORT jint JNICALL
JVM_GetMethodIxExceptionsCount(JNIEnv *env, jclass cb, jint method_index) {
  NYI();
}

/*
 * Returns the byte code sequence of a given method.
 * Places the result in the given buffer.
 *
 * The method is identified by method_index.
 */
JNIEXPORT void JNICALL
JVM_GetMethodIxByteCode(JNIEnv *env, jclass cb, jint method_index,
                        unsigned char *code) {
  NYI();
}

/*
 * Returns the length of the byte code sequence of a given method.
 * The method is identified by method_index.
 */
JNIEXPORT jint JNICALL
JVM_GetMethodIxByteCodeLength(JNIEnv *env, jclass cb, jint method_index) {
  NYI();
}

/*
 * Returns the exception table entry at entry_index of a given method.
 * Places the result in the given buffer.
 *
 * The method is identified by method_index.
 */
JNIEXPORT void JNICALL
JVM_GetMethodIxExceptionTableEntry(JNIEnv *env, jclass cb, jint method_index,
                                   jint entry_index,
                                   JVM_ExceptionTableEntryType *entry) {
  NYI();
}

/*
 * Returns the length of the exception table of a given method.
 * The method is identified by method_index.
 */
JNIEXPORT jint JNICALL
JVM_GetMethodIxExceptionTableLength(JNIEnv *env, jclass cb, int index) {
  NYI();
}

/*
 * Returns the modifiers of a given field.
 * The field is identified by field_index.
 */
JNIEXPORT jint JNICALL
JVM_GetFieldIxModifiers(JNIEnv *env, jclass cb, int index) {
  NYI();
}

/*
 * Returns the modifiers of a given method.
 * The method is identified by method_index.
 */
JNIEXPORT jint JNICALL
JVM_GetMethodIxModifiers(JNIEnv *env, jclass cb, int index) {
  NYI();
}

/*
 * Returns the number of local variables of a given method.
 * The method is identified by method_index.
 */
JNIEXPORT jint JNICALL
JVM_GetMethodIxLocalsCount(JNIEnv *env, jclass cb, int index) {
  NYI();
}

/*
 * Returns the number of arguments (including this pointer) of a given method.
 * The method is identified by method_index.
 */
JNIEXPORT jint JNICALL
JVM_GetMethodIxArgsSize(JNIEnv *env, jclass cb, int index) {
  NYI();
}

/*
 * Returns the maximum amount of stack (in words) used by a given method.
 * The method is identified by method_index.
 */
JNIEXPORT jint JNICALL
JVM_GetMethodIxMaxStack(JNIEnv *env, jclass cb, int index) {
  NYI();
}

/*
 * Is a given method a constructor.
 * The method is identified by method_index.
 */
JNIEXPORT jboolean JNICALL
JVM_IsConstructorIx(JNIEnv *env, jclass cb, int index) {
  NYI();
}

/*
 * Returns the name of a given method in UTF format.
 * The result remains valid until JVM_ReleaseUTF is called.
 *
 * The caller must treat the string as a constant and not modify it
 * in any way.
 */
JNIEXPORT const char * JNICALL
JVM_GetMethodIxNameUTF(JNIEnv *env, jclass cb, jint index) {
  NYI();
}

/*
 * Returns the signature of a given method in UTF format.
 * The result remains valid until JVM_ReleaseUTF is called.
 *
 * The caller must treat the string as a constant and not modify it
 * in any way.
 */
JNIEXPORT const char * JNICALL
JVM_GetMethodIxSignatureUTF(JNIEnv *env, jclass cb, jint index) {
  NYI();
}

/*
 * Returns the name of the field refered to at a given constant pool
 * index.
 *
 * The result is in UTF format and remains valid until JVM_ReleaseUTF
 * is called.
 *
 * The caller must treat the string as a constant and not modify it
 * in any way.
 */
JNIEXPORT const char * JNICALL
JVM_GetCPFieldNameUTF(JNIEnv *env, jclass cb, jint index) {
  NYI();
}

/*
 * Returns the name of the method refered to at a given constant pool
 * index.
 *
 * The result is in UTF format and remains valid until JVM_ReleaseUTF
 * is called.
 *
 * The caller must treat the string as a constant and not modify it
 * in any way.
 */
JNIEXPORT const char * JNICALL
JVM_GetCPMethodNameUTF(JNIEnv *env, jclass cb, jint index) {
  NYI();
}

/*
 * Returns the signature of the method refered to at a given constant pool
 * index.
 *
 * The result is in UTF format and remains valid until JVM_ReleaseUTF
 * is called.
 *
 * The caller must treat the string as a constant and not modify it
 * in any way.
 */
JNIEXPORT const char * JNICALL
JVM_GetCPMethodSignatureUTF(JNIEnv *env, jclass cb, jint index) {
  NYI();
}

/*
 * Returns the signature of the field refered to at a given constant pool
 * index.
 *
 * The result is in UTF format and remains valid until JVM_ReleaseUTF
 * is called.
 *
 * The caller must treat the string as a constant and not modify it
 * in any way.
 */
JNIEXPORT const char * JNICALL
JVM_GetCPFieldSignatureUTF(JNIEnv *env, jclass cb, jint index) {
  NYI();
}

/*
 * Returns the class name refered to at a given constant pool index.
 *
 * The result is in UTF format and remains valid until JVM_ReleaseUTF
 * is called.
 *
 * The caller must treat the string as a constant and not modify it
 * in any way.
 */
JNIEXPORT const char * JNICALL
JVM_GetCPClassNameUTF(JNIEnv *env, jclass cb, jint index) {
  NYI();
}

/*
 * Returns the class name refered to at a given constant pool index.
 *
 * The constant pool entry must refer to a CONSTANT_Fieldref.
 *
 * The result is in UTF format and remains valid until JVM_ReleaseUTF
 * is called.
 *
 * The caller must treat the string as a constant and not modify it
 * in any way.
 */
JNIEXPORT const char * JNICALL
JVM_GetCPFieldClassNameUTF(JNIEnv *env, jclass cb, jint index) {
  NYI();
}

/*
 * Returns the class name refered to at a given constant pool index.
 *
 * The constant pool entry must refer to CONSTANT_Methodref or
 * CONSTANT_InterfaceMethodref.
 *
 * The result is in UTF format and remains valid until JVM_ReleaseUTF
 * is called.
 *
 * The caller must treat the string as a constant and not modify it
 * in any way.
 */
JNIEXPORT const char * JNICALL
JVM_GetCPMethodClassNameUTF(JNIEnv *env, jclass cb, jint index) {
  NYI();
}

/*
 * Returns the modifiers of a field in calledClass. The field is
 * referred to in class cb at constant pool entry index.
 *
 * The caller must treat the string as a constant and not modify it
 * in any way.
 *
 * Returns -1 if the field does not exist in calledClass.
 */
JNIEXPORT jint JNICALL
JVM_GetCPFieldModifiers(JNIEnv *env, jclass cb, int index, jclass calledClass) {
  NYI();
}

/*
 * Returns the modifiers of a method in calledClass. The method is
 * referred to in class cb at constant pool entry index.
 *
 * Returns -1 if the method does not exist in calledClass.
 */
JNIEXPORT jint JNICALL
JVM_GetCPMethodModifiers(JNIEnv *env, jclass cb, int index, jclass calledClass) {
  NYI();
}

/*
 * Releases the UTF string obtained from the VM.
 */
JNIEXPORT void JNICALL
JVM_ReleaseUTF(const char *utf) {
  NYI();
}

/*
 * Compare if two classes are in the same package.
 */
JNIEXPORT jboolean JNICALL
JVM_IsSameClassPackage(JNIEnv *env, jclass class1, jclass class2) {
  NYI();
}

/* Write a string into the given buffer, in the platform's local encoding,
 * that describes the most recent system-level error to occur in this thread.
 * Return the length of the string or zero if no error occurred.
 */
JNIEXPORT jint JNICALL
JVM_GetLastErrorString(char *buf, int len) {
  BEGIN_JNI_EXCEPTION
  char * err = strerror_r(errno, buf, len);
  if (err != buf)
    strncpy(buf, err, len);

  RETURN_FROM_JNI(strlen(buf));

  END_JNI_EXCEPTION

  return 0;
}

/*
 * Convert a pathname into native format.  This function does syntactic
 * cleanup, such as removing redundant separator characters.  It modifies
 * the given pathname string in place.
 */
JNIEXPORT char * JNICALL
JVM_NativePath(char * path) {
  NYI();
}

/*
 * Open a file descriptor. This function returns a negative error code
 * on error, and a non-negative integer that is the file descriptor on
 * success.
 */
JNIEXPORT jint JNICALL
JVM_Open(const char *fname, jint flags, jint mode) {
  BEGIN_JNI_EXCEPTION
  int result = open(fname, flags, mode);

  result = (result == 0) ? -1 : result;
  RETURN_FROM_JNI(result);
  END_JNI_EXCEPTION

  return -1;
}

/*
 * Close a file descriptor. This function returns -1 on error, and 0
 * on success.
 *
 * fd        the file descriptor to close.
 */
JNIEXPORT jint JNICALL
JVM_Close(jint fd) {
  BEGIN_JNI_EXCEPTION
  jint res = close(fd);
  RETURN_FROM_JNI(res);
  END_JNI_EXCEPTION

  return -1;
}

/*
 * Read data from a file decriptor into a char array.
 *
 * fd        the file descriptor to read from.
 * buf       the buffer where to put the read data.
 * nbytes    the number of bytes to read.
 *
 * This function returns -1 on error, and 0 on success.
 * (WDTZ: If I return '0' on success, things die. Returning
 *  number of bytes read instead for now)
 */
JNIEXPORT jint JNICALL
JVM_Read(jint fd, char *buf, jint nbytes) {
  BEGIN_JNI_EXCEPTION
  jint res = read(fd, buf, nbytes);
  RETURN_FROM_JNI(res);
  END_JNI_EXCEPTION
  return -1;
}

/*
 * Write data from a char array to a file decriptor.
 *
 * fd        the file descriptor to read from.
 * buf       the buffer from which to fetch the data.
 * nbytes    the number of bytes to write.
 *
 * This function returns -1 on error, and 0 on success.
 */
JNIEXPORT jint JNICALL
JVM_Write(jint fd, char *buf, jint nbytes) {
  BEGIN_JNI_EXCEPTION
  jint res = write(fd, buf, nbytes);
  RETURN_FROM_JNI(res);
  END_JNI_EXCEPTION
  return -1;
}

/*
 * Returns the number of bytes available for reading from a given file
 * descriptor
 */
JNIEXPORT jint JNICALL
JVM_Available(jint fd, jlong *pbytes) {
  // From JamVM's JVM_Available implementation, GPLv2
  BEGIN_JNI_EXCEPTION
  struct stat sb;

  if(fstat(fd, &sb) == -1)
    RETURN_FROM_JNI(0);

  switch(sb.st_mode & S_IFMT) {
    case S_IFCHR:
    case S_IFIFO:
    case S_IFSOCK: {
      int n;

      if(ioctl(fd, TIOCINQ, &n) == -1)
        RETURN_FROM_JNI(0);

      *pbytes = n;
      RETURN_FROM_JNI(1);
    }

    default: {
      off64_t cur, end;

      if((cur = lseek64(fd, 0, SEEK_CUR)) == -1)
        RETURN_FROM_JNI(0);

      if((end = lseek64(fd, 0, SEEK_END)) == -1)
        RETURN_FROM_JNI(0);

      if(lseek64(fd, cur, SEEK_SET) == -1)
        RETURN_FROM_JNI(0);

      *pbytes = end - cur;
      RETURN_FROM_JNI(1);
    }
  }
  END_JNI_EXCEPTION
  return 0;
}

/*
 * Move the file descriptor pointer from whence by offset.
 *
 * fd        the file descriptor to move.
 * offset    the number of bytes to move it by.
 * whence    the start from where to move it.
 *
 * This function returns the resulting pointer location.
 */
JNIEXPORT jlong JNICALL
JVM_Lseek(jint fd, jlong offset, jint whence) {
  BEGIN_JNI_EXCEPTION
  jlong res = lseek64(fd, offset, whence);
  RETURN_FROM_JNI(res);
  END_JNI_EXCEPTION
  return 0;
}

/*
 * Set the length of the file associated with the given descriptor to the given
 * length.  If the new length is longer than the current length then the file
 * is extended; the contents of the extended portion are not defined.  Th {
NYI();
}
 * value of the file pointer is undefined after this procedure returns.
 */
JNIEXPORT jint JNICALL
JVM_SetLength(jint fd, jlong length) {
  NYI();
}

/*
 * Synchronize the file descriptor's in memory state with that of the
 * physical device.  Return of -1 is an error, 0 is OK.
 */
JNIEXPORT jint JNICALL
JVM_Sync(jint fd) {
  NYI();
}

/*
 * Networking library support
 */

JNIEXPORT jint JNICALL
JVM_InitializeSocketLibrary(void) {
  NYI();
}

JNIEXPORT jint JNICALL
JVM_Socket(jint domain, jint type, jint protocol) {
  NYI();
}

JNIEXPORT jint JNICALL
JVM_SocketClose(jint fd) {
  NYI();
}

JNIEXPORT jint JNICALL
JVM_SocketShutdown(jint fd, jint howto) {
  NYI();
}

JNIEXPORT jint JNICALL
JVM_Recv(jint fd, char *buf, jint nBytes, jint flags) {
  NYI();
}

JNIEXPORT jint JNICALL
JVM_Send(jint fd, char *buf, jint nBytes, jint flags) {
  NYI();
}

JNIEXPORT jint JNICALL
JVM_Timeout(int fd, long timeout) {
  NYI();
}

JNIEXPORT jint JNICALL
JVM_Listen(jint fd, jint count) {
  NYI();
}

JNIEXPORT jint JNICALL
JVM_Connect(jint fd, struct sockaddr *him, jint len) {
  NYI();
}

JNIEXPORT jint JNICALL
JVM_Bind(jint fd, struct sockaddr *him, jint len) {
  NYI();
}

JNIEXPORT jint JNICALL
JVM_Accept(jint fd, struct sockaddr *him, jint *len) {
  NYI();
}

JNIEXPORT jint JNICALL
JVM_RecvFrom(jint fd, char *buf, int nBytes,
                  int flags, struct sockaddr *from, int *fromlen) {
  NYI();
}

JNIEXPORT jint JNICALL
JVM_SendTo(jint fd, char *buf, int len,
                int flags, struct sockaddr *to, int tolen) {
  NYI();
}

JNIEXPORT jint JNICALL
JVM_SocketAvailable(jint fd, jint *result) {
  NYI();
}


JNIEXPORT jint JNICALL
JVM_GetSockName(jint fd, struct sockaddr *him, int *len) {
  NYI();
}

JNIEXPORT jint JNICALL
JVM_GetSockOpt(jint fd, int level, int optname, char *optval, int *optlen) {
  NYI();
}

JNIEXPORT jint JNICALL
JVM_SetSockOpt(jint fd, int level, int optname, const char *optval, int optlen) {
  NYI();
}

/*
 * These routines are only reentrant on Windows
 */

JNIEXPORT int JNICALL
JVM_GetHostName(char* name, int namelen) {
  return gethostname(name, namelen);
}

/*
 * The standard printing functions supported by the Java VM. (Should they
 * be renamed to JVM_* in the future?
 */

/*
 * BE CAREFUL! The following functions do not implement the
 * full feature set of standard C printf formats.
 */
int
jio_vsnprintf(char *str, size_t count, const char *fmt, va_list args) {
  return vsnprintf(str, count, fmt, args);
}

int
jio_snprintf(char *str, size_t count, const char *fmt, ...) {
  NYI();
}

int
jio_fprintf(FILE *, const char *fmt, ...) {
  NYI();
}

int
jio_vfprintf(FILE *, const char *fmt, va_list args) {
  NYI();
}

JNIEXPORT void * JNICALL
JVM_RawMonitorCreate(void) {
  BEGIN_JNI_EXCEPTION
  pthread_mutex_t* L = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(L, NULL);

  RETURN_FROM_JNI(L);
  END_JNI_EXCEPTION

  return 0;
}

JNIEXPORT void JNICALL
JVM_RawMonitorDestroy(void *mon) {
  BEGIN_JNI_EXCEPTION
  pthread_mutex_t* L = (pthread_mutex_t*)mon;
  pthread_mutex_destroy(L);
  free(L);
  RETURN_VOID_FROM_JNI;
  END_JNI_EXCEPTION
}

JNIEXPORT jint JNICALL
JVM_RawMonitorEnter(void *mon) {
  BEGIN_JNI_EXCEPTION
  pthread_mutex_t* L = (pthread_mutex_t*)mon;
  jint res = pthread_mutex_lock(L);
  RETURN_FROM_JNI(res);
  END_JNI_EXCEPTION
  return 0;
}

JNIEXPORT void JNICALL
JVM_RawMonitorExit(void *mon) {
  BEGIN_JNI_EXCEPTION
  pthread_mutex_t* L = (pthread_mutex_t*)mon;
  pthread_mutex_unlock(L);
  RETURN_VOID_FROM_JNI;
  END_JNI_EXCEPTION
}

/*
 * java.lang.management support
 */
JNIEXPORT void* JNICALL
JVM_GetManagement(jint version) {
  NYI();
}

/*
 * com.sun.tools.attach.VirtualMachine support
 *
 * Initialize the agent properties with the properties maintained in the VM.
 */
JNIEXPORT jobject JNICALL
JVM_InitAgentProperties(JNIEnv *env, jobject agent_props) {
  NYI();
}

/* Generics reflection support.
 *
 * Returns information about the given class's EnclosingMethod
 * attribute, if present, or null if the class had no enclosing
 * method.
 *
 * If non-null, the returned array contains three elements. Element 0
 * is the java.lang.Class of which the enclosing method is a member,
 * and elements 1 and 2 are the java.lang.Strings for the enclosing
 * method's name and descriptor, respectively.
 */
JNIEXPORT jobjectArray JNICALL
JVM_GetEnclosingMethodInfo(JNIEnv* env, jclass ofClass) {
  NYI();
}

/*
 * Returns an array of the threadStatus values representing the
 * given Java thread state.  Returns NULL if the VM version is
 * incompatible with the JDK or doesn't support the given
 * Java thread state.
 */
JNIEXPORT jintArray JNICALL
JVM_GetThreadStateValues(JNIEnv* env, jint javaThreadState) {
  NYI();
}

/*
 * Returns an array of the substate names representing the
 * given Java thread state.  Returns NULL if the VM version is
 * incompatible with the JDK or the VM doesn't support
 * the given Java thread state.
 * values must be the jintArray returned from JVM_GetThreadStateValues
 * and javaThreadState.
 */
JNIEXPORT jobjectArray JNICALL
JVM_GetThreadStateNames(JNIEnv* env, jint javaThreadState, jintArray values) {
  NYI();
}

JNIEXPORT void JNICALL
JVM_GetVersionInfo(JNIEnv* env, jvm_version_info* info, size_t info_size) {
  NYI();
}

// vim: set ft=cpp:
