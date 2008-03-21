//===------------- ctcircular.h - Mvm Garbage Collector -------------------===//
//
//                              Mvm
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef _CT_CIRCULAR_H_
#define _CT_CIRCULAR_H_

#include "ctosdep.h"


class CircularBase {
	CircularBase	*_next;
	CircularBase	*_prev;
public:
	inline CircularBase *next() { return _next; }
	inline CircularBase *prev() { return _prev; }

	inline void next(CircularBase *n) { _next = n; }
	inline void prev(CircularBase *p) { _prev = p; }

	inline CircularBase() { alone(); }
	inline CircularBase(CircularBase *p) { append(p); }

	inline void remove() { 
		_prev->_next = _next; 
		_next->_prev = _prev;
		alone();
	}

	inline void append(CircularBase *p) { 
		_prev = p;
		_next = p->_next;
		_next->_prev = this;
		_prev->_next = this;
	}

	inline void alone() { _prev = _next = this; }
};

class Circular : public CircularBase, public TObj {
public:
	inline Circular() : CircularBase() {}
	inline Circular(Circular *p) : CircularBase(p) {}
	inline ~Circular() {
		if(prev() != this) {
			next()->prev(prev());
			delete next();
		}
	}
};

#endif
