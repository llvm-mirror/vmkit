//===--------------- JavaTypes.h - Java primitives ------------------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef JNJVM_JAVA_TYPES_H
#define JNJVM_JAVA_TYPES_H

#include "types.h"

#include "mvm/Allocator.h"
#include "mvm/Object.h"

#include "JnjvmClassLoader.h"

namespace jnjvm {

class Classpath;
class ClassArray;
class CommonClass;
class JavaArray;
class JavaJIT;
class JavaObject;
class Jnjvm;
class JnjvmBootstrapLoader;
class JnjvmClassLoader;
class UserClassArray;
class UserClassPrimitive;
class UserCommonClass;
class UTF8;
class UTF8Map;

#define VOID_ID 0
#define BOOL_ID 1
#define BYTE_ID 2
#define CHAR_ID 3
#define SHORT_ID 4
#define INT_ID 5
#define FLOAT_ID 6
#define LONG_ID 7
#define DOUBLE_ID 8
#define OBJECT_ID 9
#define ARRAY_ID 10
#define NUM_ASSESSORS 11

static const char I_TAB = '[';
static const char I_END_REF = ';';
static const char I_PARG = '(';
static const char I_PARD = ')';
static const char I_BYTE = 'B';
static const char I_CHAR = 'C';
static const char I_DOUBLE = 'D';
static const char I_FLOAT = 'F';
static const char I_INT = 'I';
static const char I_LONG = 'J';
static const char I_REF = 'L';
static const char I_SHORT = 'S';
static const char I_VOID = 'V';
static const char I_BOOL = 'Z';
static const char I_SEP = '/';

/// Typedef - Each class has a Typedef representation. A Typedef is also a class
/// which has not been loaded yet. Typedefs are hashed on the name of the class.
/// Hashing is for memory purposes, not for comparison.
///
class Typedef : public mvm::PermanentObject {
public:
  
  /// keyName - The name of the Typedef. It is the representation of a class
  /// in a Java signature, e.g. "Ljava/lang/Object;".
  ///
  const UTF8* keyName;
  
  /// tPrintBuf - Prints the name of the class this Typedef represents.
  ///
  virtual void tPrintBuf(mvm::PrintBuffer* buf) const = 0;
  
  /// printString - Print the Typedef for debugging purposes.
  ///
  const char* printString() const;
  
  /// assocClass - Given the loaded, try to load the class represented by this
  /// Typedef.
  ///
  virtual UserCommonClass* assocClass(JnjvmClassLoader* loader) const = 0;
 
  virtual bool trace() const = 0;
  
  virtual bool isPrimitive() const {
    return false;
  }
  
  virtual bool isReference() const {
    return true;
  }
  
  virtual bool isUnsigned() const {
    return false;
  }

  virtual const UTF8* getName() const {
    return keyName;
  }

  const UTF8* getKey() const {
    return keyName;
  }
  
  virtual ~Typedef() {}
};

class PrimitiveTypedef : public Typedef {
private:
  UserClassPrimitive* prim;
  bool unsign;
  char charId;
  
public:
  
  /// tPrintBuf - Prints the name of the class this Typedef represents.
  ///
  virtual void tPrintBuf(mvm::PrintBuffer* buf) const;
  

  virtual bool trace() const {
    return false;
  }
  
  virtual bool isPrimitive() const {
    return true;
  }
  
  virtual bool isReference() const {
    return false;
  }

  virtual bool isUnsigned() const {
    return unsign;
  }

  virtual UserCommonClass* assocClass(JnjvmClassLoader* loader) const {
    return (UserCommonClass*)prim;
  }

  PrimitiveTypedef(const UTF8* name, UserClassPrimitive* cl, bool u, char i) {
    keyName = name;
    prim = cl;
    unsign = u;
    charId = i;
  }
  
  bool isVoid() const {
    return charId == I_VOID;
  }

  bool isLong() const {
    return charId == I_LONG;
  }

  bool isInt() const {
    return charId == I_INT;
  }

  bool isChar() const {
    return charId == I_CHAR;
  }

  bool isShort() const {
    return charId == I_SHORT;
  }

  bool isByte() const {
    return charId == I_BYTE;
  }

  bool isBool() const {
    return charId == I_BOOL;
  }

  bool isFloat() const {
    return charId == I_FLOAT;
  }

  bool isDouble() const {
    return charId == I_DOUBLE;
  }
  
};

class ArrayTypedef : public Typedef {
public:
  /// tPrintBuf - Prints the name of the class this Typedef represents.
  ///
  virtual void tPrintBuf(mvm::PrintBuffer* buf) const;

  
  virtual bool trace() const {
    return true;
  }

  virtual UserCommonClass* assocClass(JnjvmClassLoader* loader) const;

  ArrayTypedef(const UTF8* name) {
    keyName = name;
  }
};

class ObjectTypedef : public Typedef {
private:
  /// pseudoAssocClassName - The real name of the class this Typedef
  /// represents, e.g. "java/lang/Object"
  ///
  const UTF8* pseudoAssocClassName;

public:
  /// tPrintBuf - Prints the name of the class this Typedef represents.
  ///
  virtual void tPrintBuf(mvm::PrintBuffer* buf) const;
  
