//===--- JavaPrimitive.h - Native functions for primitive values ----------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef JNJVM_JAVA_PRIMITIVE_H
#define JNJVM_JAVA_PRIMITIVE_H

#include "llvm/Function.h"

#include "mvm/Object.h"

#include "types.h"

namespace jnjvm {

class AssessorDesc;
class CommonClass;
class UTF8;

class JavaPrimitive : public mvm::Object {
public:
  static VirtualTable* VT;
  AssessorDesc* funcs;
  const UTF8* className;
  CommonClass* classType;
  const UTF8* symName;
  llvm::Function* initer;
  llvm::Function* getter;

  static std::vector<JavaPrimitive*> primitives;

  static void initialise();

  static JavaPrimitive* byteIdToPrimitive(char id);
  static JavaPrimitive* asciizToPrimitive(char* asciiz);
  static JavaPrimitive* bogusClassToPrimitive(CommonClass* cl);
  static JavaPrimitive* classToPrimitive(CommonClass* cl);
  static JavaPrimitive* funcsToPrimitive(AssessorDesc* funcs);

  virtual void print(mvm::PrintBuffer* buf);
  virtual void tracer(size_t sz);
};

} // end namespace jnjvm

#endif
