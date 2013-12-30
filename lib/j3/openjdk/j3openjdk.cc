#include "vmkit/safepoint.h"
#include "j3/j3.h"
#include "j3/j3thread.h"
#include "j3/j3classloader.h"
#include "j3/j3class.h"
#include "j3/j3method.h"
#include "jvm.h"

using namespace j3;

#define enterJVM()
#define leaveJVM()

#define NYI() { J3Thread::get()->vm()->internalError(L"not yet implemented: '%s'", __PRETTY_FUNCTION__); }


/*************************************************************************
 PART 0
 ************************************************************************/

jint JNICALL JVM_GetInterfaceVersion(void) { enterJVM(); NYI(); leaveJVM(); }

/*************************************************************************
 PART 1: Functions for Native Libraries
 ************************************************************************/
/*
 * java.lang.Object
 */
jint JNICALL JVM_IHashCode(JNIEnv* env, jobject obj) { 
	enterJVM(); 
	uint32_t res = obj ? obj->hashCode() : 0;
	leaveJVM(); 
	return res;
}

void JNICALL JVM_MonitorWait(JNIEnv* env, jobject obj, jlong ms) { 
	enterJVM(); 
	obj->wait();
	leaveJVM(); 
}

void JNICALL JVM_MonitorNotify(JNIEnv* env, jobject obj) { enterJVM(); NYI(); leaveJVM(); }
void JNICALL JVM_MonitorNotifyAll(JNIEnv* env, jobject obj) { enterJVM(); NYI(); leaveJVM(); }
jobject JNICALL JVM_Clone(JNIEnv* env, jobject obj) { enterJVM(); NYI(); leaveJVM(); }

/*
 * java.lang.String
 */
jstring JNICALL JVM_InternString(JNIEnv* env, jstring str) { enterJVM(); NYI(); leaveJVM(); }

/*
 * java.lang.System
 */
jlong JNICALL JVM_CurrentTimeMillis(JNIEnv* env, jclass ignored) { enterJVM(); NYI(); leaveJVM(); }
jlong JNICALL JVM_NanoTime(JNIEnv* env, jclass ignored) { enterJVM(); NYI(); leaveJVM(); }
void JNICALL JVM_ArrayCopy(JNIEnv* env, jclass ignored, jobject src, jint src_pos, jobject dst, jint dst_pos, jint length) { 
	enterJVM(); 

	J3Type* srcType0 = src->vt()->type();
	J3Type* dstType0 = dst->vt()->type();

	if(!srcType0->isArrayClass() || !dstType0->isArrayClass() || !srcType0->isAssignableTo(dstType0))
		J3::arrayStoreException();

	if(src_pos >= src->arrayLength() || 
		 dst_pos >= dst->arrayLength() ||
		 (src_pos + length) > src->arrayLength() ||
		 (dst_pos + length) > dst->arrayLength())
		J3::arrayIndexOutOfBoundsException();

	uint32_t scale = srcType0->asArrayClass()->component()->getLogSize();

	src->rawArrayCopyTo(src_pos << scale, dst, dst_pos << scale, length << scale);

	leaveJVM(); 
}

jobject JNICALL JVM_InitProperties(JNIEnv* env, jobject p) { enterJVM(); NYI(); leaveJVM(); }

/*
 * java.io.File
 */
void JNICALL JVM_OnExit(void (*func)(void)) { enterJVM(); NYI(); leaveJVM(); }

/*
 * java.lang.Runtime
 */
void JNICALL JVM_Exit(jint code) { enterJVM(); NYI(); leaveJVM(); }
void JNICALL JVM_Halt(jint code) { enterJVM(); NYI(); leaveJVM(); }
void JNICALL JVM_GC(void) { enterJVM(); NYI(); leaveJVM(); }

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
jlong JNICALL JVM_MaxObjectInspectionAge(void) { enterJVM(); NYI(); leaveJVM(); }
void JNICALL JVM_TraceInstructions(jboolean on) { enterJVM(); NYI(); leaveJVM(); }
void JNICALL JVM_TraceMethodCalls(jboolean on) { enterJVM(); NYI(); leaveJVM(); }
jlong JNICALL JVM_TotalMemory(void) { enterJVM(); NYI(); leaveJVM(); }
jlong JNICALL JVM_FreeMemory(void) { enterJVM(); NYI(); leaveJVM(); }
jlong JNICALL JVM_MaxMemory(void) { enterJVM(); NYI(); leaveJVM(); }
jint JNICALL JVM_ActiveProcessorCount(void) { enterJVM(); NYI(); leaveJVM(); }
void * JNICALL JVM_LoadLibrary(const char *name) { enterJVM(); NYI(); leaveJVM(); }
void JNICALL JVM_UnloadLibrary(void * handle) { enterJVM(); NYI(); leaveJVM(); }
void * JNICALL JVM_FindLibraryEntry(void *handle, const char *name) { enterJVM(); NYI(); leaveJVM(); }
jboolean JNICALL JVM_IsSupportedJNIVersion(jint version) { enterJVM(); NYI(); leaveJVM(); }

/*
 * java.lang.Float and java.lang.Double
 */
jboolean JNICALL JVM_IsNaN(jdouble d) { enterJVM(); NYI(); leaveJVM(); }

/*
 * java.lang.Throwable
 */
