//===------------ Sigsegv.h - Sgsegv handler for mvm  ---------------------===//
//
//                     The Micro Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//


#ifndef MVM_SIGSEGV_H
#define MVM_SIGSEGV_H

#include <sys/types.h>

namespace mvm {

__BEGIN_DECLS

void register_sigsegv_handler(void (*fct)(int, void *));

__END_DECLS

} // end namespace mvm;

#endif //MVM_SIGSEGV_H


