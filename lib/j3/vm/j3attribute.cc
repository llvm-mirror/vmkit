#include "j3/j3attribute.h"
#include "j3/j3.h"

using namespace j3;

J3Attribute* J3Attributes::attribute(size_t n) { 
	if(n >= _nbAttributes)
		J3::internalError(L"should not happen");
	return _attributes + n; 
}

J3Attribute* J3Attributes::lookup(const vmkit::Name* id) {
	for(size_t i=0; i<_nbAttributes; i++) {
		if(_attributes[i].id() == id)
			return _attributes+i;
	}

	return 0;
}

