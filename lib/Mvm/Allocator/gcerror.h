//===---------------- gcerror.h - Mvm allocator ---------------------------===//
//
//                              Mvm
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef _GC_ERROR_H_
#define _GC_ERROR_H_


extern void (*_gcfatal)(const char *, unsigned int l, const char *, const char *, ...);

extern void (*on_fatal)(void);

extern void (*onMemoryError)(unsigned int sz);

#define gcfatal(msg, args...) do { \
															  on_fatal(); \
                                (*_gcfatal)(__FILE__, __LINE__, __PRETTY_FUNCTION__, msg, ##args); \
															} while(0)

#define gcwarning(msg, args...) do { \
                                  printf("In %s (%s line %d):\n\t", __PRETTY_FUNCTION__, __FILE__, __LINE__); \
                                  printf(msg, ##args); \
                                } while(0)

#define gcwarning2(msg)         do { \
                                  printf("In %s (%s line %d):\n\t", __PRETTY_FUNCTION__, __FILE__, __LINE__); \
                                  printf(msg); \
                                } while(0)


#endif


