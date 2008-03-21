//===------------- CLIAccess.h - CLI access description -------------------===//
//
//                              N3
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef N3_CLI_ACCESS_H
#define N3_CLI_ACCESS_H

namespace n3 {

#define ACC_STATIC        0x0010
#define ACC_VIRTUAL       0x0040
#define ACC_INTERFACE     0x0020
#define ACC_INTERNAL      0x1000
#define ACC_SYNCHRO       0x0020

#define EXPLICIT_LAYOUT 0x00000010

#define MK_VERIFIER(name, flag)                   \
  inline bool name(unsigned int param) {          \
    return (flag & param) != 0;                   \
  }                                               \

MK_VERIFIER(isStatic,     ACC_STATIC)
MK_VERIFIER(isVirtual,    ACC_VIRTUAL)
MK_VERIFIER(isInterface,  ACC_INTERFACE)
MK_VERIFIER(isInternal,   ACC_INTERNAL)
MK_VERIFIER(isSynchro,    ACC_SYNCHRO)

MK_VERIFIER(hasExplicitLayout,    EXPLICIT_LAYOUT)
  

#undef MK_VERIFIER

} // end namespace n3

#endif
