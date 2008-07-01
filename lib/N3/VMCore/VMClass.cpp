//===------------ VMClass.cpp - CLI class representation ------------------===//
//
//                              N3
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <stdarg.h>
#include <vector>

#include "llvm/DerivedTypes.h"

#include "debug.h"
#include "types.h"
#include "mvm/JIT.h"
#include "mvm/PrintBuffer.h"
#include "mvm/Threads/Locks.h"


#include "Assembly.h"
#include "CLIAccess.h"
#include "CLIJit.h"
#include "MSCorlib.h"
#include "N3.h"
#include "VirtualMachine.h"
#include "VMArray.h"
#include "VMClass.h"
#include "VMThread.h"

using namespace n3;

void VMCommonClass::print(mvm::PrintBuffer* buf) const {
  buf->write("CLIType<");
  nameSpace->print(buf);
  buf->write("::");
  name->print(buf);
  buf->write(">");
}

void VMCommonClass::aquire() {
  lockVar->lock(); 
}

void VMCommonClass::release() {
  lockVar->unlock();
}

void VMCommonClass::waitClass() {
  condVar->wait(lockVar);
}

void VMCommonClass::broadcastClass() {
  condVar->broadcast();
}

bool VMCommonClass::ownerClass() {
  return mvm::Lock::selfOwner(lockVar);
}


void VMClass::print(mvm::PrintBuffer* buf) const {
  buf->write("CLIType<");
  nameSpace->print(buf);
  buf->write("::");
  name->print(buf);
  buf->write(">");
}

void VMClassArray::print(mvm::PrintBuffer* buf) const {
  buf->write("CLITypeArray<");
  nameSpace->print(buf);
  buf->write("::");
  name->print(buf);
  buf->write(">");
}

void VMClassPointer::print(mvm::PrintBuffer* buf) const {
  buf->write("CLITypePointer<");
  nameSpace->print(buf);
  buf->write("::");
  name->print(buf);
  buf->write(">");
}

void VMMethod::print(mvm::PrintBuffer* buf) const {
  buf->write("CLIMethod<");
  classDef->nameSpace->print(buf);
  buf->write(".");
  classDef->name->print(buf);
  buf->write("::");
  name->print(buf);
  buf->write("(");
  std::vector<VMCommonClass*>::iterator i = ((VMMethod*)this)->parameters.begin();
  std::vector<VMCommonClass*>::iterator e = ((VMMethod*)this)->parameters.end();

  ++i;
  if (i != e) {
    while (true) {
      (*i)->nameSpace->print(buf);
      buf->write(".");
      (*i)->name->print(buf);
      ++i;
      if (i == e) break;
      else buf->write(" ,");
    }
  }
  buf->write(")");
  buf->write(">");
}

void VMField::print(mvm::PrintBuffer* buf) const {
  buf->write("CLIField<");
  classDef->nameSpace->print(buf);
  buf->write(".");
  classDef->name->print(buf);
  buf->write("::");
  name->print(buf);
  buf->write(">");
}
  
void Param::print(mvm::PrintBuffer* buf) const {
  buf->write("CLIParam<");
  name->print(buf);
  buf->write(">");
}

void Property::print(mvm::PrintBuffer* buf) const {
  buf->write("Property def with name <");
  name->print(buf);
  buf->write(">");
}


void VMCommonClass::initialise(VirtualMachine* vm, bool isArray) {
  this->lockVar = mvm::Lock::allocRecursive();
  this->condVar = mvm::Cond::allocCond();
  this->delegatee = 0;
  this->status = hashed;
  this->vm = vm;
  this->isArray = isArray;
  this->isPointer = false;
  this->isPrimitive = false;
  this->naturalType = llvm::OpaqueType::get();
}

const UTF8* VMClassArray::constructArrayName(const UTF8* name, uint32 dims) {
  const char* asciiz = name->UTF8ToAsciiz();
  char* res = (char*)alloca(strlen(asciiz) + (dims * 2) + 1);
  sprintf(res, asciiz);

  for (uint32 i = 0; i < dims; ++i) {
    sprintf(res, "%s[]", res);
  }

  return VMThread::get()->vm->asciizConstructUTF8(res);
}

const UTF8* VMClassPointer::constructPointerName(const UTF8* name, uint32 dims) {
  const char* asciiz = name->UTF8ToAsciiz();
  char* res = (char*)alloca(strlen(asciiz) + (dims * 2) + 1);
  sprintf(res, asciiz);

  for (uint32 i = 0; i < dims; ++i) {
    sprintf(res, "%s*", res);
  }

  return VMThread::get()->vm->asciizConstructUTF8(res);
}


