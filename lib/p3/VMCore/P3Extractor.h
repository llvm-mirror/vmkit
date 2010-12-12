#ifndef _P3_EXTRACTOR_H_
#define _P3_EXTRACTOR_H_

namespace p3 {

class P3Reader;

class P3Extractor {
	/// reader - the reader
	P3Reader* reader;

public:
	P3Extractor(P3Reader* r) { this->reader = r; }

	/// readObject - unmarshal an object
	///
	void readObject();
};

} // namespace p3

#endif
