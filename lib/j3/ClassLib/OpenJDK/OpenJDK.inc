//===---------- OpenJDK.inc - OpenJDK JVM Interface Definition ------------===//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "jvm.h"

#include "JavaConstantPool.h"
#include "Reader.h"

#include "ArrayCopy.inc"
#include "ClassContext.inc"
#include "DefineClass.inc"
#include "SetProperties.inc"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#define NYI() \
  do{ assert(0 && "Not yet implemented!"); abort(); } while(0)

// Convenience wrapper for returning a JNI ref to something
#define RETURN_REF_FROM_JNI(a,type) {\
  type __jni_ret_val = (type)th->pushJNIRef(a); \
  RETURN_FROM_JNI(__jni_ret_val); \
  }

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
  vmkit::StackWalker Walker(th);
  uint32_t i = 0;

  while (intptr_t ip = *Walker) {
    ArrayPtr::setElement(result, ip, i);
    ++i;
    ++Walker;
  }

  return result;
}

JavaObject* consStackElement(vmkit::FrameInfo* FI, intptr_t ip) {

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
  JavaObject * obj = _obj ? *(JavaObject**)_obj : 0;
  llvm_gcroot(obj, 0);

  jint hash = (obj == NULL) ? 0 : JavaObject::hashCode(obj);
  return hash;
}

JNIEXPORT void JNICALL
JVM_MonitorWait(JNIEnv *env, jobject obj, jlong ms) {
  BEGIN_JNI_EXCEPTION

  JavaObject::wait(*(JavaObject**)obj, ms, 0);
  RETURN_VOID_FROM_JNI

  END_JNI_EXCEPTION
}

JNIEXPORT void JNICALL
JVM_MonitorNotify(JNIEnv *env, jobject obj) {
  BEGIN_JNI_EXCEPTION

  JavaObject::notify(*(JavaObject**)obj);

  RETURN_VOID_FROM_JNI

  END_JNI_EXCEPTION
}

JNIEXPORT void JNICALL
JVM_MonitorNotifyAll(JNIEnv *env, jobject obj) {
  BEGIN_JNI_EXCEPTION

  JavaObject::notifyAll(*(JavaObject**)obj);
  RETURN_VOID_FROM_JNI

  END_JNI_EXCEPTION
}

JNIEXPORT jobject JNICALL
JVM_Clone(JNIEnv *env, jobject obj) {
  JavaObject * clone = 0;
  llvm_gcroot(clone, 0);
  BEGIN_JNI_EXCEPTION

  clone = JavaObject::clone(*(JavaObject**)obj);
  RETURN_REF_FROM_JNI(clone, jobject);

  END_JNI_EXCEPTION

  return 0;
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

  BEGIN_JNI_EXCEPTION

  Jnjvm* vm = JavaThread::get()->getJVM();
  array = JavaString::strToArray(str, vm);
  res = vm->constructString(array);

  RETURN_REF_FROM_JNI(res, jstring);

  END_JNI_EXCEPTION

  return 0;
}

/*
 * java.lang.System
 */

uint64 CurrentTimeMillis() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (tv.tv_sec * 1000LL) + (tv.tv_usec / 1000LL);
}

JNIEXPORT jlong JNICALL
JVM_CurrentTimeMillis(JNIEnv *env, jclass ignored) {
  return CurrentTimeMillis();
}

JNIEXPORT jlong JNICALL
JVM_NanoTime(JNIEnv *env, jclass ignored) {
  // TODO: Impl with more accurate clock?
  struct timeval tv;

  gettimeofday(&tv, NULL);

  jlong time = tv.tv_sec * 1000000000LL + tv.tv_usec * 1000LL;

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

  src = jsrc ? *(JavaObject**)jsrc : 0;
  dst = jdst ? *(JavaObject**)jdst : 0;

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

  Jnjvm* vm = JavaThread::get()->getJVM();
  const char * tmp = getenv("JAVA_COMPILER");
  if (tmp)
    setProperty(vm, prop, "java.compiler", tmp);

  // Override properties to indicate java version 1.6
  setProperty(vm, prop, "java.specification.version", "1.6");
  setProperty(vm, prop, "java.version", "1.6");
  setProperty(vm, prop, "java.runtime.version", "1.6");

  // Set additional path properties
  // For now, ignore JAVA_HOME.  We don't want to be using a location
  // other than the one we precompiled against anyway.
  setProperty(vm, prop, "java.home", OpenJDKJRE);
  setProperty(vm, prop, "java.ext.dirs", OpenJDKExtDirs);

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
  vmkit::System::Exit(code);
}

JNIEXPORT void JNICALL
JVM_Halt(jint code) {
  vmkit::System::Exit(code);
}

JNIEXPORT void JNICALL
JVM_GC(void) {
  BEGIN_JNI_EXCEPTION
  vmkit::Collector::collect();
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
  return (jlong)vmkit::Collector::getTotalMemory();
}

JNIEXPORT jlong JNICALL
JVM_FreeMemory(void) {
  return (jlong)vmkit::Collector::getFreeMemory();
}

JNIEXPORT jlong JNICALL
JVM_MaxMemory(void) {
  return (jlong)vmkit::Collector::getMaxMemory();
}

JNIEXPORT jint JNICALL
JVM_ActiveProcessorCount(void) {
  return vmkit::System::GetNumberOfProcessors();
}

JNIEXPORT void * JNICALL
JVM_LoadLibrary(const char *name) {
  BEGIN_JNI_EXCEPTION
  Jnjvm* vm = JavaThread::get()->getJVM();
  void * res = (void*)vm->bootstrapLoader->loadLib(name);
  RETURN_FROM_JNI(res);

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
  void* res = (void *)vm->bootstrapLoader->loadInLib(name, handle);
  RETURN_FROM_JNI(res);
  END_JNI_EXCEPTION

  return 0;
}