void VMCommonClass::loadParents() {
  if ((0xffff & superToken) == 0) {
    depth = 0;
    display.push_back(this);
  } else {
    super = assembly->loadType((N3*)vm, superToken, true, false, false, true);
    depth = super->depth + 1;
    for (uint32 i = 0; i < super->display.size(); ++i) {
      display.push_back(super->display[i]);
    }
    display.push_back(this);
  }
  
  for (std::vector<uint32>::iterator i = interfacesToken.begin(), 
       e = interfacesToken.end(); i!= e; ++i) {
    interfaces.push_back((VMClass*)assembly->loadType((N3*)vm, (*i), true, 
                                                      false, false, true));
  }

}

typedef void (*clinit_t)(void);

void VMCommonClass::clinitClass() {
  VMCommonClass* cl = this; 
  if (cl->status < ready) {
    cl->aquire();
    int status = cl->status;
    if (status == ready) {
      cl->release();
    } else if (status == unified) {
      cl->status = clinitParent;
      cl->release();
      if (cl->super) {
        cl->super->resolveStatic(true);
      }
      for (uint32 i = 0; i < cl->interfaces.size(); i++) {
        cl->interfaces[i]->resolveStatic(true);
      }
      
      cl->status = inClinit;
      std::vector<VMCommonClass*> args;
      args.push_back(MSCorlib::pVoid);
      VMMethod* meth = cl->lookupMethodDontThrow(N3::clinitName, args,
                                                 true, false);
      
      PRINT_DEBUG(N3_LOAD, 0, COLOR_NORMAL, "; ", 0);
      PRINT_DEBUG(N3_LOAD, 0, LIGHT_GREEN, "clinit ", 0);
      PRINT_DEBUG(N3_LOAD, 0, COLOR_NORMAL, "%s::%s\n", printString(),
                  cl->printString());
      
      if (meth) {
        llvm::Function* pred = meth->compiledPtr();
        clinit_t res = (clinit_t)
          (intptr_t)mvm::jit::executionEngine->getPointerToGlobal(pred);
        res();
      }

      cl->status = ready;
      cl->broadcastClass();
    } else if (status < unified) {
      cl->release();
      VMThread::get()->vm->unknownError("try to clinit a not-readed class...");
    } else {
      if (!cl->ownerClass()) {
        while (status < ready) cl->waitClass();
      } 
      cl->release();
    }
  }
}

void VMClass::resolveStaticFields() {
  
  VMClass* cl = this;

  std::vector<const llvm::Type*> fields;
  fields.push_back(VMObject::llvmType->getContainedType(0));
  uint64 offset = 0;
  for (std::vector<VMField*>::iterator i = cl->staticFields.begin(),
            e = cl->staticFields.end(); i!= e; ++i) {
    // preincrement because 0 is VMObject
    (*i)->offset = llvm::ConstantInt::get(llvm::Type::Int32Ty, ++offset);
  }
  for (std::vector<VMField*>::iterator i = cl->staticFields.begin(),
            e = cl->staticFields.end(); i!= e; ++i) {
    (*i)->signature->resolveType(false, false);
    fields.push_back((*i)->signature->naturalType);
  }

  cl->staticType = llvm::PointerType::getUnqual(llvm::StructType::get(fields, false));

  VirtualTable* VT = CLIJit::makeVT(cl, true);

  uint64 size = mvm::jit::getTypeSize(cl->staticType->getContainedType(0));
  cl->staticInstance = (VMObject*)gc::operator new(size, VT);
  cl->staticInstance->initialise(cl);

  for (std::vector<VMField*>::iterator i = cl->staticFields.begin(),
            e = cl->staticFields.end(); i!= e; ++i) {
    
    (*i)->initField(cl->staticInstance);
  }
}

void VMClass::unifyTypes() {
  printf("in %s\n", this->printString());

  for (std::vector<VMField*>::iterator i = virtualFields.begin(), 
       e = virtualFields.end(); i!= e; ++i) {
    printf("Resolving in %s\n", this->printString());
    (*i)->signature->resolveType(false, false);
    printf("Now I have\n");
    naturalType->print(llvm::cout);
    printf("\n");
  }
  printf("this = %s\n", this->printString());
  naturalType->print(llvm::cout);
  printf("\n");
  if (naturalType->isAbstract())
    naturalType = naturalType->getForwardedType();
  
  assert(naturalType);
}

