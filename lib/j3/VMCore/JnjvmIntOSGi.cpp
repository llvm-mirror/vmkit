
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

void Jnjvm::setBundleStaleReferenceCorrected(int64_t bundleID, bool corrected)
{
	classLoadersListType class_loaders;
	this->getBundleClassLoaders(bundleID, class_loaders);
	if (class_loaders.size() == 0) {
		this->illegalArgumentException("Invalid bundle ID"); return;}

#if DEBUG_VERBOSE_STALE_REF
	cerr << "Stale references to bundleID=" << bundleID << " are ";
	if (corrected)
		cerr << "corrected." << endl;
	else
		cerr << "no more corrected." << endl;
#endif

	classLoadersListType::iterator
		i = class_loaders.begin(),
		e = class_loaders.end();
	for (; i != e; ++i)
		(**i).setStaleReferencesCorrectionEnabled(corrected);
}

bool Jnjvm::isBundleStaleReferenceCorrected(int64_t bundleID)
{
	classLoadersListType class_loaders;
	this->getBundleClassLoaders(bundleID, class_loaders);
	if (class_loaders.size() == 0) {
		this->illegalArgumentException("Invalid bundle ID"); return false;}

	classLoadersListType::const_iterator
		i = class_loaders.begin(),
		e = class_loaders.end();
	bool corrected = true;
	for (; corrected && (i != e); ++i)
		corrected &= (**i).isStaleReferencesCorrectionEnabled();

	return corrected;
}

void Jnjvm::notifyBundleUninstalled(int64_t bundleID)
{
	classLoadersListType class_loaders;
	this->getBundleClassLoaders(bundleID, class_loaders);
	if (class_loaders.size() == 0) return;

	// Mark all bundle's class loaders as a zombies.
	// Strong references to all its loaded classes will be reset in the next garbage collection.
	classLoadersListType::iterator
		i = class_loaders.begin(),
		e = class_loaders.end();
	for (; i != e; ++i) {
		if ((**i).isStaleReferencesCorrectionEnabled())
			(**i).markZombie(true);
	}

#if DEBUG_VERBOSE_STALE_REF
	cerr << "Bundle uninstalled: bundleID=" << bundleID << endl;
#endif

	scanStaleReferences = true;		// Enable stale references scanning
	vmkit::Collector::collect();	// Start a garbage collection now
}

void Jnjvm::notifyBundleUpdated(int64_t bundleID)
{
	classLoadersListType class_loaders;
	this->getBundleClassLoaders(bundleID, class_loaders);
	if (class_loaders.size() == 0) return;

#if DEBUG_VERBOSE_STALE_REF
	cerr << "Bundle updated: bundleID=" << bundleID
		<< ", previousClassLoaders:";
#endif

	// Mark previous bundle's class loaders as a zombies.
	classLoadersListType::iterator
		i = class_loaders.begin(),
		e = class_loaders.end();

	// The first class loader is the current one, the rest are previous
	// class loaders that should be scanned for stale references.
	++i;

	for (; i != e; ++i) {
		if ((**i).isStaleReferencesCorrectionEnabled())
			(**i).markZombie(true);

#if DEBUG_VERBOSE_STALE_REF
		cerr << " " << (*i);
#endif
	}

#if DEBUG_VERBOSE_STALE_REF
	cerr << endl;
#endif

	scanStaleReferences = true;		// Enable stale references scanning
	vmkit::Collector::collect();	// Start a garbage collection now
}

void Jnjvm::dumpClassLoaders(const classLoadersListType& class_loaders)
{
	classLoadersListType::const_iterator
		j = class_loaders.begin(),
		je = class_loaders.end();

	for (; j != je; ++j)
		cerr << " " << (*j);
}

void Jnjvm::dumpBundleClassLoaders(int64_t bundleID)
{
	cerr << "bundleID=" << bundleID << " classLoaders:";
	dumpClassLoaders(bundleClassLoaders[bundleID]);
	cerr << endl;
}

