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

#include "mvm/JIT.h"
#include "mvm/Object.h"

namespace jnjvm {

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

#define VOID_ID 0
#define BOOL_ID 1
#define BYTE_ID 2
#define CHAR_ID 3
#define SHORT_ID 4
#define INT_ID 5
#define FLOAT_ID 6
#define LONG_ID 7
#define DOUBLE_ID 8
#define ARRAY_ID 9
#define OBJECT_ID 10
#define NUM_ASSESSORS 11

typedef JavaArray* (*arrayCtor_t)(uint32 len, UserClassArray* cl, Jnjvm* vm);


/// AssessorDesc - Description of a Java assessor: these are the letters found
/// in Java signatures, e.g. "I" or "(".
///
class AssessorDesc {
public:
  static VirtualTable *VT;
  static const char I_TAB;
  static const char I_END_REF;
  static const char I_PARG;
  static const char I_PARD;
  static const char I_BYTE;
  static const char I_CHAR;
  static const char I_DOUBLE;
  static const char I_FLOAT;
  static const char I_INT;
  static const char I_LONG;
  static const char I_REF;
  static const char I_SHORT;
  static const char I_VOID;
  static const char I_BOOL;
  static const char I_SEP;
  
  /// doTrace - Is this assessor for garbage collected objects? True for
  /// the "L" assessor and the "[" assessor.
  ///
  bool doTrace;

  /// byteId - Letter reprensenting this assessor.
  ///
  char byteId;

  /// nbb - Number of bytes of instances of this assessor (e.g. 4 for "I").
  ///
  uint32 nbb;

  /// nbw - Number of words of instances if this assessor (e.g. 8 for "D").
  ///
  uint32 nbw;

  /// numId - A byte identifier from 0 to the number of assessors.
  ///
  uint8 numId;

  /// asciizName - The name of the assessor in asciiz, e.g. "void" for "V".
  ///
  const char* asciizName;
  
  /// UTF8Name - The name of the assessor in UTF8, e.g. "void" for "V".
  ///
  const UTF8* UTF8Name;

  /// primitiveClass - The primitive Java class of this assessor. This class
  /// is internal to the JVM.
  ///
  UserClassPrimitive* primitiveClass;

  /// assocClassName - The associated class name, e.g. "java/lang/Integer" for
  /// "I".
  ///
  const UTF8* assocClassName;
  
  /// arrayClass - The primitive array class of the assessor, e.g. I[] for "I".
  ///
  UserClassArray* arrayClass;

  /// arrayCtor - The constructor of an array of this assessor.
  ///
  arrayCtor_t arrayCtor;

//===----------------------------------------------------------------------===//
//
// The set of assessors in Java. This set is unique and there are no other
// assessors.
//
//===----------------------------------------------------------------------===//


  /// dParg - The "(" assessor.
  ///
  static AssessorDesc* dParg;

  /// dPard - The ")" assessor.
  ///
  static AssessorDesc* dPard;

  /// dVoid - The "V" assessor.
  ///
  static AssessorDesc* dVoid;

  /// dBool - The "Z" assessor.
  ///
  static AssessorDesc* dBool;

  /// dByte - The "B" assessor.
  ///
  static AssessorDesc* dByte;

  /// dChar - The "C" assessor.
  ///
  static AssessorDesc* dChar;

  /// dShort - The "S" assessor.
  ///
  static AssessorDesc* dShort;

  /// dInt - The "I" assessor.
  ///
  static AssessorDesc* dInt;

  /// dFloat - The "F" assessor.
  ///
  static AssessorDesc* dFloat;

  /// dLong - The "J" assessor.
  ///
  static AssessorDesc* dLong;

  /// dDouble - The "D" assessor.
  ///
  static AssessorDesc* dDouble;

  /// dTab - The "[" assessor.
  ///
  static AssessorDesc* dTab;

  /// dRef - The "L" assessor.
  ///
  static AssessorDesc* dRef;
  
//===----------------------------------------------------------------------===//
//
// End of assessors.
//
//===----------------------------------------------------------------------===//

