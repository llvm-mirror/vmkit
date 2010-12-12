#include "P3.h"
#include "P3Error.h"
#include "P3Reader.h"
#include "P3Extractor.h"
#include "P3Object.h"
#include "P3Interpretor.h"

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

	reader.readU2();         // version 0xf2d1 (???)
	reader.readU2();         // magic 0xa0d
	reader.readU4();         // last modification

	P3Extractor extractor(&reader); 
	P3Object* obj = extractor.readObject();

	if(!obj->isCode())
		fatal("%s does not contain a code", fileName);

	(new P3Interpretor(obj->asCode()))->execute();
}

void P3::runApplicationImpl(int argc, char** argv) {
	if(argc < 2)
		fatal("usage: %s filename", argv[0]);

	runFile(argv[1]);
}