void Jnjvm::dumpClassLoaderBundles()
{
	bundleClassLoadersType::const_iterator
		i = bundleClassLoaders.begin(),
		ie = bundleClassLoaders.end();
	classLoadersListType::const_iterator j, je;

	cerr << "bundleID=" << i->first << " classLoaders:";
	for (; i != ie; ++i)
		dumpClassLoaders(i->second);
	cerr << endl;
}
/*
ArrayLong* Jnjvm::getReferencesToObject(const JavaObject* obj)
{
	if (!obj) return NULL;

	findReferencesToObject = obj;
	vmkit::Collector::collect();

	size_t count = foundReferencerObjects.size();
	ArrayLong* r = static_cast<ArrayLong*>(upcalls->ArrayOfLong->doNew(count, this));
	if (!r) {this->outOfMemoryError(); return r;}

	ArrayLong::ElementType* elements = ArrayLong::getElements(r);
	for (size_t i=0; i < count; ++i) {
		elements[i] = reinterpret_cast<ArrayLong::ElementType>(foundReferencerObjects[i]);
	}

	return r;
}
*/
void Jnjvm::getBundleClassLoaders(
	int64_t bundleID, classLoadersListType& class_loaders)
{
	class_loaders.clear();
	if (bundleID == -1) return;

	vmkit::LockGuard lg(bundleClassLoadersLock);

	bundleClassLoadersType::const_iterator
		i = bundleClassLoaders.find(bundleID),
		e = bundleClassLoaders.end();

	if (i == e) return;		// Bundle not found
	class_loaders = i->second;
}

struct Jnjvm_getClassLoaderBundleID_finder {
	JnjvmClassLoader* loader;
	Jnjvm_getClassLoaderBundleID_finder(JnjvmClassLoader* l) : loader(l) {}

	bool operator() (const Jnjvm::bundleClassLoadersType::value_type& pair)
	{
		const Jnjvm::classLoadersListType& class_loaders = pair.second;
		Jnjvm::classLoadersListType::const_iterator
			b = class_loaders.begin(), e = class_loaders.end();

		return std::find(b, e, loader) != e;
	}
};

int64_t Jnjvm::getClassLoaderBundleID(JnjvmClassLoader* loader)
{
	if (loader == NULL) return -1;
	vmkit::LockGuard lg(bundleClassLoadersLock);

	bundleClassLoadersType::const_iterator
		first = bundleClassLoaders.begin(),
		last = bundleClassLoaders.end();

	bundleClassLoadersType::const_iterator
		i = std::find_if(first, last, Jnjvm_getClassLoaderBundleID_finder(loader));

	return (i == last) ? -1 : i->first;
}

// Link a bundle ID (OSGi world) to a class loader (Java world).
void Jnjvm::addBundleClassLoader(int64_t bundleID, JnjvmClassLoader* loader)
{
	if (bundleID == -1) return;
	vmkit::LockGuard lg(bundleClassLoadersLock);

	classLoadersListType& class_loaders = bundleClassLoaders[bundleID];

	classLoadersListType::const_iterator
		b = class_loaders.begin(),
		e = class_loaders.end();
	if (std::find(b, e, loader) != e) return;

	class_loaders.push_front(loader);
	dumpBundleClassLoaders(bundleID);
}

void Jnjvm::removeClassLoaderFromBundles(JnjvmClassLoader* loader)
{
	if (loader == NULL) return;
	vmkit::LockGuard lg(bundleClassLoadersLock);

	bundleClassLoadersType::iterator
		first = bundleClassLoaders.begin(),
		last = bundleClassLoaders.end();

	bundleClassLoadersType::iterator
		i = std::find_if(first, last, Jnjvm_getClassLoaderBundleID_finder(loader));

	if (i == last) return;

	i->second.remove(loader);

	dumpBundleClassLoaders(i->first);

	if (i->second.size() == 0)
		bundleClassLoaders.erase(i->first);
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

extern "C" void Java_j3_vm_OSGi_notifyBundleUpdated(jlong bundleID)
{
#if RESET_STALE_REFERENCES

	Jnjvm* vm = JavaThread::get()->getJVM();
	vm->notifyBundleUpdated(bundleID);

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
/*
extern "C" ArrayLong* Java_j3_vm_OSGi_getReferencesToObject(jlong objectPointer)
{
#if RESET_STALE_REFERENCES

	Jnjvm* vm = JavaThread::get()->getJVM();
	return vm->getReferencesToObject(reinterpret_cast<const JavaObject*>((intptr_t)objectPointer));

#else
	return NULL;
#endif
}
*/
extern "C" JavaString* Java_j3_vm_OSGi_dumpObject(jlong objectPointer)
{
#if RESET_STALE_REFERENCES

	if (!objectPointer) return NULL;
	const JavaObject& obj = *reinterpret_cast<const JavaObject*>((intptr_t)objectPointer);
	std::ostringstream ss;
	ss << obj;

	Jnjvm* vm = JavaThread::get()->getJVM();
	return vm->asciizToStr(ss.str().c_str());
	
#else
	return NULL;
#endif
}
