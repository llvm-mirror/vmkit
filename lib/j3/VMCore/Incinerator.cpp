
#include "Incinerator.h"

#include "VmkitGC.h"
#include "Jnjvm.h"
#include "JnjvmClassLoader.h"
#include "JavaThread.h"
#include "JavaClass.h"
#include "VMStaticInstance.h"

#include <algorithm>

#if RESET_STALE_REFERENCES

using namespace std;

namespace j3 {

Incinerator::Incinerator(Jnjvm* j3vm) :
	scanRef(Incinerator::scanRef_Disabled),
	scanStackRef(Incinerator::scanStackRef_Disabled),
	vm(j3vm),
	needsStaleRefRescan(false)
#if DEBUG_OBJECT_REF_DUMPING
	, findReferencesToObject(NULL)
#endif
{
}

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
	vm->osgi_gateway.dumpClassLoaderBundles();

	vmkit::LockGuard lg(lock);

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

void Incinerator::setBundleStaleReferenceCorrected(OSGiGateway::bundle_id_t bundleID, bool corrected)
{
	JnjvmClassLoader * loader = vm->osgi_gateway.getBundleClassLoader(bundleID);
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

bool Incinerator::isBundleStaleReferenceCorrected(OSGiGateway::bundle_id_t bundleID) const
{
	JnjvmClassLoader const* loader = vm->osgi_gateway.getBundleClassLoader(bundleID);
	if (!loader) {
		vm->illegalArgumentException("Invalid bundle ID"); return false;}

	return loader->isStaleReferencesCorrectionEnabled();
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

OSGiGateway::bundle_id_t Incinerator::getClassLoaderBundleID(JnjvmClassLoader const * loader) const
{
	if (loader == NULL) return OSGiGateway::invalidBundleID;

	OSGiGateway::bundle_id_t r = vm->osgi_gateway.getClassLoaderBundleID(loader);
	if (r != OSGiGateway::invalidBundleID) return r;

	vmkit::LockGuard lg(lock);

	// Look up in stale bundles list
	staleBundleClassLoadersType::const_iterator
		sb = staleBundleClassLoaders.begin(),
		se = staleBundleClassLoaders.end();
	staleBundleClassLoadersType::const_iterator
		si = find_if(sb, se, UninstalledBundles_finder(loader));

	return (si == se) ? OSGiGateway::invalidBundleID : si->first;
}

// Link a bundle ID (OSGi world) to a class loader (Java world).
void Incinerator::setBundleClassLoader(OSGiGateway::bundle_id_t bundleID, JnjvmClassLoader* loader)
{
	if (bundleID == OSGiGateway::invalidBundleID) return;

	JnjvmClassLoader * previous_loader =
		vm->osgi_gateway.getBundleClassLoader(bundleID);

	bool bundleUpdated =
		(previous_loader != NULL) && (loader != NULL) &&
		(previous_loader != loader);

	vmkit::LockGuard lg(lock);

	if (bundleUpdated) {
		// Propagate the stale reference correction setting to the new
		// class loader if a previous one exists.
		loader->setStaleReferencesCorrectionEnabled(
			previous_loader->isStaleReferencesCorrectionEnabled());
	}

	// Either bundle uninstalled, or bundle updated with a different class loader
	if (bundleUpdated || ((previous_loader != NULL) && !loader)) {
		// Mark the previous class loader as stale
		staleBundleClassLoaders[bundleID].push_front(previous_loader);
		previous_loader->markStale(true);

		// Enable stale references scanning
		setScanningInclusive();
	}

	vm->osgi_gateway.setBundleClassLoader(bundleID, loader);
}

IncineratorManagedClassLoader::~IncineratorManagedClassLoader()
{
	Incinerator& incinerator = JavaThread::get()->getJVM()->incinerator;
	incinerator.classLoaderUnloaded(static_cast<JnjvmClassLoader const *>(this));
}

void Incinerator::classLoaderUnloaded(JnjvmClassLoader const * loader)
{
	OSGiGateway::bundle_id_t bundleID = getClassLoaderBundleID(loader);
	if (bundleID == OSGiGateway::invalidBundleID) {
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

#if DEBUG_OBJECT_REF_DUMPING

void Incinerator::dumpReferencesToObject(const JavaObject* object)
{
	pendingRefObject.clear();
	pendingRefObject.push_back(object);
	seenObjects.clear();

	const JavaObject* obj = NULL;
	do {
		obj = pendingRefObject.back();
		pendingRefObject.pop_back();
		seenObjects.insert(obj);

		cerr << "dumpReferencesToObject: " << *obj << ", toString=";
		obj->dumpToString();

		findReferencesToObject = obj;
		foundReferencerObjects.clear();
		setScanningInclusive();
		vmkit::Collector::collect();
	} while (!pendingRefObject.empty());
}

#endif

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
	cerr << "References scan done." << endl;
#endif
}

void Incinerator::setScanningInclusive()
{
	scanRef = Incinerator::scanRef_Inclusive;
	scanStackRef = Incinerator::scanStackRef_Inclusive;

#if DEBUG_VERBOSE_STALE_REF
	cerr << "References scan enabled." << endl;
#endif
}

void Incinerator::setScanningExclusive()
{
#if DEBUG_EXCLUDE_FINALIZABLE_STALE_OBJECTS
	scanRef = Incinerator::scanRef_Exclusive;
	scanStackRef = Incinerator::scanStackRef_Exclusive;

#if DEBUG_VERBOSE_STALE_REF
	cerr << "Excluding stale references..." << endl;
#endif

#endif
}

void Incinerator::beforeCollection()
{
#if DEBUG_OBJECT_REF_DUMPING
	if (findReferencesToObject != NULL)
		foundReferencerObjects.clear();
#endif

#if DEBUG_VERBOSE_STALE_REF
	if (needsStaleRefRescan) {
		cerr << "Some stale references were previously ignored due to"
				" finalizable stale objects." << endl;
	}
#endif

	if (!needsStaleRefRescan && !isScanningEnabled()) {
#if DEBUG_OBJECT_REF_DUMPING
		if (!findReferencesToObject)
#endif
			return;
	}

	needsStaleRefRescan = false;
	setScanningInclusive();
}

void Incinerator::markingFinalizersDone()
{
#if DEBUG_OBJECT_REF_DUMPING
	if (findReferencesToObject != NULL) return;
#endif

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
#if DEBUG_OBJECT_REF_DUMPING
	findReferencesToObject = NULL;
#endif

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

	if (!obj) return false;

	const JnjvmClassLoader* loader = NULL;
	if (VMClassLoader::isVMClassLoader(obj)) {
		loader = ((const VMClassLoader*)obj)->getClassLoader();
	} else if (VMStaticInstance::isVMStaticInstance(obj)) {
		loader = ((const VMStaticInstance*)obj)->getOwningClass()->classLoader;
	} else {
		CommonClass* ccl = JavaObject::getClass(obj);
		assert (ccl && "Object Class is not null.");

		loader = ccl->classLoader;
	}

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

	if (!ref) return true;

#if DEBUG_OBJECT_REF_DUMPING
	if (incinerator.findReferencesToObject != NULL) {
		if (incinerator.findReferencesToObject == *ref) {
			if (!source) {
				cout << *ref << " <-- (unknown)" << endl;
			} else {
				if (incinerator.foundReferencerObjects.insert(source).second) {
					cout << *ref << " <-- " << *source << endl;

					if (incinerator.seenObjects.find(source) == incinerator.seenObjects.end()) {
						// The source ref was not scanned before
						std::deque<const JavaObject*>::iterator
							b = incinerator.pendingRefObject.begin(),
							e = incinerator.pendingRefObject.end();

						if (std::find(b, e, source) == e) {
							// The source ref is not currently pending for scan
							incinerator.pendingRefObject.push_front(source);
						}
					}
				}
			}
		}
		return true;
	}
#endif

	if (!isStaleObject(*ref)) return true;

#if DEBUG_VERBOSE_STALE_REF
	if (isVMObject(*ref))
		cerr << "Stale ref (VMObject): ";
	else
		cerr << "Stale ref: ";

	cerr << ref << " ==> " << **ref;
	if (source) cerr << " source=" << *source;
	cerr << endl;
#endif

	if (isVMObject(*ref))
		return true;

	// Queue the stale reference to be eliminated.
	incinerator.staleRefList[ref] = source;

	// Skip this reference and don't trace it.
	return false;
}

bool Incinerator::scanStackRef_Inclusive(Incinerator& incinerator, const JavaMethod* method, JavaObject** ref)
{
#if DEBUG_OBJECT_REF_DUMPING
	if (ref != NULL && incinerator.findReferencesToObject != NULL) {
		if (incinerator.findReferencesToObject == *ref) {
			cout << "On stack of method=" << *method->name << " ,class=" << *method->classDef << endl;
		}
	}
#endif

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
			cerr << "Stale ref (ignored): " << ref << " ==> " << **ref;
		else
			cerr << "Excluded stale ref: " << ref << " ==> " << **ref;

		if (source) cerr << " source=" << *source;
		cerr << endl;
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
	if (isVMObject(*ref))
		return;

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

extern "C" void Java_j3_vm_OSGi_setBundleStaleReferenceCorrected(jlong bundleID, jboolean corrected)
{
#if RESET_STALE_REFERENCES
	j3::Incinerator::get()->setBundleStaleReferenceCorrected(bundleID, corrected);
#endif
}

extern "C" jboolean Java_j3_vm_OSGi_isBundleStaleReferenceCorrected(jlong bundleID)
{
#if RESET_STALE_REFERENCES
	return j3::Incinerator::get()->isBundleStaleReferenceCorrected(bundleID);
#else
	return false;
#endif
}

#if DEBUG_OBJECT_REF_DUMPING

extern "C" void Java_j3_vm_OSGi_dumpReferencesToObject(jlong obj)
{
#if RESET_STALE_REFERENCES
	j3::Incinerator::get()->dumpReferencesToObject(reinterpret_cast<const j3::JavaObject*>(obj));
#endif
}

#endif

extern "C" void Java_j3_vm_OSGi_forceStaleReferenceScanning()
{
#if RESET_STALE_REFERENCES
	j3::Incinerator::get()->forceStaleReferenceScanning();
#endif
}

#endif
