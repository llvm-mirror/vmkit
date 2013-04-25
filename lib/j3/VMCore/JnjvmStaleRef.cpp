
#include "VmkitGC.h"
#include "Jnjvm.h"
#include "VMStaticInstance.h"

#include <sstream>

#if RESET_STALE_REFERENCES

#define DEBUG_VERBOSE_STALE_REF		1

using namespace std;

namespace j3 {

void Jnjvm::resetReferenceIfStale(const void* source, void** ref)
{
	JavaObject *src = NULL;
	llvm_gcroot(src, 0);

	if (!ref || !(*ref)) return;	// Invalid or null reference
	src = const_cast<JavaObject*>(reinterpret_cast<const JavaObject*>(source));
	JavaObject **objRef = reinterpret_cast<JavaObject**>(ref);

//	if (findReferencesToObject == *objRef)
//		foundReferencerObjects.push_back(src);

	if (!scanStaleReferences) return;	// Stale references scanning disabled

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
	if (!loader->isStale()) return;
/*
	cerr << "WARNING: Ignored stale reference ref=" << ref << " obj=" << **ref;
	if (source) cerr << " source=" << *source;
	cerr << endl;
*/
#endif
}

void Jnjvm::resetReferenceIfStale(const JavaObject *source, VMStaticInstance** ref)
{
	llvm_gcroot(source, 0);

	// Don't touch fake Java objects that exist only as bridges between the
	// Java object model and the C++ object model.

#if DEBUG_VERBOSE_STALE_REF

	JnjvmClassLoader* loader = (**ref).getOwningClass()->classLoader;
	if (!loader->isStale()) return;
/*
	cerr << "WARNING: Ignored stale reference ref=" << ref << " obj=" << **ref;
	if (source) cerr << " source=" << *source;
	cerr << endl;
*/
#endif
}

void Jnjvm::resetReferenceIfStale(const JavaObject *source, JavaObject** ref)
{
	llvm_gcroot(source, 0);

#if DEBUG_VERBOSE_STALE_REF

	if (source) {
		CommonClass* ccl = JavaObject::getClass(source);
		if (ccl->classLoader->isStale())
			cerr << "WARNING: Source object is stale source=" << *source << endl;
	}

#endif

	CommonClass* ccl = JavaObject::getClass(*ref);
	assert (ccl && "Object Class is not null.");

	if (!ccl->classLoader->isStale()) return;

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
