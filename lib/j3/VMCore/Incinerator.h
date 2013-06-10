
#ifndef INCINERATOR_H
#define INCINERATOR_H

#include "vmkit/Locks.h"

#include <list>
#include <map>
#include <set>
#include <vector>
#include <stdint.h>

namespace j3 {

class Jnjvm;
class JnjvmClassLoader;
class VMClassLoader;
class VMStaticInstance;
class JavaObject;
class JavaMethod;


class Incinerator
{
public:
	Incinerator(j3::Jnjvm* j3vm);
	virtual ~Incinerator();

	void setBundleStaleReferenceCorrected(int64_t bundleID, bool corrected);
	bool isBundleStaleReferenceCorrected(int64_t bundleID) const;
	void dumpClassLoaderBundles() const;
	void dumpReferencesToObject(JavaObject* object) const;
	void forceStaleReferenceScanning();

	JnjvmClassLoader * getBundleClassLoader(int64_t bundleID) const;
	int64_t getClassLoaderBundleID(JnjvmClassLoader  const * loader) const;
	void setBundleClassLoader(int64_t bundleID, JnjvmClassLoader* loader);

	void beforeCollection();
	void   markingFinalizersDone();
	void   collectorPhaseComplete();
	void afterCollection();
	void classLoaderUnloaded(JnjvmClassLoader const * loader);

	static Incinerator* get();

	typedef bool (*scanRefFunc)(
		Incinerator& incinerator, const JavaObject* source, JavaObject** ref);
	typedef bool (*scanStackRefFunc)(
		Incinerator& incinerator, const JavaMethod* method, JavaObject** ref);

	volatile scanRefFunc scanRef;
	volatile scanStackRefFunc scanStackRef;

protected:
	typedef std::map<int64_t, std::list<JnjvmClassLoader const *> >
		staleBundleClassLoadersType;
	typedef std::map<int64_t, JnjvmClassLoader *> bundleClassLoadersType;
	typedef std::map<JavaObject**, const JavaObject*>
		StaleRefListType;	// (ref, source)

	inline static bool isStaleObject(const JavaObject* obj);
	inline static bool isVMObject(const JavaObject* obj);
	void eliminateStaleRef(const JavaObject *source, JavaObject** ref);

	bool isScanningEnabled();
	void setScanningDisabled();
	void setScanningInclusive();
	void setScanningExclusive();

	static bool scanRef_Disabled(
		Incinerator& incinerator, const JavaObject* source, JavaObject** ref);
	static bool scanRef_Inclusive(
		Incinerator& incinerator, const JavaObject* source, JavaObject** ref);
	static bool scanRef_Exclusive(
		Incinerator& incinerator, const JavaObject* source, JavaObject** ref);
	static bool scanStackRef_Disabled(
		Incinerator& incinerator, const JavaMethod* method, JavaObject** ref);
	static bool scanStackRef_Inclusive(
		Incinerator& incinerator, const JavaMethod* method, JavaObject** ref);
	static bool scanStackRef_Exclusive(
		Incinerator& incinerator, const JavaMethod* method, JavaObject** ref);

	class InstalledBundles_finder {
		JnjvmClassLoader const * loader;
	public:
		InstalledBundles_finder(JnjvmClassLoader const * l) : loader(l) {}
		bool operator() (const bundleClassLoadersType::value_type& pair) const;
	};

	class UninstalledBundles_finder {
		JnjvmClassLoader const * loader;
	public:
		UninstalledBundles_finder(JnjvmClassLoader const * l) : loader(l) {}
		bool operator() (
			const staleBundleClassLoadersType::value_type& pair) const;
	};

	j3::Jnjvm* vm;

	mutable vmkit::LockRecursive bundleClassLoadersLock;
	bundleClassLoadersType bundleClassLoaders;
	staleBundleClassLoadersType staleBundleClassLoaders;

	StaleRefListType staleRefList;
	bool needsStaleRefRescan;

	mutable JavaObject* findReferencesToObject;
	std::vector<const JavaObject*> foundReferencerObjects;
};


class IncineratorManagedClassLoader
{
protected:
	enum StaleTags {
		CorrectStaleRef = 0x1,
		RefIsStale		= 0x2
	};

	uint8_t staleRefFlags;

	IncineratorManagedClassLoader() : staleRefFlags(CorrectStaleRef) {}
	virtual ~IncineratorManagedClassLoader();

public:
	bool isStale() const {return (staleRefFlags & RefIsStale) != 0;}

	void markStale(bool stale = true) {
		if (stale)	staleRefFlags |= RefIsStale;
		else		staleRefFlags &= ~RefIsStale;
	}

	bool isStaleReferencesCorrectionEnabled() const {
		return (staleRefFlags & CorrectStaleRef) != 0;}

	void setStaleReferencesCorrectionEnabled(bool enable) {
		if (enable)	staleRefFlags |= CorrectStaleRef;
		else		staleRefFlags &= ~CorrectStaleRef;
	}
};

}

#endif
