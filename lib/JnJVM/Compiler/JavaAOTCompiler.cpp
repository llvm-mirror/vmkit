//===----- JavaAOTCompiler.cpp - Support for Ahead of Time Compiler --------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/BasicBlock.h"
#include "llvm/Constants.h"
#include "llvm/Instructions.h"
#include "llvm/Module.h"
#include "llvm/ModuleProvider.h"
#include "llvm/PassManager.h"

#include "mvm/Threads/Thread.h"

#include "jnjvm/JnjvmModule.h"

#include "JavaCache.h"
#include "JavaConstantPool.h"
#include "JavaString.h"
#include "JavaThread.h"
#include "JavaTypes.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"
#include "Reader.h"
#include "Zip.h"

#include <cstdio>

using namespace jnjvm;
using namespace llvm;

extern void* JavaArrayVT[];
extern void* ArrayObjectVT[];
extern void* JavaObjectVT[];

extern ClassArray ArrayOfBool;
extern ClassArray ArrayOfByte;
extern ClassArray ArrayOfChar;
extern ClassArray ArrayOfShort;
extern ClassArray ArrayOfInt;
extern ClassArray ArrayOfFloat;
extern ClassArray ArrayOfDouble;
extern ClassArray ArrayOfLong;

bool JavaAOTCompiler::isCompiling(const CommonClass* cl) const {
  if (cl->isClass()) {
    // A class is being static compiled if owner class is not null.
    return (((Class*)cl)->getOwnerClass() != 0);
  } else if (cl->isArray()) {
    return isCompiling(((ClassArray*)cl)->baseClass());
  } else {
    return false;
  }
}

Constant* JavaAOTCompiler::getNativeClass(CommonClass* classDef) {

  if (classDef->isClass() || 
      (classDef->isArray() && isCompiling(classDef)) ||
      (assumeCompiled && !(classDef->isArray() && 
        classDef->asArrayClass()->baseClass()->isPrimitive()))) {
    native_class_iterator End = nativeClasses.end();
    native_class_iterator I = nativeClasses.find(classDef);
    if (I == End) {
      const llvm::Type* Ty = 0;
      
      if (classDef->isArray()) {
        Ty = JnjvmModule::JavaClassArrayType->getContainedType(0); 
      } else {
        Ty = JnjvmModule::JavaClassType->getContainedType(0); 
      }
    
      GlobalVariable* varGV = 
        new GlobalVariable(Ty, false, GlobalValue::ExternalLinkage, 0,
                           classDef->printString(), getLLVMModule());
    
      nativeClasses.insert(std::make_pair(classDef, varGV));

      if (classDef->isClass() && isCompiling(classDef->asClass())) {
        Constant* C = CreateConstantFromClass((Class*)classDef);
        varGV->setInitializer(C);
      } else if (classDef->isArray()) {
        Constant* C = CreateConstantFromClassArray((ClassArray*)classDef);
        varGV->setInitializer(C);
      }

      return varGV;

    } else {
      return I->second;
    }
  } else if (classDef->isArray()) {
    array_class_iterator End = arrayClasses.end();
    array_class_iterator I = arrayClasses.find((ClassArray*)classDef);
    if (I == End) {
      const llvm::Type* Ty = JnjvmModule::JavaClassArrayType; 
    
      GlobalVariable* varGV = 
        new GlobalVariable(Ty, false, GlobalValue::InternalLinkage,
                           Constant::getNullValue(Ty),
                           classDef->printString(), getLLVMModule());
    
      arrayClasses.insert(std::make_pair((ClassArray*)classDef, varGV));
      return varGV;
    } else {
      return I->second;
    }
  } else if (classDef->isPrimitive()) {
    assert(0 && "implement me");
  }
  return 0;
}

Constant* JavaAOTCompiler::getConstantPool(JavaConstantPool* ctp) {
  llvm::Constant* varGV = 0;
  constant_pool_iterator End = constantPools.end();
  constant_pool_iterator I = constantPools.find(ctp);
  if (I == End) {
    const Type* Ty = JnjvmModule::ConstantPoolType->getContainedType(0);
    varGV = new GlobalVariable(Ty, false,
                               GlobalValue::InternalLinkage,
                               Constant::getNullValue(Ty), "", getLLVMModule());
    constantPools.insert(std::make_pair(ctp, varGV));
    return varGV;
  } else {
    return I->second;
  }
}

Constant* JavaAOTCompiler::getMethodInClass(JavaMethod* meth) {
  Class* cl = meth->classDef;
  Constant* MOffset = 0;
  Constant* Array = 0;
  method_iterator SI = virtualMethods.find(cl);
  for (uint32 i = 0; i < cl->nbVirtualMethods + cl->nbStaticMethods; ++i) {
    if (&cl->virtualMethods[i] == meth) {
      MOffset = ConstantInt::get(Type::Int32Ty, i);
      break;
    }
  }
  Array = SI->second; 
  Constant* GEPs[2] = { JnjvmModule::constantZero, MOffset };
  return ConstantExpr::getGetElementPtr(Array, GEPs, 2);
}

Constant* JavaAOTCompiler::getString(JavaString* str) {
  string_iterator SI = strings.find(str);
  if (SI != strings.end()) {
    return SI->second;
  } else {
    assert(str && "No string given");
    LLVMClassInfo* LCI = (LLVMClassInfo*)getClassInfo((Class*)str->getClass());
    const llvm::Type* Ty = LCI->getVirtualType();
    GlobalVariable* varGV = 
      new GlobalVariable(Ty->getContainedType(0), false,
                         GlobalValue::InternalLinkage,
                         0, "", getLLVMModule());
    Constant* res = ConstantExpr::getCast(Instruction::BitCast, varGV,
                                          JnjvmModule::JavaObjectType);
    strings.insert(std::make_pair(str, res));
    Constant* C = CreateConstantFromJavaString(str);
    varGV->setInitializer(C);
    return res;
  }
}

Constant* JavaAOTCompiler::getEnveloppe(Enveloppe* enveloppe) {
  enveloppe_iterator SI = enveloppes.find(enveloppe);
  if (SI != enveloppes.end()) {
    return SI->second;
  } else {
    GlobalVariable* varGV = 
      new GlobalVariable(JnjvmModule::EnveloppeType->getContainedType(0), false,
                         GlobalValue::InternalLinkage, 0, "", getLLVMModule());
    enveloppes.insert(std::make_pair(enveloppe, varGV));
    
    Constant* C = CreateConstantFromEnveloppe(enveloppe);
    varGV->setInitializer(C);
    return varGV;
  }
}

Constant* JavaAOTCompiler::getJavaClass(CommonClass* cl) {
  java_class_iterator End = javaClasses.end();
  java_class_iterator I = javaClasses.find(cl);
  if (I == End) {
    Class* javaClass = cl->classLoader->bootstrapLoader->upcalls->newClass;
    LLVMClassInfo* LCI = (LLVMClassInfo*)getClassInfo(javaClass);
    const llvm::Type* Ty = LCI->getVirtualType();
    
    GlobalVariable* varGV = 
      new GlobalVariable(Ty->getContainedType(0), false,
                         GlobalValue::InternalLinkage, 0, "", getLLVMModule());
    
    Constant* res = ConstantExpr::getCast(Instruction::BitCast, varGV,
                                          JnjvmModule::JavaObjectType);
  
    javaClasses.insert(std::make_pair(cl, res));
    varGV->setInitializer(CreateConstantFromJavaClass(cl));
    return res;
  } else {
    return I->second;
    }
}

JavaObject* JavaAOTCompiler::getFinalObject(llvm::Value* obj) {
  return 0;
}



Constant* JavaAOTCompiler::getFinalObject(JavaObject* obj) {
  final_object_iterator End = finalObjects.end();
  final_object_iterator I = finalObjects.find(obj);
  if (I == End) {
    abort();
    return 0;
  } else {
    return I->second;
  }
}