void VMClass::resolveVirtualFields() {
  VMClass* cl = this;
  if (hasExplicitLayout(flags)) {
    explicitLayoutSize = assembly->getExplicitLayout(token);
    const llvm::Type* type = llvm::IntegerType::get(explicitLayoutSize);
    ((llvm::OpaqueType*)naturalType)->refineAbstractTypeTo(type);
    naturalType = type;
  } else if (super != 0) {
    if (super == MSCorlib::pValue) {
      uint32 size = virtualFields.size();
      if (size == 1) {
        virtualFields[0]->offset = mvm::jit::constantZero;
        if (naturalType->isAbstract()) {
          ((llvm::OpaqueType*)naturalType)->refineAbstractTypeTo(virtualFields[0]->signature->naturalType);
          naturalType = virtualFields[0]->signature->naturalType;
        }
      } else if (size == 0) {
        if (naturalType->isAbstract()) {
          ((llvm::OpaqueType*)naturalType)->refineAbstractTypeTo(llvm::Type::VoidTy);
          naturalType = llvm::Type::VoidTy;
        }
      } else {
        std::vector<const llvm::Type*> Elts;
        uint32 offset = -1;
        for (std::vector<VMField*>::iterator i = virtualFields.begin(), 
          e = virtualFields.end(); i!= e; ++i) {
          (*i)->offset = llvm::ConstantInt::get(llvm::Type::Int32Ty, ++offset);
          const llvm::Type* type = (*i)->signature->naturalType;
          Elts.push_back(type);
        }
        const llvm::Type* tmp = llvm::StructType::get(Elts);
        ((llvm::OpaqueType*)naturalType)->refineAbstractTypeTo(tmp);
        naturalType = tmp;
      }
    } else if (super == MSCorlib::pEnum) {
      ((llvm::OpaqueType*)naturalType)->refineAbstractTypeTo(llvm::Type::Int32Ty); // TODO find max
      naturalType = llvm::Type::Int32Ty;
    } else {
      std::vector<const llvm::Type*> Elts;
      Elts.push_back(super->naturalType->getContainedType(0));
      uint32 offset = 0;
      for (std::vector<VMField*>::iterator i = virtualFields.begin(), 
           e = virtualFields.end(); i!= e; ++i) {
        (*i)->offset = llvm::ConstantInt::get(llvm::Type::Int32Ty, ++offset);
        const llvm::Type* type = (*i)->signature->naturalType;
        Elts.push_back(type);
      }
      const llvm::Type* tmp = llvm::PointerType::getUnqual(llvm::StructType::get(Elts));
      ((llvm::OpaqueType*)naturalType)->refineAbstractTypeTo(tmp);
      naturalType = tmp;
    }
  } else {
    if (naturalType->isAbstract()) { // interfaces
      ((llvm::OpaqueType*)naturalType)->refineAbstractTypeTo(VMObject::llvmType);
      naturalType = VMObject::llvmType;
    }
  }

  unifyTypes();
  
  if (super == MSCorlib::pValue) {
    std::vector<const llvm::Type*> Elts;
    Elts.push_back(VMObject::llvmType->getContainedType(0));
    for (std::vector<VMField*>::iterator i = virtualFields.begin(), 
         e = virtualFields.end(); i!= e; ++i) {
      Elts.push_back((*i)->signature->naturalType);
    }
    virtualType = llvm::PointerType::getUnqual(llvm::StructType::get(Elts)); 
  } else {
    virtualType = naturalType;
  }
  
  if (super != MSCorlib::pEnum) {
    VirtualTable* VT = CLIJit::makeVT(this, false);
  
    uint64 size = mvm::jit::getTypeSize(cl->virtualType->getContainedType(0));
    cl->virtualInstance = (VMObject*)gc::operator new(size, VT);
    cl->virtualInstance->initialise(cl);

    for (std::vector<VMField*>::iterator i = cl->virtualFields.begin(),
              e = cl->virtualFields.end(); i!= e; ++i) {
    
      (*i)->initField(cl->virtualInstance);
    }
  }

}


