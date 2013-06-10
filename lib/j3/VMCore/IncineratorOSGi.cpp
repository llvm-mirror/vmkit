
#include "Incinerator.h"
#include "ClasspathReflect.h"
#include "j3/jni.h"


using namespace j3;

/*
	This Java native method must be called by the framework in order to link bundles (given
	by bundle identifiers) to objects (thus, class loaders). This allows the VM to perform
	operations on bundles without actually having to know the precise structure of these.
*/
extern "C" void Java_j3_vm_OSGi_associateBundleClass(jlong bundleID, JavaObjectClass* classObject)
{
	llvm_gcroot(classObject, 0);

#if RESET_STALE_REFERENCES

	CommonClass* ccl = JavaObjectClass::getClass(classObject);
	Incinerator::get()->setBundleClassLoader(bundleID, ccl->classLoader);

#endif
}

extern "C" void Java_j3_vm_OSGi_notifyBundleUninstalled(jlong bundleID)
{
#if RESET_STALE_REFERENCES

	Incinerator::get()->setBundleClassLoader(bundleID, NULL);

#endif
}

extern "C" void Java_j3_vm_OSGi_setBundleStaleReferenceCorrected(jlong bundleID, jboolean corrected)
{
#if RESET_STALE_REFERENCES

	Incinerator::get()->setBundleStaleReferenceCorrected(bundleID, corrected);

#endif
}

extern "C" jboolean Java_j3_vm_OSGi_isBundleStaleReferenceCorrected(jlong bundleID)
{
#if RESET_STALE_REFERENCES

	return Incinerator::get()->isBundleStaleReferenceCorrected(bundleID);

#else
	return false;
#endif
}

extern "C" void Java_j3_vm_OSGi_dumpClassLoaderBundles()
{
#if RESET_STALE_REFERENCES

	Incinerator::get()->dumpClassLoaderBundles();

#endif
}

extern "C" void Java_j3_vm_OSGi_dumpReferencesToObject(jlong obj)
{
#if RESET_STALE_REFERENCES

	Incinerator::get()->dumpReferencesToObject(reinterpret_cast<JavaObject*>(obj));

#endif
}

extern "C" void Java_j3_vm_OSGi_forceStaleReferenceScanning()
{
#if RESET_STALE_REFERENCES

	Incinerator::get()->forceStaleReferenceScanning();

#endif
}
