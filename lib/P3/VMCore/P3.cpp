#include "P3.h"
#include "P3Error.h"

using namespace p3;

P3::P3(mvm::BumpPtrAllocator& alloc, mvm::VMKit* vmkit) : mvm::VirtualMachine(alloc, vmkit) {
}

void P3::finalizeObject(mvm::gc* obj) { NI(); }
mvm::gc** P3::getReferent(mvm::gc* ref) { NI(); }
void P3::setReferent(mvm::gc* ref, mvm::gc* val) { NI(); }
bool P3::enqueueReference(mvm::gc* _obj) { NI(); }
size_t P3::getObjectSize(mvm::gc* object) { NI(); }
const char* P3::getObjectTypeName(mvm::gc* object) { NI(); }

void P3::runApplicationImpl(int argc, char** argv) {
	NI();
}
