#include "j3/j3field.h"
#include "j3/j3class.h"
#include "j3/j3classloader.h"
#include "j3/j3.h"

using namespace j3;

J3ObjectHandle* J3Field::javaField() {
	if(!_javaField) {
		layout()->lock();
		_javaField = J3ObjectHandle::doNewObject(layout()->loader()->vm()->fieldClass);
		J3::internalError(L"implement me: javaField");
		layout()->unlock();
	}
	return _javaField;
}

void J3Field::dump() {
	printf("Field: %ls %ls::%ls (%d)\n", type()->name()->cStr(), layout()->name()->cStr(), name()->cStr(), access());
}