Constant* JavaAOTCompiler::CreateConstantFromStaticInstance(Class* cl) {
  LLVMClassInfo* LCI = getClassInfo(cl);
  const Type* Ty = LCI->getStaticType();
  const StructType* STy = dyn_cast<StructType>(Ty->getContainedType(0));
  
  std::vector<Constant*> Elts;
  
  for (uint32 i = 0; i < cl->nbStaticFields; ++i) {
    JavaField& field = cl->staticFields[i];
    const Typedef* type = field.getSignature();
    LLVMAssessorInfo& LAI = getTypedefInfo(type);
    const Type* Ty = LAI.llvmType;

    Attribut* attribut = field.lookupAttribut(Attribut::constantAttribut);

    if (!attribut) {
      Elts.push_back(Constant::getNullValue(Ty));
    } else {
      Reader reader(attribut, cl->bytes);
      JavaConstantPool * ctpInfo = cl->ctpInfo;
      uint16 idx = reader.readU2();
      if (type->isPrimitive()) {
        if (Ty == Type::Int64Ty) {
          Elts.push_back(ConstantInt::get(Ty, (uint64)ctpInfo->LongAt(idx)));
        } else if (Ty == Type::DoubleTy) {
          Elts.push_back(ConstantFP::get(Ty, ctpInfo->DoubleAt(idx)));
        } else if (Ty == Type::FloatTy) {
          Elts.push_back(ConstantFP::get(Ty, ctpInfo->FloatAt(idx)));
        } else {
          Elts.push_back(ConstantInt::get(Ty, (uint64)ctpInfo->IntegerAt(idx)));
        }
      } else if (type->isReference()){
        const UTF8* utf8 = ctpInfo->UTF8At(ctpInfo->ctpDef[idx]);
        JavaString* obj = ctpInfo->resolveString(utf8, idx);
        Constant* C = getString(obj);
        C = ConstantExpr::getBitCast(C, JnjvmModule::JavaObjectType);
        Elts.push_back(C);
      } else {
        fprintf(stderr, "Implement me");
        abort();
      }
    }
  }
   
  return ConstantStruct::get(STy, Elts);
}

Constant* JavaAOTCompiler::getStaticInstance(Class* classDef) {
#ifdef ISOLATE
  assert(0 && "Should not be here");
  abort();
#endif
  static_instance_iterator End = staticInstances.end();
  static_instance_iterator I = staticInstances.find(classDef);
  if (I == End) {
    
    LLVMClassInfo* LCI = getClassInfo(classDef);
    const Type* Ty = LCI->getStaticType();
    Ty = Ty->getContainedType(0);
    GlobalVariable* varGV = 
      new GlobalVariable(Ty, false, GlobalValue::ExternalLinkage,
                         0, classDef->printString("<static>"), getLLVMModule());

    Constant* res = ConstantExpr::getCast(Instruction::BitCast, varGV,
                                          JnjvmModule::ptrType);
    staticInstances.insert(std::make_pair(classDef, res));
    
    if (isCompiling(classDef)) { 
      Constant* C = CreateConstantFromStaticInstance(classDef);
      varGV->setInitializer(C);
    }

    return res;
  } else {
    return I->second;
  }
}

Constant* JavaAOTCompiler::getVirtualTable(Class* classDef) {
  LLVMClassInfo* LCI = getClassInfo((Class*)classDef);
  LCI->getVirtualType();
  llvm::Constant* res = 0;
  virtual_table_iterator End = virtualTables.end();
  virtual_table_iterator I = virtualTables.find(classDef);
  if (I == End) {
    
    const ArrayType* ATy = 
      dyn_cast<ArrayType>(JnjvmModule::VTType->getContainedType(0));
    const PointerType* PTy = dyn_cast<PointerType>(ATy->getContainedType(0));
    ATy = ArrayType::get(PTy, classDef->virtualTableSize);
    // Do not set a virtual table as a constant, because the runtime may
    // modify it.
    GlobalVariable* varGV = new GlobalVariable(ATy, false,
                                               GlobalValue::ExternalLinkage,
                                               0, 
                                               classDef->printString("<VT>"),
                                               getLLVMModule());
  
    res = ConstantExpr::getCast(Instruction::BitCast, varGV,
                                JnjvmModule::VTType);
    virtualTables.insert(std::make_pair(classDef, res));
  
    if (isCompiling(classDef)) {
      Constant* C = CreateConstantFromVT(classDef);
      varGV->setInitializer(C);
    }
    
    return res;
  } else {
    return I->second;
  } 
}

Constant* JavaAOTCompiler::getNativeFunction(JavaMethod* meth, void* ptr) {
  llvm::Constant* varGV = 0;
  native_function_iterator End = nativeFunctions.end();
  native_function_iterator I = nativeFunctions.find(meth);
  if (I == End) {
    
    LLVMSignatureInfo* LSI = getSignatureInfo(meth->getSignature());
    const llvm::Type* valPtrType = LSI->getNativePtrType();
    
    varGV = new GlobalVariable(valPtrType, true,
                               GlobalValue::InternalLinkage,
                               Constant::getNullValue(valPtrType), "", getLLVMModule());
  
    nativeFunctions.insert(std::make_pair(meth, varGV));
    return varGV;
  } else {
    return I->second;
  }
}

Constant* JavaAOTCompiler::CreateConstantForBaseObject(CommonClass* cl) {
  const StructType* STy = 
    dyn_cast<StructType>(JnjvmModule::JavaObjectType->getContainedType(0));
  
  std::vector<Constant*> Elmts;

  // virtual table
  if (cl->isClass()) {
    Elmts.push_back(getVirtualTable(cl->asClass()));
  } else {
    ClassArray* clA = cl->asArrayClass();
    if (clA->baseClass()->isPrimitive()) {
      Elmts.push_back(PrimitiveArrayVT);
    } else {
      Elmts.push_back(ReferenceArrayVT);
    }
  }
  
  // classof
  Constant* Cl = getNativeClass(cl);
  Constant* ClGEPs[2] = { JnjvmModule::constantZero,
                          JnjvmModule::constantZero };
  Cl = ConstantExpr::getGetElementPtr(Cl, ClGEPs, 2);
    
  Elmts.push_back(Cl);

  // lock
  Constant* L = ConstantInt::get(Type::Int64Ty,
                                 mvm::Thread::get()->getThreadID());
  Elmts.push_back(ConstantExpr::getIntToPtr(L, JnjvmModule::ptrType));

  return ConstantStruct::get(STy, Elmts);
}

Constant* JavaAOTCompiler::CreateConstantFromJavaClass(CommonClass* cl) {
  Class* javaClass = cl->classLoader->bootstrapLoader->upcalls->newClass;
  LLVMClassInfo* LCI = (LLVMClassInfo*)getClassInfo(javaClass);
  const StructType* STy = 
    dyn_cast<StructType>(LCI->getVirtualType()->getContainedType(0));

  std::vector<Constant*> Elmts;

  // JavaObject
  Elmts.push_back(CreateConstantForBaseObject(javaClass));
  
  // signers
  Elmts.push_back(Constant::getNullValue(JnjvmModule::JavaObjectType));
  
  // pd
  Elmts.push_back(Constant::getNullValue(JnjvmModule::JavaObjectType));
  
  // vmdata
  Constant* Cl = getNativeClass(cl);
  Cl = ConstantExpr::getCast(Instruction::BitCast, Cl,
                             JnjvmModule::JavaObjectType);
  Elmts.push_back(Cl);

  // constructor
  Elmts.push_back(Constant::getNullValue(JnjvmModule::JavaObjectType));

  return ConstantStruct::get(STy, Elmts);
}

