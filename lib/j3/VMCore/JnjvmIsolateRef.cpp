
#include "JnjvmIsolate.h"

using namespace j3;


bool Jnjvm::resetDeadIsolateReference(void* source, void** ptr)
{
	// Don't touch fake Java objects that exist only as bridges between the
	// Java object model and the C++ object model.
	if (!ptr || !(*ptr)	// NULL reference or NULL object
		|| VMClassLoader::isVMClassLoader(*(JavaObject**)ptr)
		|| VMStaticInstance::isVMStaticInstance(*(JavaObject**)ptr))
		return false;

	CommonClass* ccl = JavaObject::getClass(*((JavaObject**)ptr));
	isolate_id_t isolateID = ccl->classLoader->getIsolateID();

	// vmkit::LockGuard lg(IsolateLock);
	if (!(RunningIsolates[isolateID].state & IsolateResetReferences))
		return false;	// Isolate not marked for resetting references

	std::cout << "Resetting @reference=" << ptr << " @oldObject=" << *ptr <<
		" class=" << *ccl->name << " isolateID=" << isolateID;

	if (source) {
		CommonClass* sccl = JavaObject::getClass(((JavaObject*)source));
		isolate_id_t sourceIsolateID = sccl->classLoader->getIsolateID();

		std::cout << " fromClass=" << *sccl->name << " fromIsolateID=" << sourceIsolateID;
	}

	std::cout << std::endl;

	*ptr = NULL;	// Reset the reference
	return true;
}