void JNICALL JVM_FillInStackTrace(JNIEnv* env, jobject throwable) { enterJVM(); NYI(); leaveJVM(); }
jint JNICALL JVM_GetStackTraceDepth(JNIEnv* env, jobject throwable) { enterJVM(); NYI(); leaveJVM(); }
jobject JNICALL JVM_GetStackTraceElement(JNIEnv* env, jobject throwable, jint index) { enterJVM(); NYI(); leaveJVM(); }

/*
 * java.lang.Compiler
 */
void JNICALL JVM_InitializeCompiler (JNIEnv* env, jclass compCls) { enterJVM(); NYI(); leaveJVM(); }
jboolean JNICALL JVM_IsSilentCompiler(JNIEnv* env, jclass compCls) { enterJVM(); NYI(); leaveJVM(); }
jboolean JNICALL JVM_CompileClass(JNIEnv* env, jclass compCls, jclass cls) { enterJVM(); NYI(); leaveJVM(); }
jboolean JNICALL JVM_CompileClasses(JNIEnv* env, jclass cls, jstring jname) { enterJVM(); NYI(); leaveJVM(); }
jobject JNICALL JVM_CompilerCommand(JNIEnv* env, jclass compCls, jobject arg) { enterJVM(); NYI(); leaveJVM(); }
void JNICALL JVM_EnableCompiler(JNIEnv* env, jclass compCls) { enterJVM(); NYI(); leaveJVM(); }
void JNICALL JVM_DisableCompiler(JNIEnv* env, jclass compCls) { enterJVM(); NYI(); leaveJVM(); }

/*
 * java.lang.Thread
 */
void JNICALL JVM_StartThread(JNIEnv* env, jobject thread) { 
	enterJVM(); 
	J3Thread::start(thread);
	leaveJVM(); 
}

void JNICALL JVM_StopThread(JNIEnv* env, jobject thread, jobject exception) { enterJVM(); NYI(); leaveJVM(); }
jboolean JNICALL JVM_IsThreadAlive(JNIEnv* env, jobject thread) { 
	jboolean res;

	enterJVM(); 
	res = (jboolean)J3Thread::nativeThread(thread);
	leaveJVM(); 

	return res;
}

void JNICALL JVM_SuspendThread(JNIEnv* env, jobject thread) { enterJVM(); NYI(); leaveJVM(); }
void JNICALL JVM_ResumeThread(JNIEnv* env, jobject thread) { enterJVM(); NYI(); leaveJVM(); }

void JNICALL JVM_SetThreadPriority(JNIEnv* env, jobject thread, jint prio) { 
	enterJVM(); 
	// not yet implemented
	leaveJVM(); 
}

void JNICALL JVM_Yield(JNIEnv* env, jclass threadClass) { enterJVM(); NYI(); leaveJVM(); }
void JNICALL JVM_Sleep(JNIEnv* env, jclass threadClass, jlong millis) { enterJVM(); NYI(); leaveJVM(); }

jobject JNICALL JVM_CurrentThread(JNIEnv* env, jclass threadClass) { 
	jobject res;
	enterJVM(); 
	res = J3Thread::get()->javaThread();
	leaveJVM(); 
	return res;
}

jint JNICALL JVM_CountStackFrames(JNIEnv* env, jobject thread) { enterJVM(); NYI(); leaveJVM(); }
void JNICALL JVM_Interrupt(JNIEnv* env, jobject thread) { enterJVM(); NYI(); leaveJVM(); }
jboolean JNICALL JVM_IsInterrupted(JNIEnv* env, jobject thread, jboolean clearInterrupted) { enterJVM(); NYI(); leaveJVM(); }
jboolean JNICALL JVM_HoldsLock(JNIEnv* env, jclass threadClass, jobject obj) { enterJVM(); NYI(); leaveJVM(); }
void JNICALL JVM_DumpAllStacks(JNIEnv* env, jclass unused) { enterJVM(); NYI(); leaveJVM(); }
jobjectArray JNICALL JVM_GetAllThreads(JNIEnv* env, jclass dummy) { enterJVM(); NYI(); leaveJVM(); }
void JNICALL JVM_SetNativeThreadName(JNIEnv* env, jobject jthread, jstring name) { enterJVM(); NYI(); leaveJVM(); }

/* getStackTrace() and getAllStackTraces() method */
jobjectArray JNICALL JVM_DumpThreads(JNIEnv* env, jclass threadClass, jobjectArray threads) { enterJVM(); NYI(); leaveJVM(); }

/*
 * java.lang.SecurityManager
 */
jclass JNICALL JVM_CurrentLoadedClass(JNIEnv* env) { enterJVM(); NYI(); leaveJVM(); }
jobject JNICALL JVM_CurrentClassLoader(JNIEnv* env) { enterJVM(); NYI(); leaveJVM(); }
jobjectArray JNICALL JVM_GetClassContext(JNIEnv* env) { enterJVM(); NYI(); leaveJVM(); }
jint JNICALL JVM_ClassDepth(JNIEnv* env, jstring name) { enterJVM(); NYI(); leaveJVM(); }
jint JNICALL JVM_ClassLoaderDepth(JNIEnv* env) { enterJVM(); NYI(); leaveJVM(); }

/*
 * java.lang.Package
 */