Constant* JavaAOTCompiler::CreateConstantFromJavaObject(JavaObject* obj) {
  CommonClass* cl = obj->getClass();

  if (cl->isArray()) {
    Classpath* upcalls = cl->classLoader->bootstrapLoader->upcalls;
    CommonClass* subClass = cl->asArrayClass()->baseClass();
    if (subClass->isPrimitive()) {
      if (subClass == upcalls->OfBool) {
        return CreateConstantFromArray<ArrayUInt8>((ArrayUInt8*)obj,
                                                   Type::Int8Ty);
      } else if (subClass == upcalls->OfByte) {
        return CreateConstantFromArray<ArraySInt8>((ArraySInt8*)obj,
                                                   Type::Int8Ty);
      } else if (subClass == upcalls->OfShort) {
        return CreateConstantFromArray<ArraySInt16>((ArraySInt16*)obj,
                                                    Type::Int16Ty);
      } else if (subClass == upcalls->OfChar) {
        return CreateConstantFromArray<ArrayUInt16>((ArrayUInt16*)obj,
                                                    Type::Int16Ty);
      } else if (subClass == upcalls->OfInt) {
        return CreateConstantFromArray<ArraySInt32>((ArraySInt32*)obj,
                                                    Type::Int32Ty);
      } else if (subClass == upcalls->OfFloat) {
        return CreateConstantFromArray<ArrayFloat>((ArrayFloat*)obj,
                                                   Type::FloatTy);
      } else if (subClass == upcalls->OfLong) {
        return CreateConstantFromArray<ArrayLong>((ArrayLong*)obj,
                                                  Type::Int64Ty);
      } else if (subClass == upcalls->OfDouble) {
        return CreateConstantFromArray<ArrayDouble>((ArrayDouble*)obj,
                                                    Type::DoubleTy);
      } else {
        abort();
      }
    } else {
      return CreateConstantFromArray<ArrayObject>((ArrayObject*)obj,
                                                  JnjvmModule::JavaObjectType);
    }
  } else {
    
    std::vector<Constant*> Elmts;
    
    // JavaObject
    Constant* CurConstant = CreateConstantForBaseObject(obj->getClass());

    for (uint32 j = 0; j <= cl->depth; ++j) {
      std::vector<Constant*> TempElts;
      Elmts.push_back(CurConstant);
      TempElts.push_back(CurConstant);
      Class* curCl = cl->display[j]->asClass();
      LLVMClassInfo* LCI = getClassInfo(curCl);
      const StructType* STy = 
        dyn_cast<StructType>(LCI->getVirtualType()->getContainedType(0));

      for (uint32 i = 0; i < curCl->nbVirtualFields; ++i) {
        JavaField& field = curCl->virtualFields[i];
        const Typedef* type = field.getSignature();
        if (type->isPrimitive()) {
          const PrimitiveTypedef* prim = (PrimitiveTypedef*)type;
          if (prim->isBool() || prim->isByte()) {
            ConstantInt* CI = ConstantInt::get(Type::Int8Ty,
                                               field.getInt8Field(obj));
            TempElts.push_back(CI);
          } else if (prim->isShort() || prim->isChar()) {
            ConstantInt* CI = ConstantInt::get(Type::Int16Ty,
                                               field.getInt16Field(obj));
            TempElts.push_back(CI);
          } else if (prim->isInt()) {
            ConstantInt* CI = ConstantInt::get(Type::Int32Ty,
                                               field.getInt32Field(obj));
            TempElts.push_back(CI);
          } else if (prim->isLong()) {
            ConstantInt* CI = ConstantInt::get(Type::Int64Ty,
                                               field.getLongField(obj));
            TempElts.push_back(CI);
          } else if (prim->isFloat()) {
            ConstantFP* CF = ConstantFP::get(Type::FloatTy,
                                             field.getFloatField(obj));
            TempElts.push_back(CF);
          } else if (prim->isDouble()) {
            ConstantFP* CF = ConstantFP::get(Type::DoubleTy,
                                             field.getDoubleField(obj));
            TempElts.push_back(CF);
          } else {
            abort();
          }
        } else {
          Constant* C = getFinalObject(field.getObjectField(obj));
          TempElts.push_back(C);
        }
      }
      CurConstant = ConstantStruct::get(STy, TempElts);
    }

    return CurConstant;
  }
}

Constant* JavaAOTCompiler::CreateConstantFromJavaString(JavaString* str) {
  Class* cl = (Class*)str->getClass();
  LLVMClassInfo* LCI = (LLVMClassInfo*)getClassInfo(cl);
  const StructType* STy = 
    dyn_cast<StructType>(LCI->getVirtualType()->getContainedType(0));

  std::vector<Constant*> Elmts;

  Elmts.push_back(CreateConstantForBaseObject(cl));

  Constant* Array = getUTF8(str->value);
  Constant* ObjGEPs[2] = { JnjvmModule::constantZero,
                           JnjvmModule::constantZero };
  Array = ConstantExpr::getGetElementPtr(Array, ObjGEPs, 2);
  Elmts.push_back(Array);
  
  Elmts.push_back(ConstantInt::get(Type::Int32Ty, str->count));
  Elmts.push_back(ConstantInt::get(Type::Int32Ty, str->cachedHashCode));
  Elmts.push_back(ConstantInt::get(Type::Int32Ty, str->offset));
  
  return ConstantStruct::get(STy, Elmts);
}


Constant* JavaAOTCompiler::CreateConstantFromCacheNode(CacheNode* CN) {
  const StructType* STy = 
    dyn_cast<StructType>(JnjvmModule::CacheNodeType->getContainedType(0));

  std::vector<Constant*> Elmts;
  Elmts.push_back(Constant::getNullValue(STy->getContainedType(0)));
  Elmts.push_back(Constant::getNullValue(STy->getContainedType(1)));
  Elmts.push_back(Constant::getNullValue(STy->getContainedType(2)));
  Elmts.push_back(getEnveloppe(CN->enveloppe));
  
  return ConstantStruct::get(STy, Elmts);
}

Constant* JavaAOTCompiler::CreateConstantFromEnveloppe(Enveloppe* val) {
  
  const StructType* STy = 
    dyn_cast<StructType>(JnjvmModule::EnveloppeType->getContainedType(0));
  const StructType* CNTy = 
    dyn_cast<StructType>(JnjvmModule::CacheNodeType->getContainedType(0));
  
  std::vector<Constant*> Elmts;
  
  Constant* firstCache = CreateConstantFromCacheNode(val->firstCache);
  Elmts.push_back(new GlobalVariable(CNTy, false,
                                     GlobalValue::InternalLinkage,
                                     firstCache, "", getLLVMModule()));
  Elmts.push_back(getUTF8(val->methodName));
  Elmts.push_back(getUTF8(val->methodSign));

  Elmts.push_back(Constant::getNullValue(Type::Int8Ty));
  Elmts.push_back(getNativeClass(val->classDef));
  Elmts.push_back(firstCache);

  return ConstantStruct::get(STy, Elmts);
  
}

Constant* JavaAOTCompiler::CreateConstantFromAttribut(Attribut& attribut) {
  const StructType* STy = 
    dyn_cast<StructType>(JnjvmModule::AttributType->getContainedType(0));


  std::vector<Constant*> Elmts;

  // name
  Elmts.push_back(getUTF8(attribut.name));

  // start
  Elmts.push_back(ConstantInt::get(Type::Int32Ty, attribut.start));

  // nbb
  Elmts.push_back(ConstantInt::get(Type::Int32Ty, attribut.nbb));
  
  return ConstantStruct::get(STy, Elmts);
}

