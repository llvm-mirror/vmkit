#ifndef _SAFEPOINT_H_
#define _SAFEPOINT_H_

#include <stdint.h>

namespace vmkit {
	class Safepoint { /* directly in the data sections */
		void*    _addr;
		void*    _metaData;
		uint32_t _sourceIndex;
		uint32_t _nbLives;
		uint32_t _lives[2];

	public:
		void*    addr() { return _addr; }
		void*    metaData() { return _metaData; }
		void     updateMetaData(void* metaData) { _metaData = metaData; }
		uint32_t nbLives() { return _nbLives; }
		uint32_t liveAt(uint32_t i) { return _lives[i]; }

		Safepoint* getNext();
	};
}

#endif