JNIEXPORT jboolean JNICALL
JVM_IsSupportedJNIVersion(jint version) {
  return version == JNI_VERSION_1_1 ||
         version == JNI_VERSION_1_2 ||
         version == JNI_VERSION_1_4 ||
         version == JNI_VERSION_1_6;
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
    vmkit::FrameInfo* FI = vm->IPToFrameInfo(ip);
    if (FI->Metadata != NULL) {
      if (cur == index) {
        result = consStackElement(FI, ip);
        break;
      }
      cur++;
    }
  }

  assert(result && "No stack element found");
  RETURN_REF_FROM_JNI(result, jobject);

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
  // Unsupported.
  return false;
}

JNIEXPORT jboolean JNICALL
JVM_CompileClasses(JNIEnv *env, jclass cls, jstring jname) {
  // Unsupported.
  return false;
}

JNIEXPORT jobject JNICALL
JVM_CompilerCommand(JNIEnv *env, jclass compCls, jobject arg) {
  NYI();
}

JNIEXPORT void JNICALL
JVM_EnableCompiler(JNIEnv *env, jclass compCls) {
  // Ignore.
}

JNIEXPORT void JNICALL
JVM_DisableCompiler(JNIEnv *env, jclass compCls) {
  // Ignore.
}

void start(JavaThread* thread) {

  JavaObject* javaThread = NULL;
  JavaObject* handler = NULL;
  JavaObject* exc = NULL;
  llvm_gcroot(javaThread, 0);
  llvm_gcroot(exc, 0);
  llvm_gcroot(handler, 0);

  Jnjvm* vm = thread->getJVM();

  // Wait some time to let the creator initialise the fields
  while (thread->javaThread == NULL || thread->sleepObject == NULL) {
    vmkit::Thread::yield();
  }

  javaThread = thread->javaThread;
  assert(javaThread && "Didn't fix the javaThread of a j3 thread");

  // If the thread is not a daemon, it is added to the list of threads to
  // wait until exit.
  bool isDaemon = vm->upcalls->daemon->getInstanceInt8Field(javaThread);

  if (!isDaemon) {
    vm->threadSystem.enter();
  }

  assert(javaThread->getVirtualTable());

  // Get the class for this Thread
  UserClass* thClass = JavaObject::getClass(javaThread)->asClass();
  assert(thClass);
  assert(thClass->isResolved());

  // Lookup the correct run() method to call...
  const UTF8 * runName = thClass->classLoader->hashUTF8->lookupAsciiz("run");
  const UTF8 * runType = thClass->classLoader->hashUTF8->lookupAsciiz("()V");
  JavaMethod * meth = thClass->lookupMethodDontThrow(runName, runType, false, true, 0);
  assert(meth);

  // Sanity check
  assert(JavaThread::get() == thread);

  // Run the thread...
  TRY {
    meth->invokeIntSpecial(vm, thClass, javaThread);
  } CATCH {
    exc = thread->pendingException;
  } END_CATCH;

  // Sanity check
  assert(JavaThread::get() == thread);

  if (exc != NULL) {
    thread->clearException();
    TRY {
      handler = vm->upcalls->getUncaughtExceptionHandler->invokeJavaObjectVirtual(vm, thClass, javaThread);
      verifyNull(handler);
      vm->upcalls->uncaughtException->invokeIntVirtual(vm, thClass, handler, &javaThread, &exc);
    } CATCH {
      fprintf(stderr, "Exception in thread \"(unknown)\": "
                      "Can not print stack trace.\n");
    } END_CATCH;
  }

  // Call Thread.exit()
  vm->upcalls->threadExit->invokeIntVirtual(vm, thClass, javaThread);

  JavaObject::acquire(javaThread);
  // Indicate that the thread is done by clearing the eetop field.
  vm->upcalls->eetop->setInstanceLongField(javaThread, 0);
  // Notify all waiting threads
  JavaObject::notifyAll(javaThread);
  JavaObject::release(javaThread);

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
  JavaObject * sleepObject = 0;
  JavaObject * thread = 0;
  JavaThread * javaThread = 0;
  llvm_gcroot(thread, 0);
  llvm_gcroot(javaThread, 0);
  llvm_gcroot(sleepObject, 0);
  BEGIN_JNI_EXCEPTION

  thread = *(JavaObject**)_thread;
  assert(thread);
  assert(thread->getVirtualTable());
  Jnjvm* vm = JavaThread::get()->getJVM();

  // Create a placeholder 'sleepObject' that
  // we track in same field as Classpath's VMThread,
  // used solely for sleeping.
  sleepObject = vm->upcalls->OfObject->doNew(vm);

  JavaThread * newTh = new JavaThread(vm);
  if (!newTh) vm->outOfMemoryError();

  // Set the eetop field
  vm->upcalls->eetop->setInstanceLongField(thread, (long)newTh);

  newTh->start((void (*)(vmkit::Thread*))start);

  newTh->initialise(thread, sleepObject);

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
  jlong InternalThread =vm->upcalls->eetop->getInstanceLongField(thread);

  RETURN_FROM_JNI(InternalThread != 0);

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
  vmkit::Thread::yield();
}

static struct timeval getTimeVal(long long ms) {
  struct timeval ret;
  long long seconds = ms / 1000LL;
  ret.tv_usec = 1000LL * (ms % 1000LL);
  ret.tv_sec = seconds > LONG_MAX ? LONG_MAX : seconds;
  return ret;
}