Constant* JavaAOTCompiler::CreateConstantFromCommonClass(CommonClass* cl) {
  const StructType* STy = 
    dyn_cast<StructType>(JnjvmModule::JavaCommonClassType->getContainedType(0));

  const ArrayType* ATy = ArrayType::get(JnjvmModule::JavaCommonClassType,
                                        cl->depth + 1);
  
  std::vector<Constant*> CommonClassElts;
  std::vector<Constant*> TempElmts;
  Constant* ClGEPs[2] = { JnjvmModule::constantZero,
                          JnjvmModule::constantZero };

  // display
  for (uint32 i = 0; i <= cl->depth; ++i) {
    Constant* Cl = getNativeClass(cl->display[i]);
    if (Cl->getType() != JnjvmModule::JavaCommonClassType)
      Cl = ConstantExpr::getGetElementPtr(Cl, ClGEPs, 2);
    
    TempElmts.push_back(Cl);
  }

  Constant* display = ConstantArray::get(ATy, TempElmts);
  TempElmts.clear();
  display = new GlobalVariable(ATy, true, GlobalValue::InternalLinkage,
                               display, "", getLLVMModule());

  const llvm::Type* TempTy = JnjvmModule::JavaCommonClassType;
  display = ConstantExpr::getCast(Instruction::BitCast, display,
                                  PointerType::getUnqual(TempTy));
  CommonClassElts.push_back(display);

  // depth
  CommonClassElts.push_back(ConstantInt::get(Type::Int32Ty, cl->depth));
  
  // delegatee
  ATy = dyn_cast<ArrayType>(STy->getContainedType(2));
  assert(ATy && "Malformed type");

  Constant* TCM[1] = { getJavaClass(cl) };
  CommonClassElts.push_back(ConstantArray::get(ATy, TCM, 1));
  
  // access
  CommonClassElts.push_back(ConstantInt::get(Type::Int16Ty, cl->access));
 
  // interfaces
  if (cl->nbInterfaces) {
    for (uint32 i = 0; i < cl->nbInterfaces; ++i) {
      TempElmts.push_back(getNativeClass(cl->interfaces[i]));
    }

    ATy = ArrayType::get(JnjvmModule::JavaClassType, cl->nbInterfaces);
    Constant* interfaces = ConstantArray::get(ATy, TempElmts);
    interfaces = new GlobalVariable(ATy, true, GlobalValue::InternalLinkage,
                                    interfaces, "", getLLVMModule());
    interfaces = ConstantExpr::getCast(Instruction::BitCast, interfaces,
                            PointerType::getUnqual(JnjvmModule::JavaClassType));

    CommonClassElts.push_back(interfaces);
  } else {
    const Type* Ty = PointerType::getUnqual(JnjvmModule::JavaClassType);
    CommonClassElts.push_back(Constant::getNullValue(Ty));
  }

  // nbInterfaces
  CommonClassElts.push_back(ConstantInt::get(Type::Int16Ty, cl->nbInterfaces));

  // name
  CommonClassElts.push_back(getUTF8(cl->name));

  // super
  if (cl->super) {
    CommonClassElts.push_back(getNativeClass(cl->super));
  } else {
    TempTy = JnjvmModule::JavaClassType;
    CommonClassElts.push_back(Constant::getNullValue(TempTy));
  }

  // classLoader: store the static initializer, it will be overriden once
  // the class is loaded.
  Constant* loader = ConstantExpr::getBitCast(StaticInitializer,
                                              JnjvmModule::ptrType);
  CommonClassElts.push_back(loader);
 
  return ConstantStruct::get(STy, CommonClassElts);
}

Constant* JavaAOTCompiler::CreateConstantFromJavaField(JavaField& field) {
  const StructType* STy = 
    dyn_cast<StructType>(JnjvmModule::JavaFieldType->getContainedType(0));
  
  std::vector<Constant*> FieldElts;
  std::vector<Constant*> TempElts;
  
  // signature
  FieldElts.push_back(Constant::getNullValue(JnjvmModule::ptrType));
  
  // access
  FieldElts.push_back(ConstantInt::get(Type::Int16Ty, field.access));

  // name
  FieldElts.push_back(getUTF8(field.name));

  // type
  FieldElts.push_back(getUTF8(field.type));
  
  // attributs 
  if (field.nbAttributs) {
    const llvm::Type* AttrTy = JnjvmModule::AttributType->getContainedType(0);
    const ArrayType* ATy = ArrayType::get(AttrTy, field.nbAttributs);
    for (uint32 i = 0; i < field.nbAttributs; ++i) {
      TempElts.push_back(CreateConstantFromAttribut(field.attributs[i]));
    }

    Constant* attributs = ConstantArray::get(ATy, TempElts);
    TempElts.clear();
    attributs = new GlobalVariable(ATy, true, GlobalValue::InternalLinkage,
                                   attributs, "", getLLVMModule());
    attributs = ConstantExpr::getCast(Instruction::BitCast, attributs,
                                      JnjvmModule::AttributType);
  
    FieldElts.push_back(attributs);
  } else {
    FieldElts.push_back(Constant::getNullValue(JnjvmModule::AttributType));
  }
  
  // nbAttributs
  FieldElts.push_back(ConstantInt::get(Type::Int16Ty, field.nbAttributs));

  // classDef
  FieldElts.push_back(getNativeClass(field.classDef));

  // ptrOffset
  FieldElts.push_back(ConstantInt::get(Type::Int32Ty, field.ptrOffset));

  // num
  FieldElts.push_back(ConstantInt::get(Type::Int16Ty, field.num));

  //JInfo
  FieldElts.push_back(Constant::getNullValue(JnjvmModule::ptrType));
  
  return ConstantStruct::get(STy, FieldElts); 
}

Constant* JavaAOTCompiler::CreateConstantFromJavaMethod(JavaMethod& method) {
  const StructType* STy = 
    dyn_cast<StructType>(JnjvmModule::JavaMethodType->getContainedType(0));
  
  std::vector<Constant*> MethodElts;
  std::vector<Constant*> TempElts;
  
  // signature
  MethodElts.push_back(Constant::getNullValue(JnjvmModule::ptrType));
  
  // access
  MethodElts.push_back(ConstantInt::get(Type::Int16Ty, method.access));
 
  // attributs
  if (method.nbAttributs) {
    const llvm::Type* AttrTy = JnjvmModule::AttributType->getContainedType(0);
    const ArrayType* ATy = ArrayType::get(AttrTy, method.nbAttributs);
    for (uint32 i = 0; i < method.nbAttributs; ++i) {
      TempElts.push_back(CreateConstantFromAttribut(method.attributs[i]));
    }

    Constant* attributs = ConstantArray::get(ATy, TempElts);
    TempElts.clear();
    attributs = new GlobalVariable(ATy, true, GlobalValue::InternalLinkage,
                                   attributs, "", getLLVMModule());
    attributs = ConstantExpr::getCast(Instruction::BitCast, attributs,
                                      JnjvmModule::AttributType);

    MethodElts.push_back(attributs);
  } else {
    MethodElts.push_back(Constant::getNullValue(JnjvmModule::AttributType));
  }
  
  // nbAttributs
  MethodElts.push_back(ConstantInt::get(Type::Int16Ty, method.nbAttributs));
  
  // enveloppes
  // already allocated by the JIT, don't reallocate them.
  MethodElts.push_back(Constant::getNullValue(JnjvmModule::EnveloppeType));
  
  // nbEnveloppes
  // 0 because we're not allocating here.
  MethodElts.push_back(ConstantInt::get(Type::Int16Ty, 0));
  
  // classDef
  MethodElts.push_back(getNativeClass(method.classDef));
  
  // name
  MethodElts.push_back(getUTF8(method.name));

  // type
  MethodElts.push_back(getUTF8(method.type));
  
  // canBeInlined
  MethodElts.push_back(ConstantInt::get(Type::Int8Ty, method.canBeInlined));

  // code
  if (isAbstract(method.access)) {
    MethodElts.push_back(Constant::getNullValue(JnjvmModule::ptrType));
  } else {
    LLVMMethodInfo* LMI = getMethodInfo(&method);
    Function* func = LMI->getMethod();
    MethodElts.push_back(ConstantExpr::getCast(Instruction::BitCast, func,
                                               JnjvmModule::ptrType));
  }

  // offset
  MethodElts.push_back(ConstantInt::get(Type::Int32Ty, method.offset));

  //JInfo
  MethodElts.push_back(Constant::getNullValue(JnjvmModule::ptrType));
  
  return ConstantStruct::get(STy, MethodElts); 
}

Constant* JavaAOTCompiler::CreateConstantFromClassPrimitive(ClassPrimitive* cl) {
  const llvm::Type* JCPTy = 
    JnjvmModule::JavaClassPrimitiveType->getContainedType(0);
  const StructType* STy = dyn_cast<StructType>(JCPTy);
  
  std::vector<Constant*> ClassElts;
  
  // common class
  ClassElts.push_back(CreateConstantFromCommonClass(cl));

  // primSize
  ClassElts.push_back(ConstantInt::get(Type::Int32Ty, cl->primSize));

  return ConstantStruct::get(STy, ClassElts);
}

Constant* JavaAOTCompiler::CreateConstantFromClassArray(ClassArray* cl) {
  const StructType* STy = 
    dyn_cast<StructType>(JnjvmModule::JavaClassArrayType->getContainedType(0));
  
  std::vector<Constant*> ClassElts;
  Constant* ClGEPs[2] = { JnjvmModule::constantZero,
                          JnjvmModule::constantZero };
  
  // common class
  ClassElts.push_back(CreateConstantFromCommonClass(cl));

  // baseClass
  Constant* Cl = getNativeClass(cl->baseClass());
  if (Cl->getType() != JnjvmModule::JavaCommonClassType)
    Cl = ConstantExpr::getGetElementPtr(Cl, ClGEPs, 2);
    
  ClassElts.push_back(Cl);

  return ConstantStruct::get(STy, ClassElts);
}

