#include "vmkit/safepoint.h"
#include "j3/j3.h"
#include "j3/j3thread.h"
#include "j3/j3classloader.h"
#include "j3/j3class.h"
#include "j3/j3method.h"
#include "j3/j3constants.h"
#include "j3/j3field.h"
#include "j3/j3utf16.h"
#include "j3/j3reader.h"
#include "jvm.h"

#include <dlfcn.h>
#include <sys/utsname.h>
#include <sys/time.h>
#include <stdlib.h>
#include <fcntl.h>
#include <cxxabi.h>

using namespace j3;

#define NYI() { J3Thread::get()->vm()->internalError("not yet implemented: '%s'", __PRETTY_FUNCTION__); }

/*************************************************************************
 PART 0
 ************************************************************************/

jint JNICALL JVM_GetInterfaceVersion(void) { 
  return JVM_INTERFACE_VERSION;
}

/*************************************************************************
 PART 1: Functions for Native Libraries
 ************************************************************************/
/*
 * java.lang.Object
 */
jint JNICALL JVM_IHashCode(JNIEnv* env, jobject obj) { 
	uint32_t res;
	enterJVM(); 
	res = obj ? obj->hashCode() : 0;
	leaveJVM(); 
	return res;
}

void JNICALL JVM_MonitorWait(JNIEnv* env, jobject obj, jlong ms) { 
	enterJVM(); 
	obj->wait();
	leaveJVM(); 
}

void JNICALL JVM_MonitorNotify(JNIEnv* env, jobject obj) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL JVM_MonitorNotifyAll(JNIEnv* env, jobject obj) { enterJVM(); leaveJVM(); NYI(); }
jobject JNICALL JVM_Clone(JNIEnv* env, jobject obj) { 
	jobject res;
	enterJVM(); 
	res = obj->vt()->type()->asObjectType()->clone(obj);
	leaveJVM(); 
	return res;
}

/*
 * java.lang.String
 */
jstring JNICALL JVM_InternString(JNIEnv* env, jstring str) { 
	jstring res;
	enterJVM(); 

	J3* vm = J3Thread::get()->vm();

	res = vm->nameToString(vm->stringToName(str));

	leaveJVM(); 
	return res;
}

/*
 * java.lang.System
 */
jlong JNICALL JVM_CurrentTimeMillis(JNIEnv* env, jclass ignored) { 
	jlong res;
	enterJVM(); 
	struct timeval tv;
	gettimeofday(&tv, 0);
	res = tv.tv_sec*1e3 + tv.tv_usec/1e3;
	leaveJVM(); 
	return res;
}

jlong JNICALL JVM_NanoTime(JNIEnv* env, jclass ignored) { 
	/* TODO: better timer? */
	jlong res;
	enterJVM(); 
	struct timeval tv;
	gettimeofday(&tv, 0);
	res = tv.tv_sec*1e9 + tv.tv_usec*1e3;
	leaveJVM(); 
	return res;
}

void JNICALL JVM_ArrayCopy(JNIEnv* env, jclass ignored, jobject src, jint src_pos, jobject dst, jint dst_pos, jint length) { 
	enterJVM(); 

	J3Type* srcType0 = src->vt()->type();
	J3Type* dstType0 = dst->vt()->type();

	//fprintf(stderr, " array copy from %s to %s\n", srcType0->name()->cStr(), dstType0->name()->cStr());
	if(!srcType0->isArrayClass() || !dstType0->isArrayClass())
		J3::arrayStoreException();

	//fprintf(stderr, " array copy: [%d %d %d] [%d %d %d]\n", src_pos, length, src->arrayLength(), dst_pos, length, dst->arrayLength());
	if(src_pos < 0 || 
		 dst_pos < 0 ||
		 (src_pos + length) > src->arrayLength() ||
		 (dst_pos + length) > dst->arrayLength())
		J3::arrayIndexOutOfBoundsException();

	uint32_t scale = srcType0->asArrayClass()->component()->logSize();

	if(srcType0->isAssignableTo(dstType0))
		src->rawArrayCopyTo(src_pos << scale, dst, dst_pos << scale, length << scale);
	else {
		J3Type* srcEl = srcType0->asArrayClass()->component();
		J3Type* dstEl = dstType0->asArrayClass()->component();
		
		if(!srcEl->isObjectType() || !dstEl->isObjectType())
			J3::arrayStoreException();
		
		for(uint32_t i=0; i<length; i++) {
			J3ObjectHandle* val = src->getObjectAt(src_pos + i);
			if(!val->vt()->type()->isAssignableTo(dstEl))
				J3::arrayStoreException();
			dst->setObjectAt(dst_pos + i, val);
		}
	}

	leaveJVM(); 
}

jobject JNICALL JVM_InitProperties(JNIEnv* env, jobject p) { 
	enterJVM(); 
	J3* vm = J3Thread::get()->vm();
	J3Class* pcl = p->vt()->type()->asClass();
	J3Method* _setProp = pcl->findMethod(0,
																			 vm->names()->get("setProperty"), 
																			 pcl->loader()
																			    ->getSignature(0,
																												 vm->names()->get("(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/Object;")));

#define setProp(key, val) _setProp->invokeVirtual(p, vm->utfToString(key), vm->utfToString(val));
#define setPropEnv(key, val, def) ({ const char* tmp = getenv(val); if(!tmp) tmp = def; setProp(key, tmp); })

	/*
	** <dt>java.version         <dd>Java version number
	** <dt>java.vendor          <dd>Java vendor specific string
	** <dt>java.vendor.url      <dd>Java vendor URL
	** <dt>java.home            <dd>Java installation directory
	** <dt>java.class.version   <dd>Java class version number
	** <dt>java.class.path      <dd>Java classpath
	** <dt>os.name              <dd>Operating System Name
	** <dt>os.arch              <dd>Operating System Architecture
	** <dt>os.version           <dd>Operating System Version
	** <dt>file.separator       <dd>File separator ("/" on Unix)
	** <dt>path.separator       <dd>Path separator (":" on Unix)
	** <dt>line.separator       <dd>Line separator ("\n" on Unix)
	** <dt>user.name            <dd>User account name
	** <dt>user.home            <dd>User home directory
	** <dt>user.dir             <dd>User's current working directory
	*/

  struct utsname infos;
  uname(&infos);

  setProp("java.version", "1.8");
  setProp("java.vendor", "The VMKit Project");
  setProp("java.vendor.url", "http://vmkit.llvm.org");
	setProp("java.home", vm->options()->javaHome);
  setProp("java.class.version", "52.0");
  setProp("java.class.path", vm->options()->classpath);
	//"file:///Users/gthomas/research/vmkit4/vmkit");//vm->options()->classpath);
  setProp("os.name", infos.sysname);
  setProp("os.arch", infos.machine);
  setProp("os.version", infos.release);
  setProp("file.separator", "/");
  setProp("path.separator", ":");
  setProp("line.separator", "\n");
  setPropEnv("user.name", "USERNAME", "");
  setPropEnv("user.home", "HOME", "");
	setPropEnv("user.dir", "PWD", "");

  setProp("java.boot.class.path", vm->options()->bootClasspath);
  setProp("sun.boot.library.path", vm->options()->systemLibraryPath);
  setProp("sun.boot.class.path", vm->options()->bootClasspath);
  setProp("java.ext.dirs", vm->options()->extDirs);


#if 0
  setProp("java.vm.specification.version", "1.2");
  setProp("java.vm.specification.vendor", "Sun Microsystems, Inc");
  setProp("java.vm.specification.name", "Java Virtual Machine Specification");
  setProp("java.specification.version", "1.8");
  setProp("java.specification.vendor", "Sun Microsystems, Inc");
  setProp("java.specification.name", "Java Platform API Specification");
  setProp("java.runtime.version", "1.8");
  setProp("java.vm.version", "0.5");
  setProp("java.vm.vendor", "The VMKit Project");
  setProp("java.vm.name", "J3");
  setProp("java.specification.version", "1.8");
  setPropEnv("java.library.path", "LD_LIBRARY_PATH", "");


  setProp("java.io.tmpdir", "/tmp");
  JnjvmBootstrapLoader* JCL = vm->bootstrapLoader;

  setProperty(vm, prop, "build.compiler", "gcj");
  setProperty(vm, prop, "gcj.class.path", JCL->bootClasspathEnv);
  setProperty(vm, prop, "gnu.classpath.boot.library.path",
              JCL->libClasspathEnv);

  // Align behavior with GNU Classpath for now, to pass mauve test
  setProperty(vm, prop, "sun.lang.ClassLoader.allowArraySyntax", "true");

  if (!strcmp(infos.machine, "ppc")) {
    setProperty(vm, prop, "gnu.cpu.endian","big");
  } else {
    setProperty(vm, prop, "gnu.cpu.endian","little");
  }

  setProperty(vm, prop, "file.separator", vm->dirSeparator);
  setProperty(vm, prop, "path.separator", vm->envSeparator);
  setProperty(vm, prop, "line.separator", "\n");

  tmp = getenv("USERNAME");
  if (!tmp) tmp = getenv("LOGNAME");
  if (!tmp) tmp = getenv("NAME");
  if (!tmp) tmp = "";
  setProperty(vm, prop, "user.name", tmp);

  tmp  = getenv("HOME");
  if (!tmp) tmp = "";
  setProperty(vm, prop, "user.home", tmp);

  tmp = getenv("PWD");
  if (!tmp) tmp = "";
  setProperty(vm, prop, "user.dir", tmp);

  // Disable this property. The Classpath iconv implementation is really
  // not optimized (it over-abuses JNI calls).
  //setProperty(vm, prop, "gnu.classpath.nio.charset.provider.iconv", "true");
  setProperty(vm, prop, "file.encoding", "ISO8859_1");
  setProperty(vm, prop, "gnu.java.util.zoneinfo.dir", "/usr/share/zoneinfo");
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
#endif
	leaveJVM(); 
	return p;
}

