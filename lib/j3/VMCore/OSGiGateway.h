
#ifndef OSGIGATEWAY_H_
#define OSGIGATEWAY_H_

#include "vmkit/Locks.h"
#include "j3/jni.h"

#include <map>
#include <stdint.h>

namespace j3 {

class JnjvmClassLoader;
class JavaObjectClass;
class JavaObject;

class OSGiGateway
{
public:
	typedef int64_t	bundle_id_t;

	static const bundle_id_t invalidBundleID = (bundle_id_t)-1;

protected:
	typedef std::map<bundle_id_t, j3::JnjvmClassLoader *> bundleClassLoadersType;

	class BundlesFinder {
		j3::JnjvmClassLoader const * loader;
	public:
		BundlesFinder(j3::JnjvmClassLoader const * l) : loader(l) {}
		bool operator() (const bundleClassLoadersType::value_type& pair) const
			{return (loader == pair.second);}
	};

	mutable vmkit::LockRecursive lock;
	bundleClassLoadersType bundleClassLoaders;

public:
	OSGiGateway() {}
	virtual ~OSGiGateway() {}

	static OSGiGateway* get();

	j3::JnjvmClassLoader * getBundleClassLoader(bundle_id_t bundleID) const;
	bundle_id_t getClassLoaderBundleID(j3::JnjvmClassLoader const * loader) const;
	void setBundleClassLoader(bundle_id_t bundleID, j3::JnjvmClassLoader* loader);

	void dumpClassLoaderBundles() const;
};

}

extern "C" void Java_j3_vm_OSGi_setBundleClassLoader(
	jlong bundleID, j3::JavaObject* loaderObject);
extern "C" void Java_j3_vm_OSGi_notifyBundleUninstalled(jlong bundleID);
extern "C" void Java_j3_vm_OSGi_dumpClassLoaderBundles();

#endif