Constant* JavaAOTCompiler::CreateConstantFromClass(Class* cl) {
  const StructType* STy = 
    dyn_cast<StructType>(JnjvmModule::JavaClassType->getContainedType(0));
  
  std::vector<Constant*> ClassElts;
  std::vector<Constant*> TempElts;

  // common class
  ClassElts.push_back(CreateConstantFromCommonClass(cl));

  // virtualSize
  ClassElts.push_back(ConstantInt::get(Type::Int32Ty, cl->virtualSize));

  // virtualTable
  ClassElts.push_back(getVirtualTable(cl));

  // IsolateInfo
  const ArrayType* ATy = dyn_cast<ArrayType>(STy->getContainedType(3));
  assert(ATy && "Malformed type");
  
  const StructType* TCMTy = dyn_cast<StructType>(ATy->getContainedType(0));
  assert(TCMTy && "Malformed type");

  uint32 status = cl->needsInitialisationCheck() ? vmjc : ready;
  TempElts.push_back(ConstantInt::get(Type::Int8Ty, status));
  TempElts.push_back(ConstantInt::get(Type::Int1Ty, status == ready ? 1 : 0));
  TempElts.push_back(getStaticInstance(cl));
  Constant* CStr[1] = { ConstantStruct::get(TCMTy, TempElts) };
  TempElts.clear();
  ClassElts.push_back(ConstantArray::get(ATy, CStr, 1));

  // thinlock
  ClassElts.push_back(Constant::getNullValue(JnjvmModule::ptrType));

  if (cl->nbVirtualFields + cl->nbStaticFields) {
    ATy = ArrayType::get(JnjvmModule::JavaFieldType->getContainedType(0),
                         cl->nbVirtualFields + cl->nbStaticFields);
  }

  // virtualFields
  if (cl->nbVirtualFields) {

    for (uint32 i = 0; i < cl->nbVirtualFields; ++i) {
      TempElts.push_back(CreateConstantFromJavaField(cl->virtualFields[i]));
    }

  } 
  
  // staticFields
  if (cl->nbStaticFields) {

    for (uint32 i = 0; i < cl->nbStaticFields; ++i) {
      TempElts.push_back(CreateConstantFromJavaField(cl->staticFields[i]));
    }

  }

  Constant* fields = 0;
  if (cl->nbStaticFields + cl->nbVirtualFields) {
  
    fields = ConstantArray::get(ATy, TempElts);
    TempElts.clear();
    fields = new GlobalVariable(ATy, false, GlobalValue::InternalLinkage,
                                fields, "", getLLVMModule());
    fields = ConstantExpr::getCast(Instruction::BitCast, fields,
                                   JnjvmModule::JavaFieldType);
  } else {
    fields = Constant::getNullValue(JnjvmModule::JavaFieldType);
  }

  // virtualFields
  ClassElts.push_back(fields);

  ConstantInt* nbVirtualFields = 
    ConstantInt::get(Type::Int16Ty, cl->nbVirtualFields);
  // nbVirtualFields
  ClassElts.push_back(nbVirtualFields);
  
  // staticFields
  // Output null, getLLVMModule() will be set in  the initializer. Otherwise, the
  // assembly emitter of LLVM will try to align the data.
  ClassElts.push_back(Constant::getNullValue(JnjvmModule::JavaFieldType));

  // nbStaticFields
  ClassElts.push_back(ConstantInt::get(Type::Int16Ty, cl->nbStaticFields));
  
  // virtualMethods
  if (cl->nbVirtualMethods + cl->nbStaticMethods) {
    ATy = ArrayType::get(JnjvmModule::JavaMethodType->getContainedType(0),
                         cl->nbVirtualMethods + cl->nbStaticMethods);
  }

  if (cl->nbVirtualMethods) {
    for (uint32 i = 0; i < cl->nbVirtualMethods; ++i) {
      TempElts.push_back(CreateConstantFromJavaMethod(cl->virtualMethods[i]));
    }
  }
    
  if (cl->nbStaticMethods) {
    for (uint32 i = 0; i < cl->nbStaticMethods; ++i) {
      TempElts.push_back(CreateConstantFromJavaMethod(cl->staticMethods[i]));
    }
  }

  Constant* methods = 0;
  if (cl->nbVirtualMethods + cl->nbStaticMethods) {
    methods = ConstantArray::get(ATy, TempElts);
    TempElts.clear();
    GlobalVariable* GV = new GlobalVariable(ATy, false,
                                            GlobalValue::InternalLinkage,
                                            methods, "", getLLVMModule());
    virtualMethods.insert(std::make_pair(cl, GV));
    methods = ConstantExpr::getCast(Instruction::BitCast, GV,
                                    JnjvmModule::JavaMethodType);
  } else {
    methods = Constant::getNullValue(JnjvmModule::JavaMethodType);
  }

  // virtualMethods
  ClassElts.push_back(methods);

  ConstantInt* nbVirtualMethods = 
    ConstantInt::get(Type::Int16Ty, cl->nbVirtualMethods);
  // nbVirtualMethods
  ClassElts.push_back(nbVirtualMethods);
  
  // staticMethods
  // Output null, getLLVMModule() will be set in  the initializer.
  ClassElts.push_back(Constant::getNullValue(JnjvmModule::JavaMethodType));

  // nbStaticMethods
  ClassElts.push_back(ConstantInt::get(Type::Int16Ty, cl->nbStaticMethods));

  // ownerClass
  ClassElts.push_back(Constant::getNullValue(JnjvmModule::ptrType));
  
  // bytes
  ClassElts.push_back(Constant::getNullValue(JnjvmModule::JavaArrayUInt8Type));

  // ctpInfo
  ClassElts.push_back(Constant::getNullValue(JnjvmModule::ptrType));

  // attributs
  if (cl->nbAttributs) {
    ATy = ArrayType::get(JnjvmModule::AttributType->getContainedType(0),
                         cl->nbAttributs);

    for (uint32 i = 0; i < cl->nbAttributs; ++i) {
      TempElts.push_back(CreateConstantFromAttribut(cl->attributs[i]));
    }

    Constant* attributs = ConstantArray::get(ATy, TempElts);
    TempElts.clear();
    attributs = new GlobalVariable(ATy, true, GlobalValue::InternalLinkage,
                                   attributs, "", getLLVMModule());
    attributs = ConstantExpr::getCast(Instruction::BitCast, attributs,
                                      JnjvmModule::AttributType);
    ClassElts.push_back(attributs);
  } else {
    ClassElts.push_back(Constant::getNullValue(JnjvmModule::AttributType));
  }
  
  // nbAttributs
  ClassElts.push_back(ConstantInt::get(Type::Int16Ty, cl->nbAttributs));
  
  // innerClasses
  if (cl->nbInnerClasses) {
    for (uint32 i = 0; i < cl->nbInnerClasses; ++i) {
      TempElts.push_back(getNativeClass(cl->innerClasses[i]));
    }

    const llvm::Type* TempTy = JnjvmModule::JavaClassType;
    ATy = ArrayType::get(TempTy, cl->nbInnerClasses);
    Constant* innerClasses = ConstantArray::get(ATy, TempElts);
    innerClasses = new GlobalVariable(ATy, true, GlobalValue::InternalLinkage,
                                      innerClasses, "", getLLVMModule());
    innerClasses = ConstantExpr::getCast(Instruction::BitCast, innerClasses,
                                         PointerType::getUnqual(TempTy));

    ClassElts.push_back(innerClasses);
  } else {
    const Type* Ty = PointerType::getUnqual(JnjvmModule::JavaClassType);
    ClassElts.push_back(Constant::getNullValue(Ty));
  }

  // nbInnerClasses
  ClassElts.push_back(ConstantInt::get(Type::Int16Ty, cl->nbInnerClasses));

  // outerClass
  if (cl->outerClass) {
    ClassElts.push_back(getNativeClass(cl->outerClass));
  } else {
    ClassElts.push_back(Constant::getNullValue(JnjvmModule::JavaClassType));
  }

  // innerAccess
  ClassElts.push_back(ConstantInt::get(Type::Int16Ty, cl->innerAccess));
  
  // innerOuterResolved
  ClassElts.push_back(ConstantInt::get(Type::Int8Ty, cl->innerOuterResolved));
  
  // virtualTableSize
  ClassElts.push_back(ConstantInt::get(Type::Int32Ty, cl->virtualTableSize));
  
  // staticSize
  ClassElts.push_back(ConstantInt::get(Type::Int32Ty, cl->staticSize));

  // JInfo
  ClassElts.push_back(Constant::getNullValue(JnjvmModule::ptrType));

  // staticTracer
  const Type* FTy = STy->getContainedType(STy->getNumContainedTypes() - 1);
#ifdef WITH_TRACER
  Function* F = makeTracer(cl, true);
  Constant* staticTracer = ConstantExpr::getCast(Instruction::BitCast, F, FTy);
#else
  Constant* staticTracer = ConstantExpr::getNullValue(FTy);
#endif
  ClassElts.push_back(staticTracer);


  return ConstantStruct::get(STy, ClassElts);
}

