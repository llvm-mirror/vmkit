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

#include <cstdio>
#include <cstdlib>
#include <cstring>

#endif