  /// AssessorDesc - Construct an assessor.
  ///
  AssessorDesc(bool dt, char bid, uint32 nb, uint32 nw,
               const char* name,
               JnjvmClassLoader* loader, uint8 nid,
               const char* assocName, UserClassArray* cl,
               arrayCtor_t ctor);


  /// initialise - Construct all assessors.
  ///
  static void initialise(JnjvmBootstrapLoader* loader);
  

  /// printString - Print the assessor for debugging purposes.
  ///
  const char* printString() const;

  static void analyseIntern(const UTF8* name, uint32 pos,
                            uint32 meth, AssessorDesc*& ass,
                            uint32& ret);

  static const UTF8* constructArrayName(JnjvmClassLoader* loader, AssessorDesc* ass,
                                        uint32 steps, const UTF8* className);
  
  static void introspectArray(JnjvmClassLoader* loader, const UTF8* utf8,
                              uint32 start, AssessorDesc*& ass,
                              UserCommonClass*& res);

  static AssessorDesc* arrayType(unsigned int t);
  
  static AssessorDesc* byteIdToPrimitive(const char id);
  static AssessorDesc* classNameToPrimitive(const UTF8* name);
 
#ifdef MULTIPLE_VM
  UserClassArray* getArrayClass() const;
  UserClassPrimitive* getPrimitiveClass() const;
#else
  UserClassArray* getArrayClass() const {
    return arrayClass;
  }
  UserClassPrimitive* getPrimitiveClass() const {
    return primitiveClass;
  }
#endif
};


/// Typedef - Each class has a Typedef representation. A Typedef is also a class
/// which has not been loaded yet. Typedefs are hashed on the name of the class.
/// Hashing is for memory purposes, not for comparison.
///
class Typedef {
public:
  
  /// keyName - The name of the Typedef. It is the representation of a class
  /// in a Java signature, e.g. "Ljava/lang/Object;".
  ///
  const UTF8* keyName;

  /// pseudoAssocClassName - The real name of the class this Typedef
  /// represents, e.g. "java/lang/Object"
  ///
  const UTF8* pseudoAssocClassName;

  /// funcs - The assessor for this Typedef. All Typedef must have the dRef
  /// or the dTab assessor, except the primive Typedefs.
  ///
  const AssessorDesc* funcs;

  /// initialLoader - The loader that first loaded this typedef.
  ///
  JnjvmClassLoader* initialLoader;

  /// printString - Print the Typedef for debugging purposes.
  ///
  const char* printString() const;
  
  /// assocClass - Given the loaded, try to load the class represented by this
  /// Typedef.
  ///
  UserCommonClass* assocClass(JnjvmClassLoader* loader);

  /// humanPrintArgs - Prints the list of typedef in a human readable form.
  ///
  static void humanPrintArgs(const std::vector<Typedef*>*,
                             mvm::PrintBuffer* buf);
  
  /// Typedef - Create a new Typedef.
  ///
  Typedef(const UTF8* name, JnjvmClassLoader* loader);
  
  /// tPrintBuf - Prints the name of the class this Typedef represents.
  ///
  void tPrintBuf(mvm::PrintBuffer* buf) const;

};

/// Signdef - This class represents a Java signature. Each Java method has a
/// Java signature. Signdefs are hashed for memory purposes, not equality
/// purposes.
///
class Signdef {
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

  /// args - The arguments as Typedef of this signature.
  ///
  std::vector<Typedef*> args;

  /// ret - The Typedef return of this signature.
  ///
  Typedef* ret;

  /// initialLoader - The loader that first loaded this typedef.
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
  void printWithSign(CommonClass* cl, const UTF8* name, mvm::PrintBuffer* buf);
  
  /// Signdef - Create a new Signdef.
  ///
  Signdef(const UTF8* name, JnjvmClassLoader* loader);
  
  
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
      JInfo = new Ty(this);
    }   

    assert((void*)dynamic_cast<Ty*>(JInfo) == (void*)JInfo &&
           "Invalid concrete type or multiple inheritence for getInfo");
    return static_cast<Ty*>(JInfo);
  }
    
};


} // end namespace jnjvm

#endif