template<typename T>
Constant* JavaAOTCompiler::CreateConstantFromArray(T* val, const Type* Ty) {
  std::vector<const Type*> Elemts;
  const ArrayType* ATy = ArrayType::get(Ty, val->size);
  Elemts.push_back(JnjvmModule::JavaObjectType->getContainedType(0));
  Elemts.push_back(JnjvmModule::pointerSizeType);

  Elemts.push_back(ATy);

  const StructType* STy = StructType::get(Elemts);
  
  std::vector<Constant*> Cts;
  Cts.push_back(CreateConstantForBaseObject(val->getClass()));
  Cts.push_back(ConstantInt::get(JnjvmModule::pointerSizeType, val->size));
  
  std::vector<Constant*> Vals;
  for (sint32 i = 0; i < val->size; ++i) {
    if (Ty->isInteger()) {
      Vals.push_back(ConstantInt::get(Ty, (uint64)val->elements[i]));
    } else if (Ty->isFloatingPoint()) {
      Vals.push_back(ConstantFP::get(Ty, (double)(size_t)val->elements[i]));
    } else {
      Vals.push_back(getFinalObject((JavaObject*)(size_t)val->elements[i]));
    }
  }

  Cts.push_back(ConstantArray::get(ATy, Vals));
  
  return ConstantStruct::get(STy, Cts);
}

Constant* JavaAOTCompiler::CreateConstantFromUTF8(const UTF8* val) {
  std::vector<const Type*> Elemts;
  const ArrayType* ATy = ArrayType::get(Type::Int16Ty, val->size);
  Elemts.push_back(JnjvmModule::JavaObjectType->getContainedType(0));
  Elemts.push_back(JnjvmModule::pointerSizeType);

  Elemts.push_back(ATy);

  const StructType* STy = StructType::get(Elemts);
  
  std::vector<Constant*> Cts;
  Cts.push_back(CreateConstantForBaseObject(&ArrayOfChar));
  Cts.push_back(ConstantInt::get(JnjvmModule::pointerSizeType, val->size));
  
  std::vector<Constant*> Vals;
  for (sint32 i = 0; i < val->size; ++i) {
    Vals.push_back(ConstantInt::get(Type::Int16Ty, val->elements[i]));
  }

  Cts.push_back(ConstantArray::get(ATy, Vals));
  
  return ConstantStruct::get(STy, Cts);

}

Constant* JavaAOTCompiler::getUTF8(const UTF8* val) {
  utf8_iterator End = utf8s.end();
  utf8_iterator I = utf8s.find(val);
  if (I == End) {
    Constant* C = CreateConstantFromUTF8(val);
    GlobalVariable* varGV = new GlobalVariable(C->getType(), true,
                                               GlobalValue::InternalLinkage,
                                               C, "", getLLVMModule());
    
    Constant* res = ConstantExpr::getCast(Instruction::BitCast, varGV,
                                          JnjvmModule::UTF8Type);
    utf8s.insert(std::make_pair(val, res));

    return res;
  } else {
    return I->second;
  }
}

Constant* JavaAOTCompiler::CreateConstantFromVT(Class* classDef) {
  uint32 size = classDef->virtualTableSize;
  VirtualTable* VT = classDef->virtualVT;
  const ArrayType* ATy = 
    dyn_cast<ArrayType>(JnjvmModule::VTType->getContainedType(0));
  const PointerType* PTy = dyn_cast<PointerType>(ATy->getContainedType(0));
  ATy = ArrayType::get(PTy, size);

  ConstantPointerNull* N = ConstantPointerNull::get(PTy);
  std::vector<Constant*> Elemts;
   
  // Destructor
  Function* Finalizer = 0;
  JavaMethod* meth = ((JavaMethod**)VT)[0];
  if (meth) {
    LLVMMethodInfo* LMI = getMethodInfo(meth);
    Finalizer = LMI->getMethod();
  }
  Elemts.push_back(Finalizer ? 
      ConstantExpr::getCast(Instruction::BitCast, Finalizer, PTy) : N);
  
  // Delete
  Elemts.push_back(N);
  
  // Tracer
#ifdef WITH_TRACER
  Function* Tracer = makeTracer(classDef, false);
  Elemts.push_back(Tracer ? 
      ConstantExpr::getCast(Instruction::BitCast, Tracer, PTy) : N);
#else
  Elemts.push_back(N);
#endif

  // Printer
  Elemts.push_back(ConstantExpr::getBitCast(ObjectPrinter, PTy));
  
  // Hashcode
  Elemts.push_back(N);  

  for (uint32 i = VT_NB_FUNCS; i < size; ++i) {
    JavaMethod* meth = ((JavaMethod**)VT)[i];
    LLVMMethodInfo* LMI = getMethodInfo(meth);
    Function* F = LMI->getMethod();
    if (isAbstract(meth->access)) {
      Elemts.push_back(Constant::getNullValue(PTy));
    } else {
      Elemts.push_back(ConstantExpr::getCast(Instruction::BitCast, F, PTy));
    }
  }

  Constant* Array = ConstantArray::get(ATy, Elemts);
  
  return Array;
}

#ifdef WITH_TRACER
llvm::Function* JavaAOTCompiler::makeTracer(Class* cl, bool stat) {
  if (!generateTracers) {
    return JavaIntrinsics.JavaObjectTracerFunction;
  } else {
    return internalMakeTracer(cl, stat);
  }
}
#endif

namespace mvm {
  llvm::FunctionPass* createEscapeAnalysisPass(llvm::Function*);
}

namespace jnjvm {
  llvm::FunctionPass* createLowerConstantCallsPass();
}