/*
 * java.io.File
 */
void JNICALL JVM_OnExit(void (*func)(void)) { enterJVM(); leaveJVM(); NYI(); }

/*
 * java.lang.Runtime
 */
void JNICALL JVM_Exit(jint code) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL JVM_Halt(jint code) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL JVM_GC(void) { enterJVM(); leaveJVM(); NYI(); }

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
jlong JNICALL JVM_MaxObjectInspectionAge(void) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL JVM_TraceInstructions(jboolean on) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL JVM_TraceMethodCalls(jboolean on) { enterJVM(); leaveJVM(); NYI(); }
jlong JNICALL JVM_TotalMemory(void) { enterJVM(); leaveJVM(); NYI(); }
jlong JNICALL JVM_FreeMemory(void) { enterJVM(); leaveJVM(); NYI(); }
jlong JNICALL JVM_MaxMemory(void) { enterJVM(); leaveJVM(); NYI(); }
jint JNICALL JVM_ActiveProcessorCount(void) { 
	return sysconf(_SC_NPROCESSORS_ONLN);
}

void* JNICALL JVM_LoadLibrary(const char *name) { 
	void* res;
	enterJVM(); 
	res = dlopen(name, RTLD_LAZY | RTLD_LOCAL);
	if(res)
		J3Thread::get()->vm()->initialClassLoader->addNativeLibrary(res);
	leaveJVM(); 
	return res;
}

void JNICALL JVM_UnloadLibrary(void * handle) { enterJVM(); leaveJVM(); NYI(); }

void * JNICALL JVM_FindLibraryEntry(void *handle, const char *name) { 
	void* res;
	enterJVM(); 
	res = dlsym(handle, name);
	leaveJVM(); 
	return res;
}

jboolean JNICALL JVM_IsSupportedJNIVersion(jint version) { 
  return version == JNI_VERSION_1_1 ||
         version == JNI_VERSION_1_2 ||
         version == JNI_VERSION_1_4 ||
         version == JNI_VERSION_1_6 ||
         version == JNI_VERSION_1_8;
}

/*
 * java.lang.Float and java.lang.Double
 */
jboolean JNICALL JVM_IsNaN(jdouble d) { enterJVM(); leaveJVM(); NYI(); }

/*
 * java.lang.Throwable
 */
void JNICALL JVM_FillInStackTrace(JNIEnv* env, jobject throwable) { 
	enterJVM(); 
	uint32_t           cur = 0;
	uint32_t           max = 1024;
	int64_t            _buf[max];
	int64_t*           buf = _buf;
	vmkit::StackWalker walker;

	while(walker.next()) {
		if(cur == max) {
			void* prev = buf;
			buf = (int64_t*)malloc((max<<1)*sizeof(int64_t));
			memcpy(buf, prev, max*sizeof(int64_t));
			max <<= 1;
			if(prev != _buf) 
				free(prev);
		}
		buf[cur++] = (int64_t)(uintptr_t)walker.ip();
	}
	
	J3* vm = J3Thread::get()->vm();
	jobject backtrace = J3ObjectHandle::doNewArray(vm->typeLong->getArray(), cur);
	backtrace->setRegionLong(0, buf, 0, cur);

	if(buf != _buf)
		free(buf);

	throwable->setObject(vm->throwableClassBacktrace, backtrace);

	leaveJVM(); 
}

jint JNICALL JVM_GetStackTraceDepth(JNIEnv* env, jobject throwable) { 
	jint res = 0;
	enterJVM(); 
	J3* vm = J3Thread::get()->vm();
	J3ObjectHandle* backtrace = throwable->getObject(vm->throwableClassBacktrace);

	bool simplify = 1;
	bool ignore = 1;

	if(simplify) {
		uint32_t max = backtrace->arrayLength();
		int64_t buf[max];

		while(max && !res) {
			for(uint32_t i=0; i<max; i++) {
				uint64_t cur = backtrace->getLongAt(i);
				vmkit::Safepoint* sf = vm->getSafepoint((void*)cur);

				if(sf) {
					J3Method* m = ((J3MethodCode*)sf->unit()->getSymbol(sf->functionName()))->self;
					if(ignore) {
						if(m->name() == vm->initName && m->cl() == throwable->vt()->type()) {
							ignore = 0;
						}
					} else
						buf[res++] = cur;
				}
			}
			ignore = 0;
		}

		jobject newBt = J3ObjectHandle::doNewArray(backtrace->vt()->type()->asArrayClass(), res);
		newBt->setRegionLong(0, buf, 0, res);
		throwable->setObject(vm->throwableClassBacktrace, newBt);

	} else
		res = backtrace->arrayLength();

	leaveJVM(); 

	return res;
}

jobject JNICALL JVM_GetStackTraceElement(JNIEnv* env, jobject throwable, jint index) { 
	jobject res;

	enterJVM(); 
	J3* vm = J3Thread::get()->vm();
	uintptr_t ip = (uintptr_t)throwable->getObject(vm->throwableClassBacktrace)->getLongAt(index);
	J3ObjectHandle* className;
	J3ObjectHandle* methodName;
	J3ObjectHandle* fileName;
	uint32_t lineNumber;

	res = J3ObjectHandle::doNewObject(vm->stackTraceElementClass);

	vmkit::Safepoint* sf = vm->getSafepoint((void*)ip);

	if(!sf) {
		lineNumber = -1;
		className = vm->utfToString("<j3>");
		Dl_info info;
		
		if(dladdr((void*)(ip-1), &info)) {
			int   status;
			const char* demangled = abi::__cxa_demangle(info.dli_sname, 0, 0, &status);
			const char* name = demangled ? demangled : info.dli_sname;
			methodName = vm->utfToString(name);
			fileName = vm->utfToString(info.dli_fname);
		} else {
			char buf[256];
			snprintf(buf, 256, "??@%p", (void*)ip);
			methodName = vm->utfToString(buf);
			fileName = vm->utfToString("??");
		}
	} else {
		J3Method* m = ((J3MethodCode*)sf->unit()->getSymbol(sf->functionName()))->self;
		const vmkit::Name* cn = m->cl()->name();
		uint32_t length = cn->length()+6;
		uint32_t lastToken = 0;
		char buf[length];

		for(uint32_t i=0; i<cn->length(); i++) {
			if(cn->cStr()[i] == '/') {
				buf[i] = '.';
				lastToken = i+1;
			} else
				buf[i] = cn->cStr()[i];
		}
		buf[cn->length()] = 0;

		lineNumber = sf->sourceIndex();
		className = vm->utfToString(buf);
		methodName = m->name() == vm->initName ? vm->utfToString(buf+lastToken) : vm->nameToString(m->name());
		
		snprintf(buf, length, "%s.java", cn->cStr());
		fileName = vm->utfToString(buf);
	}

	vm->stackTraceElementClassInit->invokeSpecial(res, className, methodName, fileName, lineNumber);

	leaveJVM(); 
	return res;
}

