
#include "VmkitGC.h"
#include "Jnjvm.h"
#include "VMStaticInstance.h"

#include <sstream>

#if RESET_STALE_REFERENCES

#define DEBUG_VERBOSE_STALE_REF		1

using namespace std;

namespace j3 {

void Jnjvm::resetReferencesToBundle(int64_t bundleID)
{
	JnjvmClassLoader* loader = this->getBundleClassLoader(bundleID);
	assert(loader && "No class loader is associated with the bundle");

	// Mark this class loader as a zombie. Its references will be reset in the next
	// garbage collection phase.
	loader->markZombie();

	vmkit::Collector::collect();	// Start a garbage collection now
}

void Jnjvm::resetReferenceIfStale(const void* source, void** ref)
{
	JavaObject *src = NULL;
	llvm_gcroot(src, 0);

	if (!scanStaleReferences) return;	// Stale references scanning disabled
	if (!ref || !(*ref)) return;	// Invalid or null reference

	src = const_cast<JavaObject*>(reinterpret_cast<const JavaObject*>(source));
	JavaObject **objRef = reinterpret_cast<JavaObject**>(ref);

	// Check the type of Java object. Some Java objects are not real object, but
	// are bridges between Java and the VM objects.
	if (VMClassLoader::isVMClassLoader(*objRef))
		resetReferenceIfStale(src, reinterpret_cast<VMClassLoader**>(ref));
	else if (VMStaticInstance::isVMStaticInstance(*objRef))
		resetReferenceIfStale(src, reinterpret_cast<VMStaticInstance**>(ref));
	else
		resetReferenceIfStale(src, objRef);
}

void Jnjvm::resetReferenceIfStale(const JavaObject *source, VMClassLoader** ref)
{
	llvm_gcroot(source, 0);

	// Don't touch fake Java objects that exist only as bridges between the
	// Java object model and the C++ object model.

#if DEBUG_VERBOSE_STALE_REF

	JnjvmClassLoader* loader = (**ref).getClassLoader();
	if (!loader->isZombie()) return;

	cerr << "WARNING: Ignored stale reference ref=" << ref << " obj=" << **ref;
	if (source) cerr << " source=" << *source;
	cerr << endl;

#endif
}

void Jnjvm::resetReferenceIfStale(const JavaObject *source, VMStaticInstance** ref)
{
	llvm_gcroot(source, 0);

	// Don't touch fake Java objects that exist only as bridges between the
	// Java object model and the C++ object model.

#if DEBUG_VERBOSE_STALE_REF

	JnjvmClassLoader* loader = (**ref).getOwningClass()->classLoader;
	if (!loader->isZombie()) return;

	cerr << "WARNING: Ignored stale reference ref=" << ref << " obj=" << **ref;
	if (source) cerr << " source=" << *source;
	cerr << endl;

#endif
}

void Jnjvm::resetReferenceIfStale(const JavaObject *source, JavaObject** ref)
{
	llvm_gcroot(source, 0);

#if DEBUG_VERBOSE_STALE_REF

	if (source) {
		CommonClass* ccl = JavaObject::getClass(source);
		if (ccl->classLoader->isZombie())
			cerr << "WARNING: Source object is stale source=" << *source << endl;
	}

#endif

	CommonClass* ccl = JavaObject::getClass(*ref);
	assert (ccl && "Object Class is not null.");

	if (!ccl->classLoader->isZombie()) return;

#if DEBUG_VERBOSE_STALE_REF

	cerr << "Resetting ref=" << ref << " obj=" << **ref;
	if (source) cerr << " source=" << *source;
	cerr << endl;

#endif

	if (JavaThread* ownerThread = (JavaThread*)vmkit::ThinLock::getOwner(*ref, this->lockSystem)) {
		if (vmkit::FatLock* lock = vmkit::ThinLock::getFatLock(*ref, this->lockSystem))
			lock->markAssociatedObjectAsDead();

		// Notify all threads waiting on this object
		ownerThread->lockingThread.notifyAll(*ref, this->lockSystem, ownerThread);

		// Release this object
		while (vmkit::ThinLock::getOwner(*ref, this->lockSystem) == ownerThread)
			vmkit::ThinLock::release(*ref, this->lockSystem, ownerThread);
	}

	*ref = NULL;	// Reset the reference
}

}

#endif
