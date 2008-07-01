//===----------------- PNetLib.h - PNetLib interface ----------------------===//
//
//                              N3
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//


#ifndef N3_PNETLIB_H
#define N3_PNETLIB_H

#include "VMObject.h"


namespace n3 {

class PNetString;

class StringBuilder : public VMObject {
public:
  PNetString* buildString;
  sint32 maxCapactiy;
  sint32 needsCopy;
};

}

#endif 