jstring JNICALL JVM_GetSystemPackage(JNIEnv* env, jstring name) { enterJVM(); NYI(); leaveJVM(); }
jobjectArray JNICALL JVM_GetSystemPackages(JNIEnv* env) { enterJVM(); NYI(); leaveJVM(); }

/*
 * java.io.ObjectInputStream
 */
jobject JNICALL JVM_AllocateNewObject(JNIEnv* env, jobject obj, jclass currClass, jclass initClass) { enterJVM(); NYI(); leaveJVM(); }
jobject JNICALL JVM_AllocateNewArray(JNIEnv* env, jobject obj, jclass currClass, jint length) { enterJVM(); NYI(); leaveJVM(); }
jobject JNICALL JVM_LatestUserDefinedLoader(JNIEnv* env) { enterJVM(); NYI(); leaveJVM(); }

/*
 * This function has been deprecated and should not be considered
 * part of the specified JVM interface.
 */
jclass JNICALL
JVM_LoadClass0(JNIEnv* env, jobject obj, jclass currClass, jstring currClassName) { enterJVM(); NYI(); leaveJVM(); }

/*
 * java.lang.reflect.Array
 */
jint JNICALL JVM_GetArrayLength(JNIEnv* env, jobject arr) { enterJVM(); NYI(); leaveJVM(); }
jobject JNICALL JVM_GetArrayElement(JNIEnv* env, jobject arr, jint index) { enterJVM(); NYI(); leaveJVM(); }
jvalue JNICALL JVM_GetPrimitiveArrayElement(JNIEnv* env, jobject arr, jint index, jint wCode) { enterJVM(); NYI(); leaveJVM(); }
void JNICALL JVM_SetArrayElement(JNIEnv* env, jobject arr, jint index, jobject val) { enterJVM(); NYI(); leaveJVM(); }
void JNICALL JVM_SetPrimitiveArrayElement(JNIEnv* env, jobject arr, jint index, jvalue v, unsigned char vCode) { enterJVM(); NYI(); leaveJVM(); }
jobject JNICALL JVM_NewArray(JNIEnv* env, jclass eltClass, jint length) { enterJVM(); NYI(); leaveJVM(); }
jobject JNICALL JVM_NewMultiArray(JNIEnv* env, jclass eltClass, jintArray dim) { enterJVM(); NYI(); leaveJVM(); }

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
		J3::internalError(L"depth should be -1 while it is %d", depth);

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
		J3::internalError(L"unable to find caller class, what should I do?");

	res = caller->cl()->javaClass();

	leaveJVM(); 

	return res;
}


/*
 * Find primitive classes
 * utf: class name
 */
jclass JNICALL JVM_FindPrimitiveClass(JNIEnv* env, const char *utf) { 
	enterJVM(); 
	J3* vm = J3Thread::get()->vm();

	J3ClassLoader* loader = vm->initialClassLoader;
	vmkit::Names* names = vm->names();
	J3Class* res;

  if(!strcmp(utf, "boolean"))
		res = loader->loadClass(names->get(L"java/lang/Boolean"));
	else if(!strcmp(utf, "byte"))
		res = loader->loadClass(names->get(L"java/lang/Byte"));
	else if(!strcmp(utf, "char"))
		res = loader->loadClass(names->get(L"java/lang/Character"));
	else if(!strcmp(utf, "short"))
		res = loader->loadClass(names->get(L"java/lang/Short"));
	else if(!strcmp(utf, "int"))
		res = loader->loadClass(names->get(L"java/lang/Integer"));
	else if(!strcmp(utf, "long"))
		res = loader->loadClass(names->get(L"java/lang/Long"));
	else if(!strcmp(utf, "float"))
		res = loader->loadClass(names->get(L"java/lang/Float"));
	else if(!strcmp(utf, "double"))
		res = loader->loadClass(names->get(L"java/lang/Double"));
	else if(!strcmp(utf, "void"))
		res = loader->loadClass(names->get(L"java/lang/Void"));
	else
		J3::internalError(L"unsupported primitive: %s", utf);

	leaveJVM(); 
	return res->javaClass();
}

/*
 * Link the class
 */
void JNICALL JVM_ResolveClass(JNIEnv* env, jclass cls) { enterJVM(); NYI(); leaveJVM(); }

/*
 * Find a class from a boot class loader. Returns NULL if class not found.
 */
jclass JNICALL JVM_FindClassFromBootLoader(JNIEnv* env, const char *name) { enterJVM(); NYI(); leaveJVM(); }

/*
 * Find a class from a given class loader. Throw ClassNotFoundException
 * or NoClassDefFoundError depending on the value of the last
 * argument.
 */
jclass JNICALL JVM_FindClassFromClassLoader(JNIEnv* env, const char *name, jboolean init, jobject loader, jboolean throwError) { enterJVM(); NYI(); leaveJVM(); }

/*
 * Find a class from a given class.
 */
jclass JNICALL JVM_FindClassFromClass(JNIEnv* env, const char *name, jboolean init, jclass from) { enterJVM(); NYI(); leaveJVM(); }

/* Find a loaded class cached by the VM */
jclass JNICALL JVM_FindLoadedClass(JNIEnv* env, jobject loader, jstring name) { enterJVM(); NYI(); leaveJVM(); }

/* Define a class */
jclass JNICALL JVM_DefineClass(JNIEnv* env, const char *name, jobject loader, const jbyte *buf, jsize len, jobject pd) { enterJVM(); NYI(); leaveJVM(); }