JNIEXPORT void JNICALL
JVM_Sleep(JNIEnv *env, jclass threadClass, jlong _millis) {

  BEGIN_JNI_EXCEPTION

  Jnjvm * vm = th->getJVM();

  // Check for invalid sleep time
  if (_millis < 0)
    th->getJVM()->illegalArgumentException("Negative sleep value");

  uint64 millis = _millis;

  // If Thread is interrupted, throw exception
  if (th->lockingThread.interruptFlag != 0) {
    th->lockingThread.interruptFlag = 0;
    vm->interruptedException(NULL);
    UNREACHABLE();
  }

  // Treat sleep(0) as yield.
  if (millis == 0) {
    vmkit::Thread::yield();
    RETURN_VOID_FROM_JNI
  }

  // See the Classpath sleep() implementation for inspiration here.
  // We use a dummy per-thread 'sleepObject' to wait on instead of using
  // a VMThread, but otherwise same implementation.
  assert(th->sleepObject && "Missing sleep object!");

  uint64 now = CurrentTimeMillis();

  JavaObject::acquire(th->sleepObject);
  while(true) {
    struct timeval tv = getTimeVal(millis);
    JavaObject::timedWait(th->sleepObject, tv);

    uint64 then = now;
    now = CurrentTimeMillis();
    uint64 timePassed = now - then;

    if (timePassed >= millis) break;

    millis -= timePassed;
  }
  JavaObject::release(th->sleepObject);

  RETURN_VOID_FROM_JNI
  END_JNI_EXCEPTION
}

JNIEXPORT jobject JNICALL
JVM_CurrentThread(JNIEnv *env, jclass threadClass) {
  BEGIN_JNI_EXCEPTION
  JavaThread* th = JavaThread::get();
  RETURN_REF_FROM_JNI(th->currentThread(), jobject);
  END_JNI_EXCEPTION

  return 0;
}

JNIEXPORT jint JNICALL
JVM_CountStackFrames(JNIEnv *env, jobject thread) {
  NYI();
}

JNIEXPORT void JNICALL
JVM_Interrupt(JNIEnv *env, jobject _thread) {
  JavaObject * thread = 0;
  llvm_gcroot(thread, 0);
  BEGIN_JNI_EXCEPTION

  thread = *(JavaObject**)_thread;
  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField * field = vm->upcalls->eetop;

  // It's possible that the thread to be interrupted has not finished
  // its initialization. Wait until the initialization is done.
  while (field->getInstanceLongField(thread) == 0)
    vmkit::Thread::yield();

  JavaThread* jth = (JavaThread*)field->getInstanceLongField(thread);
  jth->lockingThread.interruptFlag = 1;
  vmkit::FatLock* lock = jth->lockingThread.waitsOn;

  // If the thread is blocked on a wait. We also verify nextWaiting in case
  // the thread has been notified.
  if (lock && jth->lockingThread.nextWaiting) {
    jth->lockingThread.state = vmkit::LockingThread::StateInterrupted;

    // Make sure the thread is waiting.
    uint32 locked = 0;
    while (true) {
      locked = (lock->tryAcquire() == 0);
      if (locked || (lock->getOwner() != jth && lock->getOwner() != 0))
        break;
      else vmkit::Thread::yield();
    }

    // Interrupt the thread.
    jth->lockingThread.varcond.signal();

    // Release the lock if we acquired it.
    if (locked) lock->release(lock->getAssociatedObject(), vm->lockSystem);
  }

  // Here we could also raise a signal for interrupting I/O

  RETURN_VOID_FROM_JNI
  END_JNI_EXCEPTION
}

JNIEXPORT jboolean JNICALL
JVM_IsInterrupted(JNIEnv *env, jobject _thread, jboolean clearInterrupted) {
  JavaObject * thread = 0;
  llvm_gcroot(thread, 0);

  bool interrupt = false;

  BEGIN_JNI_EXCEPTION

  thread = *(JavaObject**)_thread;
  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaField * field = vm->upcalls->eetop;

  JavaThread* jth = (JavaThread*)field->getInstanceLongField(thread);
  interrupt = (jboolean)jth->lockingThread.interruptFlag;

  if (clearInterrupted)
    jth->lockingThread.interruptFlag = 0;

  RETURN_FROM_JNI(interrupt);

  END_JNI_EXCEPTION

  return false;
}

JNIEXPORT jboolean JNICALL
JVM_HoldsLock(JNIEnv *env, jclass threadClass, jobject obj) {
  BEGIN_JNI_EXCEPTION
  jboolean res = JavaObject::owner(*(JavaObject**)obj);
  RETURN_FROM_JNI(res);
  END_JNI_EXCEPTION
  return false;
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
  ArrayObject * res = 0;
  llvm_gcroot(res, 0);
  BEGIN_JNI_EXCEPTION

  res = GetClassContext(1);

  RETURN_REF_FROM_JNI(res, jobjectArray);
  END_JNI_EXCEPTION

  return 0;
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
  // TODO: Actually implement this
  // Not doing so doesn't seem to hurt anything, yet.
  return name;
}

JNIEXPORT jobjectArray JNICALL
JVM_GetSystemPackages(JNIEnv *env) {
  ArrayObject * res = 0;
  llvm_gcroot(res, 0);

  BEGIN_JNI_EXCEPTION

  Jnjvm* vm = JavaThread::get()->getJVM();
  res = vm->bootstrapLoader->getBootPackages(vm);

  RETURN_REF_FROM_JNI(res, jobjectArray);

  END_JNI_EXCEPTION

  return NULL;
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
  JavaObject* res = 0;
  llvm_gcroot(res, 0);

  BEGIN_JNI_EXCEPTION

  JavaThread* th = JavaThread::get();
  res = th->getNonNullClassLoader();

  RETURN_REF_FROM_JNI(res, jobject);
  END_JNI_EXCEPTION

  return 0;
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
  ArrayObject* array = 0;
  llvm_gcroot(array, 0);

  BEGIN_JNI_EXCEPTION
  array = *(ArrayObject**)arr;
  jint size = ArrayObject::getSize(array);
  RETURN_FROM_JNI(size);

  END_JNI_EXCEPTION

  return 0;
}

