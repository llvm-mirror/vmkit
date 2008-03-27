//===---------------- cterror.h - Mvm common threads ----------------------===//
//
//                              Mvm
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef _CT_ERROR_H_
#define _CT_ERROR_H_

extern const char *ctperror();
extern void	_gcdomsgf(const char *file, unsigned int l, const char *func, const char *msg, ...);
extern void (*pgcdomsgf)(const char *, unsigned int l, const char *, const char *, ...);
extern void (**gcdomsgf)(const char *, unsigned int l, const char *, const char *, ...);

#define ctfatal(msg) (*gcdomsgf)(__FILE__, __LINE__, __PRETTY_FUNCTION__, msg)

#endif