/* Define a class with a source (added in JDK1.5) */
jclass JNICALL JVM_DefineClassWithSource(JNIEnv* env, const char *name, jobject loader, 
																									 const jbyte *buf, jsize len, jobject pd,
																									 const char *source) { enterJVM(); NYI(); leaveJVM(); }

/*
 * Reflection support functions
 */

jstring JNICALL JVM_GetClassName(JNIEnv* env, jclass cls) { enterJVM(); NYI(); leaveJVM(); }
jobjectArray JNICALL JVM_GetClassInterfaces(JNIEnv* env, jclass cls) { enterJVM(); NYI(); leaveJVM(); }
jobject JNICALL JVM_GetClassLoader(JNIEnv* env, jclass cls) { 
	enterJVM(); 
	jobject res = J3Class::nativeClass(cls)->loader()->javaClassLoader();
	leaveJVM(); 
	return res;
}

jboolean JNICALL JVM_IsInterface(JNIEnv* env, jclass cls) { enterJVM(); NYI(); leaveJVM(); }
jobjectArray JNICALL JVM_GetClassSigners(JNIEnv* env, jclass cls) { enterJVM(); NYI(); leaveJVM(); }
void JNICALL JVM_SetClassSigners(JNIEnv* env, jclass cls, jobjectArray signers) { enterJVM(); NYI(); leaveJVM(); }
jobject JNICALL JVM_GetProtectionDomain(JNIEnv* env, jclass cls) { enterJVM(); NYI(); leaveJVM(); }
jboolean JNICALL JVM_IsArrayClass(JNIEnv* env, jclass cls) { enterJVM(); NYI(); leaveJVM(); }
jboolean JNICALL JVM_IsPrimitiveClass(JNIEnv* env, jclass cls) { enterJVM(); NYI(); leaveJVM(); }
jclass JNICALL JVM_GetComponentType(JNIEnv* env, jclass cls) { enterJVM(); NYI(); leaveJVM(); }
jint JNICALL JVM_GetClassModifiers(JNIEnv* env, jclass cls) { enterJVM(); NYI(); leaveJVM(); }
jobjectArray JNICALL JVM_GetDeclaredClasses(JNIEnv* env, jclass ofClass) { enterJVM(); NYI(); leaveJVM(); }
jclass JNICALL JVM_GetDeclaringClass(JNIEnv* env, jclass ofClass) { enterJVM(); NYI(); leaveJVM(); }

/* Generics support (JDK 1.5) */
jstring JNICALL JVM_GetClassSignature(JNIEnv* env, jclass cls) { enterJVM(); NYI(); leaveJVM(); }

/* Annotations support (JDK 1.5) */
jbyteArray JNICALL JVM_GetClassAnnotations(JNIEnv* env, jclass cls) { enterJVM(); NYI(); leaveJVM(); }

/* Type use annotations support (JDK 1.8) */
jbyteArray JNICALL JVM_GetClassTypeAnnotations(JNIEnv* env, jclass cls) { enterJVM(); NYI(); leaveJVM(); }


/*
 * New (JDK 1.4) reflection implementation
 */

jobjectArray JNICALL JVM_GetClassDeclaredMethods(JNIEnv* env, jclass ofClass, jboolean publicOnly) { enterJVM(); NYI(); leaveJVM(); }
jobjectArray JNICALL JVM_GetClassDeclaredFields(JNIEnv* env, jclass ofClass, jboolean publicOnly) { enterJVM(); NYI(); leaveJVM(); }
jobjectArray JNICALL JVM_GetClassDeclaredConstructors(JNIEnv* env, jclass ofClass, jboolean publicOnly) { enterJVM(); NYI(); leaveJVM(); }

/* Differs from JVM_GetClassModifiers in treatment of inner classes.
   This returns the access flags for the class as specified in the
   class file rather than searching the InnerClasses attribute (if
   present) to find the source-level access flags. Only the values of
   the low 13 bits (i.e., a mask of 0x1FFF) are guaranteed to be
   valid. */
jint JNICALL JVM_GetClassAccessFlags(JNIEnv* env, jclass cls) { enterJVM(); NYI(); leaveJVM(); }

/* The following two reflection routines are still needed due to startup time issues */
/*
 * java.lang.reflect.Method
 */
jobject JNICALL JVM_InvokeMethod(JNIEnv* env, jobject method, jobject obj, jobjectArray args0) { enterJVM(); NYI(); leaveJVM(); }

/*
 * java.lang.reflect.Constructor
 */
jobject JNICALL JVM_NewInstanceFromConstructor(JNIEnv* env, jobject c, jobjectArray args0) { enterJVM(); NYI(); leaveJVM(); }

/*
 * Constant pool access; currently used to implement reflective access to annotations (JDK 1.5)
 */