JNIEXPORT jobject JNICALL
JVM_GetArrayElement(JNIEnv *env, jobject arr, jint index) {
  ArrayObject* array = 0;
  JavaObject* element = 0;
  llvm_gcroot(array, 0);
  llvm_gcroot(element, 0);

  BEGIN_JNI_EXCEPTION
  array = *(ArrayObject**)arr;
  element = ArrayObject::getElement(array, index);

  RETURN_REF_FROM_JNI(element, jobject);

  END_JNI_EXCEPTION

  return 0;
}

JNIEXPORT jvalue JNICALL
JVM_GetPrimitiveArrayElement(JNIEnv *env, jobject arr, jint index, jint wCode) {
  NYI();
}

JNIEXPORT void JNICALL
JVM_SetArrayElement(JNIEnv *env, jobject arr, jint index, jobject val) {
  ArrayObject* array = 0;
  JavaObject* element = 0;
  llvm_gcroot(array, 0);
  llvm_gcroot(element, 0);

  BEGIN_JNI_EXCEPTION
  verifyNull(arr);
  array = *(ArrayObject**)arr;
  element = val ? *(JavaObject**)val : NULL;

  Jnjvm* vm = th->getJVM();

  // Verify this is, in fact, an array
  UserCommonClass * cl = JavaObject::getClass(array);
  if (!cl || !cl->isArray())
    vm->illegalArgumentException("Invalid object, expected array");

  // Bounds check
  if (index < 0 || index >= ArrayObject::getSize(array))
    vm->indexOutOfBounds(array, index);

  // Check the object is of the correct type
  UserCommonClass * base = cl->asArrayClass()->baseClass();
  if (element && !JavaObject::instanceOf(element, base))
    vm->illegalArgumentException("Wrong object for array type");

  ArrayObject::setElement(array, element, index);

  RETURN_VOID_FROM_JNI
  END_JNI_EXCEPTION
}

JNIEXPORT void JNICALL
JVM_SetPrimitiveArrayElement(JNIEnv *env, jobject arr, jint index, jvalue v,
                             unsigned char vCode) {
  NYI();
}

JNIEXPORT jobject JNICALL
JVM_NewArray(JNIEnv *env, jclass eltClass, jint length) {
  JavaObject* res = 0;
  JavaObject* eltClassObj = 0;
  llvm_gcroot(res, 0);
  llvm_gcroot(eltClassObj, 0);

  BEGIN_JNI_EXCEPTION

  Jnjvm* vm = th->getJVM();
  if (length < 0) vm->negativeArraySizeException(length);

  eltClassObj = *(JavaObject**)eltClass;
  UserCommonClass* base =
    UserCommonClass::resolvedImplClass(vm, eltClassObj, true);
  JnjvmClassLoader* loader = base->classLoader;
  const UTF8* name = base->getName();
  const UTF8* arrayName = loader->constructArrayName(1, name);
  if (base->isPrimitive()) {
    UserClassArray *array = 0;
    if (base == vm->upcalls->OfBool) {
      array = vm->upcalls->ArrayOfBool;
    } else if (base == vm->upcalls->OfByte) {
      array = vm->upcalls->ArrayOfByte;
    } else if (base == vm->upcalls->OfShort) {
      array = vm->upcalls->ArrayOfShort;
    } else if (base == vm->upcalls->OfChar) {
      array = vm->upcalls->ArrayOfChar;
    } else if (base == vm->upcalls->OfInt) {
      array = vm->upcalls->ArrayOfInt;
    } else if (base == vm->upcalls->OfFloat) {
      array = vm->upcalls->ArrayOfFloat;
    } else if (base == vm->upcalls->OfLong) {
      array = vm->upcalls->ArrayOfLong;
    } else if (base == vm->upcalls->OfDouble) {
      array = vm->upcalls->ArrayOfDouble;
    } else {
      vm->illegalArgumentException("Invalid array primitive type!");
      abort();
    }
    res = (JavaObject*)array->doNew(length, vm);
  } else {
    UserClassArray* array = loader->constructArray(arrayName, base);
    res = (JavaObject*)array->doNew(length, vm);
  }

  RETURN_REF_FROM_JNI(res, jobject);

  END_JNI_EXCEPTION

  return 0;
}

extern JavaObject* multiCallNewIntern(UserClassArray* cl, uint32 len,
                               sint32* dims, Jnjvm* vm);
