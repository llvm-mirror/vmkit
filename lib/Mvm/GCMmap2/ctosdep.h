//===---------------- ctosdep.h - Mvm Garbage Collector -------------------===//
//
//                              Mvm
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef _CT_OSDEP_H_
#define _CT_OSDEP_H_

#include <sys/types.h>

__BEGIN_DECLS

extern void *				malloc(size_t);
extern void					free(void *);
extern int 					printf(const char *, ...);

__END_DECLS

#define tmalloc					malloc
#define tfree						free

class TObj {
public:
	void *operator new(size_t sz) { return tmalloc(sz); }
	void operator delete(void *ptr, size_t sz) { return tfree(ptr); }
};

#endif