/*
 * java.lang.Compiler
 */
void JNICALL JVM_InitializeCompiler (JNIEnv* env, jclass compCls) { enterJVM(); leaveJVM(); NYI(); }
jboolean JNICALL JVM_IsSilentCompiler(JNIEnv* env, jclass compCls) { enterJVM(); leaveJVM(); NYI(); }
jboolean JNICALL JVM_CompileClass(JNIEnv* env, jclass compCls, jclass cls) { enterJVM(); leaveJVM(); NYI(); }
jboolean JNICALL JVM_CompileClasses(JNIEnv* env, jclass cls, jstring jname) { enterJVM(); leaveJVM(); NYI(); }
jobject JNICALL JVM_CompilerCommand(JNIEnv* env, jclass compCls, jobject arg) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL JVM_EnableCompiler(JNIEnv* env, jclass compCls) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL JVM_DisableCompiler(JNIEnv* env, jclass compCls) { enterJVM(); leaveJVM(); NYI(); }

/*
 * java.lang.Thread
 */
void JNICALL JVM_StartThread(JNIEnv* env, jobject thread) { 
	enterJVM(); 
	J3Thread::start(thread);
	leaveJVM(); 
}

void JNICALL JVM_StopThread(JNIEnv* env, jobject thread, jobject exception) { enterJVM(); leaveJVM(); NYI(); }
jboolean JNICALL JVM_IsThreadAlive(JNIEnv* env, jobject thread) { 
	jboolean res;

	enterJVM(); 
	res = (jboolean)J3Thread::nativeThread(thread);
	leaveJVM(); 

	return res;
}

void JNICALL JVM_SuspendThread(JNIEnv* env, jobject thread) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL JVM_ResumeThread(JNIEnv* env, jobject thread) { enterJVM(); leaveJVM(); NYI(); }

void JNICALL JVM_SetThreadPriority(JNIEnv* env, jobject thread, jint prio) { 
	enterJVM(); 
	// not yet implemented
	leaveJVM(); 
}

void JNICALL JVM_Yield(JNIEnv* env, jclass threadClass) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL JVM_Sleep(JNIEnv* env, jclass threadClass, jlong millis) { enterJVM(); leaveJVM(); NYI(); }

jobject JNICALL JVM_CurrentThread(JNIEnv* env, jclass threadClass) { 
	jobject res;
	enterJVM(); 
	res = J3Thread::get()->javaThread();
	leaveJVM(); 
	return res;
}

jint JNICALL JVM_CountStackFrames(JNIEnv* env, jobject thread) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL JVM_Interrupt(JNIEnv* env, jobject thread) { enterJVM(); leaveJVM(); NYI(); }
jboolean JNICALL JVM_IsInterrupted(JNIEnv* env, jobject thread, jboolean clearInterrupted) { 
	jboolean res;
	enterJVM(); 
	res = J3Thread::nativeThread(thread)->isInterrupted();
	leaveJVM(); 
	return res;
}

jboolean JNICALL JVM_HoldsLock(JNIEnv* env, jclass threadClass, jobject obj) { 
	jboolean res;
	enterJVM(); 
	res = obj->isLockOwner();
	leaveJVM(); 
	return res;
}

void JNICALL JVM_DumpAllStacks(JNIEnv* env, jclass unused) { enterJVM(); leaveJVM(); NYI(); }
jobjectArray JNICALL JVM_GetAllThreads(JNIEnv* env, jclass dummy) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL JVM_SetNativeThreadName(JNIEnv* env, jobject jthread, jstring name) { enterJVM(); leaveJVM(); NYI(); }

/* getStackTrace() and getAllStackTraces() method */
jobjectArray JNICALL JVM_DumpThreads(JNIEnv* env, jclass threadClass, jobjectArray threads) { enterJVM(); leaveJVM(); NYI(); }

/*
 * java.lang.SecurityManager
 */
jclass JNICALL JVM_CurrentLoadedClass(JNIEnv* env) { enterJVM(); leaveJVM(); NYI(); }
jobject JNICALL JVM_CurrentClassLoader(JNIEnv* env) { enterJVM(); leaveJVM(); NYI(); }
jobjectArray JNICALL JVM_GetClassContext(JNIEnv* env) { enterJVM(); leaveJVM(); NYI(); }
jint JNICALL JVM_ClassDepth(JNIEnv* env, jstring name) { enterJVM(); leaveJVM(); NYI(); }
jint JNICALL JVM_ClassLoaderDepth(JNIEnv* env) { enterJVM(); leaveJVM(); NYI(); }

/*
 * java.lang.Package
 */
jstring JNICALL JVM_GetSystemPackage(JNIEnv* env, jstring name) { enterJVM(); leaveJVM(); NYI(); }
jobjectArray JNICALL JVM_GetSystemPackages(JNIEnv* env) { enterJVM(); leaveJVM(); NYI(); }

/*
 * java.io.ObjectInputStream
 */
jobject JNICALL JVM_AllocateNewObject(JNIEnv* env, jobject obj, jclass currClass, jclass initClass) { enterJVM(); leaveJVM(); NYI(); }
jobject JNICALL JVM_AllocateNewArray(JNIEnv* env, jobject obj, jclass currClass, jint length) { enterJVM(); leaveJVM(); NYI(); }
jobject JNICALL JVM_LatestUserDefinedLoader(JNIEnv* env) { enterJVM(); leaveJVM(); NYI(); }

/*
 * This function has been deprecated and should not be considered
 * part of the specified JVM interface.
 */
jclass JNICALL
JVM_LoadClass0(JNIEnv* env, jobject obj, jclass currClass, jstring currClassName) { enterJVM(); leaveJVM(); NYI(); }

/*
 * java.lang.reflect.Array
 */
jint JNICALL JVM_GetArrayLength(JNIEnv* env, jobject arr) { enterJVM(); leaveJVM(); NYI(); }
jobject JNICALL JVM_GetArrayElement(JNIEnv* env, jobject arr, jint index) { enterJVM(); leaveJVM(); NYI(); }
jvalue JNICALL JVM_GetPrimitiveArrayElement(JNIEnv* env, jobject arr, jint index, jint wCode) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL JVM_SetArrayElement(JNIEnv* env, jobject arr, jint index, jobject val) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL JVM_SetPrimitiveArrayElement(JNIEnv* env, jobject arr, jint index, jvalue v, unsigned char vCode) { enterJVM(); leaveJVM(); NYI(); }

jobject JNICALL JVM_NewArray(JNIEnv* env, jclass eltClass, jint length) { 
	jobject res;
	enterJVM(); 
  if(length < 0) J3::negativeArraySizeException(length);
	res = J3ObjectHandle::doNewArray(J3Class::nativeClass(eltClass)->getArray(), length);
	leaveJVM(); 
	return res;
}

jobject JNICALL JVM_NewMultiArray(JNIEnv* env, jclass eltClass, jintArray dim) { enterJVM(); leaveJVM(); NYI(); }

/*
 * java.lang.Class and java.lang.ClassLoader
 */

/*
 * Returns the immediate caller class of the native method invoking
 * JVM_GetCallerClass.  The Method.invoke and other frames due to
 * reflection machinery are skipped.
 *
 * The depth parameter must be -1 (JVM_DEPTH). The caller is expected
 * to be marked with sun.reflect.CallerSensitive.  The JVM will throw
 * an error if it is not marked propertly.
 */
