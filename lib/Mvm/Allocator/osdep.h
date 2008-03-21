//===------------------ osdep.h - Mvm allocator ---------------------------===//
//
//                              Mvm
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef __OSDEP_H_
#define __OSDEP_H_

#include <sys/types.h>
#include <stdarg.h>

/* sys/pages.h ?? */
#define PAGE_SHIFT		12
#define PAGE_SIZE			(1 << PAGE_SHIFT)

/* stdio.h */
#include <stdio.h>

__BEGIN_DECLS

/* stdio.h */
extern int   printf(const char *, ...);
extern int   fprintf(FILE *, const char *, ...);
extern int   vfprintf(FILE *, const char *, va_list);


/* stdlib.h */
extern void exit(int) throw ();

/* string.h */
extern void *memcpy(void *, const void *, size_t) throw ();
//extern void  bzero(void *, size_t);

__END_DECLS

#endif
