
#ifndef INCINERATOR_H
#define INCINERATOR_H

#include "OSGiGateway.h"
#include "vmkit/Locks.h"
#include "j3/jni.h"

#include <map>
#include <list>
#include <vector>
#include <set>
#include <deque>
#include <stdint.h>

#define DEBUG_VERBOSE_STALE_REF		1
#define DEBUG_EXCLUDE_FINALIZABLE_STALE_OBJECTS		0
#define DEBUG_OBJECT_REF_DUMPING	0

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

	void setBundleStaleReferenceCorrected(OSGiGateway::bundle_id_t bundleID, bool corrected);
	bool isBundleStaleReferenceCorrected(OSGiGateway::bundle_id_t bundleID) const;
	void dumpClassLoaderBundles() const;
	void forceStaleReferenceScanning();

#if DEBUG_OBJECT_REF_DUMPING
	void dumpReferencesToObject(const JavaObject* object);
#endif

	OSGiGateway::bundle_id_t getClassLoaderBundleID(JnjvmClassLoader const * loader) const;
	void setBundleClassLoader(OSGiGateway::bundle_id_t bundleID, JnjvmClassLoader* loader);

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
	typedef std::map<OSGiGateway::bundle_id_t, std::list<JnjvmClassLoader const *> >
		staleBundleClassLoadersType;
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

	class UninstalledBundles_finder {
		JnjvmClassLoader const * loader;
	public:
		UninstalledBundles_finder(JnjvmClassLoader const * l) : loader(l) {}
		bool operator() (
			const staleBundleClassLoadersType::value_type& pair) const;
	};

	j3::Jnjvm* vm;

	mutable vmkit::LockRecursive lock;
	staleBundleClassLoadersType staleBundleClassLoaders;

	StaleRefListType staleRefList;
	bool needsStaleRefRescan;

#if DEBUG_OBJECT_REF_DUMPING

	mutable const JavaObject* findReferencesToObject;
	std::set<const JavaObject*> foundReferencerObjects;

	std::deque<const JavaObject*> pendingRefObject;
	std::set<const JavaObject*> seenObjects;

#endif
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

extern "C" void Java_j3_vm_OSGi_setBundleStaleReferenceCorrected(jlong bundleID, jboolean corrected);
extern "C" jboolean Java_j3_vm_OSGi_isBundleStaleReferenceCorrected(jlong bundleID);
extern "C" void Java_j3_vm_OSGi_forceStaleReferenceScanning();

#if DEBUG_OBJECT_REF_DUMPING
extern "C" void Java_j3_vm_OSGi_dumpReferencesToObject(jlong obj);
#endif

#endif
