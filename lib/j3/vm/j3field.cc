#include "j3/j3field.h"
#include "j3/j3class.h"
#include "j3/j3classloader.h"
#include "j3/j3.h"
#include "j3/j3method.h"
#include "j3/j3thread.h"

using namespace j3;

J3ObjectHandle* J3Field::javaField() {
	if(!_javaField) {
		layout()->lock();

		J3ObjectHandle* prev = J3Thread::get()->tell();
		_javaField = layout()->loader()->globalReferences()->add(J3ObjectHandle::doNewObject(layout()->loader()->vm()->fieldClass));

		J3* vm = layout()->loader()->vm();

		vm->fieldClassInit->invokeSpecial(_javaField,                       /* this */
																			0,//layout()->javaClass(),            /* declaring class */
																			0,//vm->nameToString(name()),         /* name */
																			0,//type()->javaClass(),              /* type */
																			0,//access(),                         /* access */
																			0,//slot(),                           /* slot */
																			0,//vm->nameToString(type()->name()), /* signature */
																			0);                               /* annotations */


		J3Thread::get()->restore(prev);
		J3::internalError(L"implement me: javaField");
		layout()->unlock();
	}
	return _javaField;
}

void J3Field::dump() {
	printf("Field: %ls %ls::%ls (%d)\n", type()->name()->cStr(), layout()->name()->cStr(), name()->cStr(), access());
}

