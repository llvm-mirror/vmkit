//===------- NativeUtil.h - Methods to call native functions --------------===//
//
//                              N3
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef N3_NATIVE_UTIL_H
#define N3_NATIVE_UTIL_H

namespace n3 {

class VMCommonClass;
class VMMethod;

class NativeUtil {
public:

  static void* nativeLookup(VMCommonClass* cl, VMMethod* meth);
  static void initialise();
};

}

#endif
