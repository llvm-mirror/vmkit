//===-------------- cterror.cc - Mvm common threads -----------------------===//
//
//                              Mvm
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "cterror.h"
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

void (*pgcdomsgf)(const char *, unsigned int l, const char *, const char *, ...) = _gcdomsgf;
void (**gcdomsgf)(const char *, unsigned int l, const char *, const char *, ...)	= &pgcdomsgf;

const char *ctperror() {
	return strerror(errno);
}

void _gcdomsgf(const char *file, unsigned int l, const char *func, const char *msg, ...)
{
  va_list va;
  va_start(va, msg);
  fprintf(stderr, "GC[error] in %s line %d (function %s)\n", file, l, func);
  vfprintf(stderr, msg, va);
  fprintf(stderr, "\n");
  va_end(va);
  exit(0);
}