jobject JNICALL JVM_GetClassConstantPool(JNIEnv* env, jclass cls) { enterJVM(); NYI(); leaveJVM(); }
jint JNICALL JVM_ConstantPoolGetSize(JNIEnv* env, jobject unused, jobject jcpool) { enterJVM(); NYI(); leaveJVM(); }
jclass JNICALL JVM_ConstantPoolGetClassAt(JNIEnv* env, jobject unused, jobject jcpool, jint index) { enterJVM(); NYI(); leaveJVM(); }
jclass JNICALL JVM_ConstantPoolGetClassAtIfLoaded(JNIEnv* env, jobject unused, jobject jcpool, jint index) { enterJVM(); NYI(); leaveJVM(); }
jobject JNICALL JVM_ConstantPoolGetMethodAt(JNIEnv* env, jobject unused, jobject jcpool, jint index) { enterJVM(); NYI(); leaveJVM(); }
jobject JNICALL JVM_ConstantPoolGetMethodAtIfLoaded(JNIEnv* env, jobject unused, jobject jcpool, jint index) { enterJVM(); NYI(); leaveJVM(); }
jobject JNICALL JVM_ConstantPoolGetFieldAt(JNIEnv* env, jobject unused, jobject jcpool, jint index) { enterJVM(); NYI(); leaveJVM(); }
jobject JNICALL JVM_ConstantPoolGetFieldAtIfLoaded(JNIEnv* env, jobject unused, jobject jcpool, jint index) { enterJVM(); NYI(); leaveJVM(); }
jobjectArray JNICALL JVM_ConstantPoolGetMemberRefInfoAt(JNIEnv* env, jobject unused, jobject jcpool, jint index) { enterJVM(); NYI(); leaveJVM(); }
jint JNICALL JVM_ConstantPoolGetIntAt(JNIEnv* env, jobject unused, jobject jcpool, jint index) { enterJVM(); NYI(); leaveJVM(); }
jlong JNICALL JVM_ConstantPoolGetLongAt(JNIEnv* env, jobject unused, jobject jcpool, jint index) { enterJVM(); NYI(); leaveJVM(); }
jfloat JNICALL JVM_ConstantPoolGetFloatAt(JNIEnv* env, jobject unused, jobject jcpool, jint index) { enterJVM(); NYI(); leaveJVM(); }
jdouble JNICALL JVM_ConstantPoolGetDoubleAt(JNIEnv* env, jobject unused, jobject jcpool, jint index) { enterJVM(); NYI(); leaveJVM(); }
jstring JNICALL JVM_ConstantPoolGetStringAt(JNIEnv* env, jobject unused, jobject jcpool, jint index) { enterJVM(); NYI(); leaveJVM(); }
jstring JNICALL JVM_ConstantPoolGetUTF8At(JNIEnv* env, jobject unused, jobject jcpool, jint index) { enterJVM(); NYI(); leaveJVM(); }

/*
 * Parameter reflection
 */

jobjectArray JNICALL JVM_GetMethodParameters(JNIEnv* env, jobject method) { enterJVM(); NYI(); leaveJVM(); }

/*
 * java.security.*
 */

jobject JNICALL JVM_DoPrivileged(JNIEnv* env, jclass cls, jobject action, jobject context, jboolean wrapException) { 
	enterJVM(); 

  // For now, we don't do anything special,
  // just call the requested 'run()' method...
	jmethodID mid = env->GetMethodID(env->GetObjectClass(action), "run", "()Ljava/lang/Object;");
	jobject res = env->CallObjectMethod(action, mid);
	leaveJVM(); 
	return res;
}

jobject JNICALL JVM_GetInheritedAccessControlContext(JNIEnv* env, jclass cls) { enterJVM(); NYI(); leaveJVM(); }
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

void * JNICALL JVM_RegisterSignal(jint sig, void *handler) { enterJVM(); NYI(); leaveJVM(); }
jboolean JNICALL JVM_RaiseSignal(jint sig) { enterJVM(); NYI(); leaveJVM(); }
jint JNICALL JVM_FindSignal(const char *name) { enterJVM(); NYI(); leaveJVM(); }

/*
 * Retrieve the assertion directives for the specified class.
 */
jboolean JNICALL JVM_DesiredAssertionStatus(JNIEnv* env, jclass unused, jclass cls) { 
	enterJVM(); 
	/* TODO: take into account the class name and its package (see j3options) */
	jboolean res = J3Thread::get()->vm()->options()->assertionsEnabled ? JNI_TRUE : JNI_FALSE;
	leaveJVM(); 
	return res;
}

/*
 * Retrieve the assertion directives from the VM.
 */
jobject JNICALL JVM_AssertionStatusDirectives(JNIEnv* env, jclass unused) { enterJVM(); NYI(); leaveJVM(); }

/*
 * java.util.concurrent.atomic.AtomicLong
 */
jboolean JNICALL JVM_SupportsCX8(void) { enterJVM(); NYI(); leaveJVM(); }

/*
 * Get the version number the JVM was built with
 */
jint JNICALL JVM_DTraceGetVersion(JNIEnv* env) { enterJVM(); NYI(); leaveJVM(); }

/*
 * Register new probe with given signature, return global handle
 *
 * The version passed in is the version that the library code was
 * built with.
 */
jlong JNICALL JVM_DTraceActivate(JNIEnv* env, jint version, jstring module_name, 
																					 jint providers_count, JVM_DTraceProvider* providers) { enterJVM(); NYI(); leaveJVM(); }

/*
 * Check JSDT probe
 */
jboolean JNICALL JVM_DTraceIsProbeEnabled(JNIEnv* env, jmethodID method) { enterJVM(); NYI(); leaveJVM(); }

/*
 * Destroy custom DOF
 */
void JNICALL JVM_DTraceDispose(JNIEnv* env, jlong activation_handle) { enterJVM(); NYI(); leaveJVM(); }

