//===---------- JnjvmConfig.h - Jnjvm configuration file ------------------===//
//
//                            The VMKit project
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

namespace j3 {

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


#define NR_ISOLATES 1

#endif // JNJVM_CONFIG_H