void VMClassArray::makeType() {
  std::vector<const llvm::Type*> arrayFields;
  arrayFields.push_back(VMObject::llvmType->getContainedType(0));
  arrayFields.push_back(llvm::Type::Int32Ty);
  arrayFields.push_back(llvm::ArrayType::get(baseClass->naturalType, 0));
  const llvm::Type* type = llvm::PointerType::getUnqual(llvm::StructType::get(arrayFields, false));
  ((llvm::OpaqueType*)naturalType)->refineAbstractTypeTo(type);
  naturalType = type;
  virtualType = naturalType;
  arrayVT = CLIJit::makeArrayVT(this);
}

void VMClassPointer::makeType() {
  const llvm::Type* type = (baseClass->naturalType == llvm::Type::VoidTy) ? llvm::Type::Int8Ty : baseClass->naturalType;
  const llvm::Type* pType = llvm::PointerType::getUnqual(type);
  ((llvm::OpaqueType*)naturalType)->refineAbstractTypeTo(pType);
  naturalType = pType;
}

void VMCommonClass::resolveType(bool stat, bool clinit) {
  VMCommonClass* cl = this;
  //printf("*** Resolving: %s\n", cl->printString());
  if (cl->status < resolved) {
    cl->aquire();
    int status = cl->status;
    if (status >= resolved) {
      cl->release();
    } else if (status <  loaded) {
      cl->release();
      VMThread::get()->vm->unknownError("try to resolve a not-readed class");
    } else if (status == loaded) {
      if (cl->isArray) {
        VMClassArray* arrayCl = (VMClassArray*)cl;
        VMCommonClass* baseClass =  arrayCl->baseClass;
        baseClass->resolveType(false, false);
        arrayCl->makeType();
        cl->status = resolved;
      } else if (cl->isPointer) {
        VMClassPointer* pointerCl = (VMClassPointer*)cl;
        VMCommonClass* baseClass =  pointerCl->baseClass;
        baseClass->resolveType(false, false);
        pointerCl->makeType();
        cl->status = resolved;
      } else {
        cl->release();
        cl->loadParents();
        cl->aquire();
        cl->status = prepared;
        assembly->readClass(cl);
        cl->status = readed;
        ((VMClass*)cl)->resolveVirtualFields();
        cl->status = resolved;
      }
      cl->release();
    } else {
      if (!(cl->ownerClass())) {
        while (status < resolved) {
          cl->waitClass();
        }
      }
      cl->release();
    }
  }
  if (stat) cl->resolveStatic(clinit);
}

void VMCommonClass::resolveStatic(bool clinit) {
  VMCommonClass* cl = this;
  if (cl->status < unified) {
    cl->aquire();
    int status = cl->status;
    if (status >= unified) {
      cl->release();
    } else if (status < resolved) {
      cl->release();
      VMThread::get()->vm->unknownError("try to resolve static of a not virtual-resolved class");
    } else if (status == resolved) {
      if (cl->isArray) {
        VMClassArray* arrayCl = (VMClassArray*)cl;
        VMCommonClass* baseClass =  arrayCl->baseClass;
        baseClass->resolveStatic(false);
        cl->status = unified;
      } else if (cl->isPointer) {
        VMClassPointer* pointerCl = (VMClassPointer*)cl;
        VMCommonClass* baseClass =  pointerCl->baseClass;
        baseClass->resolveStatic(false);
        cl->status = unified;
      } else {
        ((VMClass*)cl)->resolveStaticFields();
        cl->status = unified;
      }
      cl->release();
    } else {
      if (!(cl->ownerClass())) {
        while (status < unified) {
          cl->waitClass();
        }
      }
      cl->release();
    }
  }
  if (clinit) cl->clinitClass();
}


VMMethod* VMCommonClass::lookupMethodDontThrow(const UTF8* name,
                                               std::vector<VMCommonClass*>& args, 
                                               bool isStatic, bool recurse) {
  
  std::vector<VMMethod*>* meths = (isStatic? &staticMethods : 
                                             &virtualMethods);
  
  VMMethod *cur, *res = 0;
  int i = 0;
  int nbm = meths->size();

  while (!res && i < nbm) {
    cur = meths->at(i);
    if (cur->name == name && cur->signatureEquals(args)) {
      return cur;
    }
    ++i;
  }

  if (recurse) {
    if (super) res = super->lookupMethodDontThrow(name, args, isStatic,
                                                  recurse);
    if (!res && isStatic) {
      int nbi = interfaces.size();
      i = 0;
      while (res == 0 && i < nbi) {
        res = interfaces[i]->lookupMethodDontThrow(name, args, isStatic,
                                                   recurse);
        ++i;
      }
    }
  }

  return res;
}

