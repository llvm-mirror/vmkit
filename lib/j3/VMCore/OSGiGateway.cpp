
#include "OSGiGateway.h"
#include "ClasspathReflect.h"
#include "Jnjvm.h"
#include "Incinerator.h"

#include <iostream>
#include <algorithm>

#define DEBUG_VERBOSE_OSGI_GATEWAY	0

using namespace std;

namespace j3 {

OSGiGateway* OSGiGateway::get()
{
	vmkit::Thread* th = vmkit::Thread::get();
	assert(th && "Invalid current thread.");
	if (!th) return NULL;

	return &static_cast<Jnjvm*>(th->MyVM)->osgi_gateway;
}

void OSGiGateway::dumpClassLoaderBundles() const
{
	vmkit::LockGuard lg(lock);
	bundleClassLoadersType::const_iterator
		i = bundleClassLoaders.begin(), e = bundleClassLoaders.end();

	for (; i != e; ++i)
		cerr << "bundleID=" << i->first << " classLoader=" << i->second << endl;
}

JnjvmClassLoader * OSGiGateway::getBundleClassLoader(bundle_id_t bundleID) const
{
	if (bundleID == invalidBundleID) return NULL;

	vmkit::LockGuard lg(lock);

	bundleClassLoadersType::const_iterator
		i = bundleClassLoaders.find(bundleID), e = bundleClassLoaders.end();
	return (i == e) ? NULL : i->second;
}

OSGiGateway::bundle_id_t OSGiGateway::getClassLoaderBundleID(
	JnjvmClassLoader const * loader) const
{
	if (loader == NULL) return invalidBundleID;
	vmkit::LockGuard lg(lock);

	bundleClassLoadersType::const_iterator
		b = bundleClassLoaders.begin(),
		e = bundleClassLoaders.end();
	bundleClassLoadersType::const_iterator
		i = find_if(b, e, BundlesFinder(loader));

	return (i == e) ? invalidBundleID : i->first;
}

void OSGiGateway::setBundleClassLoader(bundle_id_t bundleID, JnjvmClassLoader* loader)
{
	if (bundleID == invalidBundleID) return;
	vmkit::LockGuard lg(lock);

	JnjvmClassLoader * previous_loader = bundleClassLoaders[bundleID];

	if (!loader) {
		// Unloaded bundle
		bundleClassLoaders.erase(bundleID);

#if DEBUG_VERBOSE_OSGI_GATEWAY
	cerr << "Bundle uninstalled: bundleID=" << bundleID
		<< " classLoader=" << previous_loader << endl;
#endif
	} else {
		// Installed/Updated bundle
		if (previous_loader == loader)
			return;	// Same class loader already associated with the bundle, do nothing

		// Associate the class loader with the bundle
		bundleClassLoaders[bundleID] = loader;

#if DEBUG_VERBOSE_OSGI_GATEWAY
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
}

}

extern "C" void Java_j3_vm_OSGi_setBundleClassLoader(
	jlong bundleID, j3::JavaObject* loaderObject)
{
	llvm_gcroot(loaderObject, 0);

	j3::Jnjvm* vm = j3::JavaThread::get()->getJVM();
	j3::JnjvmClassLoader* loader =
		j3::JnjvmClassLoader::getJnjvmLoaderFromJavaObject(loaderObject, vm);

	assert((loader != NULL) && "Invalid class loader");

#if RESET_STALE_REFERENCES
	j3::Incinerator::get()->
#else
	j3::OSGiGateway::get()->
#endif
		setBundleClassLoader(bundleID, loader);
}

extern "C" void Java_j3_vm_OSGi_notifyBundleUninstalled(jlong bundleID)
{
#if RESET_STALE_REFERENCES
	j3::Incinerator::get()->
#else
	j3::OSGiGateway::get()->
#endif
		setBundleClassLoader(bundleID, NULL);
}

extern "C" void Java_j3_vm_OSGi_dumpClassLoaderBundles()
{
#if RESET_STALE_REFERENCES
	j3::Incinerator::get()->
#else
	j3::OSGiGateway::get()->
#endif
		dumpClassLoaderBundles();
}
