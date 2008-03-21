//===--------------- gcerror.cc - Mvm allocator ---------------------------===//
//
//                              Mvm
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "gcerror.h"
#include <stdarg.h>
#include "osdep.h"

static void __gcfatal(const char *file, unsigned int l, const char *func, const char *msg, ...)
{
  va_list va;
  va_start(va, msg);
  fprintf(stderr, "GC[error] in %s line %d (function %s)\n", file, l, func);
  vfprintf(stderr, msg, va);
  fprintf(stderr, "\n");
  va_end(va);
  exit(0);
}

static void _on_fatal() {}

void (*_gcfatal)(const char *, unsigned int l, const char *, const char *, ...) = __gcfatal;
void (*on_fatal)(void) = _on_fatal;

static void _defaultOnMemoryError(unsigned int sz)
{
  fprintf(stderr, "GC[error] out of memory for %d bytes\n", sz);
  exit(0);
}

void (*onMemoryError)(unsigned int) = _defaultOnMemoryError;