VMMethod* VMCommonClass::lookupMethod(const UTF8* name, 
                                      std::vector<VMCommonClass*>& args, 
                                      bool isStatic, bool recurse) {
  
  VMMethod* res = lookupMethodDontThrow(name, args, isStatic, recurse);
  if (!res) {
    VMThread::get()->vm->error(VirtualMachine::MissingMethodException, 
                               "unable to find %s in %s",
                               name->printString(), this->printString());
  }
  return res;
}

VMField* VMCommonClass::lookupFieldDontThrow(const UTF8* name,
                                             VMCommonClass* type,
                                             bool isStatic, bool recurse) {
  
  std::vector<VMField*>* fields = (isStatic? &staticFields : &virtualFields);
  
  VMField *cur, *res = 0;
  int i = 0;
  int nbm = fields->size();

  while (!res && i < nbm) {
    cur = fields->at(i);
    if (cur->name == name && cur->signature == type) {
      return cur;
    }
    ++i;
  }

  if (recurse) {
    if (super) res = super->lookupFieldDontThrow(name, type, isStatic,
                                                 recurse);
    if (!res && isStatic) {
      int nbi = interfaces.size();
      i = 0;
      while (res == 0 && i < nbi) {
        res = interfaces[i]->lookupFieldDontThrow(name, type, isStatic,
                                                  recurse);
        ++i;
      }
    }
  }

  return res;
}

VMField* VMCommonClass::lookupField(const UTF8* name, VMCommonClass* type,
                                    bool isStatic, bool recurse) {
  
  VMField* res = lookupFieldDontThrow(name, type, isStatic, recurse);
  if (!res) {
    VMThread::get()->vm->error(VirtualMachine::MissingFieldException, 
                               "unable to find %s in %s",
                               name->printString(), this->printString());
  }
  return res;
}

VMObject* VMClass::initialiseObject(VMObject* obj) {
  uint64 size = mvm::jit::getTypeSize(virtualType->getContainedType(0));
  memcpy(obj, virtualInstance, size);
  return obj;
}

VMObject* VMClass::doNew() {
  uint64 size = mvm::jit::getTypeSize(virtualType->getContainedType(0));
  VMObject* res = (VMObject*)
    gc::operator new(size, virtualInstance->getVirtualTable());
  memcpy(res, virtualInstance, size);
  return res;
}

VMObject* VMClassArray::doNew(uint32 nb) {
  uint64 size = mvm::jit::getTypeSize(baseClass->naturalType);
  VMArray* res = (VMArray*)
    gc::operator new(size * nb + sizeof(VMObject) + sizeof(sint32), arrayVT);
  memset(res->elements, 0, size * nb);
  res->initialise(this);
  res->size = nb;
  return res;
}

static VMObject* doMultiNewIntern(VMClassArray* cl, uint32 dim, sint32* buf) {
  if (dim <= 0) VMThread::get()->vm->error("Can't happen");
  sint32 n = buf[0];
  if (n < 0) VMThread::get()->vm->negativeArraySizeException(n);
  
  VMArray* res = (VMArray*)cl->doNew(n);
  if (dim > 1) {
    VMCommonClass* base = cl->baseClass;
    if (n > 0) {
      for (sint32 i = 0; i < n; ++i) {
        res->elements[i] = doMultiNewIntern((VMClassArray*)base, dim - 1, &(buf[1]));
      }
    }
    for (uint32 i = 1; i < dim; ++i) {
      if (buf[i] < 0) VMThread::get()->vm->negativeArraySizeException(buf[i]);
    }
  }
  return res;
}

extern "C" VMObject* doMultiNew(VMClassArray* cl, ...) {
  sint32* dimSizes = (sint32*)alloca(cl->dims * sizeof(sint32));
  va_list ap;
  va_start(ap, cl);
  for (uint32 i = 0; i < cl->dims; ++i) {
    dimSizes[i] = va_arg(ap, sint32);
  }
  va_end(ap);
  return doMultiNewIntern(cl, cl->dims, dimSizes);
}


static void disassembleStruct(std::vector<const llvm::Type*> &args, 
                              const llvm::Type* arg) {
  const llvm::StructType* STy = llvm::dyn_cast<llvm::StructType>(arg);
  for (llvm::StructType::element_iterator I = STy->element_begin(),
       E = STy->element_end(); I != E; ++I) {
    if ((*I)->isSingleValueType()) {
      args.push_back(*I);
    } else {
      disassembleStruct(args, *I);
    }
  }
}