JNIEXPORT jobject JNICALL
JVM_NewMultiArray(JNIEnv *env, jclass eltClass, jintArray _dim) {
  ArraySInt32* dim = 0;
  JavaObjectClass * eltClassObj = 0;
  JavaObject* array = 0;

  llvm_gcroot(dim, 0);
  llvm_gcroot(eltClassObj, 0);
  llvm_gcroot(array, 0);

  BEGIN_JNI_EXCEPTION

  Jnjvm* vm = th->getJVM();

  verifyNull(eltClass);
  verifyNull(_dim);

  eltClassObj = *(JavaObjectClass**)eltClass;
  dim = *(ArraySInt32**)_dim;

  int dim_len = ArraySInt32::getSize(dim);
  if(dim_len <= 0 || dim_len > 255) {
    vm->illegalArgumentException("Too many dimensions for array");
    UNREACHABLE();
  }

  UserCommonClass* base = JavaObjectClass::getClass(eltClassObj);
  JnjvmClassLoader* loader = base->classLoader;

  // Build array name
  vmkit::ThreadAllocator allocator;
  const UTF8* arrayName;
  if (base->isPrimitive()) {
    char baseChar;
    if (base == vm->upcalls->OfBool) {
      baseChar = I_BOOL;
    } else if (base == vm->upcalls->OfByte) {
      baseChar = I_BYTE;
    } else if (base == vm->upcalls->OfShort) {
      baseChar = I_SHORT;
    } else if (base == vm->upcalls->OfChar) {
      baseChar = I_CHAR;
    } else if (base == vm->upcalls->OfInt) {
      baseChar = I_INT;
    } else if (base == vm->upcalls->OfFloat) {
      baseChar = I_FLOAT;
    } else if (base == vm->upcalls->OfLong) {
      baseChar = I_LONG;
    } else if (base == vm->upcalls->OfDouble) {
      baseChar = I_DOUBLE;
    } else {
      vm->illegalArgumentException("Invalid array primitive type!");
      UNREACHABLE();
    }
    char* buf = (char*)allocator.Allocate(dim_len+2);
    for(int i = 0; i < dim_len; ++i) buf[i] = I_TAB;
    buf[dim_len] = baseChar;
    buf[dim_len+1] = '\0';
    arrayName = loader->asciizConstructUTF8(buf);
  }
  else {
    const UTF8* baseName = base->getName();
    arrayName = loader->constructArrayName(dim_len, baseName);
  }

  // Get class corresponding to array name
  UserClassArray* arrayCl =
    (UserClassArray*)loader->loadClassFromUserUTF8(arrayName, true, false, NULL);

  // Convert dimensions array to internal representation
  sint32* dims = (sint32*)allocator.Allocate(sizeof(sint32) * dim_len);
  for (sint32 i = 0; i < dim_len; ++i){
    dims[i] = ArraySInt32::getElement(dim, i);
  }

  // Invoke recursive helper to create this array
  array = multiCallNewIntern(arrayCl, dim_len, dims, vm);

  RETURN_REF_FROM_JNI(array, jobject);

  END_JNI_EXCEPTION

  return 0;
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

  // Find the requested frame
  JavaMethod * meth = th->getCallingMethodLevel(n);

  // Grab the next frame up if the frame requested is Method.invoke()
  // TODO: Better integrate this throughout the reflection API?
  // Note that this isn't necessary in the Classpath port only because
  // it doesn't enforce any language access control for reflection
  if (meth == vm->upcalls->ReflectInvokeMethod)
    meth = th->getCallingMethodLevel(n+1);

  UserClass *cl = meth->classDef;
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
  JavaObjectClass * Cl = 0;
  llvm_gcroot(Cl, 0);
  BEGIN_JNI_EXCEPTION

  Cl = *(JavaObjectClass**)cls;

  UserCommonClass * C = JavaObjectClass::getClass(Cl);
  assert(C && C->isClass());

  C->asClass()->resolveClass();

  RETURN_VOID_FROM_JNI
  END_JNI_EXCEPTION
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
  JavaString * str = 0;
  llvm_gcroot(loader, 0);
  llvm_gcroot(str, 0);
  BEGIN_JNI_EXCEPTION

  Jnjvm* vm = JavaThread::get()->getJVM();
  JnjvmClassLoader* JCL = NULL;

  loader = *(JavaObject**)_loader;
  str = *(JavaString**)name;

  CommonClass * Cl = 0;

  JCL = JnjvmClassLoader::getJnjvmLoaderFromJavaObject(loader, vm);
  Cl = JCL->lookupClassFromJavaString(str);
  jclass res = Cl ? (jclass)Cl->getClassDelegateePtr(vm) : 0;
  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION

  return 0;
}

/* Define a class */
JNIEXPORT jclass JNICALL
JVM_DefineClass(JNIEnv *env, const char *name, jobject _loader, const jbyte *buf,
                jsize len, jobject pd) {
  JavaObject * loader = 0;
  llvm_gcroot(loader, 0);
  BEGIN_JNI_EXCEPTION

  loader = _loader ? *(JavaObject**)_loader : 0;

  jclass res = 0;

  Jnjvm* vm = JavaThread::get()->getJVM();
  JnjvmClassLoader* JCL = NULL;
  JCL = JnjvmClassLoader::getJnjvmLoaderFromJavaObject(loader, vm);
  const UTF8* utfName = JCL->asciizConstructUTF8(name);

  UserClass* cl = defineClass(JCL, utfName, (const char*)buf, len);

  res = (jclass)cl->getClassDelegateePtr(vm, pd ? *(JavaObject**)pd : 0);

  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION

  return 0;
}

/* Define a class with a source (added in JDK1.5) */
JNIEXPORT jclass JNICALL
JVM_DefineClassWithSource(JNIEnv *env, const char *name, jobject loader,
                          const jbyte *buf, jsize len, jobject pd,
                          const char *source) {
  // Discard source for now
  return JVM_DefineClass(env, name, loader, buf, len, pd);
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
  // Discard source and verification
  return JVM_DefineClass(env, name, loader, buf, len, pd);
}

/*
 * Reflection support functions
 */

JNIEXPORT jstring JNICALL
JVM_GetClassName(JNIEnv *env, jclass cls) {
  JavaObject * result = 0;
  JavaObject * Cl = 0;
  llvm_gcroot(result, 0);
  llvm_gcroot(Cl, 0);
  BEGIN_JNI_EXCEPTION

  Cl = *(JavaObject**)cls;

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserCommonClass* cl = UserCommonClass::resolvedImplClass(vm, Cl, false);

  const UTF8* iname = cl->getName();
  result = JavaString::internalToJava(iname, vm);

  RETURN_REF_FROM_JNI(result, jstring);

  END_JNI_EXCEPTION

  return 0;
}

JNIEXPORT jobjectArray JNICALL
JVM_GetClassInterfaces(JNIEnv *env, jclass cls) {
  ArrayObject* ret = 0;
  llvm_gcroot(ret, 0);

  BEGIN_JNI_EXCEPTION

  ret = JavaObjectClass::getInterfaces(*(JavaObjectClass**)cls);

  RETURN_REF_FROM_JNI(ret, jobjectArray);

  END_JNI_EXCEPTION

  return 0;
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
  RETURN_REF_FROM_JNI(res, jobject);

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
  // Let them 'set' signers, since we don't implement this yet and
  // don't allow them to 'get' the value back (above).
  //NYI();
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

  RETURN_REF_FROM_JNI(pd, jobject);

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

  jboolean ret = cl->isArray();
  RETURN_FROM_JNI(ret);

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

  jboolean ret = cl->isPrimitive();
  RETURN_FROM_JNI(ret);

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
  JavaObjectClass * Cl = 0;
  llvm_gcroot(Cl, 0);
  BEGIN_JNI_EXCEPTION

  Cl = *(JavaObjectClass**)cls;

  jint res = JavaObjectClass::getModifiers(Cl);
  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION

  return 0;
}

