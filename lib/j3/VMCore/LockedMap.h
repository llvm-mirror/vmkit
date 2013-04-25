//===------- LockedMap.h - A thread-safe map implementation ---------------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines thread-safe maps that must be deallocated by the owning
// object. For example a class loader is responsible for deallocating the
// types stored in a TypeMap.
//
//===----------------------------------------------------------------------===//

#ifndef JNJVM_LOCKED_MAP_H
#define JNJVM_LOCKED_MAP_H


#include <map>

#include <cstring>

#include "types.h"

#include "vmkit/Allocator.h"
#include "vmkit/VmkitDenseMap.h"
#include "vmkit/Locks.h"
#include "UTF8.h"

namespace j3 {

class Signdef;
class Typedef;
class UserCommonClass;

class ClassMap : public vmkit::PermanentObject {
public:
  ClassMap() {}
  ClassMap(vmkit::VmkitDenseMap<const vmkit::UTF8*, UserCommonClass*>* precompiled) : map(*precompiled) {}

  vmkit::LockRecursive lock;
  vmkit::VmkitDenseMap<const vmkit::UTF8*, UserCommonClass*> map;
  typedef vmkit::VmkitDenseMap<const vmkit::UTF8*, UserCommonClass*>::iterator iterator;
};

class TypeMap : public vmkit::PermanentObject {
public:
  vmkit::LockNormal lock;
  vmkit::VmkitDenseMap<const vmkit::UTF8*, Typedef*> map;
  typedef vmkit::VmkitDenseMap<const vmkit::UTF8*, Typedef*>::iterator iterator;
};

class SignMap : public vmkit::PermanentObject {
public:
  vmkit::LockNormal lock;
  vmkit::VmkitDenseMap<const vmkit::UTF8*, Signdef*> map;
  typedef vmkit::VmkitDenseMap<const vmkit::UTF8*, Signdef*>::iterator iterator;
};

} // end namespace j3

#endif
