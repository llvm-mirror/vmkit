#ifndef _P3_INTERPRETOR_H_
#define _P3_INTERPRETOR_H_

namespace p3 {

class P3Code;
class P3Object;

class P3Interpretor {
	static const char *opcodeNames[];
	P3Code*           code;

public:
	P3Interpretor(P3Code* c) { this->code = c; }

	P3Object* execute();
};

} // namespace p3

#endif
