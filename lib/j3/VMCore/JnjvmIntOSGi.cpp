
#include <algorithm>
#include <iostream>

#include "VmkitGC.h"
#include "Jnjvm.h"
#include "ClasspathReflect.h"
#include "j3/jni.h"


using namespace std;

#if RESET_STALE_REFERENCES

#define DEBUG_VERBOSE_STALE_REF		1

namespace j3 {

void Jnjvm::setBundleStaleReferenceCorrected(int64_t bundleID, bool corrected)
{
	JnjvmClassLoader* loader = this->getBundleClassLoader(bundleID);
	if (!loader) {
		this->illegalArgumentException("Invalid bundle ID"); return;}

#if DEBUG_VERBOSE_STALE_REF
	cerr << "Stale references to bundleID=" << bundleID << " are ";
	if (corrected)
		cerr << "corrected." << endl;
	else
		cerr << "no more corrected." << endl;
#endif

	loader->setStaleReferencesCorrectionEnabled(corrected);
}

bool Jnjvm::isBundleStaleReferenceCorrected(int64_t bundleID)
{
	JnjvmClassLoader* loader = this->getBundleClassLoader(bundleID);
	if (!loader) {
		this->illegalArgumentException("Invalid bundle ID"); return false;}

	return loader->isStaleReferencesCorrectionEnabled();
}

void Jnjvm::notifyBundleUninstalled(int64_t bundleID)
{
	JnjvmClassLoader* loader = this->getBundleClassLoader(bundleID);
	if (!loader) return;

	if (!loader->isStaleReferencesCorrectionEnabled()) return;

	// Mark this class loader as a zombie.
	// Strong references to all its loaded classes will be reset in the next garbage collection.
	loader->markZombie(true);

#if DEBUG_VERBOSE_STALE_REF
	cerr << "Bundle uninstalled: bundleID=" << bundleID << endl;
#endif

	scanStaleReferences = true;		// Enable stale references scanning
	vmkit::Collector::collect();	// Start a garbage collection now
}

void Jnjvm::dumpClassLoaderBundles()
{
	for (bundleClassLoadersType::const_iterator i = bundleClassLoaders.begin(), e = bundleClassLoaders.end(); i != e; ++i) {
		cerr << "classLoader=" << i->second << "\tbundleID=" << i->first << endl;
	}
}

JnjvmClassLoader* Jnjvm::getBundleClassLoader(int64_t bundleID)
{
	if (bundleID == -1) return NULL;
	vmkit::LockGuard lg(bundleClassLoadersLock);

	bundleClassLoadersType::const_iterator i = bundleClassLoaders.find(bundleID);
	return (i == bundleClassLoaders.end()) ? NULL : i->second;
}

struct Jnjvm_getClassLoaderBundleID_finder {
	JnjvmClassLoader* loader;
	Jnjvm_getClassLoaderBundleID_finder(JnjvmClassLoader* l) : loader(l) {}
	bool operator() (const Jnjvm::bundleClassLoadersType::value_type& pair) {return pair.second == loader;}
};

int64_t Jnjvm::getClassLoaderBundleID(JnjvmClassLoader* loader)
{
	if (loader == NULL) return -1;
	vmkit::LockGuard lg(bundleClassLoadersLock);

	bundleClassLoadersType::const_iterator
		first = bundleClassLoaders.begin(),
		last = bundleClassLoaders.end(),
		i = std::find_if(first, last, Jnjvm_getClassLoaderBundleID_finder(loader));

	return (i == last) ? -1 : i->first;
}

// Link a bundle ID (OSGi world) to a class loader (Java world).
void Jnjvm::setBundleClassLoader(int64_t bundleID, JnjvmClassLoader* loader)
{
	if (bundleID == -1) return;
	vmkit::LockGuard lg(bundleClassLoadersLock);

	if (!loader)
		bundleClassLoaders.erase(bundleID);
	else
		bundleClassLoaders[bundleID] = loader;
}

}

#endif

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
	ccl->classLoader->setAssociatedBundleID(bundleID);

#endif
}

extern "C" void Java_j3_vm_OSGi_notifyBundleUninstalled(jlong bundleID)
{
#if RESET_STALE_REFERENCES

	Jnjvm* vm = JavaThread::get()->getJVM();
	vm->notifyBundleUninstalled(bundleID);

#endif
}

extern "C" void Java_j3_vm_OSGi_setBundleStaleReferenceCorrected(jlong bundleID, jboolean corrected)
{
#if RESET_STALE_REFERENCES

	Jnjvm* vm = JavaThread::get()->getJVM();
	vm->setBundleStaleReferenceCorrected(bundleID, corrected);

#endif
}

extern "C" jboolean Java_j3_vm_OSGi_isBundleStaleReferenceCorrected(jlong bundleID)
{
#if RESET_STALE_REFERENCES

	Jnjvm* vm = JavaThread::get()->getJVM();
	return vm->isBundleStaleReferenceCorrected(bundleID);

#else
	return false;
#endif
}

extern "C" void Java_j3_vm_OSGi_dumpClassLoaderBundles()
{
#if RESET_STALE_REFERENCES

	Jnjvm* vm = JavaThread::get()->getJVM();
	vm->dumpClassLoaderBundles();

#endif
}