JavaAOTCompiler::JavaAOTCompiler(const std::string& ModuleID) :
  JavaLLVMCompiler(ModuleID) {
 
  generateTracers = true;
  generateStubs = true;
  assumeCompiled = false;

  const Type* ATy = JnjvmModule::VTType->getContainedType(0);
  PrimitiveArrayVT = new GlobalVariable(ATy, true,
                                        GlobalValue::ExternalLinkage,
                                        0, "JavaArrayVT", getLLVMModule());

  ReferenceArrayVT = new GlobalVariable(ATy, true, 
                                        GlobalValue::ExternalLinkage,
                                        0, "ArrayObjectVT", getLLVMModule());



  ATy = JnjvmModule::JavaClassArrayType->getContainedType(0);
  GlobalVariable* varGV = 0;
  
#define PRIMITIVE_ARRAY(name) \
  varGV = new GlobalVariable(ATy, true, GlobalValue::ExternalLinkage, \
                             0, #name, getLLVMModule()); \
  arrayClasses.insert(std::make_pair(&name, varGV));
  
  PRIMITIVE_ARRAY(ArrayOfBool)
  PRIMITIVE_ARRAY(ArrayOfByte)
  PRIMITIVE_ARRAY(ArrayOfChar)
  PRIMITIVE_ARRAY(ArrayOfShort)
  PRIMITIVE_ARRAY(ArrayOfInt)
  PRIMITIVE_ARRAY(ArrayOfFloat)
  PRIMITIVE_ARRAY(ArrayOfDouble)
  PRIMITIVE_ARRAY(ArrayOfLong)

#undef PRIMITIVE_ARRAY

  std::vector<const llvm::Type*> llvmArgs;
  llvmArgs.push_back(JnjvmModule::ptrType); // class loader.
  const FunctionType* FTy = FunctionType::get(Type::VoidTy, llvmArgs, false);

  StaticInitializer = Function::Create(FTy, GlobalValue::InternalLinkage,
                                       "Init", getLLVMModule());

  llvmArgs.clear();
  llvmArgs.push_back(JnjvmModule::JavaMethodType);
  
  FTy = FunctionType::get(JnjvmModule::ptrType, llvmArgs, false);

  NativeLoader = Function::Create(FTy, GlobalValue::ExternalLinkage,
                                  "vmjcNativeLoader", getLLVMModule());
  
  llvmArgs.clear();
  FTy = FunctionType::get(Type::VoidTy, llvmArgs, false);
  ObjectPrinter = Function::Create(FTy, GlobalValue::ExternalLinkage,
                                   "printJavaObject", getLLVMModule());

  TheModuleProvider = new ExistingModuleProvider(TheModule);
  addJavaPasses();
      
}

void JavaAOTCompiler::printStats() {
  fprintf(stderr, "----------------- Info from the module -----------------\n");
  fprintf(stderr, "Number of native classes            : %llu\n", 
          (unsigned long long int) nativeClasses.size());
  fprintf(stderr, "Number of Java classes              : %llu\n",
          (unsigned long long int) javaClasses.size());
  fprintf(stderr, "Number of external array classes    : %llu\n",
          (unsigned long long int) arrayClasses.size());
  fprintf(stderr, "Number of virtual tables            : %llu\n", 
          (unsigned long long int) virtualTables.size());
  fprintf(stderr, "Number of static instances          : %llu\n", 
          (unsigned long long int) staticInstances.size());
  fprintf(stderr, "Number of constant pools            : %llu\n", 
          (unsigned long long int) constantPools.size());
  fprintf(stderr, "Number of strings                   : %llu\n", 
          (unsigned long long int) strings.size());
  fprintf(stderr, "Number of enveloppes                : %llu\n", 
          (unsigned long long int) enveloppes.size());
  fprintf(stderr, "Number of native functions          : %llu\n", 
          (unsigned long long int) nativeFunctions.size());
  fprintf(stderr, "----------------- Total size in .data ------------------\n");
  uint64 size = 0;
  Module* Mod = getLLVMModule();
  for (Module::const_global_iterator i = Mod->global_begin(),
       e = Mod->global_end(); i != e; ++i) {
    size += JnjvmModule::getTypeSize(i->getType());
  }
  fprintf(stderr, "%lluB\n", (unsigned long long int)size);
}


#ifdef SERVICE
Value* JavaAOTCompiler::getIsolate(Jnjvm* isolate, Value* Where) {
  llvm::Constant* varGV = 0;
  isolate_iterator End = isolates.end();
  isolate_iterator I = isolates.find(isolate);
  if (I == End) {
  
    
    Constant* cons = 
      ConstantExpr::getIntToPtr(ConstantInt::get(Type::Int64Ty,
                                                 uint64_t(isolate)),
                                ptrType);

    varGV = new GlobalVariable(ptrType, !staticCompilation,
                               GlobalValue::ExternalLinkage,
                               cons, "", getLLVMModule());
  
    isolates.insert(std::make_pair(isolate, varGV));
  } else {
    varGV = I->second;
  }
  if (BasicBlock* BB = dyn_cast<BasicBlock>(Where)) {
    return new LoadInst(varGV, "", BB);
  } else {
    assert(dyn_cast<Instruction>(Where) && "Wrong use of module");
    return new LoadInst(varGV, "", dyn_cast<Instruction>(Where));
  }
}
#endif

void JavaAOTCompiler::CreateStaticInitializer() {

  std::vector<const llvm::Type*> llvmArgs;
  llvmArgs.push_back(JnjvmModule::ptrType); // class loader
  llvmArgs.push_back(JnjvmModule::JavaCommonClassType); // cl
  const FunctionType* FTy = FunctionType::get(Type::VoidTy, llvmArgs, false);

  Function* AddClass = Function::Create(FTy, GlobalValue::ExternalLinkage,
                                        "vmjcAddPreCompiledClass", getLLVMModule());
 
  llvmArgs.clear();
  // class loader
  llvmArgs.push_back(JnjvmModule::ptrType);
  // array ptr
  llvmArgs.push_back(PointerType::getUnqual(JnjvmModule::JavaClassArrayType));
  // name
  llvmArgs.push_back(JnjvmModule::UTF8Type);
  FTy = FunctionType::get(Type::VoidTy, llvmArgs, false);
  
  Function* GetClassArray = Function::Create(FTy, GlobalValue::ExternalLinkage,
                                             "vmjcGetClassArray", getLLVMModule());
  
  BasicBlock* currentBlock = BasicBlock::Create("enter", StaticInitializer);
  Function::arg_iterator loader = StaticInitializer->arg_begin();
  
  Value* Args[3];
  // If we have defined some strings.
  if (strings.begin() != strings.end()) {
    llvmArgs.clear();
    llvmArgs.push_back(JnjvmModule::ptrType); // class loader
    llvmArgs.push_back(strings.begin()->second->getType()); // val
    FTy = FunctionType::get(Type::VoidTy, llvmArgs, false);
  
    Function* AddString = Function::Create(FTy, GlobalValue::ExternalLinkage,
                                           "vmjcAddString", getLLVMModule());
  

  
    for (string_iterator i = strings.begin(), e = strings.end(); i != e; ++i) {
      Args[0] = loader;
      Args[1] = i->second;
      CallInst::Create(AddString, Args, Args + 2, "", currentBlock);
    }
  }
  
  for (native_class_iterator i = nativeClasses.begin(), 
       e = nativeClasses.end(); i != e; ++i) {
    if (isCompiling(i->first)) {
      Args[0] = loader;
      Args[1] = ConstantExpr::getBitCast(i->second,
                                         JnjvmModule::JavaCommonClassType);
      CallInst::Create(AddClass, Args, Args + 2, "", currentBlock);
    }
  }
  
  for (array_class_iterator i = arrayClasses.begin(), 
       e = arrayClasses.end(); i != e; ++i) {
    if (!(i->first->baseClass()->isPrimitive())) {
      Args[0] = loader;
      Args[1] = i->second;
      Args[2] = getUTF8(i->first->name);
      CallInst::Create(GetClassArray, Args, Args + 3, "", currentBlock);
    }
  }
  

  ReturnInst::Create(currentBlock);
}

void JavaAOTCompiler::setNoInline(Class* cl) {
  for (uint32 i = 0; i < cl->nbVirtualMethods; ++i) {
    JavaMethod& meth = cl->virtualMethods[i];
    if (!isAbstract(meth.access)) {
      LLVMMethodInfo* LMI = getMethodInfo(&meth);
      Function* func = LMI->getMethod();
      func->addFnAttr(Attribute::NoInline);
    }
  }
  
  for (uint32 i = 0; i < cl->nbStaticMethods; ++i) {
    JavaMethod& meth = cl->staticMethods[i];
    if (!isAbstract(meth.access)) {
      LLVMMethodInfo* LMI = getMethodInfo(&meth);
      Function* func = LMI->getMethod();
      func->addFnAttr(Attribute::NoInline);
    }
  }
}

void JavaAOTCompiler::makeVT(Class* cl) {
  internalMakeVT(cl);
#ifndef WITHOUT_VTABLE
  VirtualTable* VT = cl->virtualVT;
  for (uint32 i = 0; i < cl->nbVirtualMethods; ++i) {
    JavaMethod& meth = cl->virtualMethods[i];
    ((void**)VT)[meth.offset] = &meth;
  }
  if (!cl->super) ((void**)VT)[0] = 0;
#endif 
}

void JavaAOTCompiler::setMethod(JavaMethod* meth, void* ptr, const char* name) {
  Function* func = getMethodInfo(meth)->getMethod();
  func->setName(name);
  func->setLinkage(GlobalValue::ExternalLinkage);
}

Function* JavaAOTCompiler::addCallback(Class* cl, uint16 index,
                                      Signdef* sign, bool stat) {
 
  fprintf(stderr, "Warning: emitting a callback from %s (%d)\n",
          cl->printString(), index);
  Function* func = 0;
  LLVMSignatureInfo* LSI = getSignatureInfo(sign);
  
  const FunctionType* type = stat ? LSI->getStaticType() : 
                                    LSI->getVirtualType();
  
  func = Function::Create(type, GlobalValue::ExternalLinkage, "staticCallback",
                          TheModule);
  
  return func;
}

void JavaAOTCompiler::compileClass(Class* cl) {
  
  // Make sure the class is emitted.
  getNativeClass(cl);

  for (uint32 i = 0; i < cl->nbVirtualMethods; ++i) {
    JavaMethod& meth = cl->virtualMethods[i];
    if (!isAbstract(meth.access)) parseFunction(&meth);
    if (generateStubs) compileAllStubs(meth.getSignature());
  }
  
  for (uint32 i = 0; i < cl->nbStaticMethods; ++i) {
    JavaMethod& meth = cl->staticMethods[i];
    if (!isAbstract(meth.access)) parseFunction(&meth);
    if (generateStubs) compileAllStubs(meth.getSignature());
  }
}

static const char* name;

void mainCompilerStart(JavaThread* th) {
  
  Jnjvm* vm = th->getJVM();
  JnjvmBootstrapLoader* bootstrapLoader = vm->bootstrapLoader;
  JavaAOTCompiler* M = (JavaAOTCompiler*)bootstrapLoader->getCompiler();
  try {

    bootstrapLoader->analyseClasspathEnv(vm->classpath);
    bootstrapLoader->upcalls->initialiseClasspath(bootstrapLoader);
  
    uint32 size = strlen(name);
    
    if (size > 4 && 
       (!strcmp(&name[size - 4], ".jar") || !strcmp(&name[size - 4], ".zip"))) {
  

      std::vector<Class*> classes;

      ArrayUInt8* bytes = Reader::openFile(bootstrapLoader, name);
      if (!bytes) vm->unknownError("Can't find zip file.");
      ZipArchive archive(bytes, bootstrapLoader->allocator);
    
      char* realName = (char*)alloca(4096);
      for (ZipArchive::table_iterator i = archive.filetable.begin(), 
           e = archive.filetable.end(); i != e; ++i) {
        ZipFile* file = i->second;
      
        size = strlen(file->filename);
        if (size > 6 && !strcmp(&(file->filename[size - 6]), ".class")) {
          UserClassArray* array = bootstrapLoader->upcalls->ArrayOfByte;
          ArrayUInt8* res = 
            (ArrayUInt8*)array->doNew(file->ucsize, bootstrapLoader->allocator);
          int ok = archive.readFile(res, file);
          if (!ok) vm->unknownError("Wrong zip file.");
      
        
          memcpy(realName, file->filename, size);
          realName[size - 6] = 0;
          const UTF8* utf8 = bootstrapLoader->asciizConstructUTF8(realName);
          Class* cl = bootstrapLoader->constructClass(utf8, res);
          classes.push_back(cl);  
        }
      }

      // First resolve everyone so that there can not be unknown references in
      // constant pools.
      for (std::vector<Class*>::iterator i = classes.begin(),
           e = classes.end(); i != e; ++i) {
        Class* cl = *i;
        cl->resolveClass();
        cl->setOwnerClass(JavaThread::get());
        
        for (uint32 i = 0; i < cl->nbVirtualMethods; ++i) {
          LLVMMethodInfo* LMI = M->getMethodInfo(&cl->virtualMethods[i]);
          LMI->getMethod();
        }

        for (uint32 i = 0; i < cl->nbStaticMethods; ++i) {
          LLVMMethodInfo* LMI = M->getMethodInfo(&cl->staticMethods[i]);
          LMI->getMethod();
        }

      }
      
      for (std::vector<Class*>::iterator i = classes.begin(), e = classes.end();
           i != e; ++i) {
        Class* cl = *i;
        M->compileClass(cl);
      }

    } else {
      char* realName = (char*)alloca(size + 1);
      if (size > 6 && !strcmp(&name[size - 6], ".class")) {
        memcpy(realName, name, size - 6);
        realName[size - 6] = 0;
      } else {
        memcpy(realName, name, size + 1);
      }

      const UTF8* utf8 = bootstrapLoader->asciizConstructUTF8(realName);
      UserClass* cl = bootstrapLoader->loadName(utf8, true, true);
      cl->setOwnerClass(JavaThread::get());
      M->compileClass(cl);
    }
   
    M->CreateStaticInitializer();

  } catch(std::string str) {
    fprintf(stderr, "Error : %s\n", str.c_str());
  }
  
#define SET_INLINE(NAME) { \
  const UTF8* name = vm->asciizToUTF8(NAME); \
  Class* cl = (Class*)vm->bootstrapLoader->lookupClass(name); \
  if (cl) M->setNoInline(cl); }

  SET_INLINE("java/util/concurrent/atomic/AtomicReferenceFieldUpdater")
  SET_INLINE("java/util/concurrent/atomic/AtomicReferenceFieldUpdater"
             "$AtomicReferenceFieldUpdaterImpl")
  SET_INLINE("java/util/concurrent/atomic/AtomicIntegerFieldUpdater")
  SET_INLINE("java/util/concurrent/atomic/AtomicIntegerFieldUpdater"
             "$AtomicIntegerFieldUpdaterImpl")
  SET_INLINE("java/util/concurrent/atomic/AtomicLongFieldUpdater")
  SET_INLINE("java/util/concurrent/atomic/AtomicLongFieldUpdater"
             "$CASUpdater")
  SET_INLINE("java/util/concurrent/atomic/AtomicLongFieldUpdater"
             "$LockedUpdater")
#undef SET_INLINE

  vm->threadSystem.nonDaemonLock.lock();
  --(vm->threadSystem.nonDaemonThreads);
  if (vm->threadSystem.nonDaemonThreads == 0)
      vm->threadSystem.nonDaemonVar.signal();
  vm->threadSystem.nonDaemonLock.unlock();  
  
}

