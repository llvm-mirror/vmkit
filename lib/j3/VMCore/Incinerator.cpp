
#include "VmkitGC.h"
#include "Jnjvm.h"
#include "VMStaticInstance.h"
#include "JavaReferenceQueue.h"

#include <sstream>
#include <algorithm>

#if RESET_STALE_REFERENCES

#define DEBUG_VERBOSE_STALE_REF		0

using namespace std;

namespace j3 {

Incinerator::Incinerator(Jnjvm* j3vm) :
	scanRef(Incinerator::scanRef_Disabled),
	scanStackRef(Incinerator::scanStackRef_Disabled),
	vm(j3vm),
	needsStaleRefRescan(false),
	findReferencesToObject(NULL) {}

Incinerator::~Incinerator()
{
}

Incinerator* Incinerator::get()
{
	vmkit::Thread* th = vmkit::Thread::get();
	assert(th && "Invalid current thread.");
	if (!th) return NULL;

	return &static_cast<Jnjvm*>(th->MyVM)->incinerator;
}

void Incinerator::dumpClassLoaderBundles() const
{
	vmkit::LockGuard lg(bundleClassLoadersLock);
	bundleClassLoadersType::const_iterator
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

void Incinerator::setBundleStaleReferenceCorrected(int64_t bundleID, bool corrected)
{
	JnjvmClassLoader * loader = this->getBundleClassLoader(bundleID);
	if (!loader) {
		vm->illegalArgumentException("Invalid bundle ID"); return;}

#if DEBUG_VERBOSE_STALE_REF
	cerr << "Stale references to bundleID=" << bundleID << " are ";
	if (corrected)
		cerr << "corrected." << endl;
	else
		cerr << "no more corrected." << endl;
#endif

	loader->setStaleReferencesCorrectionEnabled(corrected);
}

bool Incinerator::isBundleStaleReferenceCorrected(int64_t bundleID) const
{
	JnjvmClassLoader const* loader = this->getBundleClassLoader(bundleID);
	if (!loader) {
		vm->illegalArgumentException("Invalid bundle ID"); return false;}

	return loader->isStaleReferencesCorrectionEnabled();
}

JnjvmClassLoader * Incinerator::getBundleClassLoader(int64_t bundleID) const
{
	if (bundleID == -1) return NULL;

	vmkit::LockGuard lg(bundleClassLoadersLock);

	bundleClassLoadersType::const_iterator
		i = bundleClassLoaders.find(bundleID), e = bundleClassLoaders.end();
	return (i == e) ? NULL : i->second;
}

bool Incinerator::InstalledBundles_finder::operator() (
	const bundleClassLoadersType::value_type& pair) const
{
	return (loader == pair.second);
}

bool Incinerator::UninstalledBundles_finder::operator() (
	const staleBundleClassLoadersType::value_type& pair) const
{
	staleBundleClassLoadersType::mapped_type::const_iterator
		b = pair.second.begin(), e = pair.second.end();
	staleBundleClassLoadersType::mapped_type::const_iterator
		i = find(b, e, loader);
	return (i != e);
}

int64_t Incinerator::getClassLoaderBundleID(JnjvmClassLoader const * loader) const
{
	if (loader == NULL) return -1;
	vmkit::LockGuard lg(bundleClassLoadersLock);

	bundleClassLoadersType::const_iterator
		b = bundleClassLoaders.begin(),
		e = bundleClassLoaders.end();
	bundleClassLoadersType::const_iterator
		i = find_if(b, e, InstalledBundles_finder(loader));

	if (i != e) return i->first;

	// Look up in stale bundles list
	staleBundleClassLoadersType::const_iterator
		sb = staleBundleClassLoaders.begin(),
		se = staleBundleClassLoaders.end();
	staleBundleClassLoadersType::const_iterator
		si = find_if(sb, se, UninstalledBundles_finder(loader));

	return (si == se) ? -1 : si->first;
}

// Link a bundle ID (OSGi world) to a class loader (Java world).
void Incinerator::setBundleClassLoader(int64_t bundleID, JnjvmClassLoader* loader)
{
	if (bundleID == -1) return;
	vmkit::LockGuard lg(bundleClassLoadersLock);

	JnjvmClassLoader * previous_loader = bundleClassLoaders[bundleID];

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

		// Propagate the stale reference correction setting to the new
		// class loader if a previous one exists.
		if (previous_loader != NULL) {
			loader->setStaleReferencesCorrectionEnabled(
				previous_loader->isStaleReferencesCorrectionEnabled());
		}

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

	if (!previous_loader)
		return;	// No previous class loader, nothing to clean up

	// Mark the previous class loader as stale
	staleBundleClassLoaders[bundleID].push_front(previous_loader);
	previous_loader->markStale(true);

	// Enable stale references scanning
	setScanningInclusive();
}

IncineratorManagedClassLoader::~IncineratorManagedClassLoader()
{
	Incinerator& incinerator = JavaThread::get()->getJVM()->incinerator;
	incinerator.classLoaderUnloaded(static_cast<JnjvmClassLoader const *>(this));
}

void Incinerator::classLoaderUnloaded(JnjvmClassLoader const * loader)
{
	int64_t bundleID = getClassLoaderBundleID(loader);
	if (bundleID == -1) {
#if DEBUG_VERBOSE_STALE_REF
		cerr << "Class loader unloaded: " << loader << endl;
#endif
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

void Incinerator::dumpReferencesToObject(JavaObject* object) const
{
	findReferencesToObject = object;
	vmkit::Collector::collect();
}

void Incinerator::forceStaleReferenceScanning()
{
	setScanningInclusive();
	vmkit::Collector::collect();
}

bool Incinerator::isScanningEnabled()
{
	return scanRef != Incinerator::scanRef_Disabled;
}

void Incinerator::setScanningDisabled()
{
	scanRef = Incinerator::scanRef_Disabled;
	scanStackRef = Incinerator::scanStackRef_Disabled;

#if DEBUG_VERBOSE_STALE_REF
	cerr << "Looking for stale references done." << endl;
#endif
}

void Incinerator::setScanningInclusive()
{
	scanRef = Incinerator::scanRef_Inclusive;
	scanStackRef = Incinerator::scanStackRef_Inclusive;

#if DEBUG_VERBOSE_STALE_REF
	cerr << "Looking for stale references..." << endl;
#endif
}

void Incinerator::setScanningExclusive()
{
	scanRef = Incinerator::scanRef_Exclusive;
	scanStackRef = Incinerator::scanStackRef_Exclusive;

#if DEBUG_VERBOSE_STALE_REF
	cerr << "Excluding stale references..." << endl;
#endif
}

void Incinerator::beforeCollection()
{
	if (findReferencesToObject != NULL)
		foundReferencerObjects.clear();

#if DEBUG_VERBOSE_STALE_REF
	if (needsStaleRefRescan) {
		cerr << "Some stale references were previously ignored due to"
				" finalizable stale objects."
				" Scanning for stale references enabled." << endl;
	}
#endif

	if (!needsStaleRefRescan && !isScanningEnabled()) return;

	needsStaleRefRescan = false;
	setScanningInclusive();
}

void Incinerator::markingFinalizersDone()
{
	if (!isScanningEnabled()) return;
	setScanningExclusive();
}

void Incinerator::collectorPhaseComplete()
{
	for (StaleRefListType::const_iterator
		i = staleRefList.begin(), e = staleRefList.end(); i != e; ++i)
	{
		eliminateStaleRef(i->second, i->first);
	}

	staleRefList.clear();
}

void Incinerator::afterCollection()
{
	findReferencesToObject = NULL;

	if (!isScanningEnabled()) return;

#if DEBUG_VERBOSE_STALE_REF
	if (needsStaleRefRescan) {
		cerr << "Some stale references were ignored due to finalizable"
				" stale objects. Another garbage collection is needed." << endl;
	}
#endif

	setScanningDisabled();
}

bool Incinerator::isStaleObject(const JavaObject* obj)
{
	llvm_gcroot(obj, 0);
	if (!obj || isVMObject(obj)) return false;

	CommonClass* ccl = JavaObject::getClass(obj);
	assert (ccl && "Object Class is not null.");

	JnjvmClassLoader* loader = ccl->classLoader;
	return loader->isStale() && loader->isStaleReferencesCorrectionEnabled();
}

bool Incinerator::isVMObject(const JavaObject* obj)
{
	llvm_gcroot(obj, 0);

	// Check the type of Java object.
	// Some Java objects are not real object, but are bridges between Java
	// and the VM C++ objects.
	return (obj != NULL) && (
		VMClassLoader::isVMClassLoader(obj)
		|| VMStaticInstance::isVMStaticInstance(obj));
}

bool Incinerator::scanRef_Disabled(Incinerator&, const JavaObject* source, JavaObject** ref)
{
	llvm_gcroot(source, 0);
#if DEBUG_VERBOSE_STALE_REF
	if (!ref || !(*ref)) return true;
	if (JavaObject::getClass(*ref)->name->elements[0] == 'i')
		cout << ref << " ==> " << **ref << endl;
#endif
	return true;
}

bool Incinerator::scanStackRef_Disabled(Incinerator&, const JavaMethod* method, JavaObject** ref)
{
#if DEBUG_VERBOSE_STALE_REF
	if (!ref || !(*ref)) return true;
	if (method && method->classDef->name->elements[0] == 'i')
		cout << *method << ": " << ref << " ==> " << **ref << endl;
#endif
	return true;
}

bool Incinerator::scanRef_Inclusive(Incinerator& incinerator, const JavaObject* source, JavaObject** ref)
{
	llvm_gcroot(source, 0);

	if (!ref || !isStaleObject(*ref)) return true;

#if DEBUG_VERBOSE_STALE_REF
	cerr << "Stale ref: " << ref << "==>" << **ref << endl;
#endif

	// Queue the stale reference to be eliminated.
	incinerator.staleRefList[ref] = source;

	// Skip this reference and don't trace it.
	return false;
}

bool Incinerator::scanStackRef_Inclusive(Incinerator& incinerator, const JavaMethod* method, JavaObject** ref)
{
#if DEBUG_VERBOSE_STALE_REF
	if (!ref || !(*ref)) return true;
	if (method && method->classDef->name->elements[0] == 'i')
		cout << *method << ": " << ref << " ==> " << **ref << endl;
#endif

	return Incinerator::scanRef_Inclusive(incinerator, NULL, ref);
}

bool Incinerator::scanRef_Exclusive(Incinerator& incinerator, const JavaObject* source, JavaObject** ref)
{
	llvm_gcroot(source, 0);

	// Do not eliminate any stale references traced via finalizable objects.
	if ((ref != NULL) && isStaleObject(*ref)) {
#if DEBUG_VERBOSE_STALE_REF
		size_t removed =
#endif

		incinerator.staleRefList.erase(ref);
		incinerator.needsStaleRefRescan = true;

#if DEBUG_VERBOSE_STALE_REF
		if (!removed)
			cerr << "Stale ref (ignored): " << ref << "==>" << **ref << endl;
		else
			cerr << "Excluded stale ref: " << ref << "==>" << **ref << endl;
#endif
	}
	return true;	// Trace this reference.
}

bool Incinerator::scanStackRef_Exclusive(Incinerator& incinerator, const JavaMethod* method, JavaObject** ref)
{
#if DEBUG_VERBOSE_STALE_REF
	if (!ref || !(*ref)) return true;
	if (method && method->classDef->name->elements[0] == 'i')
		cout << *method << ": " << ref << " ==> " << **ref << endl;
#endif

	return Incinerator::scanRef_Exclusive(incinerator, NULL, ref);
}

void Incinerator::eliminateStaleRef(const JavaObject *source, JavaObject** ref)
{
	CommonClass* ccl = JavaObject::getClass(*ref);
	assert (ccl && "Object Class is not null.");

#if DEBUG_VERBOSE_STALE_REF
	cerr << "Resetting stale ref=" << ref << " obj=" << **ref << " classLoader=" << ccl->classLoader;
	if (source) cerr << " source=" << *source;
	cerr << endl;
#endif

	if (!ccl->classLoader->isStaleReferencesCorrectionEnabled()) {
#if DEBUG_VERBOSE_STALE_REF
		cerr << "WARNING: Ignoring stale ref=" << ref << " obj=" << **ref << " classLoader=" << ccl->classLoader;
		if (source) cerr << " source=" << *source;
		cerr << endl;
#endif
		return;
	}

	if (JavaThread* ownerThread = (JavaThread*)vmkit::ThinLock::getOwner(*ref, vm->lockSystem)) {
		if (vmkit::FatLock* lock = vmkit::ThinLock::getFatLock(*ref, vm->lockSystem))
			lock->markAssociatedObjectAsDead();

		// Notify all threads waiting on this object
		ownerThread->lockingThread.notifyAll(*ref, vm->lockSystem, ownerThread);

		// Release this object
		while (vmkit::ThinLock::getOwner(*ref, vm->lockSystem) == ownerThread)
			vmkit::ThinLock::release(*ref, vm->lockSystem, ownerThread);
	}

	*ref = NULL;	// Reset the reference
}

}

#endif