  virtual bool trace() const {
    return true;
  }
  
  virtual UserCommonClass* assocClass(JnjvmClassLoader* loader) const;

  ObjectTypedef(const UTF8*name, UTF8Map* map);
  
  virtual const UTF8* getName() const {
    return pseudoAssocClassName;
  }

};


/// Signdef - This class represents a Java signature. Each Java method has a
/// Java signature. Signdefs are hashed for memory purposes, not equality
/// purposes.
///
class Signdef : public mvm::PermanentObject {
private:
  
  /// _staticCallBuf - A dynamically generated method which calls a static Java
  /// function with the specific signature and receive the arguments in a
  /// buffer.
  ///
  intptr_t _staticCallBuf;
  intptr_t staticCallBuf();

  /// _virtualCallBuf - A dynamically generated method which calls a virtual
  /// Java function with the specific signature and receive the arguments in a
  /// buffer.
  ///
  intptr_t _virtualCallBuf;
  intptr_t virtualCallBuf();
  
  /// _staticCallAP - A dynamically generated method which calls a static Java
  /// function with the specific signature and receive the arguments in a
  /// variable argument handle.
  ///
  intptr_t _staticCallAP;
  intptr_t staticCallAP();
  
  /// _virtualCallBuf - A dynamically generated method which calls a virtual
  /// Java function with the specific signature and receive the arguments in a
  /// variable argument handle.
  ///
  intptr_t _virtualCallAP; 
  intptr_t virtualCallAP();
  
public:

  /// initialLoader - The loader that first loaded this signdef.
  ///
  JnjvmClassLoader* initialLoader;

  /// keyName - The Java name of the signature, e.g. "()V".
  ///
  const UTF8* keyName;
  
  /// printString - Print the signature for debugging purposes.
  ///
  const char* printString() const;

  /// printWithSign - Print the signature of a method with the method's class
  /// and name.
  ///
  void printWithSign(CommonClass* cl, const UTF8* name,
                     mvm::PrintBuffer* buf) const;
  
  /// Signdef - Create a new Signdef.
  ///
  Signdef(const UTF8* name, JnjvmClassLoader* loader,
          std::vector<Typedef*>& args, Typedef* ret);
  
  /// operator new - Redefines the new operator of this class to allocate
  /// the arguments in the object itself.
  ///
  void* operator new(size_t sz, mvm::BumpPtrAllocator& allocator,
                     sint32 size) {
    return allocator.Allocate(sizeof(Signdef) + size * sizeof(Typedef));
  }

  
//===----------------------------------------------------------------------===//
//
// Inline calls to get the dynamically generated functions to call Java
// functions. Note that this calls the JIT.
//
//===----------------------------------------------------------------------===//

  intptr_t getStaticCallBuf() {
    if(!_staticCallBuf) return staticCallBuf();
    return _staticCallBuf;
  }

  intptr_t getVirtualCallBuf() {
    if(!_virtualCallBuf) return virtualCallBuf();
    return _virtualCallBuf;
  }
  
  intptr_t getStaticCallAP() {
    if (!_staticCallAP) return staticCallAP();
    return _staticCallAP;
  }

  intptr_t getVirtualCallAP() {
    if (!_virtualCallAP) return virtualCallAP();
    return _virtualCallAP;
  }
  
  void setStaticCallBuf(intptr_t code) {
    _staticCallBuf = code;
  }

  void setVirtualCallBuf(intptr_t code) {
    _virtualCallBuf = code;
  }
  
  void setStaticCallAP(intptr_t code) {
    _staticCallAP = code;
  }

  void setVirtualCallAP(intptr_t code) {
    _virtualCallAP = code;
  }

//===----------------------------------------------------------------------===//
//
// End of inlined methods of getting dynamically generated functions.
//
//===----------------------------------------------------------------------===//

  
  /// JInfo - Holds info useful for the JIT.
  ///
  mvm::JITInfo* JInfo;

  /// getInfo - Get the JIT info of this signature. The info is created lazely.
  ///
  template<typename Ty> 
  Ty *getInfo() {
    if (!JInfo) {
      JInfo = new(initialLoader->allocator) Ty(this);
    }   

    assert((void*)dynamic_cast<Ty*>(JInfo) == (void*)JInfo &&
           "Invalid concrete type or multiple inheritence for getInfo");
    return static_cast<Ty*>(JInfo);
  }
  
  /// nbArguments - The number of arguments in the signature. 
  ///
  uint32 nbArguments;
  
  /// getReturnType - Get the type of the return of this signature.
  ///
  Typedef* getReturnType() const {
    return arguments[0];
  }

  /// getArgumentsType - Get the list of arguments of this signature.
  ///
  Typedef* const* getArgumentsType() const {
    return &(arguments[1]);
  }

private:
  /// humanPrintArgs - Prints the list of typedef in a human readable form.
  ///
  void humanPrintArgs(mvm::PrintBuffer* buf) const;
  
  /// arguments - The list of arguments of the signature. First is the return
  /// type.
  ///
  Typedef* arguments[1];

    
};


} // end namespace jnjvm

#endif
