//===---------- JnjvmConfig.h - Jnjvm configuration file ------------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//


#ifndef JNJVM_CONFIG_H
#define JNJVM_CONFIG_H

#ifdef ISOLATE_SHARING
#define ISOLATE_STATIC
#else

namespace jnjvm {

class ClassArray;
class ClassPrimitive;
class Class;
class CommonClass;
class JavaConstantPool;

#define UserClassArray ClassArray
#define UserClassPrimitive ClassPrimitive
#define UserClass Class
#define UserCommonClass CommonClass
#define UserConstantPool JavaConstantPool

}
#define ISOLATE_STATIC static
#endif

#endif // JNJVM_CONFIG_H