jclass JNICALL JVM_GetCallerClass(JNIEnv* env, int depth) { 
	jclass res;
	enterJVM(); 

	if(depth != -1)
		J3::internalError("depth should be -1 while it is %d", depth);

	depth = 3;
	J3Method* caller = 0;
	J3* vm = J3Thread::get()->vm();
	vmkit::Safepoint* sf = 0;
	vmkit::StackWalker walker;

	while(!caller && walker.next()) {
		vmkit::Safepoint* sf = vm->getSafepoint(walker.ip());

		if(sf) {
			if(!--depth)
				caller = ((J3MethodCode*)sf->unit()->getSymbol(sf->functionName()))->self;
		}
	}

	if(!caller)
		J3::internalError("unable to find caller class, what should I do?");

	res = caller->cl()->javaClass();

	leaveJVM(); 

	return res;
}


/*
 * Find primitive classes
 * utf: class name
 */
jclass JNICALL JVM_FindPrimitiveClass(JNIEnv* env, const char *utf) { 
	jclass res;

	enterJVM(); 
	J3* vm = J3Thread::get()->vm();
	J3ClassLoader* loader = vm->initialClassLoader;
	vmkit::Names* names = vm->names();
	J3Type* cl;

  if(!strcmp(utf, "boolean"))
		cl = vm->typeBoolean;
	else if(!strcmp(utf, "byte"))
		cl = vm->typeByte;
	else if(!strcmp(utf, "char"))
		cl = vm->typeCharacter;
	else if(!strcmp(utf, "short"))
		cl = vm->typeShort;
	else if(!strcmp(utf, "int"))
		cl = vm->typeInteger;
	else if(!strcmp(utf, "long"))
		cl = vm->typeLong;
	else if(!strcmp(utf, "float"))
		cl = vm->typeFloat;
	else if(!strcmp(utf, "double"))
		cl = vm->typeDouble;
	else if(!strcmp(utf, "void"))
		cl = vm->typeVoid;
	else
		J3::internalError("unsupported primitive: %s", utf);

	res = cl->javaClass();
	leaveJVM(); 
	return res;
}

/*
 * Link the class
 */
void JNICALL JVM_ResolveClass(JNIEnv* env, jclass cls) { enterJVM(); leaveJVM(); NYI(); }

/*
 * Find a class from a boot class loader. Returns NULL if class not found.
 */
jclass JNICALL JVM_FindClassFromBootLoader(JNIEnv* env, const char *name) { 
	jclass res;
	enterJVM(); 
	J3* vm = J3Thread::get()->vm();
	J3ObjectType* cl = vm->initialClassLoader->getTypeFromQualified(0, name);
	res = cl ? cl->javaClass() : 0;
	leaveJVM(); 
	return res;
}

/*
 * Find a class from a given class loader. Throw ClassNotFoundException
 * or NoClassDefFoundError depending on the value of the last
 * argument.
 */
jclass JNICALL JVM_FindClassFromClassLoader(JNIEnv* env, const char *jname, jboolean init, jobject jloader, jboolean throwError) { 
	jclass res;
	enterJVM();
	J3* vm = J3Thread::get()->vm();
	if(jloader)
		J3::internalError("implement me: jloader");
	J3ClassLoader* loader = J3Thread::get()->vm()->initialClassLoader;
	J3ObjectType* cl = loader->getTypeFromQualified(0, jname);

	if(!cl) {
		const vmkit::Name* name = vm->names()->get(jname);
		if(throwError)
			J3::noClassDefFoundError(name);
		else
			J3::classNotFoundException(name);
	}

	if(init)
		cl->initialise();
	else
		cl->resolve();

	res = cl->javaClass();

	leaveJVM(); 
	return res;
}

/*
 * Find a class from a given class.
 */
jclass JNICALL JVM_FindClassFromClass(JNIEnv* env, const char *name, jboolean init, jclass from) { enterJVM(); leaveJVM(); NYI(); }

/* Find a loaded class cached by the VM */
jclass JNICALL JVM_FindLoadedClass(JNIEnv* env, jobject jloader, jstring name) { 
	jclass res;
	enterJVM(); 
	J3Class* cl = J3ClassLoader::nativeClassLoader(jloader)->findLoadedClass(J3Thread::get()->vm()->stringToName(name));
	res = cl ? cl->javaClass() : 0;
	leaveJVM(); 
	return res;
}

/* Define a class */
jclass JNICALL JVM_DefineClass(JNIEnv* env, const char *name, jobject loader, const jbyte *buf, jsize len, jobject pd) { 
	return JVM_DefineClassWithSource(env, name, loader, buf, len, pd, 0);
}

/* Define a class with a source (added in JDK1.5) */
jclass JNICALL JVM_DefineClassWithSource(JNIEnv* env, const char *name, jobject _loader, 
																				 const jbyte *buf, jsize len, jobject pd,
																				 const char *source) { 
	jclass res;
	enterJVM(); 
	J3* vm = J3Thread::get()->vm();
	J3ClassLoader* loader = _loader ? J3ClassLoader::nativeClassLoader(_loader) : vm->initialClassLoader;
	J3ClassBytes* bytes = new(loader->allocator(), len) J3ClassBytes((uint8_t*)buf, len);
	res = loader->defineClass(vm->names()->get(name), bytes, pd, source)->javaClass();
	leaveJVM(); 
	return res;
}

/*
 * Reflection support functions
 */

jstring JNICALL JVM_GetClassName(JNIEnv* env, jclass cls) { 
	jstring res;
	enterJVM(); 
	res = J3Thread::get()->vm()->nameToString(J3ObjectType::nativeClass(cls)->name(), 1);
	leaveJVM(); 
	return res;
}

jobjectArray JNICALL JVM_GetClassInterfaces(JNIEnv* env, jclass cls) { 
	jobject res;
	enterJVM(); 
	J3* vm = J3Thread::get()->vm();
	J3ObjectType* type = J3ObjectType::nativeClass(cls);
	J3Class** interfaces;
	uint32_t  nbInterfaces;
	
	if(type->isClass()) {
		J3Class* cl = type->asClass();
		interfaces = cl->interfaces();
		nbInterfaces = cl->nbInterfaces();
	} else {
		interfaces = vm->arrayInterfaces;
		nbInterfaces = vm->nbArrayInterfaces;
	}

	res = J3ObjectHandle::doNewArray(vm->classClass->getArray(), nbInterfaces);

	for(uint32_t i=0; i<nbInterfaces; i++)
		res->setObjectAt(i, interfaces[i]->javaClass());

	leaveJVM(); 

	return res;
}

jobject JNICALL JVM_GetClassLoader(JNIEnv* env, jclass cls) { 
	jobject res;
	enterJVM(); 
	res = J3ObjectType::nativeClass(cls)->loader()->javaClassLoader();
	leaveJVM(); 
	return res;
}

jboolean JNICALL JVM_IsInterface(JNIEnv* env, jclass cls) { 
	jboolean res;
	enterJVM(); 
	J3ObjectType* type = J3ObjectType::nativeClass(cls);
	res = type->isArrayClass() ? 0 : J3Cst::isInterface(type->asClass()->access());
	leaveJVM(); 
	return res;
}

jobjectArray JNICALL JVM_GetClassSigners(JNIEnv* env, jclass cls) { enterJVM(); leaveJVM(); NYI(); }
void JNICALL JVM_SetClassSigners(JNIEnv* env, jclass cls, jobjectArray signers) { enterJVM(); leaveJVM(); NYI(); }
jobject JNICALL JVM_GetProtectionDomain(JNIEnv* env, jclass cls) { enterJVM(); leaveJVM(); NYI(); }

jboolean JNICALL JVM_IsArrayClass(JNIEnv* env, jclass cls) { 
	jboolean res;
	enterJVM(); 
	res = J3ObjectType::nativeClass(cls)->isArrayClass();
	leaveJVM(); 
	return res;
}

