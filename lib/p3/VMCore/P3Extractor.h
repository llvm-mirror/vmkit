#ifndef _P3_EXTRACTOR_H_
#define _P3_EXTRACTOR_H_

namespace p3 {

class P3Object;
class P3Reader;

class P3Extractor {
	/// reader - the reader
	P3Reader* reader;

public:
	P3Extractor(P3Reader* r) { this->reader = r; }

	/// readObject - unmarshal an object
	///
	P3Object* readObject();
};

} // namespace p3

#endif