JNIEXPORT jobjectArray JNICALL
JVM_GetDeclaredClasses(JNIEnv *env, jclass ofClass) {
  ArrayObject* ret = 0;
  llvm_gcroot(ret, 0);

  BEGIN_JNI_EXCEPTION

  // TODO: Verify boolean parameter here, assuming should default to true.
  ret = JavaObjectClass::getDeclaredClasses(*(JavaObjectClass**)ofClass, false);

  RETURN_REF_FROM_JNI(ret, jobjectArray);

  END_JNI_EXCEPTION

  return 0;
}

JNIEXPORT jclass JNICALL
JVM_GetDeclaringClass(JNIEnv *env, jclass ofClass) {
  JavaObject* ret = 0;
  llvm_gcroot(ret, 0);

  BEGIN_JNI_EXCEPTION

  ret = JavaObjectClass::getDeclaringClass(*(JavaObjectClass**)ofClass);

  RETURN_REF_FROM_JNI(ret, jclass);

  END_JNI_EXCEPTION

  return 0;
}

/* Generics support (JDK 1.5) */
JNIEXPORT jstring JNICALL
JVM_GetClassSignature(JNIEnv *env, jclass cls) {
  JavaObjectClass * Cl = 0;
  jstring sig = 0;
  llvm_gcroot(Cl, 0);

  BEGIN_JNI_EXCEPTION

  Cl = cls ? *(JavaObjectClass**)cls : 0;
  verifyNull(Cl);

  Jnjvm* vm = th->getJVM();
  CommonClass* CC =
    UserCommonClass::resolvedImplClass(vm, Cl, false);
  Class* cl = CC->asClass();

  // Primitives, arrays, interfaces...
  if (!cl) RETURN_FROM_JNI(0);

  Attribut* sigAtt = cl->lookupAttribut(Attribut::signatureAttribut);

  if (sigAtt) {
    Reader reader(sigAtt, cl->bytes);
    uint16 index = reader.readU2();

    sig = (jstring)cl->classLoader->UTF8ToStr(cl->getConstantPool()->UTF8At(index));
  }

  RETURN_FROM_JNI(sig);

  END_JNI_EXCEPTION

  return 0;
}

/* Annotations support (JDK 1.5) */
JNIEXPORT jbyteArray JNICALL
JVM_GetClassAnnotations(JNIEnv *env, jclass cls) {
  ArraySInt8 * ret = 0;
  JavaObjectClass* Cl = 0;
  llvm_gcroot(ret, 0);
  llvm_gcroot(Cl, 0);

  BEGIN_JNI_EXCEPTION

  Cl = cls ? *(JavaObjectClass**)cls : 0;
  verifyNull(Cl);

  Jnjvm* vm = th->getJVM();
  CommonClass* CC = JavaObjectClass::getClass(Cl);
  assert(CC);
  Class* cl = CC->asClass();
  assert(cl);

  Attribut* annotationsAtt =
    cl->lookupAttribut(Attribut::annotationsAttribut);

  if (annotationsAtt) {
    uint32 len = annotationsAtt->nbb;
    ret = (ArraySInt8*)vm->upcalls->ArrayOfByte->doNew(len, vm);

    Reader reader(annotationsAtt, cl->bytes);
    for(uint32 i = 0; i < len; ++i) {
      ArraySInt8::setElement(ret, reader.readS1(), i);
    }
    RETURN_REF_FROM_JNI(ret, jbyteArray);
  }

  RETURN_FROM_JNI(0);

  END_JNI_EXCEPTION

  return 0;
}

/*
 * New (JDK 1.4) reflection implementation
 */

JNIEXPORT jobjectArray JNICALL
JVM_GetClassDeclaredMethods(JNIEnv *env, jclass ofClass, jboolean publicOnly) {
  ArrayObject* ret = 0;
  llvm_gcroot(ret, 0);

  BEGIN_JNI_EXCEPTION

  ret = JavaObjectClass::getDeclaredMethods(*(JavaObjectClass**)ofClass, publicOnly);

  RETURN_REF_FROM_JNI(ret, jobjectArray);

  END_JNI_EXCEPTION

  return 0;
}

JNIEXPORT jobjectArray JNICALL
JVM_GetClassDeclaredFields(JNIEnv *env, jclass ofClass, jboolean publicOnly) {
  ArrayObject* ret = 0;
  llvm_gcroot(ret, 0);

  BEGIN_JNI_EXCEPTION

  ret = JavaObjectClass::getDeclaredFields(*(JavaObjectClass**)ofClass, publicOnly);

  RETURN_REF_FROM_JNI(ret, jobjectArray);

  END_JNI_EXCEPTION

  return 0;
}

JNIEXPORT jobjectArray JNICALL
JVM_GetClassDeclaredConstructors(JNIEnv *env, jclass ofClass, jboolean publicOnly) {
  ArrayObject* ret = 0;
  llvm_gcroot(ret, 0);

  BEGIN_JNI_EXCEPTION

  ret = JavaObjectClass::getDeclaredConstructors(*(JavaObjectClass**)ofClass, publicOnly);

  RETURN_REF_FROM_JNI(ret, jobjectArray);

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
  jint res = cl->asClass()->access;
  RETURN_FROM_JNI(res);

  END_JNI_EXCEPTION

  return 0;
}

/* The following two reflection routines are still needed due to startup time issues */
/*
 * java.lang.reflect.Method
 */