/*
 * Check to see if DTrace is supported by OS
 */
jboolean JNICALL JVM_DTraceIsSupported(JNIEnv* env) { enterJVM(); NYI(); leaveJVM(); }

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
const char * JNICALL JVM_GetClassNameUTF(JNIEnv* env, jclass cb) { enterJVM(); NYI(); leaveJVM(); }

/*
 * Returns the constant pool types in the buffer provided by "types."
 */
void JNICALL JVM_GetClassCPTypes(JNIEnv* env, jclass cb, unsigned char *types) { enterJVM(); NYI(); leaveJVM(); }

/*
 * Returns the number of Constant Pool entries.
 */
jint JNICALL JVM_GetClassCPEntriesCount(JNIEnv* env, jclass cb) { enterJVM(); NYI(); leaveJVM(); }

/*
 * Returns the number of *declared* fields or methods.
 */
jint JNICALL JVM_GetClassFieldsCount(JNIEnv* env, jclass cb) { enterJVM(); NYI(); leaveJVM(); }
jint JNICALL JVM_GetClassMethodsCount(JNIEnv* env, jclass cb) { enterJVM(); NYI(); leaveJVM(); }

/*
 * Returns the CP indexes of exceptions raised by a given method.
 * Places the result in the given buffer.
 *
 * The method is identified by method_index.
 */
void JNICALL JVM_GetMethodIxExceptionIndexes(JNIEnv* env, jclass cb, jint method_index, unsigned short *exceptions) { enterJVM(); NYI(); leaveJVM(); }
/*
 * Returns the number of exceptions raised by a given method.
 * The method is identified by method_index.
 */
jint JNICALL JVM_GetMethodIxExceptionsCount(JNIEnv* env, jclass cb, jint method_index) { enterJVM(); NYI(); leaveJVM(); }

/*
 * Returns the byte code sequence of a given method.
 * Places the result in the given buffer.
 *
 * The method is identified by method_index.
 */
void JNICALL JVM_GetMethodIxByteCode(JNIEnv* env, jclass cb, jint method_index, unsigned char *code) { enterJVM(); NYI(); leaveJVM(); }

/*
 * Returns the length of the byte code sequence of a given method.
 * The method is identified by method_index.
 */
jint JNICALL JVM_GetMethodIxByteCodeLength(JNIEnv* env, jclass cb, jint method_index) { enterJVM(); NYI(); leaveJVM(); }

/*
 * Returns the exception table entry at entry_index of a given method.
 * Places the result in the given buffer.
 *
 * The method is identified by method_index.
 */
void JNICALL JVM_GetMethodIxExceptionTableEntry(JNIEnv* env, jclass cb, jint method_index, jint entry_index,
																													JVM_ExceptionTableEntryType *entry) { enterJVM(); NYI(); leaveJVM(); }

/*
 * Returns the length of the exception table of a given method.
 * The method is identified by method_index.
 */
jint JNICALL JVM_GetMethodIxExceptionTableLength(JNIEnv* env, jclass cb, int index) { enterJVM(); NYI(); leaveJVM(); }

/*
 * Returns the modifiers of a given field.
 * The field is identified by field_index.
 */
jint JNICALL JVM_GetFieldIxModifiers(JNIEnv* env, jclass cb, int index) { enterJVM(); NYI(); leaveJVM(); }

/*
 * Returns the modifiers of a given method.
 * The method is identified by method_index.
 */
jint JNICALL JVM_GetMethodIxModifiers(JNIEnv* env, jclass cb, int index) { enterJVM(); NYI(); leaveJVM(); }

/*
 * Returns the number of local variables of a given method.
 * The method is identified by method_index.
 */
jint JNICALL JVM_GetMethodIxLocalsCount(JNIEnv* env, jclass cb, int index) { enterJVM(); NYI(); leaveJVM(); }

/*
 * Returns the number of arguments (including this pointer) of a given method.
 * The method is identified by method_index.
 */
jint JNICALL JVM_GetMethodIxArgsSize(JNIEnv* env, jclass cb, int index) { enterJVM(); NYI(); leaveJVM(); }

/*
 * Returns the maximum amount of stack (in words) used by a given method.
 * The method is identified by method_index.
 */
jint JNICALL JVM_GetMethodIxMaxStack(JNIEnv* env, jclass cb, int index) { enterJVM(); NYI(); leaveJVM(); }

/*
 * Is a given method a constructor.
 * The method is identified by method_index.
 */
jboolean JNICALL JVM_IsConstructorIx(JNIEnv* env, jclass cb, int index) { enterJVM(); NYI(); leaveJVM(); }

/*
 * Is the given method generated by the VM.
 * The method is identified by method_index.
 */
jboolean JNICALL JVM_IsVMGeneratedMethodIx(JNIEnv* env, jclass cb, int index) { enterJVM(); NYI(); leaveJVM(); }

/*
 * Returns the name of a given method in UTF format.
 * The result remains valid until JVM_ReleaseUTF is called.
 *
 * The caller must treat the string as a constant and not modify it
 * in any way.
 */
const char * JNICALL JVM_GetMethodIxNameUTF(JNIEnv* env, jclass cb, jint index) { enterJVM(); NYI(); leaveJVM(); }

