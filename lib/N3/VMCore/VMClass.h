//===------------- VMClass.h - CLI class representation -------------------===//
//
//                              N3
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef N3_VM_CLASS_H
#define N3_VM_CLASS_H

#include "types.h"

#include "mvm/Method.h"
#include "mvm/Object.h"
#include "mvm/Threads/Locks.h"
#include "mvm/Threads/Cond.h"

#include "llvm/Constants.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/Function.h"
#include "llvm/Type.h"

namespace n3 {

class ArraySInt32;
class Assembly;
class Enveloppe;
class Param;
class Property;
class UTF8;
class VirtualMachine;
class VMClass;
class VMField;
class VMMethod;
class VMObject;

typedef enum VMClassState {
  hashed = 0, loaded, prepared, readed, resolved, unified, clinitParent, inClinit, ready
}VMClassState;


class VMCommonClass : public mvm::Object {
public:
  static VirtualTable* VT;
  virtual void print(mvm::PrintBuffer* buf) const;
  virtual void tracer(size_t sz);

  std::vector<VMMethod*> virtualMethods;
  std::vector<VMMethod*> staticMethods;
  std::vector<VMField*>  virtualFields;
  std::vector<VMField*>  staticFields;
  std::vector<VMClass*> interfaces;
  std::vector<uint32> interfacesToken;
  VMCommonClass* super;
  VirtualMachine* vm;
  const UTF8* name;
  const UTF8* nameSpace;
  mvm::Lock* lockVar;
  mvm::Cond* condVar;
  VMObject* delegatee;
  std::vector<VMCommonClass*> display;
  Assembly* assembly;
  std::vector<Property*> properties;
  
  llvm::Function* virtualTracer;
  llvm::Function* staticTracer;
  mvm::Code* codeVirtualTracer;
  mvm::Code* codeStaticTracer;

  uint32 superToken;
  uint32 token;
  bool isArray;
  bool isPointer;
  bool isPrimitive;
  uint32 depth;
  VMClassState status;
  uint32 flags;
  
  static const uint32 maxDisplay;

  void aquire();
  void release();
  void waitClass();
  void broadcastClass();
  bool ownerClass();

  void initialise(VirtualMachine* vm, bool isArray);

  const llvm::Type* naturalType;  // true type
  const llvm::Type* virtualType;  // true type or box
  const llvm::Type* staticType;   // static type

  bool inheritName(const UTF8* Tname);
  bool isOfTypeName(const UTF8* Tname);
  bool implements(VMCommonClass* cl);
  bool instantiationOfArray(VMCommonClass* cl);
  bool subclassOf(VMCommonClass* cl);
  bool isAssignableFrom(VMCommonClass* cl);
  
  void assignType();
  void clinitClass();
  void resolveStatic(bool clinit);
  void resolveType(bool stat, bool clinit);
  void loadParents();
  
  VMMethod* lookupMethodDontThrow(const UTF8* name, 
                                  std::vector<VMCommonClass*>& args,
                                  bool isStatic, bool recurse);
  
  VMMethod* lookupMethod(const UTF8* name, std::vector<VMCommonClass*>& args,
                         bool isStatic, bool recurse);
  
  VMField* lookupFieldDontThrow(const UTF8* name, VMCommonClass* type,
                                bool isStatic, bool recurse);
  
  VMField* lookupField(const UTF8* name, VMCommonClass* type, bool isStatic,
                       bool recurse);
  
  llvm::GlobalVariable* llvmVar();
  llvm::GlobalVariable* _llvmVar;
  
  VMObject* getClassDelegatee();
};



class VMClass : public VMCommonClass {
public:
  static VirtualTable* VT;
  virtual void print(mvm::PrintBuffer* buf) const;
  virtual void tracer(size_t sz);
  
  void resolveFields();
  void resolveStaticFields();
  void resolveVirtualFields();
  void unifyTypes();
  
  VMObject* staticInstance;
  VMObject* virtualInstance;
  std::vector<VMClass*> innerClasses;
  VMClass* outerClass;
  
  VMObject* operator()();
  VMObject* doNew();
  VMObject* initialiseObject(VMObject*);

  uint32 explicitLayoutSize;
};

class VMClassArray : public VMCommonClass {
public:
  static VirtualTable* VT;
  virtual void print(mvm::PrintBuffer* buf) const;
  virtual void tracer(size_t sz);

