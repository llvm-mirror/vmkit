
#include <algorithm>

#include "VmkitGC.h"
#include "Jnjvm.h"
#include "ClasspathReflect.h"
#include "j3/jni.h"

namespace j3 {

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


using namespace j3;

extern "C" void Java_j3_vm_OSGi_associateBundleClass(jlong bundleID, JavaObjectClass* classObject)
{
	llvm_gcroot(classObject, 0);

	CommonClass* ccl = JavaObjectClass::getClass(classObject);
	ccl->classLoader->setAssociatedBundleID(bundleID);
}

extern "C" void Java_j3_vm_OSGi_resetReferencesToBundle(jlong bundleID)
{
	Jnjvm* vm = JavaThread::get()->getJVM();
	vm->resetReferencesToBundle(bundleID);
}
