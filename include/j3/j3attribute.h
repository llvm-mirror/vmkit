#ifndef _J3_ATTRIBUTE_H_
#define _J3_ATTRIBUTE_H_

#include "vmkit/allocator.h"

namespace vmkit {
	class Name;
}

namespace j3 {
	class J3Attribute : public vmkit::PermanentObject {
		friend class J3Class;

		const vmkit::Name* _id;
		uint32_t           _offset;
	public:
		
		const vmkit::Name* id() { return _id; }
		uint32_t           offset() { return _offset; }
	};

	class J3Attributes : public vmkit::PermanentObject {
		size_t       _nbAttributes;
		J3Attribute  _attributes[1];
	public:
		J3Attributes(size_t n) { _nbAttributes = n; }

		void* operator new(size_t unused, vmkit::BumpAllocator* allocator, size_t n) {
			return vmkit::PermanentObject::operator new(sizeof(J3Attributes) + (n - 1) * sizeof(J3Attribute), allocator);
		}

		size_t       nbAttributes() { return _nbAttributes; }
		J3Attribute* attribute(size_t n);
		
		J3Attribute* lookup(const vmkit::Name* attr);
	};
}

#endif