jboolean JNICALL JVM_IsPrimitiveClass(JNIEnv* env, jclass cls) { 
	jboolean res = 0;
	enterJVM(); 
	J3* vm = J3Thread::get()->vm();
	if(0) {}
#define testIt(id, ctype, llvmType, scale)			\
	else if(cls->isSame(vm->type##id->javaClass()))	\
		res = 1;
	onJavaTypes(testIt)
#undef testIt
	leaveJVM(); 
	return res;
}

jclass JNICALL JVM_GetComponentType(JNIEnv* env, jclass cls) { 
	jclass res;
	enterJVM();
	J3ObjectType* cl = J3ObjectType::nativeClass(cls);
	res = cl->isArrayClass() ? cl->asArrayClass()->component()->javaClass() : 0;
	leaveJVM(); 
	return res;
}

jint JNICALL JVM_GetClassModifiers(JNIEnv* env, jclass cls) { 
	jint res;
	enterJVM(); 
	res = J3ObjectType::nativeClass(cls)->modifiers();
	leaveJVM(); 
	return res;
}

jobjectArray JNICALL JVM_GetDeclaredClasses(JNIEnv* env, jclass ofClass) { enterJVM(); leaveJVM(); NYI(); }
jclass JNICALL JVM_GetDeclaringClass(JNIEnv* env, jclass ofClass) { enterJVM(); leaveJVM(); NYI(); }

/* Generics support (JDK 1.5) */
jstring JNICALL JVM_GetClassSignature(JNIEnv* env, jclass cls) { enterJVM(); leaveJVM(); NYI(); }

/* Annotations support (JDK 1.5) */
jbyteArray JNICALL JVM_GetClassAnnotations(JNIEnv* env, jclass cls) { enterJVM(); leaveJVM(); NYI(); }

/* Type use annotations support (JDK 1.8) */
jbyteArray JNICALL JVM_GetClassTypeAnnotations(JNIEnv* env, jclass cls) { enterJVM(); leaveJVM(); NYI(); }


/*
 * New (JDK 1.4) reflection implementation
 */

jobjectArray JNICALL JVM_GetClassDeclaredMethods(JNIEnv* env, jclass ofClass, jboolean publicOnly) { 
	jobjectArray res;

	enterJVM(); 
	J3* vm = J3Thread::get()->vm();
	J3ObjectType* type = J3ObjectType::nativeClass(ofClass);
	
	if(type->isClass()) {
		J3Class* cl = type->asClass();
		cl->resolve();
		res = J3ObjectHandle::doNewArray(vm->methodClass->getArray(), 
																		 publicOnly ? 
																		 cl->staticLayout()->nbPublicMethods() + cl->nbPublicMethods() - cl->nbPublicConstructors() : 
																		 cl->staticLayout()->nbMethods() + cl->nbMethods() - cl->nbConstructors());
		
		uint32_t pos = 0;
		for(uint32_t i=0; i<cl->nbMethods(); i++) {
			J3Method* m = cl->methods()[i];
			if(m->name() != vm->initName && (!publicOnly || J3Cst::isPublic(m->access())))
				res->setObjectAt(pos++, m->javaMethod());
		}

		for(uint32_t i=0; i<cl->staticLayout()->nbMethods(); i++) {
			J3Method* m = cl->staticLayout()->methods()[i];
			if(!publicOnly || J3Cst::isPublic(m->access()))
				res->setObjectAt(pos++, m->javaMethod());
		}
	} else
		res = J3ObjectHandle::doNewArray(vm->constructorClass->getArray(), 0);

	leaveJVM(); 
	return res;
}

jobjectArray JNICALL JVM_GetClassDeclaredFields(JNIEnv* env, jclass ofClass, jboolean publicOnly) { 
	jobjectArray res;

	enterJVM(); 
	J3ObjectType* type = J3ObjectType::nativeClass(ofClass);
	J3* vm = J3Thread::get()->vm();

	if(type->isClass()) {
		J3Class* cl = type->asClass();
		cl->resolve();
		size_t total = publicOnly ? 
			cl->nbPublicFields() + cl->staticLayout()->nbPublicFields() :
			cl->nbFields() + cl->staticLayout()->nbFields();

		res = J3ObjectHandle::doNewArray(vm->fieldClass->getArray(), total);

		size_t cur = 0;
		for(uint32_t i=0; i<cl->nbFields(); i++)
			if(!publicOnly || J3Cst::isPublic(cl->fields()[i].access()))
				res->setObjectAt(cur++, cl->fields()[i].javaField());
		for(uint32_t i=0; i<cl->staticLayout()->nbFields(); i++)
			if(!publicOnly || J3Cst::isPublic(cl->staticLayout()->fields()[i].access()))
				res->setObjectAt(cur++, cl->staticLayout()->fields()[i].javaField());
	} else
		res = J3ObjectHandle::doNewArray(vm->fieldClass->getArray(), 0);

	leaveJVM();

	return res;
}

jobjectArray JNICALL JVM_GetClassDeclaredConstructors(JNIEnv* env, jclass ofClass, jboolean publicOnly) { 
	jobjectArray res;

	enterJVM(); 
	J3* vm = J3Thread::get()->vm();
	J3ObjectType* type = J3ObjectType::nativeClass(ofClass);
	if(type->isClass()) {
		J3Class* cl = type->asClass();
		res = J3ObjectHandle::doNewArray(vm->constructorClass->getArray(), 
																		 publicOnly ? cl->nbPublicConstructors() : cl->nbConstructors());

		for(uint32_t i=0, pos=0; i<cl->nbMethods(); i++) {
			J3Method* m = cl->methods()[i];
			if(m->name() == vm->initName && (!publicOnly || J3Cst::isPublic(m->access())))
				res->setObjectAt(pos++, m->javaMethod());
		}
	} else
		res = J3ObjectHandle::doNewArray(vm->constructorClass->getArray(), 0);

	leaveJVM(); 
	return res;
}

/* Differs from JVM_GetClassModifiers in treatment of inner classes.
   This returns the access flags for the class as specified in the
   class file rather than searching the InnerClasses attribute (if
   present) to find the source-level access flags. Only the values of
   the low 13 bits (i.e., a mask of 0x1FFF) are guaranteed to be
   valid. */
jint JNICALL JVM_GetClassAccessFlags(JNIEnv* env, jclass cls) { 
	jint res;
	enterJVM(); 
	res = J3ObjectType::nativeClass(cls)->access();
	leaveJVM(); 
	return res;
}

/* The following two reflection routines are still needed due to startup time issues */
/*
 * java.lang.reflect.Method
 */
jobject JNICALL JVM_InvokeMethod(JNIEnv* env, jobject method, jobject obj, jobjectArray args0) { enterJVM(); leaveJVM(); NYI(); }

/*
 * java.lang.reflect.Constructor
 */
jobject JNICALL JVM_NewInstanceFromConstructor(JNIEnv* env, jobject c, jobjectArray args0) { 
	jobject res;
	enterJVM(); 
	J3Method* cons = J3Method::nativeMethod(c);
	res = J3ObjectHandle::doNewObject(cons->cl());
	if(cons->signature()->nbIns())
		J3::internalError("implement me: JVM_NewInstanceFromConstructor with arguments");
	cons->invokeSpecial(res);
	leaveJVM(); 
	return res;
}

/*
 * Constant pool access; currently used to implement reflective access to annotations (JDK 1.5)
 */

jobject JNICALL JVM_GetClassConstantPool(JNIEnv* env, jclass cls) { enterJVM(); leaveJVM(); NYI(); }
jint JNICALL JVM_ConstantPoolGetSize(JNIEnv* env, jobject unused, jobject jcpool) { enterJVM(); leaveJVM(); NYI(); }
jclass JNICALL JVM_ConstantPoolGetClassAt(JNIEnv* env, jobject unused, jobject jcpool, jint index) { enterJVM(); leaveJVM(); NYI(); }
jclass JNICALL JVM_ConstantPoolGetClassAtIfLoaded(JNIEnv* env, jobject unused, jobject jcpool, jint index) { enterJVM(); leaveJVM(); NYI(); }
jobject JNICALL JVM_ConstantPoolGetMethodAt(JNIEnv* env, jobject unused, jobject jcpool, jint index) { enterJVM(); leaveJVM(); NYI(); }
jobject JNICALL JVM_ConstantPoolGetMethodAtIfLoaded(JNIEnv* env, jobject unused, jobject jcpool, jint index) { enterJVM(); leaveJVM(); NYI(); }
jobject JNICALL JVM_ConstantPoolGetFieldAt(JNIEnv* env, jobject unused, jobject jcpool, jint index) { enterJVM(); leaveJVM(); NYI(); }
jobject JNICALL JVM_ConstantPoolGetFieldAtIfLoaded(JNIEnv* env, jobject unused, jobject jcpool, jint index) { enterJVM(); leaveJVM(); NYI(); }
jobjectArray JNICALL JVM_ConstantPoolGetMemberRefInfoAt(JNIEnv* env, jobject unused, jobject jcpool, jint index) { enterJVM(); leaveJVM(); NYI(); }
jint JNICALL JVM_ConstantPoolGetIntAt(JNIEnv* env, jobject unused, jobject jcpool, jint index) { enterJVM(); leaveJVM(); NYI(); }
jlong JNICALL JVM_ConstantPoolGetLongAt(JNIEnv* env, jobject unused, jobject jcpool, jint index) { enterJVM(); leaveJVM(); NYI(); }
jfloat JNICALL JVM_ConstantPoolGetFloatAt(JNIEnv* env, jobject unused, jobject jcpool, jint index) { enterJVM(); leaveJVM(); NYI(); }
jdouble JNICALL JVM_ConstantPoolGetDoubleAt(JNIEnv* env, jobject unused, jobject jcpool, jint index) { enterJVM(); leaveJVM(); NYI(); }
jstring JNICALL JVM_ConstantPoolGetStringAt(JNIEnv* env, jobject unused, jobject jcpool, jint index) { enterJVM(); leaveJVM(); NYI(); }
jstring JNICALL JVM_ConstantPoolGetUTF8At(JNIEnv* env, jobject unused, jobject jcpool, jint index) { enterJVM(); leaveJVM(); NYI(); }

/*
 * Parameter reflection
 */

jobjectArray JNICALL JVM_GetMethodParameters(JNIEnv* env, jobject method) { enterJVM(); leaveJVM(); NYI(); }

/*
 * java.security.*
 */

jobject JNICALL JVM_DoPrivileged(JNIEnv* env, jclass cls, jobject action, jobject context, jboolean wrapException) { 
	jobject res;
	enterJVM(); 

  // For now, we don't do anything special,
  // just call the requested 'run()' method...
	jmethodID mid = env->GetMethodID(env->GetObjectClass(action), "run", "()Ljava/lang/Object;");
	res = env->CallObjectMethod(action, mid);
	leaveJVM(); 
	return res;
}

jobject JNICALL JVM_GetInheritedAccessControlContext(JNIEnv* env, jclass cls) { enterJVM(); leaveJVM(); NYI(); }
jobject JNICALL JVM_GetStackAccessControlContext(JNIEnv* env, jclass cls) { 
	enterJVM(); 
  // not yet implemented
	leaveJVM(); 
	return 0;
}

/*
 * Signal support, used to implement the shutdown sequence.  Every VM must
 * support JVM_SIGINT and JVM_SIGTERM, raising the former for user interrupts
 * (^C) and the latter for external termination (kill, system shutdown, etc.).
 * Other platform-dependent signal values may also be supported.
 */

void* JNICALL JVM_RegisterSignal(jint sig, void *handler) {
	void* res;
	enterJVM(); 
	if(sig == SIGINT || sig == SIGTERM || sig == SIGHUP) { /* fixme: I need a full jvm to support this joke */
		//J3Thread::get()->registerSignal(sig, (J3Thread::sa_action_t)handler);		
		res = handler;
	} else
		res = (void*)-1;
	leaveJVM(); 
	return res;
}

jboolean JNICALL JVM_RaiseSignal(jint sig) { enterJVM(); leaveJVM(); NYI(); }

jint JNICALL JVM_FindSignal(const char *name) { 
	jint res = 0;

	enterJVM(); 

  static struct {
    const char * name;
    int num;
  } SignalMap[] =
			{
				{ "TERM", SIGTERM },
				{ "HUP", SIGHUP },
				{ "INT", SIGINT }
			};
  static uint32_t signal_count = sizeof(SignalMap)/sizeof(SignalMap[0]);

  for(uint32_t i = 0; i < signal_count; ++i) {
    if (!strcmp(name, SignalMap[i].name))
      res = SignalMap[i].num;
  }

	leaveJVM(); 
	
	return res;
}

/*
 * Retrieve the assertion directives for the specified class.
 */
jboolean JNICALL JVM_DesiredAssertionStatus(JNIEnv* env, jclass unused, jclass cls) { 
	jboolean res;
	enterJVM(); 
	/* TODO: take into account the class name and its package (see j3options) */
	res = J3Thread::get()->vm()->options()->assertionsEnabled ? JNI_TRUE : JNI_FALSE;
	leaveJVM(); 
	return res;
}

/*
 * Retrieve the assertion directives from the VM.
 */
jobject JNICALL JVM_AssertionStatusDirectives(JNIEnv* env, jclass unused) { enterJVM(); leaveJVM(); NYI(); }

/*
 * java.util.concurrent.atomic.AtomicLong
 */
jboolean JNICALL JVM_SupportsCX8(void) { enterJVM(); leaveJVM(); NYI(); }

/*
 * Get the version number the JVM was built with
 */
jint JNICALL JVM_DTraceGetVersion(JNIEnv* env) { enterJVM(); leaveJVM(); NYI(); }

/*
 * Register new probe with given signature, return global handle
 *
 * The version passed in is the version that the library code was
 * built with.
 */
jlong JNICALL JVM_DTraceActivate(JNIEnv* env, jint version, jstring module_name, 
																					 jint providers_count, JVM_DTraceProvider* providers) { enterJVM(); leaveJVM(); NYI(); }

/*
 * Check JSDT probe
 */
jboolean JNICALL JVM_DTraceIsProbeEnabled(JNIEnv* env, jmethodID method) { enterJVM(); leaveJVM(); NYI(); }

/*
 * Destroy custom DOF
 */
void JNICALL JVM_DTraceDispose(JNIEnv* env, jlong activation_handle) { enterJVM(); leaveJVM(); NYI(); }

/*
 * Check to see if DTrace is supported by OS
 */
jboolean JNICALL JVM_DTraceIsSupported(JNIEnv* env) { enterJVM(); leaveJVM(); NYI(); }

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
const char * JNICALL JVM_GetClassNameUTF(JNIEnv* env, jclass cb) { enterJVM(); leaveJVM(); NYI(); }

/*
 * Returns the constant pool types in the buffer provided by "types."
 */
void JNICALL JVM_GetClassCPTypes(JNIEnv* env, jclass cb, unsigned char *types) { enterJVM(); leaveJVM(); NYI(); }

/*
 * Returns the number of Constant Pool entries.
 */
jint JNICALL JVM_GetClassCPEntriesCount(JNIEnv* env, jclass cb) { enterJVM(); leaveJVM(); NYI(); }

/*
 * Returns the number of *declared* fields or methods.
 */
jint JNICALL JVM_GetClassFieldsCount(JNIEnv* env, jclass cb) { enterJVM(); leaveJVM(); NYI(); }
jint JNICALL JVM_GetClassMethodsCount(JNIEnv* env, jclass cb) { enterJVM(); leaveJVM(); NYI(); }

/*
 * Returns the CP indexes of exceptions raised by a given method.
 * Places the result in the given buffer.
 *
 * The method is identified by method_index.
 */
void JNICALL JVM_GetMethodIxExceptionIndexes(JNIEnv* env, jclass cb, jint method_index, unsigned short *exceptions) { enterJVM(); leaveJVM(); NYI(); }
/*
 * Returns the number of exceptions raised by a given method.
 * The method is identified by method_index.
 */
jint JNICALL JVM_GetMethodIxExceptionsCount(JNIEnv* env, jclass cb, jint method_index) { enterJVM(); leaveJVM(); NYI(); }

/*
 * Returns the byte code sequence of a given method.
 * Places the result in the given buffer.
 *
 * The method is identified by method_index.
 */
void JNICALL JVM_GetMethodIxByteCode(JNIEnv* env, jclass cb, jint method_index, unsigned char *code) { enterJVM(); leaveJVM(); NYI(); }

/*
 * Returns the length of the byte code sequence of a given method.
 * The method is identified by method_index.
 */
jint JNICALL JVM_GetMethodIxByteCodeLength(JNIEnv* env, jclass cb, jint method_index) { enterJVM(); leaveJVM(); NYI(); }

/*
 * Returns the exception table entry at entry_index of a given method.
 * Places the result in the given buffer.
 *
 * The method is identified by method_index.
 */
void JNICALL JVM_GetMethodIxExceptionTableEntry(JNIEnv* env, jclass cb, jint method_index, jint entry_index,
																													JVM_ExceptionTableEntryType *entry) { enterJVM(); leaveJVM(); NYI(); }

/*
 * Returns the length of the exception table of a given method.
 * The method is identified by method_index.
 */
jint JNICALL JVM_GetMethodIxExceptionTableLength(JNIEnv* env, jclass cb, int index) { enterJVM(); leaveJVM(); NYI(); }

/*
 * Returns the modifiers of a given field.
 * The field is identified by field_index.
 */
jint JNICALL JVM_GetFieldIxModifiers(JNIEnv* env, jclass cb, int index) { enterJVM(); leaveJVM(); NYI(); }

/*
 * Returns the modifiers of a given method.
 * The method is identified by method_index.
 */
jint JNICALL JVM_GetMethodIxModifiers(JNIEnv* env, jclass cb, int index) { enterJVM(); leaveJVM(); NYI(); }

/*
 * Returns the number of local variables of a given method.
 * The method is identified by method_index.
 */
jint JNICALL JVM_GetMethodIxLocalsCount(JNIEnv* env, jclass cb, int index) { enterJVM(); leaveJVM(); NYI(); }

/*
 * Returns the number of arguments (including this pointer) of a given method.
 * The method is identified by method_index.
 */
jint JNICALL JVM_GetMethodIxArgsSize(JNIEnv* env, jclass cb, int index) { enterJVM(); leaveJVM(); NYI(); }

/*
 * Returns the maximum amount of stack (in words) used by a given method.
 * The method is identified by method_index.
 */
jint JNICALL JVM_GetMethodIxMaxStack(JNIEnv* env, jclass cb, int index) { enterJVM(); leaveJVM(); NYI(); }

/*
 * Is a given method a constructor.
 * The method is identified by method_index.
 */
jboolean JNICALL JVM_IsConstructorIx(JNIEnv* env, jclass cb, int index) { enterJVM(); leaveJVM(); NYI(); }

/*
 * Is the given method generated by the VM.
 * The method is identified by method_index.
 */
jboolean JNICALL JVM_IsVMGeneratedMethodIx(JNIEnv* env, jclass cb, int index) { enterJVM(); leaveJVM(); NYI(); }

/*
 * Returns the name of a given method in UTF format.
 * The result remains valid until JVM_ReleaseUTF is called.
 *
 * The caller must treat the string as a constant and not modify it
 * in any way.
 */
const char * JNICALL JVM_GetMethodIxNameUTF(JNIEnv* env, jclass cb, jint index) { enterJVM(); leaveJVM(); NYI(); }

/*
 * Returns the signature of a given method in UTF format.
 * The result remains valid until JVM_ReleaseUTF is called.
 *
 * The caller must treat the string as a constant and not modify it
 * in any way.
 */
const char * JNICALL JVM_GetMethodIxSignatureUTF(JNIEnv* env, jclass cb, jint index) { enterJVM(); leaveJVM(); NYI(); }

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
const char * JNICALL JVM_GetCPFieldNameUTF(JNIEnv* env, jclass cb, jint index) { enterJVM(); leaveJVM(); NYI(); }

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
const char * JNICALL JVM_GetCPMethodNameUTF(JNIEnv* env, jclass cb, jint index) { enterJVM(); leaveJVM(); NYI(); }

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
const char * JNICALL JVM_GetCPMethodSignatureUTF(JNIEnv* env, jclass cb, jint index) { enterJVM(); leaveJVM(); NYI(); }

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
const char * JNICALL JVM_GetCPFieldSignatureUTF(JNIEnv* env, jclass cb, jint index) { enterJVM(); leaveJVM(); NYI(); }

/*
 * Returns the class name refered to at a given constant pool index.
 *
 * The result is in UTF format and remains valid until JVM_ReleaseUTF
 * is called.
 *
 * The caller must treat the string as a constant and not modify it
 * in any way.
 */
const char * JNICALL JVM_GetCPClassNameUTF(JNIEnv* env, jclass cb, jint index) { enterJVM(); leaveJVM(); NYI(); }

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
const char * JNICALL JVM_GetCPFieldClassNameUTF(JNIEnv* env, jclass cb, jint index) { enterJVM(); leaveJVM(); NYI(); }

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
const char * JNICALL JVM_GetCPMethodClassNameUTF(JNIEnv* env, jclass cb, jint index) { enterJVM(); leaveJVM(); NYI(); }

/*
 * Returns the modifiers of a field in calledClass. The field is
 * referred to in class cb at constant pool entry index.
 *
 * The caller must treat the string as a constant and not modify it
 * in any way.
 *
 * Returns -1 if the field does not exist in calledClass.
 */
jint JNICALL JVM_GetCPFieldModifiers(JNIEnv* env, jclass cb, int index, jclass calledClass) { enterJVM(); leaveJVM(); NYI(); }

/*
 * Returns the modifiers of a method in calledClass. The method is
 * referred to in class cb at constant pool entry index.
 *
 * Returns -1 if the method does not exist in calledClass.
 */
jint JNICALL JVM_GetCPMethodModifiers(JNIEnv* env, jclass cb, int index, jclass calledClass) { enterJVM(); leaveJVM(); NYI(); }

/*
 * Releases the UTF string obtained from the VM.
 */
void JNICALL JVM_ReleaseUTF(const char *utf) { enterJVM(); leaveJVM(); NYI(); }

/*
 * Compare if two classes are in the same package.
 */
jboolean JNICALL JVM_IsSameClassPackage(JNIEnv* env, jclass class1, jclass class2) { enterJVM(); leaveJVM(); NYI(); }

/*************************************************************************
 PART 3: I/O and Network Support
 ************************************************************************/

/* Write a string into the given buffer, in the platform's local encoding,
 * that describes the most recent system-level error to occur in this thread.
 * Return the length of the string or zero if no error occurred.
 */
jint JNICALL JVM_GetLastErrorString(char *buf, int len) { enterJVM(); leaveJVM(); NYI(); }

/*
 * Convert a pathname into native format.  This function does syntactic
 * cleanup, such as removing redundant separator characters.  It modifies
 * the given pathname string in place.
 */
char* JNICALL JVM_NativePath(char* path) { 
	char buf[PATH_MAX];
	enterJVM(); 
	realpath(path, buf);
	strcpy(path, buf);
	leaveJVM(); 
	return path;
}

/*
 * Open a file descriptor. This function returns a negative error code
 * on error, and a non-negative integer that is the file descriptor on
 * success.
 */
jint JNICALL JVM_Open(const char *fname, jint flags, jint mode) { 
	jint res;
	enterJVM(); 
  // Special flag the JVM uses
  // means to delete the file after opening.
  static const int O_DELETE = 0x10000;
	res = open(fname, flags & ~O_DELETE, mode);

  // Map EEXIST to special JVM_EEXIST, otherwise all errors are -1
  if (res < 0) {
    if (errno == EEXIST)
      res = JVM_EEXIST;
		else
			res = -1;
	}

  // Handle O_DELETE flag, if specified
  if (flags & O_DELETE)
    unlink(fname);
	leaveJVM(); 

	return res;
}

/*
 * Close a file descriptor. This function returns -1 on error, and 0
 * on success.
 *
 * fd        the file descriptor to close.
 */
jint JNICALL JVM_Close(jint fd) { enterJVM(); leaveJVM(); NYI(); }

/*
 * Read data from a file decriptor into a char array.
 *
 * fd        the file descriptor to read from.
 * buf       the buffer where to put the read data.
 * nbytes    the number of bytes to read.
 *
 * This function returns -1 on error, and 0 on success.
 */
jint JNICALL JVM_Read(jint fd, char *buf, jint nbytes) { enterJVM(); leaveJVM(); NYI(); }

/*
 * Write data from a char array to a file decriptor.
 *
 * fd        the file descriptor to read from.
 * buf       the buffer from which to fetch the data.
 * nbytes    the number of bytes to write.
 *
 * This function returns -1 on error, and 0 on success.
 */
jint JNICALL JVM_Write(jint fd, char *buf, jint nbytes) { enterJVM(); leaveJVM(); NYI(); }

/*
 * Returns the number of bytes available for reading from a given file
 * descriptor
 */
jint JNICALL JVM_Available(jint fd, jlong *pbytes) { enterJVM(); leaveJVM(); NYI(); }

/*
 * Move the file descriptor pointer from whence by offset.
 *
 * fd        the file descriptor to move.
 * offset    the number of bytes to move it by.
 * whence    the start from where to move it.
 *
 * This function returns the resulting pointer location.
 */
jlong JNICALL JVM_Lseek(jint fd, jlong offset, jint whence) { enterJVM(); leaveJVM(); NYI(); }

/*
 * Set the length of the file associated with the given descriptor to the given
 * length.  If the new length is longer than the current length then the file
 * is extended; the contents of the extended portion are not defined.  The
 * value of the file pointer is undefined after this procedure returns.
 */
jint JNICALL JVM_SetLength(jint fd, jlong length) { enterJVM(); leaveJVM(); NYI(); }

/*
 * Synchronize the file descriptor's in memory state with that of the
 * physical device.  Return of -1 is an error, 0 is OK.
 */
jint JNICALL JVM_Sync(jint fd) { enterJVM(); leaveJVM(); NYI(); }

/*
 * Networking library support
 */

jint JNICALL JVM_InitializeSocketLibrary(void) { enterJVM(); leaveJVM(); NYI(); }
jint JNICALL JVM_Socket(jint domain, jint type, jint protocol) { enterJVM(); leaveJVM(); NYI(); }
jint JNICALL JVM_SocketClose(jint fd) { enterJVM(); leaveJVM(); NYI(); }
jint JNICALL JVM_SocketShutdown(jint fd, jint howto) { enterJVM(); leaveJVM(); NYI(); }
jint JNICALL JVM_Recv(jint fd, char *buf, jint nBytes, jint flags) { enterJVM(); leaveJVM(); NYI(); }
jint JNICALL JVM_Send(jint fd, char *buf, jint nBytes, jint flags) { enterJVM(); leaveJVM(); NYI(); }
jint JNICALL JVM_Timeout(int fd, long timeout) { enterJVM(); leaveJVM(); NYI(); }
jint JNICALL JVM_Listen(jint fd, jint count) { enterJVM(); leaveJVM(); NYI(); }
jint JNICALL JVM_Connect(jint fd, struct sockaddr *him, jint len) { enterJVM(); leaveJVM(); NYI(); }
jint JNICALL JVM_Bind(jint fd, struct sockaddr *him, jint len) { enterJVM(); leaveJVM(); NYI(); }
jint JNICALL JVM_Accept(jint fd, struct sockaddr *him, jint *len) { enterJVM(); leaveJVM(); NYI(); }
jint JNICALL JVM_RecvFrom(jint fd, char *buf, int nBytes, int flags, struct sockaddr *from, int *fromlen) { enterJVM(); leaveJVM(); NYI(); }
jint JNICALL JVM_SendTo(jint fd, char *buf, int len, int flags, struct sockaddr *to, int tolen) { enterJVM(); leaveJVM(); NYI(); }
jint JNICALL JVM_SocketAvailable(jint fd, jint *result) { enterJVM(); leaveJVM(); NYI(); }
jint JNICALL JVM_GetSockName(jint fd, struct sockaddr *him, int *len) { enterJVM(); leaveJVM(); NYI(); }
jint JNICALL JVM_GetSockOpt(jint fd, int level, int optname, char *optval, int *optlen) { enterJVM(); leaveJVM(); NYI(); }
jint JNICALL JVM_SetSockOpt(jint fd, int level, int optname, const char *optval, int optlen) { enterJVM(); leaveJVM(); NYI(); }
int JNICALL JVM_GetHostName(char* name, int namelen) { enterJVM(); leaveJVM(); NYI(); }

/*
 * The standard printing functions supported by the Java VM. (Should they
 * be renamed to JVM_* in the future?
 */

/*
 * BE CAREFUL! The following functions do not implement the
 * full feature set of standard C printf formats.
 */
int jio_vsnprintf(char *str, size_t count, const char *fmt, va_list va) { 
	return vsnprintf(str, count, fmt, va);
}

int jio_snprintf(char *str, size_t count, const char *fmt, ...) { 
	va_list va;
	va_start(va, fmt);
	int res = jio_vsnprintf(str, count, fmt, va);
	va_end(va);
	return res;
}

int jio_fprintf(FILE *f, const char *fmt, ...) { 
	va_list va;
	va_start(va, fmt);
	int res = jio_vfprintf(f, fmt, va);
	va_end(va);
	return res;
}

int jio_vfprintf(FILE *f, const char *fmt, va_list va) { 
	return vfprintf(f, fmt, va);
}


void* JNICALL JVM_RawMonitorCreate(void) { 
	void* res;
	enterJVM();
	res = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
	if(!res)
		J3::outOfMemoryError();
	leaveJVM(); 
	return res;
}

void JNICALL JVM_RawMonitorDestroy(void *mon) { 
	enterJVM(); 
	free(mon);
	leaveJVM(); 
}

jint JNICALL JVM_RawMonitorEnter(void *mon) { 
	jint res;
	enterJVM(); 
	res = pthread_mutex_lock((pthread_mutex_t*)mon);
	leaveJVM(); 
	return res;
}

void JNICALL JVM_RawMonitorExit(void *mon) { 
	enterJVM(); 
	pthread_mutex_unlock((pthread_mutex_t*)mon);
	leaveJVM(); 
}

/*
 * java.lang.management support
 */
void* JNICALL JVM_GetManagement(jint version) { enterJVM(); leaveJVM(); NYI(); }

/*
 * com.sun.tools.attach.VirtualMachine support
 *
 * Initialize the agent properties with the properties maintained in the VM.
 */
jobject JNICALL JVM_InitAgentProperties(JNIEnv* env, jobject agent_props) { enterJVM(); leaveJVM(); NYI(); }

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
jobjectArray JNICALL JVM_GetEnclosingMethodInfo(JNIEnv* env, jclass ofClass) { enterJVM(); leaveJVM(); NYI(); }

/*
 * Returns an array of the threadStatus values representing the
 * given Java thread state.  Returns NULL if the VM version is
 * incompatible with the JDK or doesn't support the given
 * Java thread state.
 */
jintArray JNICALL JVM_GetThreadStateValues(JNIEnv* env, jint javaThreadState) { enterJVM(); leaveJVM(); NYI(); }

/*
 * Returns an array of the substate names representing the
 * given Java thread state.  Returns NULL if the VM version is
 * incompatible with the JDK or the VM doesn't support
 * the given Java thread state.
 * values must be the jintArray returned from JVM_GetThreadStateValues
 * and javaThreadState.
 */
jobjectArray JNICALL JVM_GetThreadStateNames(JNIEnv* env, jint javaThreadState, jintArray values) { enterJVM(); leaveJVM(); NYI(); }

/* =========================================================================
 * The following defines a private JVM interface that the JDK can query
 * for the JVM version and capabilities.  sun.misc.Version defines
 * the methods for getting the VM version and its capabilities.
 *
 * When a new bit is added, the following should be updated to provide
 * access to the new capability:
 *    HS:   JVM_GetVersionInfo and Abstract_VM_Version class
 *    SDK:  Version class
 *
 * Similary, a private JDK interface JDK_GetVersionInfo0 is defined for
 * JVM to query for the JDK version and capabilities.
 *
 * When a new bit is added, the following should be updated to provide
 * access to the new capability:
 *    HS:   JDK_Version class
 *    SDK:  JDK_GetVersionInfo0
 *
 * ==========================================================================
 */
void JNICALL JVM_GetVersionInfo(JNIEnv* env, jvm_version_info* info, size_t info_size) { 
	enterJVM(); 
  memset(info, 0, sizeof(info_size));

  info->jvm_version = (8 << 24) | (0 << 16) | 0;
  info->update_version = 0;
  info->special_update_version = 0;

  // when we add a new capability in the jvm_version_info struct, we should also
  // consider to expose this new capability in the sun.rt.jvmCapabilities jvmstat
  // counter defined in runtimeService.cpp.
  info->is_attach_supported = 0;//AttachListener::is_attach_supported();

	leaveJVM(); 
}
