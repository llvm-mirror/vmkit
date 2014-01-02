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
			_javaField = layout()->loader()->globalReferences()->add(J3ObjectHandle::doNewObject(layout()->loader()->vm()->fieldClass));

			J3* vm = layout()->loader()->vm();

			vm->fieldClassInit->invokeSpecial(_javaField,                       /* this */
																				layout()->javaClass(),            /* declaring class */
																				vm->nameToString(name()),         /* name */
																				type()->javaClass(),              /* type */
																				access(),                         /* access */
																				slot(),                           /* slot */
																				vm->nameToString(type()->name()), /* signature */
																				layout()->extractAttribute(attributes()->lookup(vm->annotationsAttribute)));/* annotations */

			J3Thread::get()->restore(prev);
		}

		layout()->unlock();
	}
	return _javaField;
}

void J3Field::dump() {
	printf("Field: %ls %ls::%ls (%d)\n", type()->name()->cStr(), layout()->name()->cStr(), name()->cStr(), access());
}

