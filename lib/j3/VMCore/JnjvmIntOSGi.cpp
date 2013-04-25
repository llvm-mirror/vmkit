
#include <algorithm>
#include <iostream>
#include <sstream>

#include "VmkitGC.h"
#include "Jnjvm.h"
#include "ClasspathReflect.h"
#include "JavaUpcalls.h"
#include "j3/jni.h"
#include "JavaArray.h"


using namespace std;

#if RESET_STALE_REFERENCES

#define DEBUG_VERBOSE_STALE_REF		1

namespace j3 {

void Jnjvm::dumpClassLoaderBundles()
{
	vmkit::LockGuard lg(bundleClassLoadersLock);
	Jnjvm::bundleClassLoadersType::const_iterator
		i = bundleClassLoaders.begin(), e = bundleClassLoaders.end();

	for (; i != e; ++i)
		cerr << "bundleID=" << i->first << " classLoader=" << i->second << endl;

	staleBundleClassLoadersType::const_iterator
		si = staleBundleClassLoaders.begin(), se = staleBundleClassLoaders.end();
	staleBundleClassLoadersType::mapped_type::const_iterator li, le;
	for (; si != se; ++si) {
		cerr << "stale bundleID=" << si->first << " classLoaders={";
		le = si->second.end();
		li = si->second.begin();
		for (; li != le; ++li) cerr << " " << *li;
		cerr << "}" << endl;
	}
}

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

JnjvmClassLoader* Jnjvm::getBundleClassLoader(int64_t bundleID)
{
	if (bundleID == -1) return NULL;

	vmkit::LockGuard lg(bundleClassLoadersLock);

	bundleClassLoadersType::const_iterator
		i = bundleClassLoaders.find(bundleID), e = bundleClassLoaders.end();
	return (i == e) ? NULL : i->second;
}

struct Jnjvm_getClassLoaderBundleID_InstalledBundles_finder {
	JnjvmClassLoader* loader;
	Jnjvm_getClassLoaderBundleID_InstalledBundles_finder(
		JnjvmClassLoader* l) : loader(l) {}

	bool operator() (const Jnjvm::bundleClassLoadersType::value_type& pair) {
		return (loader == pair.second);
	}
};

struct Jnjvm_getClassLoaderBundleID_UninstalledBundles_finder {
	JnjvmClassLoader* loader;
	Jnjvm_getClassLoaderBundleID_UninstalledBundles_finder(
		JnjvmClassLoader* l) : loader(l) {}

	bool operator() (const Jnjvm::staleBundleClassLoadersType::value_type& pair) {
		Jnjvm::staleBundleClassLoadersType::mapped_type::const_iterator
			b = pair.second.begin(), e = pair.second.end();
		Jnjvm::staleBundleClassLoadersType::mapped_type::const_iterator
			i = std::find(b, e, loader);
		return (i != e);
	}
};

int64_t Jnjvm::getClassLoaderBundleID(JnjvmClassLoader* loader)
{
	if (loader == NULL) return -1;
	vmkit::LockGuard lg(bundleClassLoadersLock);

	bundleClassLoadersType::const_iterator
		b = bundleClassLoaders.begin(),
		e = bundleClassLoaders.end();
	bundleClassLoadersType::const_iterator
		i = std::find_if(b, e,
			Jnjvm_getClassLoaderBundleID_InstalledBundles_finder(loader));

	if (i != e) return i->first;

	// Look up in stale bundles list
	staleBundleClassLoadersType::const_iterator
		sb = staleBundleClassLoaders.begin(),
		se = staleBundleClassLoaders.end();
	staleBundleClassLoadersType::const_iterator
		si = std::find_if(sb, se,
			Jnjvm_getClassLoaderBundleID_UninstalledBundles_finder(loader));

	return (si == se) ? -1 : si->first;
}

// Link a bundle ID (OSGi world) to a class loader (Java world).
void Jnjvm::setBundleClassLoader(int64_t bundleID, JnjvmClassLoader* loader)
{
	if (bundleID == -1) return;
	vmkit::LockGuard lg(bundleClassLoadersLock);

	JnjvmClassLoader* previous_loader = bundleClassLoaders[bundleID];

	if (!loader) {
		// Unloaded bundle
		bundleClassLoaders.erase(bundleID);

#if DEBUG_VERBOSE_STALE_REF
	cerr << "Bundle uninstalled: bundleID=" << bundleID
		<< " classLoader=" << previous_loader << endl;
#endif
	} else {
		// Installed/Updated bundle
		if (previous_loader == loader)
			return;	// Same class loader already associated with the bundle, do nothing

		// Associate the class loader with the bundle
		bundleClassLoaders[bundleID] = loader;

#if DEBUG_VERBOSE_STALE_REF
		if (!previous_loader) {
			cerr << "Bundle installed: bundleID=" << bundleID
				<< " classLoader=" << loader << endl;
		} else {
			cerr << "Bundle updated: bundleID=" << bundleID
				<< " classLoader=" << loader
				<< " previousClassLoader=" << previous_loader << endl;
		}
#endif
	}

	if (previous_loader != NULL) {
		// Mark the previous class loader as stale
		if (previous_loader->isStaleReferencesCorrectionEnabled()) {
			previous_loader->markStale(true);
			staleBundleClassLoaders[bundleID].push_front(previous_loader);
		}

		// Enable stale references scanning
		scanStaleReferences = true;

		// Start a garbage collection now
		//vmkit::Collector::collect();
	}
}

void Jnjvm::classLoaderUnloaded(JnjvmClassLoader* loader)
{
	int64_t bundleID = getClassLoaderBundleID(loader);
	if (bundleID == -1) {
		cerr << "Class loader unloaded: " << loader << endl;
		return;
	}

	staleBundleClassLoaders[bundleID].remove(loader);
	if (staleBundleClassLoaders[bundleID].size() == 0)
		staleBundleClassLoaders.erase(bundleID);

#if DEBUG_VERBOSE_STALE_REF
	cerr << "Class loader unloaded: " << loader
		<< " bundleID=" << bundleID << endl;
#endif
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

	Jnjvm* vm = JavaThread::get()->getJVM();
	CommonClass* ccl = JavaObjectClass::getClass(classObject);
	vm->setBundleClassLoader(bundleID, ccl->classLoader);

#endif
}

extern "C" void Java_j3_vm_OSGi_notifyBundleUninstalled(jlong bundleID)
{
#if RESET_STALE_REFERENCES

	Jnjvm* vm = JavaThread::get()->getJVM();
	vm->setBundleClassLoader(bundleID, NULL);

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