/*
 * Returns the signature of a given method in UTF format.
 * The result remains valid until JVM_ReleaseUTF is called.
 *
 * The caller must treat the string as a constant and not modify it
 * in any way.
 */
const char * JNICALL JVM_GetMethodIxSignatureUTF(JNIEnv* env, jclass cb, jint index) { enterJVM(); NYI(); leaveJVM(); }

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
const char * JNICALL JVM_GetCPFieldNameUTF(JNIEnv* env, jclass cb, jint index) { enterJVM(); NYI(); leaveJVM(); }

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
const char * JNICALL JVM_GetCPMethodNameUTF(JNIEnv* env, jclass cb, jint index) { enterJVM(); NYI(); leaveJVM(); }

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
const char * JNICALL JVM_GetCPMethodSignatureUTF(JNIEnv* env, jclass cb, jint index) { enterJVM(); NYI(); leaveJVM(); }

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
const char * JNICALL JVM_GetCPFieldSignatureUTF(JNIEnv* env, jclass cb, jint index) { enterJVM(); NYI(); leaveJVM(); }

/*
 * Returns the class name refered to at a given constant pool index.
 *
 * The result is in UTF format and remains valid until JVM_ReleaseUTF
 * is called.
 *
 * The caller must treat the string as a constant and not modify it
 * in any way.
 */
const char * JNICALL JVM_GetCPClassNameUTF(JNIEnv* env, jclass cb, jint index) { enterJVM(); NYI(); leaveJVM(); }

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
const char * JNICALL JVM_GetCPFieldClassNameUTF(JNIEnv* env, jclass cb, jint index) { enterJVM(); NYI(); leaveJVM(); }

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
const char * JNICALL JVM_GetCPMethodClassNameUTF(JNIEnv* env, jclass cb, jint index) { enterJVM(); NYI(); leaveJVM(); }

/*
 * Returns the modifiers of a field in calledClass. The field is
 * referred to in class cb at constant pool entry index.
 *
 * The caller must treat the string as a constant and not modify it
 * in any way.
 *
 * Returns -1 if the field does not exist in calledClass.
 */
jint JNICALL JVM_GetCPFieldModifiers(JNIEnv* env, jclass cb, int index, jclass calledClass) { enterJVM(); NYI(); leaveJVM(); }

/*
 * Returns the modifiers of a method in calledClass. The method is
 * referred to in class cb at constant pool entry index.
 *
 * Returns -1 if the method does not exist in calledClass.
 */
jint JNICALL JVM_GetCPMethodModifiers(JNIEnv* env, jclass cb, int index, jclass calledClass) { enterJVM(); NYI(); leaveJVM(); }

/*
 * Releases the UTF string obtained from the VM.
 */
void JNICALL JVM_ReleaseUTF(const char *utf) { enterJVM(); NYI(); leaveJVM(); }

/*
 * Compare if two classes are in the same package.
 */
jboolean JNICALL JVM_IsSameClassPackage(JNIEnv* env, jclass class1, jclass class2) { enterJVM(); NYI(); leaveJVM(); }

/*************************************************************************
 PART 3: I/O and Network Support
 ************************************************************************/

/* Write a string into the given buffer, in the platform's local encoding,
 * that describes the most recent system-level error to occur in this thread.
 * Return the length of the string or zero if no error occurred.
 */
jint JNICALL JVM_GetLastErrorString(char *buf, int len) { enterJVM(); NYI(); leaveJVM(); }

/*
 * Convert a pathname into native format.  This function does syntactic
 * cleanup, such as removing redundant separator characters.  It modifies
 * the given pathname string in place.
 */
char * JNICALL JVM_NativePath(char *) { enterJVM(); NYI(); leaveJVM(); }

/*
 * Open a file descriptor. This function returns a negative error code
 * on error, and a non-negative integer that is the file descriptor on
 * success.
 */
jint JNICALL JVM_Open(const char *fname, jint flags, jint mode) { enterJVM(); NYI(); leaveJVM(); }

/*
 * Close a file descriptor. This function returns -1 on error, and 0
 * on success.
 *
 * fd        the file descriptor to close.
 */
jint JNICALL JVM_Close(jint fd) { enterJVM(); NYI(); leaveJVM(); }

/*
 * Read data from a file decriptor into a char array.
 *
 * fd        the file descriptor to read from.
 * buf       the buffer where to put the read data.
 * nbytes    the number of bytes to read.
 *
 * This function returns -1 on error, and 0 on success.
 */
jint JNICALL JVM_Read(jint fd, char *buf, jint nbytes) { enterJVM(); NYI(); leaveJVM(); }

/*
 * Write data from a char array to a file decriptor.
 *
 * fd        the file descriptor to read from.
 * buf       the buffer from which to fetch the data.
 * nbytes    the number of bytes to write.
 *
 * This function returns -1 on error, and 0 on success.
 */
jint JNICALL JVM_Write(jint fd, char *buf, jint nbytes) { enterJVM(); NYI(); leaveJVM(); }

/*
 * Returns the number of bytes available for reading from a given file
 * descriptor
 */
jint JNICALL JVM_Available(jint fd, jlong *pbytes) { enterJVM(); NYI(); leaveJVM(); }

/*
 * Move the file descriptor pointer from whence by offset.
 *
 * fd        the file descriptor to move.
 * offset    the number of bytes to move it by.
 * whence    the start from where to move it.
 *
 * This function returns the resulting pointer location.
 */