  uint32 dims;
  VMCommonClass* baseClass;
  
  static VMCommonClass* SuperArray;
  static std::vector<VMClass*>      InterfacesArray;
  static std::vector<VMMethod*>     VirtualMethodsArray;
  static std::vector<VMMethod*>     StaticMethodsArray;
  static std::vector<VMField*>      VirtualFieldsArray;
  static std::vector<VMField*>      StaticFieldsArray;
  
  static const UTF8* constructArrayName(const UTF8* name, uint32 dims);
  void makeType();
  VMObject* doNew(uint32 nb);
  VirtualTable* arrayVT;
};

class VMClassPointer : public VMCommonClass {
public:
  static VirtualTable* VT;
  virtual void print(mvm::PrintBuffer* buf) const;
  virtual void tracer(size_t sz);

  uint32 dims;
  VMCommonClass* baseClass;
  static const UTF8* constructPointerName(const UTF8* name, uint32 dims);
  void makeType();
};

class VMMethod : public mvm::Object {
public:
  static VirtualTable* VT;
  virtual void print(mvm::PrintBuffer* buf) const;
  virtual void tracer(size_t sz);
  
  uint32 flags;
  uint32 offset;
  uint32 implFlags;
  uint32 token;

  VMObject* delegatee;
  VMObject* getMethodDelegatee();
  std::vector<Param*> params;
  std::vector<Enveloppe*> caches;
  std::vector<VMCommonClass*> parameters;
  VMClass* classDef;
  llvm::Function* compiledPtr();
  llvm::Function* methPtr;
  const UTF8* name;
  const llvm::FunctionType* _signature;
  bool structReturn;
  bool virt;
  bool canBeInlined;

  mvm::Code* code;
  
  llvm::GenericValue operator()(...);
  llvm::GenericValue operator()(va_list ap);
  llvm::GenericValue operator()(VMObject* obj, va_list ap);
  llvm::GenericValue operator()(std::vector<llvm::GenericValue>& args);
  llvm::GenericValue run(...);
  
  const llvm::FunctionType* getSignature();
  static const llvm::FunctionType* resolveSignature(
      std::vector<VMCommonClass*>& params, bool isVirt, bool &structRet);
  bool signatureEquals(std::vector<VMCommonClass*>& args);
  llvm::GlobalVariable* llvmVar();
  llvm::GlobalVariable* _llvmVar;
};

class VMField : public mvm::Object {
public:
  static VirtualTable* VT;
  virtual void print(mvm::PrintBuffer* buf) const;
  virtual void tracer(size_t sz);
  
  uint32 flags;
  llvm::ConstantInt* offset;
  uint32 token;
  const UTF8* name;
  VMClass* classDef;
  VMCommonClass* signature;
  uint64 ptrOffset;
  
  void initField(VMObject* obj);

  llvm::GenericValue operator()(VMObject* obj = 0); 
  void operator()(VMObject* obj, bool val);
  void operator()(VMObject* obj, float val);
  void operator()(VMObject* obj, double val);
  void operator()(VMObject* obj, sint32 val);
  void operator()(VMObject* obj, sint64 val);
  void operator()(VMObject* obj, VMObject* val);
  void operator()(bool val);
  void operator()(float val);
  void operator()(double val);
  void operator()(sint32 val);
  void operator()(sint64 val);

  llvm::GlobalVariable* llvmVar();
  llvm::GlobalVariable* _llvmVar;
};

class Param : public mvm::Object {
public:
  static VirtualTable* VT;
  virtual void print(mvm::PrintBuffer* buf) const;
  virtual void tracer(size_t sz);

  uint32 flags;
  uint32 sequence;
  VMMethod* method;
  const UTF8* name;
};

class Property : public mvm::Object {
public:
  static VirtualTable* VT;
  virtual void print(mvm::PrintBuffer* buf) const;
  virtual void tracer(size_t sz);
  
  std::vector<VMCommonClass*> parameters;
  VMCommonClass* type;
  const llvm::FunctionType* _signature;
  const llvm::FunctionType* getSignature();
  bool virt;
  const UTF8* name;
  VMObject* delegatee;
  uint32 flags;
  VMObject* getPropertyDelegatee();
};



} // end namespace n3

#endif
