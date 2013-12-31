#ifndef _J3_FIELD_H_
#define _J3_FIELD_H_

#include "vmkit/allocator.h"

namespace vmkit {
	class Name;
}

namespace j3 {
	class J3Layout;
	class J3Type;
	class J3Attributes;
	class J3ObjectHandle;

	class J3Field : public vmkit::PermanentObject {
		friend class J3Class;

		J3Layout*                 _layout;
		uint16_t                  _access;
		const vmkit::Name*        _name;
		J3Type*                   _type;
		J3Attributes*             _attributes;
		uintptr_t                 _offset;
		J3ObjectHandle* volatile  _javaField;

	public:
		J3Field() {}
		J3Field(uint16_t access, const vmkit::Name* name, J3Type* type) { _access = access; _name = name; _type = type; }

		J3ObjectHandle*    javaField();
		J3Attributes*      attributes() const { return _attributes; }
		uint16_t           access() { return _access; }
		J3Layout*          layout()  { return _layout; }
		const vmkit::Name* name() { return _name; }
		J3Type*            type() { return _type; }

		uintptr_t          offset() { return _offset; }

		void               dump();
	};
}

#endif