void JavaAOTCompiler::compileFile(JnjvmClassLoader* JCL, const char* n) {
  name = n;
  mvm::BumpPtrAllocator A;
  Jnjvm* vm = new(A) Jnjvm(A, (JnjvmBootstrapLoader*)JCL);
  JavaThread* th = new JavaThread(0, 0, vm);
  vm->setBootstrapThread(th);
  th->start((void (*)(mvm::Thread*))mainCompilerStart);
  vm->waitForExit();
}

/// compileAllStubs - Compile all the native -> Java stubs. 
/// TODO: Once LLVM supports va_arg, enable AP.
///
void JavaAOTCompiler::compileAllStubs(Signdef* sign) {
  sign->getStaticCallBuf();
  // getStaticCallAP();
  sign->getVirtualCallBuf();
  // getVirtualCallAP();
}

void JavaAOTCompiler::generateMain(const char* name, bool jit) {

  // Type Definitions
  std::vector<const Type*> FuncArgs;
  FuncArgs.push_back(Type::Int32Ty);
  FuncArgs.push_back(PointerType::getUnqual(JavaIntrinsics.ptrType));
  
  FunctionType* FuncTy = FunctionType::get(Type::Int32Ty, FuncArgs, false);

  Function* MainFunc = Function::Create(FuncTy, GlobalValue::ExternalLinkage,
                                        "main", TheModule);
  BasicBlock* currentBlock = BasicBlock::Create("enter", MainFunc);
 
  GlobalVariable* GvarArrayStr = new GlobalVariable(
    ArrayType::get(Type::Int8Ty, strlen(name) + 1), true,
    GlobalValue::InternalLinkage, 0, "mainClass", TheModule);


  Constant* NameArray = ConstantArray::get(name, true);
  GvarArrayStr->setInitializer(NameArray);
  Value* Indices[2] = { JavaIntrinsics.constantZero,
                        JavaIntrinsics.constantZero };
  Value* ArgName = ConstantExpr::getGetElementPtr(GvarArrayStr, Indices, 2);

  Function::arg_iterator FuncVals = MainFunc->arg_begin();
  Value* Argc = FuncVals++;
  Value* Argv = FuncVals++;
  Value* Args[3] = { Argc, Argv, ArgName };

  FuncArgs.push_back(Args[2]->getType());

  FuncTy = FunctionType::get(Type::Int32Ty, FuncArgs, false);

  Function* CalledFunc = 
    Function::Create(FuncTy, GlobalValue::ExternalLinkage,
                     jit ? "StartJnjvmWithJIT" : "StartJnjvmWithoutJIT",
                     TheModule);

  Value* res = CallInst::Create(CalledFunc, Args, Args + 3, "", currentBlock);
  ReturnInst::Create(res, currentBlock);

}
