#include "j3/j3field.h"
#include "j3/j3class.h"
#include "j3/j3classloader.h"
#include "j3/j3.h"
#include "j3/j3method.h"
#include "j3/j3thread.h"
#include "j3/j3attribute.h"

using namespace j3;

J3ObjectHandle* J3Field::javaField() {
	if(!_javaField) {
		layout()->lock();

		if(!_javaField) {
			J3ObjectHandle* prev = J3Thread::get()->tell();
			J3* vm = J3Thread::get()->vm();

			_javaField = layout()->loader()->globalReferences()->add(J3ObjectHandle::doNewObject(vm->fieldClass));
			vm->fieldClassInit->invokeSpecial(_javaField,                          /* this */
																				layout()->javaClass(0),              /* declaring class */
																				vm->nameToString(name(), 0),         /* name */
																				type()->javaClass(0),                /* type */
																				access(),                            /* access */
																				slot(),                              /* slot */
																				vm->nameToString(type()->name(), 0), /* signature */
																				layout()->extractAttribute(attributes()->lookup(vm->annotationsAttribute)));/* annotations */

			J3Thread::get()->restore(prev);
		}

		layout()->unlock();
	}
	return _javaField;
}

void J3Field::dump() {
	printf("Field: %s %s::%s (%d)\n", type()->name()->cStr(), layout()->name()->cStr(), name()->cStr(), access());
}

