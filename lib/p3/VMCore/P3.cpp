#include "P3.h"
#include "P3Error.h"
#include "P3Reader.h"
#include "P3Extractor.h"

using namespace p3;

P3::P3(mvm::BumpPtrAllocator& alloc, mvm::VMKit* vmkit) : mvm::VirtualMachine(alloc, vmkit) {
}

void P3::finalizeObject(mvm::gc* obj) { NI(); }
mvm::gc** P3::getReferent(mvm::gc* ref) { NI(); }
void P3::setReferent(mvm::gc* ref, mvm::gc* val) { NI(); }
bool P3::enqueueReference(mvm::gc* _obj) { NI(); }
size_t P3::getObjectSize(mvm::gc* object) { NI(); }
const char* P3::getObjectTypeName(mvm::gc* object) { NI(); }

void P3::runFile(const char* fileName) {
	mvm::BumpPtrAllocator allocator;
	P3ByteCode* bc = P3Reader::openFile(allocator, fileName); 
	if(!bc)
		fatal("unable to open: %s", fileName);
	P3Reader reader(bc);

	uint16 ver = reader.readU2();
	printf("ver: 0x%x\n", ver);
	uint16 magic = reader.readU2();
	printf("magic: 0x%x\n", magic);
	reader.readU4(); // last modification

	P3Extractor extractor(&reader); 
	extractor.readObject();
}

void P3::runApplicationImpl(int argc, char** argv) {
	if(argc < 2)
		fatal("usage: %s filename", argv[0]);

	runFile(argv[1]);
}