jlong JNICALL JVM_Lseek(jint fd, jlong offset, jint whence) { enterJVM(); NYI(); leaveJVM(); }

/*
 * Set the length of the file associated with the given descriptor to the given
 * length.  If the new length is longer than the current length then the file
 * is extended; the contents of the extended portion are not defined.  The
 * value of the file pointer is undefined after this procedure returns.
 */
jint JNICALL JVM_SetLength(jint fd, jlong length) { enterJVM(); NYI(); leaveJVM(); }

/*
 * Synchronize the file descriptor's in memory state with that of the
 * physical device.  Return of -1 is an error, 0 is OK.
 */
jint JNICALL JVM_Sync(jint fd) { enterJVM(); NYI(); leaveJVM(); }

/*
 * Networking library support
 */

jint JNICALL JVM_InitializeSocketLibrary(void) { enterJVM(); NYI(); leaveJVM(); }
jint JNICALL JVM_Socket(jint domain, jint type, jint protocol) { enterJVM(); NYI(); leaveJVM(); }
jint JNICALL JVM_SocketClose(jint fd) { enterJVM(); NYI(); leaveJVM(); }
jint JNICALL JVM_SocketShutdown(jint fd, jint howto) { enterJVM(); NYI(); leaveJVM(); }
jint JNICALL JVM_Recv(jint fd, char *buf, jint nBytes, jint flags) { enterJVM(); NYI(); leaveJVM(); }
jint JNICALL JVM_Send(jint fd, char *buf, jint nBytes, jint flags) { enterJVM(); NYI(); leaveJVM(); }
jint JNICALL JVM_Timeout(int fd, long timeout) { enterJVM(); NYI(); leaveJVM(); }
jint JNICALL JVM_Listen(jint fd, jint count) { enterJVM(); NYI(); leaveJVM(); }
jint JNICALL JVM_Connect(jint fd, struct sockaddr *him, jint len) { enterJVM(); NYI(); leaveJVM(); }
jint JNICALL JVM_Bind(jint fd, struct sockaddr *him, jint len) { enterJVM(); NYI(); leaveJVM(); }
jint JNICALL JVM_Accept(jint fd, struct sockaddr *him, jint *len) { enterJVM(); NYI(); leaveJVM(); }
jint JNICALL JVM_RecvFrom(jint fd, char *buf, int nBytes, int flags, struct sockaddr *from, int *fromlen) { enterJVM(); NYI(); leaveJVM(); }
jint JNICALL JVM_SendTo(jint fd, char *buf, int len, int flags, struct sockaddr *to, int tolen) { enterJVM(); NYI(); leaveJVM(); }
jint JNICALL JVM_SocketAvailable(jint fd, jint *result) { enterJVM(); NYI(); leaveJVM(); }
jint JNICALL JVM_GetSockName(jint fd, struct sockaddr *him, int *len) { enterJVM(); NYI(); leaveJVM(); }
jint JNICALL JVM_GetSockOpt(jint fd, int level, int optname, char *optval, int *optlen) { enterJVM(); NYI(); leaveJVM(); }
jint JNICALL JVM_SetSockOpt(jint fd, int level, int optname, const char *optval, int optlen) { enterJVM(); NYI(); leaveJVM(); }
int JNICALL JVM_GetHostName(char* name, int namelen) { enterJVM(); NYI(); leaveJVM(); }

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


void * JNICALL JVM_RawMonitorCreate(void) { enterJVM(); NYI(); leaveJVM(); }
void JNICALL JVM_RawMonitorDestroy(void *mon) { enterJVM(); NYI(); leaveJVM(); }
jint JNICALL JVM_RawMonitorEnter(void *mon) { enterJVM(); NYI(); leaveJVM(); }
void JNICALL JVM_RawMonitorExit(void *mon) { enterJVM(); NYI(); leaveJVM(); }

/*
 * java.lang.management support
 */
void* JNICALL JVM_GetManagement(jint version) { enterJVM(); NYI(); leaveJVM(); }

/*
 * com.sun.tools.attach.VirtualMachine support
 *
 * Initialize the agent properties with the properties maintained in the VM.
 */
jobject JNICALL JVM_InitAgentProperties(JNIEnv* env, jobject agent_props) { enterJVM(); NYI(); leaveJVM(); }

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
jobjectArray JNICALL JVM_GetEnclosingMethodInfo(JNIEnv* env, jclass ofClass) { enterJVM(); NYI(); leaveJVM(); }

/*
 * Returns an array of the threadStatus values representing the
 * given Java thread state.  Returns NULL if the VM version is
 * incompatible with the JDK or doesn't support the given
 * Java thread state.
 */
jintArray JNICALL JVM_GetThreadStateValues(JNIEnv* env, jint javaThreadState) { enterJVM(); NYI(); leaveJVM(); }

/*
 * Returns an array of the substate names representing the
 * given Java thread state.  Returns NULL if the VM version is
 * incompatible with the JDK or the VM doesn't support
 * the given Java thread state.
 * values must be the jintArray returned from JVM_GetThreadStateValues
 * and javaThreadState.
 */
jobjectArray JNICALL JVM_GetThreadStateNames(JNIEnv* env, jint javaThreadState, jintArray values) { enterJVM(); NYI(); leaveJVM(); }

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