JNIEXPORT jobject JNICALL
JVM_InvokeMethod(JNIEnv *env, jobject _m, jobject _obj, jobjectArray args0) {
  JavaObject * ret = 0;
  JavaObjectMethod * m = 0;
  llvm_gcroot(ret, 0);
  llvm_gcroot(m, 0);

  BEGIN_JNI_EXCEPTION

  m = *(JavaObjectMethod**)_m;

  ret = proceedMethod(
    m,
    _obj ? *(JavaObject**)_obj : 0,
    args0? *(ArrayObject**)args0 : 0,
    JavaObjectMethod::getClass(m)->getDelegatee(),
    0 /* unused */ );

  RETURN_REF_FROM_JNI(ret, jobject);
  END_JNI_EXCEPTION

  return 0;
}

/*
 * java.lang.reflect.Constructor
 */
JNIEXPORT jobject JNICALL
JVM_NewInstanceFromConstructor(JNIEnv *env, jobject _c, jobjectArray args0) {
  JavaObject * ret = 0;
  JavaObjectConstructor * c = 0;
  llvm_gcroot(ret, 0);
  llvm_gcroot(c, 0);

  BEGIN_JNI_EXCEPTION

  c = *(JavaObjectConstructor**)_c;

  ret = proceedConstructor(
    c,
    args0? *(ArrayObject**)args0 : 0,
    JavaObjectConstructor::getClass(c)->getDelegatee(),
    0 /* unused */ );

  RETURN_REF_FROM_JNI(ret, jobject);
  END_JNI_EXCEPTION

  return 0;
}

/*
 * Constant pool access; currently used to implement reflective access to annotations (JDK 1.5
 */

JNIEXPORT jobject JNICALL
JVM_GetClassConstantPool(JNIEnv *env, jclass cls) {
  JavaObjectClass * Cl = 0;
  JavaObject* CP = 0;
  llvm_gcroot(Cl, 0);
  llvm_gcroot(CP, 0);
  BEGIN_JNI_EXCEPTION

  Cl = cls ? *(JavaObjectClass**)cls : 0;
  verifyNull(Cl);

  Jnjvm* vm = th->getJVM();

  CP = vm->upcalls->constantPoolClass->doNew(vm);

  // Set the constantPoolOop field to point to the Class this
  // ConstantPool object is supposed to represent the constant pool for.
  vm->upcalls->constantPoolOop->setInstanceObjectField(CP, Cl);

  RETURN_REF_FROM_JNI(CP, jobject);

  END_JNI_EXCEPTION

  return 0;
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

  res = meth->invokeJavaObjectVirtual(vm, cl, obj);

  RETURN_REF_FROM_JNI(res, jobject);

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
  // Don't let signals be registered, yet.
  return (void*)-1;
}

JNIEXPORT jboolean JNICALL
JVM_RaiseSignal(jint sig) {
  NYI();
}

JNIEXPORT jint JNICALL
JVM_FindSignal(const char *name) {
  // Map Signal name to integer...
  static struct {
    const char * name;
    int num;
  } SignalMap[] =
  {
    { "TERM", SIGTERM },
    { "HUP", SIGHUP },
    { "INT", SIGINT }
  };
  static uint32 signal_count = sizeof(SignalMap)/sizeof(SignalMap[0]);

  BEGIN_JNI_EXCEPTION

  for(uint32 i = 0; i < signal_count; ++i) {
    if (!strcmp(name, SignalMap[i].name))
      RETURN_FROM_JNI(SignalMap[i].num);
  }

  fprintf(stderr, "Unknown Signal \"%s\", missing entry in table?\n", name);
  UNIMPLEMENTED();

  END_JNI_EXCEPTION

  return 0;
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
  JavaObject* asdObj = 0;
  JavaObject* emptyStringArr = 0;
  llvm_gcroot(asdObj, 0);
  llvm_gcroot(emptyStringArr, 0);
  BEGIN_JNI_EXCEPTION

  Jnjvm* vm = th->getJVM();
  UserClass * asdCl = vm->upcalls->assertionStatusDirectivesClass;
  asdObj = asdCl->doNew(vm);

  emptyStringArr = vm->upcalls->ArrayOfString->doNew(0, vm);

  const UTF8 * classesName = asdCl->classLoader->hashUTF8->lookupAsciiz("classes");
  const UTF8 * packagesName = asdCl->classLoader->hashUTF8->lookupAsciiz("packages");
  const UTF8 * stringArrType = asdCl->classLoader->hashUTF8->lookupAsciiz("[Ljava/lang/String;");
  JavaField* classes = asdCl->lookupFieldDontThrow(classesName, stringArrType, false, true, 0);
  JavaField* packages = asdCl->lookupFieldDontThrow(packagesName, stringArrType, false, true, 0);
  assert(classes);
  assert(packages);

  classes->setInstanceObjectField(asdObj, emptyStringArr);
  packages->setInstanceObjectField(asdObj, emptyStringArr);

  RETURN_REF_FROM_JNI(asdObj, jobject);
  END_JNI_EXCEPTION

  return NULL;
}

/*
 * java.util.concurrent.AtomicLong
 */
JNIEXPORT jboolean JNICALL
JVM_SupportsCX8(void) {
  return JNI_FALSE;
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

  jint len = strlen(buf);
  RETURN_FROM_JNI(len);

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
  // TODO: Implement me?
  return path;
}

/*
 * Open a file descriptor. This function returns a negative error code
 * on error, and a non-negative integer that is the file descriptor on
 * success.
 */