const llvm::FunctionType* VMMethod::resolveSignature(
                  std::vector<VMCommonClass*> & parameters, bool isVirt,
                  bool& structRet) {
    const llvm::Type* ret;
    std::vector<const llvm::Type*> args;
    std::vector<VMCommonClass*>::iterator i = parameters.begin(),
                                          e = parameters.end();
    if ((*i)->naturalType->isAbstract()) {
      (*i)->resolveType(false, false);
    }
    ret = (*i)->naturalType;
    ++i;
    
    if (isVirt) {
      VMCommonClass* cur = (*i);
      ++i;
      if (cur->naturalType->isAbstract()) {
        cur->resolveType(false, false);
      }
      if (cur->super != MSCorlib::pValue && cur->super != MSCorlib::pEnum) {
        args.push_back(cur->naturalType);
      } else {
        args.push_back(llvm::PointerType::getUnqual(cur->naturalType));
      }
    }
    
    for ( ; i!= e; ++i) {
      VMCommonClass* cur = (*i);
      if (cur->naturalType->isAbstract()) {
        cur->resolveType(false, false);
      }
      if (cur->naturalType->isSingleValueType()) {
        args.push_back(cur->naturalType);
      } else {
        args.push_back(llvm::PointerType::getUnqual(cur->naturalType));
      }
    }

    if (!(ret->isSingleValueType()) && ret != llvm::Type::VoidTy) {
      args.push_back(llvm::PointerType::getUnqual(ret));
      ret = llvm::Type::VoidTy;
      structRet = true;
    } else {
      structRet = false;
    }
    return llvm::FunctionType::get(ret, args, false);
}

const llvm::FunctionType* VMMethod::getSignature() {
  if (!_signature) {
    _signature = resolveSignature(parameters, !isStatic(flags), structReturn);
  }
  return _signature;
}

const llvm::FunctionType* Property::getSignature() {
  if (!_signature) {
    bool structReturn = false;
    _signature = VMMethod::resolveSignature(parameters, virt, structReturn);
  }
  return _signature;
}

bool VMCommonClass::implements(VMCommonClass* cl) {
  if (this == cl) return true;
  else {
    for (uint32 i = 0; i < interfaces.size(); i++) {
      VMCommonClass* cur = interfaces[i];
      if (cur == cl) return true;
      else if (cur->implements(cl)) return true;
    }
    if (super) {
      return super->implements(cl);
    }
  }
  return false;
}

bool VMCommonClass::instantiationOfArray(VMCommonClass* cl) {
  if (this == cl) return true;
  else {
    if (isArray && cl->isArray) {
      VMCommonClass* baseThis = ((VMClassArray*)this)->baseClass;
      VMCommonClass* baseCl = ((VMClassArray*)cl)->baseClass;

      if (isInterface(baseThis->flags) && isInterface(baseCl->flags)) {
        return baseThis->implements(baseCl);
      } else {
        return baseThis->isAssignableFrom(baseCl);
      }
    }
  }
  return false;
}

bool VMCommonClass::subclassOf(VMCommonClass* cl) {
  if (cl->depth < display.size()) {
    return display[cl->depth] == cl;
  } else {
    return false;
  }
}

bool VMCommonClass::isAssignableFrom(VMCommonClass* cl) {
  if (this == cl) {
    return true;
  } else if (isInterface(cl->flags)) {
    return this->implements(cl);
  } else if (cl->isArray) {
    return this->instantiationOfArray(cl);
  } else if (cl->isPointer){
    VMThread::get()->vm->error("implement me");
    return false;
  } else {
    return this->subclassOf(cl);
  }
}


bool VMMethod::signatureEquals(std::vector<VMCommonClass*>& args) {
  bool stat = isStatic(flags);
  if (args.size() != parameters.size()) return false;
  else {
    std::vector<VMCommonClass*>::iterator i = parameters.begin(), 
          a = args.begin(), e = args.end(); 
    
    if ((*i) != (*a)) return false;
    ++i; ++a;
    if (!stat) {
      ++i; ++a;
    }
    for( ; a != e; ++i, ++a) {
      if ((*i) != (*a)) return false;
    }
  }
  return true;
}

void VMGenericClass::print(mvm::PrintBuffer* buf) const {
  buf->write("Generic Class");
}