JNIEXPORT jint JNICALL
JVM_Open(const char *fname, jint flags, jint mode) {
  // Special flag the JVM uses
  // means to delete the file after opening.
  static const int O_DELETE = 0x10000;

  BEGIN_JNI_EXCEPTION

  int result = open(fname, flags & ~O_DELETE, mode);

  // Map EEXIST to special JVM_EEXIST, otherwise all errors are -1
  if (result < 0) {
    if (errno == EEXIST) {
      RETURN_FROM_JNI(JVM_EEXIST);
    } else {
      RETURN_FROM_JNI(-1);
    }
  }

  // Handle O_DELETE flag, if specified
  if (flags & O_DELETE)
    unlink(fname);

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
 * is extended; the contents of the extended portion are not defined.  The
 * value of the file pointer is undefined after this procedure returns.
 */
JNIEXPORT jint JNICALL
JVM_SetLength(jint fd, jlong length) {
  int res = ftruncate64(fd, length);
  return res != -1;
}

/*
 * Synchronize the file descriptor's in memory state with that of the
 * physical device.  Return of -1 is an error, 0 is OK.
 */
JNIEXPORT jint JNICALL
JVM_Sync(jint fd) {
  return fsync(fd);
}

/*
 * Networking library support
 */

JNIEXPORT jint JNICALL
JVM_InitializeSocketLibrary(void) {
  // Nothing to do here.
  return 0;
}

JNIEXPORT jint JNICALL
JVM_Socket(jint domain, jint type, jint protocol) {
  return socket(domain, type, protocol);
}

JNIEXPORT jint JNICALL
JVM_SocketClose(jint fd) {
  return close(fd);
}

JNIEXPORT jint JNICALL
JVM_SocketShutdown(jint fd, jint howto) {
  // Nothing to do here.
  return true;
}

JNIEXPORT jint JNICALL
JVM_Recv(jint fd, char *buf, jint nBytes, jint flags) {
  return recv(fd, buf, nBytes, flags);
}

JNIEXPORT jint JNICALL
JVM_Send(jint fd, char *buf, jint nBytes, jint flags) {
  return send(fd, buf, nBytes, flags);
}

JNIEXPORT jint JNICALL
JVM_Timeout(int fd, long timeout) {
  NYI();
}

JNIEXPORT jint JNICALL
JVM_Listen(jint fd, jint count) {
  return listen(fd, count);
}

JNIEXPORT jint JNICALL
JVM_Connect(jint fd, struct sockaddr *him, jint len) {
  return connect(fd, him, len);
}

JNIEXPORT jint JNICALL
JVM_Bind(jint fd, struct sockaddr *him, jint len) {
  return bind(fd, him, len);
}

JNIEXPORT jint JNICALL
JVM_Accept(jint fd, struct sockaddr *him, jint *len) {
  return accept(fd, him, (socklen_t*)len);
}

JNIEXPORT jint JNICALL
JVM_RecvFrom(jint fd, char *buf, int nBytes,
                  int flags, struct sockaddr *from, int *fromlen) {
  return recvfrom(fd, buf, nBytes, flags, from, (socklen_t*)fromlen);
}

JNIEXPORT jint JNICALL
JVM_SendTo(jint fd, char *buf, int len,
                int flags, struct sockaddr *to, int tolen) {
  return sendto(fd, buf, len, flags, to, (socklen_t)tolen);
}

JNIEXPORT jint JNICALL
JVM_SocketAvailable(jint fd, jint *result) {

  BEGIN_JNI_EXCEPTION

  if(ioctl(fd, TIOCINQ, result) == -1)
    RETURN_FROM_JNI(JNI_FALSE);

  RETURN_FROM_JNI(JNI_TRUE);
  END_JNI_EXCEPTION

  RETURN_FROM_JNI(JNI_FALSE);
}


JNIEXPORT jint JNICALL
JVM_GetSockName(jint fd, struct sockaddr *him, int *len) {
  return getsockname(fd, him, (socklen_t*)len);
}

JNIEXPORT jint JNICALL
JVM_GetSockOpt(jint fd, int level, int optname, char *optval, int *optlen) {
  return getsockopt(fd, level, optname, optval, (socklen_t*)optlen);
}

JNIEXPORT jint JNICALL
JVM_SetSockOpt(jint fd, int level, int optname, const char *optval, int optlen) {
  return setsockopt(fd, level, optname, optval, optlen);
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
  va_list args;
  int res;

  va_start(args, fmt);
  res = vsnprintf(str, count, fmt, args);
  va_end(args);

  return res;
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
  JavaObjectClass * Cl = 0;
  ArrayObject * arr = 0;
  llvm_gcroot(Cl, 0);
  llvm_gcroot(arr, 0);

  BEGIN_JNI_EXCEPTION
  Jnjvm * vm = th->getJVM();

  Cl = ofClass ? *(JavaObjectClass**)ofClass : 0;
  verifyNull(Cl);

  CommonClass* CC =
    UserCommonClass::resolvedImplClass(vm, Cl, false);
  Class* cl = CC->asClass();

  // Primitives arrays, interfaces...
  if (!cl) RETURN_FROM_JNI(0);

  Attribut* emAtt = cl->lookupAttribut(Attribut::enclosingMethodAttribut);

  if (emAtt) {
    Reader reader(emAtt, cl->bytes);
    uint16 class_index = reader.readU2();
    uint16 method_index = reader.readU2();

    JavaConstantPool * cpl = cl->getConstantPool();
    CommonClass * enclosingClass = cpl->loadClass(class_index, true);
    assert(enclosingClass);

    arr = (ArrayObject*)vm->upcalls->ArrayOfObject->doNew(3,vm);
    ArrayObject::setElement(arr, enclosingClass->getClassDelegatee(vm), 0);

    if (method_index) {
      Signdef * sign = cl->getConstantPool()->resolveNameAndSign(method_index);
      assert(sign);

      const UTF8* methodName = cpl->UTF8At((cpl->ctpDef[method_index] >> 16) & 0xFFFF);
      assert(methodName);

      ArrayObject::setElement(arr, *cl->classLoader->UTF8ToStr(methodName), 1);
      ArrayObject::setElement(arr, *cl->classLoader->UTF8ToStr(sign->keyName), 2);
    }
  }

  RETURN_REF_FROM_JNI(arr, jobjectArray);

  END_JNI_EXCEPTION

  return 0;
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
