//===----- JavaAOTCompiler.cpp - Support for Ahead of Time Compiler --------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/BasicBlock.h"
#include "llvm/Constants.h"
#include "llvm/Instructions.h"
#include "llvm/LLVMContext.h"
#include "llvm/Module.h"
#include "llvm/PassManager.h"
#include "llvm/Support/raw_ostream.h"

#include "mvm/Threads/Thread.h"

#include "j3/J3Intrinsics.h"
#include "j3/JavaAOTCompiler.h"
#include "j3/JavaJITCompiler.h"

#include "JavaArray.h"
#include "JavaConstantPool.h"
#include "JavaString.h"
#include "JavaThread.h"
#include "JavaTypes.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"
#include "Reader.h"
#include "Zip.h"

#include <cstdio>

using namespace j3;
using namespace llvm;

bool JavaAOTCompiler::isCompiling(const CommonClass* cl) const {
  if (cl->isClass()) {
    // A class is being static compiled if owner class is not null.
    return cl->asClass()->getOwnerClass() != 0;
  } else if (cl->isArray()) {
    // Only compile an aray if we are compiling rt.jar.
    return compileRT;
  } else if (cl->isPrimitive() && compileRT) {
    return true;
  } else {
    return false;
  }
}

Constant* JavaAOTCompiler::getNativeClass(CommonClass* classDef) {

  if (classDef->isClass() || isCompiling(classDef) || assumeCompiled) {

    native_class_iterator End = nativeClasses.end();
    native_class_iterator I = nativeClasses.find(classDef);
    if (I == End) {
      const llvm::Type* Ty = 0;
      
      if (classDef->isArray()) {
        Ty = JavaIntrinsics.JavaClassArrayType->getContainedType(0); 
      } else if (classDef->isPrimitive()) {
        Ty = JavaIntrinsics.JavaClassPrimitiveType->getContainedType(0); 
      } else {
        Ty = JavaIntrinsics.JavaClassType->getContainedType(0); 
      }
    
      GlobalVariable* varGV =
        new GlobalVariable(*getLLVMModule(), Ty, false,
                        GlobalValue::ExternalLinkage, 0,
                        UTF8Buffer(classDef->name).toCompileName()->cString());
    
      nativeClasses.insert(std::make_pair(classDef, varGV));

      if (classDef->isClass() && isCompiling(classDef)) {
        Constant* C = CreateConstantFromClass(classDef->asClass());
        varGV->setInitializer(C);
      } else if (classDef->isArray()) {
        Constant* C = CreateConstantFromClassArray(classDef->asArrayClass());
        varGV->setInitializer(C);
      } else if (classDef->isPrimitive()) {
        Constant* C = 
          CreateConstantFromClassPrimitive(classDef->asPrimitiveClass());
        varGV->setInitializer(C);
      }

      return varGV;

    } else {
      return I->second;
    }
  } else if (classDef->isArray()) {
    array_class_iterator End = arrayClasses.end();
    array_class_iterator I = arrayClasses.find(classDef->asArrayClass());
    if (I == End) {
      const llvm::Type* Ty = JavaIntrinsics.JavaClassArrayType;
      Module& Mod = *getLLVMModule();
    
      GlobalVariable* varGV = 
        new GlobalVariable(Mod, Ty, false, GlobalValue::InternalLinkage,
                           Constant::getNullValue(Ty),
                        UTF8Buffer(classDef->name).toCompileName()->cString());
    
      arrayClasses.insert(std::make_pair(classDef->asArrayClass(), varGV));
      return varGV;
    } else {
      return I->second;
    }
  } else {
    assert(0 && "Implement me");
  }
  return 0;
}

Constant* JavaAOTCompiler::getConstantPool(JavaConstantPool* ctp) {
  llvm::Constant* varGV = 0;
  constant_pool_iterator End = constantPools.end();
  constant_pool_iterator I = constantPools.find(ctp);
  if (I == End) {
    const Type* Ty = JavaIntrinsics.ConstantPoolType->getContainedType(0);
    Module& Mod = *getLLVMModule();
    
    varGV = new GlobalVariable(Mod, Ty, false,
                               GlobalValue::InternalLinkage,
                               Constant::getNullValue(Ty), "");
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

  for (uint32 i = 0; i < cl->nbVirtualMethods + cl->nbStaticMethods; ++i) {
    if (&cl->virtualMethods[i] == meth) {
      MOffset = ConstantInt::get(Type::getInt32Ty(getLLVMContext()), i);
      break;
    }
  }
  assert(MOffset && "No offset for method");

  method_iterator SI = virtualMethods.find(cl);
  if (SI != virtualMethods.end()) {
    Array = SI->second; 
    assert(Array && "No array in class");
  } else {
    std::string name(UTF8Buffer(cl->name).toCompileName()->cString());
    name += "_VirtualMethods";
    Module& Mod = *getLLVMModule();
    const Type* ATy =
      ArrayType::get(JavaIntrinsics.JavaMethodType->getContainedType(0),
                     cl->nbVirtualMethods + cl->nbStaticMethods);

    Array = new GlobalVariable(Mod, ATy, false, GlobalValue::ExternalLinkage,
                               0, name);
    virtualMethods.insert(std::make_pair(cl, Array));
  }
    
  Constant* GEPs[2] = { getIntrinsics()->constantZero, MOffset };
  return ConstantExpr::getGetElementPtr(Array, GEPs, 2);
}

Constant* JavaAOTCompiler::getString(JavaString* str) {
  string_iterator SI = strings.find(str);
  if (SI != strings.end()) {
    return SI->second;
  } else {
    assert(str && "No string given");
    LLVMClassInfo* LCI = getClassInfo(JavaObject::getClass(str)->asClass());
    const llvm::Type* Ty = LCI->getVirtualType();
    Module& Mod = *getLLVMModule();
    
    GlobalVariable* varGV = 
      new GlobalVariable(Mod, Ty->getContainedType(0), false,
                         GlobalValue::InternalLinkage, 0, "");
    Constant* res = ConstantExpr::getCast(Instruction::BitCast, varGV,
                                          JavaIntrinsics.JavaObjectType);
    strings.insert(std::make_pair(str, res));
    Constant* C = CreateConstantFromJavaString(str);
    varGV->setInitializer(C);
    return res;
  }
}

Constant* JavaAOTCompiler::getStringPtr(JavaString** str) {
  fprintf(stderr, "Implement me");
  abort();
}

Constant* JavaAOTCompiler::getJavaClass(CommonClass* cl) {
  java_class_iterator End = javaClasses.end();
  java_class_iterator I = javaClasses.find(cl);
  if (I == End) {
    final_object_iterator End = finalObjects.end();
    final_object_iterator I = finalObjects.find(cl->delegatee[0]);
    if (I == End) {
    
      Class* javaClass = cl->classLoader->bootstrapLoader->upcalls->newClass;
      LLVMClassInfo* LCI = getClassInfo(javaClass);
      const llvm::Type* Ty = LCI->getVirtualType();
      Module& Mod = *getLLVMModule();
    
      GlobalVariable* varGV = 
        new GlobalVariable(Mod, Ty->getContainedType(0), false,
                           GlobalValue::InternalLinkage, 0, "");
    
      Constant* res = ConstantExpr::getCast(Instruction::BitCast, varGV,
                                            JavaIntrinsics.JavaObjectType);
  
      javaClasses.insert(std::make_pair(cl, res));
      varGV->setInitializer(CreateConstantFromJavaClass(cl));
      return res;
    } else {
      return I->second;
    }
  } else {
    return I->second;
  }
}

Constant* JavaAOTCompiler::getJavaClassPtr(CommonClass* cl) {
#ifdef ISOLATE
  abort();
  return 0;
#else
  // Make sure it's emitted.
  getJavaClass(cl);

  Constant* Cl = getNativeClass(cl);
  Cl = ConstantExpr::getBitCast(Cl, JavaIntrinsics.JavaCommonClassType);

  Constant* GEP[2] = { getIntrinsics()->constantZero,
                       getIntrinsics()->constantZero };
  
  Constant* TCMArray = ConstantExpr::getGetElementPtr(Cl, GEP, 2);
    
  Constant* GEP2[2] = { getIntrinsics()->constantZero,
                        getIntrinsics()->constantZero };

  Constant* Ptr = ConstantExpr::getGetElementPtr(TCMArray, GEP2, 2);
  return Ptr;
#endif
}

JavaObject* JavaAOTCompiler::getFinalObject(llvm::Value* obj) {
  if (Constant* CI = dyn_cast<Constant>(obj)) {
    reverse_final_object_iterator End = reverseFinalObjects.end();
    reverse_final_object_iterator I = reverseFinalObjects.find(CI);
    if (I != End) return I->second;
  }
  
  return 0;
}

Constant* JavaAOTCompiler::HandleMagic(JavaObject* obj, CommonClass* objCl) {

  static const UTF8* AddressArray = objCl->classLoader->asciizConstructUTF8("org/vmmagic/unboxed/AddressArray");
  static const UTF8* WordArray = objCl->classLoader->asciizConstructUTF8("org/vmmagic/unboxed/WordArray");
  static const UTF8* ExtentArray = objCl->classLoader->asciizConstructUTF8("org/vmmagic/unboxed/ExtentArray");
  static const UTF8* ObjectReferenceArray = objCl->classLoader->asciizConstructUTF8("org/vmmagic/unboxed/ObjectReferenceArray");
  static const UTF8* OffsetArray = objCl->classLoader->asciizConstructUTF8("org/vmmagic/unboxed/OffsetArray");
  const UTF8* name = objCl->name;

  if (name->equals(AddressArray) || name->equals(WordArray) ||
      name->equals(ExtentArray) || name->equals(ObjectReferenceArray) || 
      name->equals(OffsetArray)) {
    
    intptr_t* realObj = (intptr_t*)obj;
    intptr_t size = realObj[0];

    const ArrayType* ATy = ArrayType::get(JavaIntrinsics.JavaObjectType,
                                          size + 1);
  
    std::vector<Constant*> Vals;
    for (sint32 i = 0; i < size + 1; ++i) {
      Constant* CI = ConstantInt::get(Type::getInt64Ty(getLLVMContext()),
                                      uint64_t(realObj[i]));
      CI = ConstantExpr::getIntToPtr(CI, JavaIntrinsics.JavaObjectType);
      Vals.push_back(CI);
    }

    Constant* CA = ConstantArray::get(ATy, Vals);
  
    GlobalVariable* varGV = new GlobalVariable(*getLLVMModule(), CA->getType(),
                                               false,
                                               GlobalValue::InternalLinkage,
                                               CA, "");
 
    return ConstantExpr::getBitCast(varGV, JavaIntrinsics.JavaObjectType);

  } else {
    Constant* CI = ConstantInt::get(Type::getInt64Ty(getLLVMContext()),
                                    uint64_t(obj));
    CI = ConstantExpr::getIntToPtr(CI, JavaIntrinsics.JavaObjectType);
    return CI;
  }
}


Constant* JavaAOTCompiler::getFinalObject(JavaObject* obj, CommonClass* objCl) {
  llvm::GlobalVariable* varGV = 0;
  final_object_iterator End = finalObjects.end();
  final_object_iterator I = finalObjects.find(obj);
  if (I == End) {
  
    if (mvm::Collector::begOf(obj)) {
      const Type* Ty = 0;
      CommonClass* cl = JavaObject::getClass(obj);
      
      if (cl->isArray()) {
        Classpath* upcalls = cl->classLoader->bootstrapLoader->upcalls;
        CommonClass* subClass = cl->asArrayClass()->baseClass();
        if (subClass->isPrimitive()) {
          if (subClass == upcalls->OfBool) {
            Ty = Type::getInt8Ty(getLLVMContext());
          } else if (subClass == upcalls->OfByte) {
            Ty = Type::getInt8Ty(getLLVMContext());
          } else if (subClass == upcalls->OfShort) {
            Ty = Type::getInt16Ty(getLLVMContext());
          } else if (subClass == upcalls->OfChar) {
            Ty = Type::getInt16Ty(getLLVMContext());
          } else if (subClass == upcalls->OfInt) {
            Ty = Type::getInt32Ty(getLLVMContext());
          } else if (subClass == upcalls->OfFloat) {
            Ty = Type::getFloatTy(getLLVMContext());
          } else if (subClass == upcalls->OfLong) {
            Ty = Type::getInt64Ty(getLLVMContext());
          } else if (subClass == upcalls->OfDouble) {
            Ty = Type::getDoubleTy(getLLVMContext());
          } else {
            abort();
          }
        } else {
          Ty = JavaIntrinsics.JavaObjectType;
        }
        
        std::vector<const Type*> Elemts;
        const ArrayType* ATy = ArrayType::get(Ty, JavaArray::getSize(obj));
        Elemts.push_back(JavaIntrinsics.JavaObjectType->getContainedType(0));
        Elemts.push_back(JavaIntrinsics.pointerSizeType);
        Elemts.push_back(ATy);
        Ty = StructType::get(getLLVMModule()->getContext(), Elemts);

      } else {
        LLVMClassInfo* LCI = getClassInfo(cl->asClass());
        Ty = LCI->getVirtualType()->getContainedType(0);
      }

      Module& Mod = *getLLVMModule();
      // Set as External, so that inlining MMTk code works.
      varGV = new GlobalVariable(Mod, Ty, false, GlobalValue::ExternalLinkage,
                                 0, "finalObject");

      Constant* C = ConstantExpr::getBitCast(varGV,
                                             JavaIntrinsics.JavaObjectType);
  
      finalObjects.insert(std::make_pair(obj, C));
      reverseFinalObjects.insert(std::make_pair(C, obj));
    
      varGV->setInitializer(CreateConstantFromJavaObject(obj));
      return C;
    } else {
      Constant* CI = HandleMagic(obj, objCl);
      finalObjects.insert(std::make_pair(obj, CI));
      return CI;
    }
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
      if (cl->getStaticInstance() != NULL) {
        if (type->isPrimitive()) {
          const PrimitiveTypedef* prim = (const PrimitiveTypedef*)type;
          if (prim->isBool() || prim->isByte()) {
            ConstantInt* CI = ConstantInt::get(
                Type::getInt8Ty(getLLVMContext()),
                field.getStaticInt8Field());
            Elts.push_back(CI);
          } else if (prim->isShort() || prim->isChar()) {
            ConstantInt* CI = ConstantInt::get(
                Type::getInt16Ty(getLLVMContext()),
                field.getStaticInt16Field());
            Elts.push_back(CI);
          } else if (prim->isInt()) {
            ConstantInt* CI = ConstantInt::get(
                Type::getInt32Ty(getLLVMContext()),
                field.getStaticInt32Field());
            Elts.push_back(CI);
          } else if (prim->isLong()) {
            ConstantInt* CI = ConstantInt::get(
                Type::getInt64Ty(getLLVMContext()),
                field.getStaticLongField());
            Elts.push_back(CI);
          } else if (prim->isFloat()) {
            Constant* CF = ConstantFP::get(
                Type::getFloatTy(getLLVMContext()),
                field.getStaticFloatField());
            Elts.push_back(CF);
          } else if (prim->isDouble()) {
            Constant* CF = ConstantFP::get(
                Type::getDoubleTy(getLLVMContext()),
                field.getStaticDoubleField());
            Elts.push_back(CF);
          } else {
            abort();
          }
        } else {
          JavaObject* val = field.getStaticObjectField();
          if (val) {
            JnjvmClassLoader* JCL = cl->classLoader;
            CommonClass* FieldCl = field.getSignature()->assocClass(JCL);
            Constant* CO = getFinalObject(val, FieldCl);
            Elts.push_back(CO);
          } else {
            Elts.push_back(Constant::getNullValue(Ty));
          }
        }
      } else {
        Elts.push_back(Constant::getNullValue(Ty));
      }
    } else {
      Reader reader(attribut, &(cl->bytes));
      JavaConstantPool * ctpInfo = cl->ctpInfo;
      uint16 idx = reader.readU2();
      if (type->isPrimitive()) {
        if (Ty == Type::getInt64Ty(getLLVMContext())) {
          Elts.push_back(ConstantInt::get(Ty, (uint64)ctpInfo->LongAt(idx)));
        } else if (Ty == Type::getDoubleTy(getLLVMContext())) {
          Elts.push_back(ConstantFP::get(Ty, ctpInfo->DoubleAt(idx)));
        } else if (Ty == Type::getFloatTy(getLLVMContext())) {
          Elts.push_back(ConstantFP::get(Ty, ctpInfo->FloatAt(idx)));
        } else {
          Elts.push_back(ConstantInt::get(Ty, (uint64)ctpInfo->IntegerAt(idx)));
        }
      } else if (type->isReference()){
        const UTF8* utf8 = ctpInfo->UTF8At(ctpInfo->ctpDef[idx]);
        JavaString* obj = ctpInfo->resolveString(utf8, idx);
        Constant* C = getString(obj);
        C = ConstantExpr::getBitCast(C, JavaIntrinsics.JavaObjectType);
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
    std::string name(UTF8Buffer(classDef->name).toCompileName()->cString());
    name += "_static";
    Module& Mod = *getLLVMModule();
    GlobalVariable* varGV = 
      new GlobalVariable(Mod, Ty, false, GlobalValue::ExternalLinkage,
                         0, name);

    Constant* res = ConstantExpr::getCast(Instruction::BitCast, varGV,
                                          JavaIntrinsics.ptrType);
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

Constant* JavaAOTCompiler::getVirtualTable(JavaVirtualTable* VT) {
  CommonClass* classDef = VT->cl;
  uint32 size = 0;
  if (classDef->isClass()) {
    LLVMClassInfo* LCI = getClassInfo(classDef->asClass());
    LCI->getVirtualType();
    size = classDef->asClass()->virtualTableSize;
  } else {
    size = JavaVirtualTable::getBaseSize();
  }
  llvm::Constant* res = 0;
  virtual_table_iterator End = virtualTables.end();
  virtual_table_iterator I = virtualTables.find(VT);
  if (I == End) {
    
    const ArrayType* ATy = 
      dyn_cast<ArrayType>(JavaIntrinsics.VTType->getContainedType(0));
    const PointerType* PTy = dyn_cast<PointerType>(ATy->getContainedType(0));
    ATy = ArrayType::get(PTy, size);
    std::string name(UTF8Buffer(classDef->name).toCompileName()->cString());
    name += "_VT";
    // Do not set a virtual table as a constant, because the runtime may
    // modify it.
    Module& Mod = *getLLVMModule();
    GlobalVariable* varGV = new GlobalVariable(Mod, ATy, false,
                                               GlobalValue::ExternalLinkage,
                                               0, name);
  
    res = ConstantExpr::getCast(Instruction::BitCast, varGV,
                                JavaIntrinsics.VTType);
    virtualTables.insert(std::make_pair(VT, res));
  
    if (isCompiling(classDef) || assumeCompiled) {
      Constant* C = CreateConstantFromVT(VT);
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
    
    Module& Mod = *getLLVMModule();
    varGV = new GlobalVariable(Mod, valPtrType, false,
                               GlobalValue::InternalLinkage,
                               Constant::getNullValue(valPtrType), "");
  
    nativeFunctions.insert(std::make_pair(meth, varGV));
    return varGV;
  } else {
    return I->second;
  }
}

Constant* JavaAOTCompiler::CreateConstantForBaseObject(CommonClass* cl) {
  const StructType* STy = 
    dyn_cast<StructType>(JavaIntrinsics.JavaObjectType->getContainedType(0));
  
  std::vector<Constant*> Elmts;

  // VT
  Elmts.push_back(getVirtualTable(cl->virtualVT));
  
  // lock
  Constant* L = ConstantInt::get(Type::getInt64Ty(getLLVMContext()), 0);
  Elmts.push_back(ConstantExpr::getIntToPtr(L, JavaIntrinsics.ptrType));

  return ConstantStruct::get(STy, Elmts);
}

Constant* JavaAOTCompiler::CreateConstantFromJavaClass(CommonClass* cl) {
  Class* javaClass = cl->classLoader->bootstrapLoader->upcalls->newClass;
  LLVMClassInfo* LCI = getClassInfo(javaClass);
  const StructType* STy = 
    dyn_cast<StructType>(LCI->getVirtualType()->getContainedType(0));

  std::vector<Constant*> Elmts;

  // JavaObject
  Elmts.push_back(CreateConstantForBaseObject(javaClass));
  
  // signers
  Elmts.push_back(Constant::getNullValue(JavaIntrinsics.JavaObjectType));
  
  // pd
  Elmts.push_back(Constant::getNullValue(JavaIntrinsics.JavaObjectType));
  
  // vmdata
  Constant* Cl = getNativeClass(cl);
  Cl = ConstantExpr::getCast(Instruction::BitCast, Cl,
                             JavaIntrinsics.JavaObjectType);
  Elmts.push_back(Cl);

  // constructor
  Elmts.push_back(Constant::getNullValue(JavaIntrinsics.JavaObjectType));

  return ConstantStruct::get(STy, Elmts);
}

Constant* JavaAOTCompiler::CreateConstantFromJavaObject(JavaObject* obj) {
  CommonClass* cl = JavaObject::getClass(obj);

  if (cl->isArray()) {
    Classpath* upcalls = cl->classLoader->bootstrapLoader->upcalls;
    CommonClass* subClass = cl->asArrayClass()->baseClass();
    if (subClass->isPrimitive()) {
      if (subClass == upcalls->OfBool) {
        return CreateConstantFromIntArray<ArrayUInt8>((ArrayUInt8*)obj,
                                        Type::getInt8Ty(getLLVMContext()));
      } else if (subClass == upcalls->OfByte) {
        return CreateConstantFromIntArray<ArraySInt8>((ArraySInt8*)obj,
                                        Type::getInt8Ty(getLLVMContext()));
      } else if (subClass == upcalls->OfShort) {
        return CreateConstantFromIntArray<ArraySInt16>((ArraySInt16*)obj,
                                        Type::getInt16Ty(getLLVMContext()));
      } else if (subClass == upcalls->OfChar) {
        return CreateConstantFromIntArray<ArrayUInt16>((ArrayUInt16*)obj,
                                        Type::getInt16Ty(getLLVMContext()));
      } else if (subClass == upcalls->OfInt) {
        return CreateConstantFromIntArray<ArraySInt32>((ArraySInt32*)obj,
                                        Type::getInt32Ty(getLLVMContext()));
      } else if (subClass == upcalls->OfFloat) {
        return CreateConstantFromFPArray<ArrayFloat>((ArrayFloat*)obj,
                                        Type::getFloatTy(getLLVMContext()));
      } else if (subClass == upcalls->OfLong) {
        return CreateConstantFromIntArray<ArrayLong>((ArrayLong*)obj,
                                        Type::getInt64Ty(getLLVMContext()));
      } else if (subClass == upcalls->OfDouble) {
        return CreateConstantFromFPArray<ArrayDouble>((ArrayDouble*)obj,
                                        Type::getDoubleTy(getLLVMContext()));
      } else {
        abort();
      }
    } else {
      return CreateConstantFromObjectArray((ArrayObject*)obj);
    }
  } else {
    
    std::vector<Constant*> Elmts;
    
    // JavaObject
    Constant* CurConstant =
        CreateConstantForBaseObject(JavaObject::getClass(obj));

    for (uint32 j = 1; j <= cl->virtualVT->depth; ++j) {
      std::vector<Constant*> TempElts;
      Elmts.push_back(CurConstant);
      TempElts.push_back(CurConstant);
      Class* curCl = cl->virtualVT->display[j]->cl->asClass();
      LLVMClassInfo* LCI = getClassInfo(curCl);
      const StructType* STy = 
        dyn_cast<StructType>(LCI->getVirtualType()->getContainedType(0));

      for (uint32 i = 0; i < curCl->nbVirtualFields; ++i) {
        JavaField& field = curCl->virtualFields[i];
        const Typedef* type = field.getSignature();
        if (type->isPrimitive()) {
          const PrimitiveTypedef* prim = (const PrimitiveTypedef*)type;
          if (prim->isBool() || prim->isByte()) {
            ConstantInt* CI = ConstantInt::get(Type::getInt8Ty(getLLVMContext()),
                                               field.getInstanceInt8Field(obj));
            TempElts.push_back(CI);
          } else if (prim->isShort() || prim->isChar()) {
            ConstantInt* CI = ConstantInt::get(Type::getInt16Ty(getLLVMContext()),
                                               field.getInstanceInt16Field(obj));
            TempElts.push_back(CI);
          } else if (prim->isInt()) {
            ConstantInt* CI = ConstantInt::get(Type::getInt32Ty(getLLVMContext()),
                                               field.getInstanceInt32Field(obj));
            TempElts.push_back(CI);
          } else if (prim->isLong()) {
            ConstantInt* CI = ConstantInt::get(Type::getInt64Ty(getLLVMContext()),
                                               field.getInstanceLongField(obj));
            TempElts.push_back(CI);
          } else if (prim->isFloat()) {
            Constant* CF = ConstantFP::get(Type::getFloatTy(getLLVMContext()),
                                           field.getInstanceFloatField(obj));
            TempElts.push_back(CF);
          } else if (prim->isDouble()) {
            Constant* CF = ConstantFP::get(Type::getDoubleTy(getLLVMContext()),
                                           field.getInstanceDoubleField(obj));
            TempElts.push_back(CF);
          } else {
            abort();
          }
        } else {
          JavaObject* val = field.getInstanceObjectField(obj);
          if (val) {
            JnjvmClassLoader* JCL = cl->classLoader;
            CommonClass* FieldCl = field.getSignature()->assocClass(JCL);
            Constant* C = getFinalObject(val, FieldCl);
            TempElts.push_back(C);
          } else {
            const llvm::Type* Ty = JavaIntrinsics.JavaObjectType;
            TempElts.push_back(Constant::getNullValue(Ty));
          }
        }
      }
      CurConstant = ConstantStruct::get(STy, TempElts);
    }

    return CurConstant;
  }
}

Constant* JavaAOTCompiler::CreateConstantFromJavaString(JavaString* str) {
  Class* cl = JavaObject::getClass(str)->asClass();
  LLVMClassInfo* LCI = getClassInfo(cl);
  const StructType* STy = 
    dyn_cast<StructType>(LCI->getVirtualType()->getContainedType(0));

  std::vector<Constant*> Elmts;

  Elmts.push_back(CreateConstantForBaseObject(cl));

  Constant* Array =
    CreateConstantFromIntArray<ArrayUInt16>(JavaString::getValue(str),
                                            Type::getInt16Ty(getLLVMContext()));
  

  Module& Mod = *getLLVMModule();
  GlobalVariable* varGV = new GlobalVariable(Mod, Array->getType(), false,
                                             GlobalValue::InternalLinkage,
                                             Array, "");
 
	Array = ConstantExpr::getBitCast(varGV, JavaIntrinsics.JavaObjectType);

  Elmts.push_back(Array);
  Elmts.push_back(ConstantInt::get(Type::getInt32Ty(getLLVMContext()),
                                   str->count));
  Elmts.push_back(ConstantInt::get(Type::getInt32Ty(getLLVMContext()),
                                   str->cachedHashCode));
  Elmts.push_back(ConstantInt::get(Type::getInt32Ty(getLLVMContext()),
                                   str->offset));
 
  return ConstantStruct::get(STy, Elmts);
}


Constant* JavaAOTCompiler::CreateConstantFromAttribut(Attribut& attribut) {
  const StructType* STy = 
    dyn_cast<StructType>(JavaIntrinsics.AttributType->getContainedType(0));


  std::vector<Constant*> Elmts;

  // name
  Elmts.push_back(getUTF8(attribut.name));

  // start
  Elmts.push_back(ConstantInt::get(Type::getInt32Ty(getLLVMContext()), attribut.start));

  // nbb
  Elmts.push_back(ConstantInt::get(Type::getInt32Ty(getLLVMContext()), attribut.nbb));
  
  return ConstantStruct::get(STy, Elmts);
}

Constant* JavaAOTCompiler::CreateConstantFromCommonClass(CommonClass* cl) {
  const StructType* STy = 
    dyn_cast<StructType>(JavaIntrinsics.JavaCommonClassType->getContainedType(0));
  Module& Mod = *getLLVMModule();
  
  const llvm::Type* TempTy = 0;

  std::vector<Constant*> CommonClassElts;
  std::vector<Constant*> TempElmts;

  // delegatee
  const ArrayType* ATy = dyn_cast<ArrayType>(STy->getContainedType(0));
  assert(ATy && "Malformed type");

  Constant* TCM[1] = { getJavaClass(cl) };
  CommonClassElts.push_back(ConstantArray::get(ATy, TCM, 1));
  
  // access
  CommonClassElts.push_back(ConstantInt::get(Type::getInt16Ty(getLLVMContext()), cl->access));
 
  // interfaces
  if (cl->nbInterfaces) {
    for (uint32 i = 0; i < cl->nbInterfaces; ++i) {
      TempElmts.push_back(getNativeClass(cl->interfaces[i]));
    }

    ATy = ArrayType::get(JavaIntrinsics.JavaClassType, cl->nbInterfaces);
    Constant* interfaces = ConstantArray::get(ATy, TempElmts);
    interfaces = new GlobalVariable(Mod, ATy, true,
                                    GlobalValue::InternalLinkage,
                                    interfaces, "");
    interfaces = ConstantExpr::getCast(Instruction::BitCast, interfaces,
                            PointerType::getUnqual(JavaIntrinsics.JavaClassType));

    CommonClassElts.push_back(interfaces);
  } else {
    const Type* Ty = PointerType::getUnqual(JavaIntrinsics.JavaClassType);
    CommonClassElts.push_back(Constant::getNullValue(Ty));
  }

  // nbInterfaces
  CommonClassElts.push_back(ConstantInt::get(Type::getInt16Ty(getLLVMContext()), cl->nbInterfaces));

  // name
  CommonClassElts.push_back(getUTF8(cl->name));

  // super
  if (cl->super) {
    CommonClassElts.push_back(getNativeClass(cl->super));
  } else {
    TempTy = JavaIntrinsics.JavaClassType;
    CommonClassElts.push_back(Constant::getNullValue(TempTy));
  }

  // classLoader: store the static initializer, it will be overriden once
  // the class is loaded.
  Constant* loader = ConstantExpr::getBitCast(StaticInitializer,
                                              JavaIntrinsics.ptrType);
  CommonClassElts.push_back(loader);
  
  // virtualTable
  if (cl->virtualVT) {
    CommonClassElts.push_back(getVirtualTable(cl->virtualVT));
  } else {
    TempTy = JavaIntrinsics.VTType;
    CommonClassElts.push_back(Constant::getNullValue(TempTy));
  }
  return ConstantStruct::get(STy, CommonClassElts);
}

Constant* JavaAOTCompiler::CreateConstantFromJavaField(JavaField& field) {
  const StructType* STy = 
    dyn_cast<StructType>(JavaIntrinsics.JavaFieldType->getContainedType(0));
  
  std::vector<Constant*> FieldElts;
  std::vector<Constant*> TempElts;
  
  // signature
  FieldElts.push_back(Constant::getNullValue(JavaIntrinsics.ptrType));
  
  // access
  FieldElts.push_back(ConstantInt::get(Type::getInt16Ty(getLLVMContext()), field.access));

  // name
  FieldElts.push_back(getUTF8(field.name));

  // type
  FieldElts.push_back(getUTF8(field.type));
  
  // attributs 
  if (field.nbAttributs) {
    const llvm::Type* AttrTy = JavaIntrinsics.AttributType->getContainedType(0);
    const ArrayType* ATy = ArrayType::get(AttrTy, field.nbAttributs);
    for (uint32 i = 0; i < field.nbAttributs; ++i) {
      TempElts.push_back(CreateConstantFromAttribut(field.attributs[i]));
    }

    Constant* attributs = ConstantArray::get(ATy, TempElts);
    TempElts.clear();
    attributs = new GlobalVariable(*getLLVMModule(), ATy, true,
                                   GlobalValue::InternalLinkage,
                                   attributs, "");
    attributs = ConstantExpr::getCast(Instruction::BitCast, attributs,
                                      JavaIntrinsics.AttributType);
  
    FieldElts.push_back(attributs);
  } else {
    FieldElts.push_back(Constant::getNullValue(JavaIntrinsics.AttributType));
  }
  
  // nbAttributs
  FieldElts.push_back(ConstantInt::get(Type::getInt16Ty(getLLVMContext()), field.nbAttributs));

  // classDef
  FieldElts.push_back(getNativeClass(field.classDef));

  // ptrOffset
  FieldElts.push_back(ConstantInt::get(Type::getInt32Ty(getLLVMContext()), field.ptrOffset));

  // num
  FieldElts.push_back(ConstantInt::get(Type::getInt16Ty(getLLVMContext()), field.num));

  return ConstantStruct::get(STy, FieldElts); 
}

Constant* JavaAOTCompiler::CreateConstantFromJavaMethod(JavaMethod& method) {
  const StructType* STy = 
    dyn_cast<StructType>(JavaIntrinsics.JavaMethodType->getContainedType(0));
  Module& Mod = *getLLVMModule();
  
  std::vector<Constant*> MethodElts;
  std::vector<Constant*> TempElts;
  
  // signature
  MethodElts.push_back(Constant::getNullValue(JavaIntrinsics.ptrType));
  
  // access
  MethodElts.push_back(ConstantInt::get(Type::getInt16Ty(getLLVMContext()), method.access));
 
  // attributs
  if (method.nbAttributs) {
    const llvm::Type* AttrTy = JavaIntrinsics.AttributType->getContainedType(0);
    const ArrayType* ATy = ArrayType::get(AttrTy, method.nbAttributs);
    for (uint32 i = 0; i < method.nbAttributs; ++i) {
      TempElts.push_back(CreateConstantFromAttribut(method.attributs[i]));
    }

    Constant* attributs = ConstantArray::get(ATy, TempElts);
    TempElts.clear();
    attributs = new GlobalVariable(Mod, ATy, true,
                                   GlobalValue::InternalLinkage,
                                   attributs, "");
    attributs = ConstantExpr::getCast(Instruction::BitCast, attributs,
                                      JavaIntrinsics.AttributType);

    MethodElts.push_back(attributs);
  } else {
    MethodElts.push_back(Constant::getNullValue(JavaIntrinsics.AttributType));
  }
  
  // nbAttributs
  MethodElts.push_back(ConstantInt::get(Type::getInt16Ty(getLLVMContext()), method.nbAttributs));
  
  // classDef
  MethodElts.push_back(getNativeClass(method.classDef));
  
  // name
  MethodElts.push_back(getUTF8(method.name));

  // type
  MethodElts.push_back(getUTF8(method.type));
  
  // canBeInlined
  MethodElts.push_back(ConstantInt::get(Type::getInt8Ty(getLLVMContext()), method.canBeInlined));

  // code
  if (isAbstract(method.access)) {
    MethodElts.push_back(Constant::getNullValue(JavaIntrinsics.ptrType));
  } else {
    LLVMMethodInfo* LMI = getMethodInfo(&method);
    Function* func = LMI->getMethod();
    MethodElts.push_back(ConstantExpr::getCast(Instruction::BitCast, func,
                                               JavaIntrinsics.ptrType));
  }
  
  // codeInfo
  MethodElts.push_back(Constant::getNullValue(JavaIntrinsics.CodeLineInfoType));
  
  // codeInfoLength
  MethodElts.push_back(ConstantInt::get(Type::getInt16Ty(getLLVMContext()), 0));

  // offset
  MethodElts.push_back(ConstantInt::get(Type::getInt32Ty(getLLVMContext()), method.offset));

  return ConstantStruct::get(STy, MethodElts); 
}

Constant* JavaAOTCompiler::CreateConstantFromClassPrimitive(ClassPrimitive* cl) {
  const llvm::Type* JCPTy = 
    JavaIntrinsics.JavaClassPrimitiveType->getContainedType(0);
  const StructType* STy = dyn_cast<StructType>(JCPTy);
  
  std::vector<Constant*> ClassElts;
  
  // common class
  ClassElts.push_back(CreateConstantFromCommonClass(cl));

  // primSize
  ClassElts.push_back(ConstantInt::get(Type::getInt32Ty(getLLVMContext()), cl->logSize));

  return ConstantStruct::get(STy, ClassElts);
}

Constant* JavaAOTCompiler::CreateConstantFromClassArray(ClassArray* cl) {
  const StructType* STy = 
    dyn_cast<StructType>(JavaIntrinsics.JavaClassArrayType->getContainedType(0));
  
  std::vector<Constant*> ClassElts;
  Constant* ClGEPs[2] = { getIntrinsics()->constantZero,
                          getIntrinsics()->constantZero };
  
  // common class
  ClassElts.push_back(CreateConstantFromCommonClass(cl));

  // baseClass
  Constant* Cl = getNativeClass(cl->baseClass());
  if (Cl->getType() != JavaIntrinsics.JavaCommonClassType)
    Cl = ConstantExpr::getGetElementPtr(Cl, ClGEPs, 2);
    
  ClassElts.push_back(Cl);
  
  return ConstantStruct::get(STy, ClassElts);
}

Constant* JavaAOTCompiler::CreateConstantFromClass(Class* cl) {
  const StructType* STy = 
    dyn_cast<StructType>(JavaIntrinsics.JavaClassType->getContainedType(0));
  Module& Mod = *getLLVMModule();
  
  std::vector<Constant*> ClassElts;
  std::vector<Constant*> TempElts;

  // common class
  ClassElts.push_back(CreateConstantFromCommonClass(cl));

  // virtualSize
  ClassElts.push_back(ConstantInt::get(Type::getInt32Ty(getLLVMContext()),
                                       cl->virtualSize));
  
  // alginment
  ClassElts.push_back(ConstantInt::get(Type::getInt32Ty(getLLVMContext()),
                                       cl->alignment));

  // IsolateInfo
  const ArrayType* ATy = dyn_cast<ArrayType>(STy->getContainedType(3));
  assert(ATy && "Malformed type");
  
  const StructType* TCMTy = dyn_cast<StructType>(ATy->getContainedType(0));
  assert(TCMTy && "Malformed type");

  uint32 status = cl->needsInitialisationCheck() ? vmjc : ready;
  TempElts.push_back(ConstantInt::get(Type::getInt8Ty(getLLVMContext()),
                                      status));
  TempElts.push_back(ConstantInt::get(Type::getInt1Ty(getLLVMContext()),
                                      status == ready ? 1 : 0));
  TempElts.push_back(getStaticInstance(cl));
  Constant* CStr[1] = { ConstantStruct::get(TCMTy, TempElts) };
  TempElts.clear();
  ClassElts.push_back(ConstantArray::get(ATy, CStr, 1));

  if (cl->nbVirtualFields + cl->nbStaticFields) {
    ATy = ArrayType::get(JavaIntrinsics.JavaFieldType->getContainedType(0),
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
    fields = new GlobalVariable(Mod, ATy, false,
                                GlobalValue::InternalLinkage,
                                fields, "");
    fields = ConstantExpr::getCast(Instruction::BitCast, fields,
                                   JavaIntrinsics.JavaFieldType);
  } else {
    fields = Constant::getNullValue(JavaIntrinsics.JavaFieldType);
  }

  // virtualFields
  ClassElts.push_back(fields);

  ConstantInt* nbVirtualFields = 
    ConstantInt::get(Type::getInt16Ty(getLLVMContext()), cl->nbVirtualFields);
  // nbVirtualFields
  ClassElts.push_back(nbVirtualFields);
  
  // staticFields
  // Output null, getLLVMModule() will be set in  the initializer. Otherwise, the
  // assembly emitter of LLVM will try to align the data.
  ClassElts.push_back(Constant::getNullValue(JavaIntrinsics.JavaFieldType));

  // nbStaticFields
  ClassElts.push_back(ConstantInt::get(Type::getInt16Ty(getLLVMContext()), cl->nbStaticFields));
  
  // virtualMethods
  if (cl->nbVirtualMethods + cl->nbStaticMethods) {
    ATy = ArrayType::get(JavaIntrinsics.JavaMethodType->getContainedType(0),
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

    GlobalVariable* GV = NULL;
    method_iterator SI = virtualMethods.find(cl);
    if (SI != virtualMethods.end()) {
      GV = dyn_cast<GlobalVariable>(SI->second);
      GV->setInitializer(methods);
    } else {
      std::string name(UTF8Buffer(cl->name).toCompileName()->cString());
      name += "_VirtualMethods";
      GV = new GlobalVariable(Mod, ATy, false, GlobalValue::ExternalLinkage,
                              methods, name);
      virtualMethods.insert(std::make_pair(cl, GV));
    }
    methods = ConstantExpr::getCast(Instruction::BitCast, GV,
                                    JavaIntrinsics.JavaMethodType);
  } else {
    methods = Constant::getNullValue(JavaIntrinsics.JavaMethodType);
  }

  // virtualMethods
  ClassElts.push_back(methods);

  ConstantInt* nbVirtualMethods = 
    ConstantInt::get(Type::getInt16Ty(getLLVMContext()), cl->nbVirtualMethods);
  // nbVirtualMethods
  ClassElts.push_back(nbVirtualMethods);
  
  // staticMethods
  // Output null, getLLVMModule() will be set in  the initializer.
  ClassElts.push_back(Constant::getNullValue(JavaIntrinsics.JavaMethodType));

  // nbStaticMethods
  ClassElts.push_back(ConstantInt::get(Type::getInt16Ty(getLLVMContext()), cl->nbStaticMethods));

  // ownerClass
  ClassElts.push_back(Constant::getNullValue(JavaIntrinsics.ptrType));
  
  // bytes
  ClassElts.push_back(Constant::getNullValue(JavaIntrinsics.JavaArrayUInt8Type));

  // ctpInfo
  ClassElts.push_back(Constant::getNullValue(JavaIntrinsics.ptrType));

  // attributs
  if (cl->nbAttributs) {
    ATy = ArrayType::get(JavaIntrinsics.AttributType->getContainedType(0),
                         cl->nbAttributs);

    for (uint32 i = 0; i < cl->nbAttributs; ++i) {
      TempElts.push_back(CreateConstantFromAttribut(cl->attributs[i]));
    }

    Constant* attributs = ConstantArray::get(ATy, TempElts);
    TempElts.clear();
    attributs = new GlobalVariable(*getLLVMModule(), ATy, true,
                                   GlobalValue::InternalLinkage,
                                   attributs, "");
    attributs = ConstantExpr::getCast(Instruction::BitCast, attributs,
                                      JavaIntrinsics.AttributType);
    ClassElts.push_back(attributs);
  } else {
    ClassElts.push_back(Constant::getNullValue(JavaIntrinsics.AttributType));
  }
  
  // nbAttributs
  ClassElts.push_back(ConstantInt::get(Type::getInt16Ty(getLLVMContext()), cl->nbAttributs));
  
  // innerClasses
  if (cl->nbInnerClasses) {
    for (uint32 i = 0; i < cl->nbInnerClasses; ++i) {
      TempElts.push_back(getNativeClass(cl->innerClasses[i]));
    }

    const llvm::Type* TempTy = JavaIntrinsics.JavaClassType;
    ATy = ArrayType::get(TempTy, cl->nbInnerClasses);
    Constant* innerClasses = ConstantArray::get(ATy, TempElts);
    innerClasses = new GlobalVariable(*getLLVMModule(), ATy, true,
                                      GlobalValue::InternalLinkage,
                                      innerClasses, "");
    innerClasses = ConstantExpr::getCast(Instruction::BitCast, innerClasses,
                                         PointerType::getUnqual(TempTy));

    ClassElts.push_back(innerClasses);
  } else {
    const Type* Ty = PointerType::getUnqual(JavaIntrinsics.JavaClassType);
    ClassElts.push_back(Constant::getNullValue(Ty));
  }

  // nbInnerClasses
  ClassElts.push_back(ConstantInt::get(Type::getInt16Ty(getLLVMContext()), cl->nbInnerClasses));

  // outerClass
  if (cl->outerClass) {
    ClassElts.push_back(getNativeClass(cl->outerClass));
  } else {
    ClassElts.push_back(Constant::getNullValue(JavaIntrinsics.JavaClassType));
  }

  // innerAccess
  ClassElts.push_back(ConstantInt::get(Type::getInt16Ty(getLLVMContext()), cl->innerAccess));
  
  // innerOuterResolved
  ClassElts.push_back(ConstantInt::get(Type::getInt8Ty(getLLVMContext()), cl->innerOuterResolved));
  
  // isAnonymous
  ClassElts.push_back(ConstantInt::get(Type::getInt8Ty(getLLVMContext()), cl->isAnonymous));
  
  // virtualTableSize
  ClassElts.push_back(ConstantInt::get(Type::getInt32Ty(getLLVMContext()), cl->virtualTableSize));
  
  // staticSize
  ClassElts.push_back(ConstantInt::get(Type::getInt32Ty(getLLVMContext()), cl->staticSize));

  return ConstantStruct::get(STy, ClassElts);
}

template<typename T>
Constant* JavaAOTCompiler::CreateConstantFromIntArray(const T* val, const Type* Ty) {
  std::vector<const Type*> Elemts;
  const ArrayType* ATy = ArrayType::get(Ty, T::getSize(val));
  Elemts.push_back(JavaIntrinsics.JavaObjectType->getContainedType(0));
  Elemts.push_back(JavaIntrinsics.pointerSizeType);
  

  Elemts.push_back(ATy);

  const StructType* STy = StructType::get(getLLVMModule()->getContext(), Elemts);
  
  std::vector<Constant*> Cts;
  Cts.push_back(CreateConstantForBaseObject(JavaObject::getClass(val)));
  Cts.push_back(ConstantInt::get(JavaIntrinsics.pointerSizeType, T::getSize(val)));
  
  std::vector<Constant*> Vals;
  for (sint32 i = 0; i < T::getSize(val); ++i) {
    Vals.push_back(ConstantInt::get(Ty, (uint64)T::getElement(val, i)));
  }

  Cts.push_back(ConstantArray::get(ATy, Vals));
  
  return ConstantStruct::get(STy, Cts);
}

template<typename T>
Constant* JavaAOTCompiler::CreateConstantFromFPArray(const T* val, const Type* Ty) {
  std::vector<const Type*> Elemts;
  const ArrayType* ATy = ArrayType::get(Ty, T::getSize(val));
  Elemts.push_back(JavaIntrinsics.JavaObjectType->getContainedType(0));
  Elemts.push_back(JavaIntrinsics.pointerSizeType);
  

  Elemts.push_back(ATy);

  const StructType* STy = StructType::get(getLLVMModule()->getContext(), Elemts);
  
  std::vector<Constant*> Cts;
  Cts.push_back(CreateConstantForBaseObject(JavaObject::getClass(val)));
  Cts.push_back(ConstantInt::get(JavaIntrinsics.pointerSizeType, T::getSize(val)));
  
  std::vector<Constant*> Vals;
  for (sint32 i = 0; i < T::getSize(val); ++i) {
    Vals.push_back(ConstantFP::get(Ty, (double)T::getElement(val, i)));
  }

  Cts.push_back(ConstantArray::get(ATy, Vals));
  
  return ConstantStruct::get(STy, Cts);
}

Constant* JavaAOTCompiler::CreateConstantFromObjectArray(const ArrayObject* val) {
  std::vector<const Type*> Elemts;
  const llvm::Type* Ty = JavaIntrinsics.JavaObjectType;
  const ArrayType* ATy = ArrayType::get(Ty, ArrayObject::getSize(val));
  Elemts.push_back(JavaIntrinsics.JavaObjectType->getContainedType(0));
  Elemts.push_back(JavaIntrinsics.pointerSizeType);
  

  Elemts.push_back(ATy);

  const StructType* STy = StructType::get(getLLVMModule()->getContext(), Elemts);
  
  std::vector<Constant*> Cts;
  Cts.push_back(CreateConstantForBaseObject(JavaObject::getClass(val)));
  Cts.push_back(ConstantInt::get(JavaIntrinsics.pointerSizeType,
        ArrayObject::getSize(val)));
  
  std::vector<Constant*> Vals;
  for (sint32 i = 0; i < ArrayObject::getSize(val); ++i) {
    if (ArrayObject::getElement(val, i)) {
      Vals.push_back(getFinalObject(ArrayObject::getElement(val, i),
          JavaObject::getClass(val)->asArrayClass()->baseClass()));
    } else {
      Vals.push_back(Constant::getNullValue(JavaIntrinsics.JavaObjectType));
    }
  }

  Cts.push_back(ConstantArray::get(ATy, Vals));
  
  return ConstantStruct::get(STy, Cts);
}

Constant* JavaAOTCompiler::CreateConstantFromUTF8(const UTF8* val) {
  std::vector<const Type*> Elemts;
  const ArrayType* ATy = ArrayType::get(Type::getInt16Ty(getLLVMContext()), val->size);
  Elemts.push_back(JavaIntrinsics.pointerSizeType);

  Elemts.push_back(ATy);

  const StructType* STy = StructType::get(getLLVMModule()->getContext(),
                                          Elemts);
  
  std::vector<Constant*> Cts;
  Cts.push_back(ConstantInt::get(JavaIntrinsics.pointerSizeType, val->size));
  
  std::vector<Constant*> Vals;
  for (sint32 i = 0; i < val->size; ++i) {
    Vals.push_back(ConstantInt::get(Type::getInt16Ty(getLLVMContext()), val->elements[i]));
  }

  Cts.push_back(ConstantArray::get(ATy, Vals));
  
  return ConstantStruct::get(STy, Cts);

}

Constant* JavaAOTCompiler::getUTF8(const UTF8* val) {
  utf8_iterator End = utf8s.end();
  utf8_iterator I = utf8s.find(val);
  if (I == End) {
    Constant* C = CreateConstantFromUTF8(val);
    Module& Mod = *getLLVMModule();
    GlobalVariable* varGV = new GlobalVariable(Mod, C->getType(), true,
                                               GlobalValue::InternalLinkage,
                                               C, "");
    
    Constant* res = ConstantExpr::getCast(Instruction::BitCast, varGV,
                                          JavaIntrinsics.UTF8Type);
    utf8s.insert(std::make_pair(val, res));

    return res;
  } else {
    return I->second;
  }
}

Constant* JavaAOTCompiler::CreateConstantFromVT(JavaVirtualTable* VT) {
  CommonClass* classDef = VT->cl;
  uint32 size = classDef->isClass() ? classDef->asClass()->virtualTableSize :
                                      JavaVirtualTable::getBaseSize();
  JavaVirtualTable* RealVT = classDef->isClass() ? 
    VT : ClassArray::SuperArray->virtualVT;

  const ArrayType* ATy = 
    dyn_cast<ArrayType>(JavaIntrinsics.VTType->getContainedType(0));
  const PointerType* PTy = dyn_cast<PointerType>(ATy->getContainedType(0));
  ATy = ArrayType::get(PTy, size);

  ConstantPointerNull* N = ConstantPointerNull::get(PTy);
  std::vector<Constant*> Elemts;
   
  // Destructor
  Function* Finalizer = 0;
  JavaMethod* meth = (JavaMethod*)(RealVT->destructor);
  if (meth) {
    LLVMMethodInfo* LMI = getMethodInfo(meth);
    Finalizer = LMI->getMethod();
  }
  
  Elemts.push_back(Finalizer ? 
      ConstantExpr::getCast(Instruction::BitCast, Finalizer, PTy) : N);
  
  // Delete
  Elemts.push_back(N);
  
  // Tracer
  Function* Tracer = 0;
  if (classDef->isArray()) {
    if (classDef->asArrayClass()->baseClass()->isPrimitive()) {
      Tracer = JavaObjectTracer;
    } else {
      Tracer = ArrayObjectTracer;
    }
  } else if (classDef->isClass()) {
    if (classDef->isAssignableFrom(
          classDef->classLoader->bootstrapLoader->upcalls->newReference)) {
      Tracer = ReferenceObjectTracer;
    } else {
      Tracer = RegularObjectTracer;
    }
  }

  Elemts.push_back(Tracer ? 
      ConstantExpr::getCast(Instruction::BitCast, Tracer, PTy) : N);
  
  for (uint32_t i = 0; i < VirtualTable::numberOfSpecializedTracers(); i++) {
    // Push null for now.
    Elemts.push_back(N);
  }

  // Class
  Elemts.push_back(ConstantExpr::getCast(Instruction::BitCast,
                                         getNativeClass(classDef), PTy));

  // depth
  Elemts.push_back(ConstantExpr::getIntToPtr(
        ConstantInt::get(Type::getInt64Ty(getLLVMContext()), VT->depth), PTy));
  
  // offset
  Elemts.push_back(ConstantExpr::getIntToPtr(
        ConstantInt::get(Type::getInt64Ty(getLLVMContext()), VT->offset), PTy));
  
  // cache
  Elemts.push_back(N);
  
  // display
  for (uint32 i = 0; i < JavaVirtualTable::getDisplayLength(); ++i) {
    if (VT->display[i]) {
      Constant* Temp = getVirtualTable(VT->display[i]);
      Temp = ConstantExpr::getBitCast(Temp, PTy);
      Elemts.push_back(Temp);
    } else {
      Elemts.push_back(Constant::getNullValue(PTy));
    }
  }
  
  // nbSecondaryTypes
  Elemts.push_back(ConstantExpr::getIntToPtr(
        ConstantInt::get(Type::getInt64Ty(getLLVMContext()), VT->nbSecondaryTypes), PTy));
  
  // secondaryTypes
  const ArrayType* DTy = ArrayType::get(JavaIntrinsics.VTType,
                                        VT->nbSecondaryTypes);
  
  std::vector<Constant*> TempElmts;
  for (uint32 i = 0; i < VT->nbSecondaryTypes; ++i) {
    assert(VT->secondaryTypes[i] && "No secondary type");
    Constant* Cl = getVirtualTable(VT->secondaryTypes[i]);
    TempElmts.push_back(Cl);
  }
  Constant* display = ConstantArray::get(DTy, TempElmts);
  TempElmts.clear();
  
  display = new GlobalVariable(*getLLVMModule(), DTy, true,
                               GlobalValue::InternalLinkage,
                               display, "");

  display = ConstantExpr::getCast(Instruction::BitCast, display, PTy);
  
  Elemts.push_back(display);
    
  // baseClassVT
  if (VT->baseClassVT) {
    Constant* Temp = getVirtualTable(VT->baseClassVT);
    Temp = ConstantExpr::getBitCast(Temp, PTy);
    Elemts.push_back(Temp);
  } else {
    Elemts.push_back(Constant::getNullValue(PTy));
  }
   
  // IMT
  if (!VT->IMT) {
    Elemts.push_back(Constant::getNullValue(PTy));
  } else {
    Class* cl = classDef->asClass();
    assert(cl && "Not a class");
    std::set<JavaMethod*> contents[InterfaceMethodTable::NumIndexes];
    classDef->asClass()->fillIMT(contents);
  

    const ArrayType* ATy = 
      dyn_cast<ArrayType>(JavaIntrinsics.VTType->getContainedType(0));
    const PointerType* PTy = dyn_cast<PointerType>(ATy->getContainedType(0));
    ATy = ArrayType::get(PTy, InterfaceMethodTable::NumIndexes);
  
    ConstantPointerNull* N = ConstantPointerNull::get(PTy);
    std::vector<Constant*> IElemts;

    for (uint32_t i = 0; i < InterfaceMethodTable::NumIndexes; ++i) {
      std::set<JavaMethod*>& atIndex = contents[i];
      uint32_t size = atIndex.size();
      if (size == 1) {
        JavaMethod* Imeth = *(atIndex.begin());
        JavaMethod* meth = cl->lookupMethodDontThrow(Imeth->name,
                                                     Imeth->type,
                                                     false, true, 0);
        assert(meth && "No method found");
        LLVMMethodInfo* LMI = getMethodInfo(meth);
        Function* func = LMI->getMethod();
        IElemts.push_back(ConstantExpr::getBitCast(func, PTy));
      } else if (size > 1) {
        std::vector<JavaMethod*> methods;
        bool SameMethod = true;
        JavaMethod* OldMethod = 0;
      
        for (std::set<JavaMethod*>::iterator it = atIndex.begin(),
             et = atIndex.end(); it != et; ++it) {
          JavaMethod* Imeth = *it;
          JavaMethod* Cmeth = cl->lookupMethodDontThrow(Imeth->name, Imeth->type,
                                                      false, true, 0);
       
          if (OldMethod && OldMethod != Cmeth) SameMethod = false;
          else OldMethod = Cmeth;
        
          methods.push_back(Cmeth);
        }

        if (SameMethod) {
          assert(methods[0] && "No method found");
          LLVMMethodInfo* LMI = getMethodInfo(methods[0]);
          Function* func = LMI->getMethod();
          IElemts.push_back(ConstantExpr::getBitCast(func, PTy));
        } else {

          uint32_t length = 2 * size;
        
          const ArrayType* ATy = 
            dyn_cast<ArrayType>(JavaIntrinsics.VTType->getContainedType(0));
          ATy = ArrayType::get(PTy, length);
          std::vector<Constant*> InternalElemts;
     

          std::set<JavaMethod*>::iterator Interf = atIndex.begin();
          for (std::vector<JavaMethod*>::iterator it = methods.begin(),
               et = methods.end(); it != et; ++it, ++Interf) {
            JavaMethod* Imeth = *Interf;
            JavaMethod* Cmeth = *it;
            assert(Cmeth && "No method found");

            LLVMMethodInfo* LMI = getMethodInfo(Cmeth);
            Function* func = LMI->getMethod();
            InternalElemts.push_back(
              ConstantExpr::getBitCast(getMethodInClass(Imeth), PTy));
            InternalElemts.push_back(ConstantExpr::getBitCast(func, PTy));
          }
          Constant* Array = ConstantArray::get(ATy, InternalElemts);
    
          GlobalVariable* GV = new GlobalVariable(*getLLVMModule(), ATy, false,
                                                  GlobalValue::InternalLinkage,
                                                  Array, "");
     
          Constant* CI =
            ConstantExpr::getPtrToInt(GV, JavaIntrinsics.pointerSizeType);
          CI = ConstantExpr::getAdd(CI,
            ConstantExpr::getIntegerCast(JavaIntrinsics.constantOne,
                                         JavaIntrinsics.pointerSizeType,
                                         false));
          CI = ConstantExpr::getIntToPtr(CI, PTy);
          IElemts.push_back(CI);
        }
      } else {
        IElemts.push_back(N);
      }
    }
 
    Constant* Array = ConstantArray::get(ATy, IElemts);
    GlobalVariable* GV = new GlobalVariable(*getLLVMModule(), ATy, false,
                                            GlobalValue::InternalLinkage,
                                            Array, "");
    Elemts.push_back(ConstantExpr::getBitCast(GV, PTy));
  }
 
  // methods
  for (uint32 i = JavaVirtualTable::getFirstJavaMethodIndex(); i < size; ++i) {
    JavaMethod* meth = ((JavaMethod**)RealVT)[i];
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

namespace mvm {
  llvm::FunctionPass* createEscapeAnalysisPass(llvm::Function*);
}

namespace j3 {
  llvm::FunctionPass* createLowerConstantCallsPass();
}

JavaAOTCompiler::JavaAOTCompiler(const std::string& ModuleID) :
  JavaLLVMCompiler(ModuleID) {
 
  generateStubs = true;
  assumeCompiled = false;
  compileRT = false;

  std::vector<const llvm::Type*> llvmArgs;
  llvmArgs.push_back(JavaIntrinsics.ptrType); // class loader.
  const FunctionType* FTy = FunctionType::get(Type::getVoidTy(getLLVMContext()),
                                              llvmArgs, false);

  StaticInitializer = Function::Create(FTy, GlobalValue::InternalLinkage,
                                       "Init", getLLVMModule());
  
  llvmArgs.clear();
  FTy = FunctionType::get(Type::getVoidTy(getLLVMContext()), llvmArgs, false);
  Callback = Function::Create(FTy, GlobalValue::ExternalLinkage,
                              "staticCallback", getLLVMModule());

  llvmArgs.clear();
  llvmArgs.push_back(JavaIntrinsics.JavaMethodType);
  
  FTy = FunctionType::get(JavaIntrinsics.ptrType, llvmArgs, false);

  NativeLoader = Function::Create(FTy, GlobalValue::ExternalLinkage,
                                  "vmjcNativeLoader", getLLVMModule());
  
  llvmArgs.clear();
  FTy = FunctionType::get(Type::getVoidTy(getLLVMContext()), llvmArgs, false);
  ObjectPrinter = Function::Create(FTy, GlobalValue::ExternalLinkage,
                                   "printJavaObject", getLLVMModule());
  
  ArrayObjectTracer = Function::Create(FTy, GlobalValue::ExternalLinkage,
                                       "ArrayObjectTracer", getLLVMModule());
  
  RegularObjectTracer = Function::Create(FTy,
                                         GlobalValue::ExternalLinkage,
                                         "RegularObjectTracer",
                                         getLLVMModule());
  
  JavaObjectTracer = Function::Create(FTy, GlobalValue::ExternalLinkage,
                                      "JavaObjectTracer", getLLVMModule());
  
  ReferenceObjectTracer = Function::Create(FTy,
                                           GlobalValue::ExternalLinkage,
                                           "ReferenceObjectTracer",
                                           getLLVMModule());
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
  fprintf(stderr, "Number of native functions          : %llu\n", 
          (unsigned long long int) nativeFunctions.size());
  fprintf(stderr, "----------------- Total size in .data ------------------\n");
  uint64 size = 0;
  Module* Mod = getLLVMModule();
  for (Module::const_global_iterator i = Mod->global_begin(),
       e = Mod->global_end(); i != e; ++i) {
    size += mvm::MvmModule::getTypeSize(i->getType());
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
      ConstantExpr::getIntToPtr(ConstantInt::get(Type::getInt64Ty(getLLVMContext()),
                                                 uint64_t(isolate)),
                                ptrType);

    Module& Mod = *getLLVMModule();
    varGV = new GlobalVariable(Mod, ptrType, !staticCompilation,
                               GlobalValue::ExternalLinkage,
                               cons, "");
  
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
  llvmArgs.push_back(JavaIntrinsics.ptrType); // class loader
  llvmArgs.push_back(JavaIntrinsics.JavaCommonClassType); // cl
  const FunctionType* FTy =
    FunctionType::get(Type::getVoidTy(getLLVMContext()), llvmArgs, false);

  Function* AddClass = Function::Create(FTy, GlobalValue::ExternalLinkage,
                                        "vmjcAddPreCompiledClass",
                                        getLLVMModule());
 
  llvmArgs.clear();
  // class loader
  llvmArgs.push_back(JavaIntrinsics.ptrType);
  // array ptr
  llvmArgs.push_back(PointerType::getUnqual(JavaIntrinsics.JavaClassArrayType));
  // name
  llvmArgs.push_back(JavaIntrinsics.UTF8Type);
  FTy = FunctionType::get(Type::getVoidTy(getLLVMContext()), llvmArgs, false);
  
  Function* GetClassArray = Function::Create(FTy, GlobalValue::ExternalLinkage,
                                             "vmjcGetClassArray", getLLVMModule());
  
  BasicBlock* currentBlock = BasicBlock::Create(getLLVMContext(), "enter",
                                                StaticInitializer);
  Function::arg_iterator loader = StaticInitializer->arg_begin();
  
  Value* Args[3];
  // If we have defined some strings.
  if (strings.begin() != strings.end()) {
    llvmArgs.clear();
    llvmArgs.push_back(JavaIntrinsics.ptrType); // class loader
    llvmArgs.push_back(strings.begin()->second->getType()); // val
    FTy = FunctionType::get(Type::getVoidTy(getLLVMContext()), llvmArgs, false);
  
    Function* AddString = Function::Create(FTy, GlobalValue::ExternalLinkage,
                                           "vmjcAddString", getLLVMModule());
  

  
    for (string_iterator i = strings.begin(), e = strings.end(); i != e; ++i) {
      Args[0] = loader;
      Args[1] = i->second;
      CallInst::Create(AddString, Args, Args + 2, "", currentBlock);
    }
  }
 
#if 0
  // Disable initialization of UTF8s, it makes the Init method too big.
  // If we have defined some UTF8s.
  if (utf8s.begin() != utf8s.end()) {
    llvmArgs.clear();
    llvmArgs.push_back(JavaIntrinsics.ptrType); // class loader
    llvmArgs.push_back(utf8s.begin()->second->getType()); // val
    FTy = FunctionType::get(Type::getVoidTy(getLLVMContext()), llvmArgs, false);
  
    Function* AddUTF8 = Function::Create(FTy, GlobalValue::ExternalLinkage,
                                         "vmjcAddUTF8", getLLVMModule());
  

  
    for (utf8_iterator i = utf8s.begin(), e = utf8s.end(); i != e; ++i) {
      Args[0] = loader;
      Args[1] = i->second;
      CallInst::Create(AddUTF8, Args, Args + 2, "", currentBlock);
    }
  }
#endif

  for (native_class_iterator i = nativeClasses.begin(), 
       e = nativeClasses.end(); i != e; ++i) {
    if (isCompiling(i->first)) {
      Args[0] = loader;
      Args[1] = ConstantExpr::getBitCast(i->second,
                                         JavaIntrinsics.JavaCommonClassType);
      CallInst::Create(AddClass, Args, Args + 2, "", currentBlock);
    }
  }
  
  for (array_class_iterator i = arrayClasses.begin(), 
       e = arrayClasses.end(); i != e; ++i) {
    Args[0] = loader;
    Args[1] = i->second;
    Args[2] = getUTF8(i->first->name);
    CallInst::Create(GetClassArray, Args, Args + 3, "", currentBlock);
  }
  

  ReturnInst::Create(getLLVMContext(), currentBlock);
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
  JavaVirtualTable* VT = cl->virtualVT;
  
  if (cl->super) {
    // Copy the super VT into the current VT.
    uint32 size = cl->super->virtualTableSize - 
        JavaVirtualTable::getFirstJavaMethodIndex();
    memcpy(VT->getFirstJavaMethod(), cl->super->virtualVT->getFirstJavaMethod(),
           size * sizeof(uintptr_t));
    VT->destructor = cl->super->virtualVT->destructor;
  }
  
  for (uint32 i = 0; i < cl->nbVirtualMethods; ++i) {
    JavaMethod& meth = cl->virtualMethods[i];
    ((void**)VT)[meth.offset] = &meth;
  }

  if (!cl->super) VT->destructor = 0;

}

void JavaAOTCompiler::makeIMT(Class* cl) {
}

void JavaAOTCompiler::setMethod(Function* func, void* ptr, const char* name) {
  func->setName(name);
  func->setLinkage(GlobalValue::ExternalLinkage);
}

Value* JavaAOTCompiler::addCallback(Class* cl, uint16 index,
                                    Signdef* sign, bool stat,
                                    BasicBlock* insert) {
 
  JavaConstantPool* ctpInfo = cl->ctpInfo;
  Signdef* signature = 0;
  const UTF8* name = 0;
  const UTF8* methCl = 0;
  ctpInfo->nameOfStaticOrSpecialMethod(index, methCl, name, signature);


  fprintf(stderr, "Warning: emitting a callback from %s (%s.%s)\n",
          UTF8Buffer(cl->name).cString(), UTF8Buffer(methCl).cString(),
          UTF8Buffer(name).cString());

  LLVMSignatureInfo* LSI = getSignatureInfo(sign);
  
  const FunctionType* type = stat ? LSI->getStaticType() : 
                                    LSI->getVirtualType();
  
  Value* func = ConstantExpr::getBitCast(Callback,
                                         PointerType::getUnqual(type));
  
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



void extractFiles(ArrayUInt8* bytes,
                  JavaAOTCompiler* M,
                  JnjvmBootstrapLoader* bootstrapLoader,
                  std::vector<Class*>& classes) {
  ZipArchive archive(bytes, bootstrapLoader->allocator);
   
  mvm::ThreadAllocator allocator; 
  char* realName = (char*)allocator.Allocate(4096);
  for (ZipArchive::table_iterator i = archive.filetable.begin(), 
       e = archive.filetable.end(); i != e; ++i) {
    ZipFile* file = i->second;
     
    char* name = file->filename;
    uint32 size = strlen(name);
    if (size > 6 && !strcmp(&(name[size - 6]), ".class")) {
      UserClassArray* array = bootstrapLoader->upcalls->ArrayOfByte;
      ArrayUInt8* res = 
        (ArrayUInt8*)array->doNew(file->ucsize, JavaThread::get()->getJVM());
      int ok = archive.readFile(res, file);
      if (!ok) return;
      
      memcpy(realName, name, size);
      realName[size - 6] = 0;
      const UTF8* utf8 = bootstrapLoader->asciizConstructUTF8(realName);
      Class* cl = bootstrapLoader->constructClass(utf8, res);
      if (cl == ClassArray::SuperArray) M->compileRT = true;
      classes.push_back(cl);  
    } else if (size > 4 && (!strcmp(&name[size - 4], ".jar") || 
                            !strcmp(&name[size - 4], ".zip"))) {
      UserClassArray* array = bootstrapLoader->upcalls->ArrayOfByte;
      ArrayUInt8* res = 
        (ArrayUInt8*)array->doNew(file->ucsize, JavaThread::get()->getJVM());
      int ok = archive.readFile(res, file);
      if (!ok) return;
      
      extractFiles(res, M, bootstrapLoader, classes);
    }
  }
}


static const char* name;

void mainCompilerStart(JavaThread* th) {
  
  Jnjvm* vm = th->getJVM();
  JnjvmBootstrapLoader* bootstrapLoader = vm->bootstrapLoader;
  JavaAOTCompiler* M = (JavaAOTCompiler*)bootstrapLoader->getCompiler();
  JavaJITCompiler* Comp = 0;
  mvm::ThreadAllocator allocator;
  bootstrapLoader->analyseClasspathEnv(vm->bootstrapLoader->bootClasspathEnv);
  if (!M->clinits->empty()) {
    Comp = JavaJITCompiler::CreateCompiler("JIT");
    Comp->EmitFunctionName = true;
    bootstrapLoader->setCompiler(Comp);
    bootstrapLoader->analyseClasspathEnv(vm->classpath);
  } else {
    bootstrapLoader->analyseClasspathEnv(vm->classpath);
    bootstrapLoader->upcalls->initialiseClasspath(bootstrapLoader);
  }
  
  uint32 size = strlen(name);
    
  if (size > 4 && 
      (!strcmp(&name[size - 4], ".jar") || !strcmp(&name[size - 4], ".zip"))) {
  
    std::vector<Class*> classes;
    ArrayUInt8* bytes = Reader::openFile(bootstrapLoader, name);
      
    if (!bytes) {
      fprintf(stderr, "Can't find zip file.\n");
      goto end;
    }

    extractFiles(bytes, M, bootstrapLoader, classes);
    // Now that we know if we can trust this compiler, add the Java passes.
    M->addJavaPasses(M->compileRT);


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

      if (!M->clinits->empty()) {
        vm->loadBootstrap();

        for (std::vector<std::string>::iterator i = M->clinits->begin(),
             e = M->clinits->end(); i != e; ++i) {
          
          if (i->at(i->length() - 1) == '*') {
            for (std::vector<Class*>::iterator ii = classes.begin(),
                 ee = classes.end(); ii != ee; ++ii) {
              Class* cl = *ii;
              if (!strncmp(UTF8Buffer(cl->name).cString(), i->c_str(),
                           i->length() - 1)) {
                TRY {
                  cl->asClass()->initialiseClass(vm);
                } CATCH {
                  fprintf(stderr, "Error when initializing %s\n",
                          UTF8Buffer(cl->name).cString());
                  abort();
                } END_CATCH;
              }
            }
          } else {

            const UTF8* name = bootstrapLoader->asciizConstructUTF8(i->c_str());
            CommonClass* cl = bootstrapLoader->lookupClass(name);
            if (cl && cl->isClass()) {
              TRY {
                cl->asClass()->initialiseClass(vm);
              } CATCH {
                fprintf(stderr, "Error when initializing %s\n",
                        UTF8Buffer(cl->name).cString());
                abort();
              } END_CATCH;
            } else {
              fprintf(stderr, "Class %s does not exist or is an array class.\n",
                      i->c_str());
            }
          }
        }
        bootstrapLoader->setCompiler(M);
      }
      
      for (std::vector<Class*>::iterator i = classes.begin(), e = classes.end();
           i != e; ++i) {
        Class* cl = *i;
        cl->setOwnerClass(JavaThread::get());
      }
      
      for (std::vector<Class*>::iterator i = classes.begin(), e = classes.end();
           i != e; ++i) {
        Class* cl = *i;
        M->compileClass(cl);
      }

    } else {
      M->addJavaPasses(false);
      char* realName = (char*)allocator.Allocate(size + 1);
      if (size > 6 && !strcmp(&name[size - 6], ".class")) {
        memcpy(realName, name, size - 6);
        realName[size - 6] = 0;
      } else {
        memcpy(realName, name, size + 1);
      }
     
      const UTF8* utf8 = bootstrapLoader->asciizConstructUTF8(realName);
      UserClass* cl = bootstrapLoader->loadName(utf8, true, true, NULL);
      
      if (!M->clinits->empty()) {
        vm->loadBootstrap();
        cl->initialiseClass(vm);
        bootstrapLoader->setCompiler(M);
      }
      
      cl->setOwnerClass(JavaThread::get());
      cl->resolveInnerOuterClasses();
      for (uint32 i = 0; i < cl->nbInnerClasses; ++i) {
        cl->innerClasses[i]->setOwnerClass(JavaThread::get());
        M->compileClass(cl->innerClasses[i]);
      }
      M->compileClass(cl);
    }
 
    if (M->compileRT) {
      // Make sure that if we compile RT, the native classes are emitted.
      M->getNativeClass(bootstrapLoader->upcalls->OfVoid);
      M->getNativeClass(bootstrapLoader->upcalls->OfBool);
      M->getNativeClass(bootstrapLoader->upcalls->OfByte);
      M->getNativeClass(bootstrapLoader->upcalls->OfChar);
      M->getNativeClass(bootstrapLoader->upcalls->OfShort);
      M->getNativeClass(bootstrapLoader->upcalls->OfInt);
      M->getNativeClass(bootstrapLoader->upcalls->OfFloat);
      M->getNativeClass(bootstrapLoader->upcalls->OfLong);
      M->getNativeClass(bootstrapLoader->upcalls->OfDouble);

      // Also do not allow inling of some functions.
#define SET_INLINE(NAME) { \
      const UTF8* name = bootstrapLoader->asciizConstructUTF8(NAME); \
      Class* cl = (Class*)bootstrapLoader->lookupClass(name); \
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
    }

    M->CreateStaticInitializer();

end:

  vm->threadSystem.nonDaemonLock.lock();
  --(vm->threadSystem.nonDaemonThreads);
  if (vm->threadSystem.nonDaemonThreads == 0)
      vm->threadSystem.nonDaemonVar.signal();
  vm->threadSystem.nonDaemonLock.unlock();  
  
}

void JavaAOTCompiler::compileFile(Jnjvm* vm, const char* n) {
  name = n;
  JavaThread* th = new JavaThread(0, 0, vm);
  vm->setMainThread(th);
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
  FuncArgs.push_back(Type::getInt32Ty(getLLVMContext()));
  FuncArgs.push_back(PointerType::getUnqual(JavaIntrinsics.ptrType));
  
  FunctionType* FuncTy = FunctionType::get(Type::getInt32Ty(getLLVMContext()),
                                           FuncArgs, false);

  Function* MainFunc = Function::Create(FuncTy, GlobalValue::ExternalLinkage,
                                        "main", TheModule);
  BasicBlock* currentBlock = BasicBlock::Create(getLLVMContext(), "enter",
                                                MainFunc);
 
  GlobalVariable* GvarArrayStr = new GlobalVariable(
    *TheModule, ArrayType::get(Type::getInt8Ty(getLLVMContext()),
                               strlen(name) + 1),
    true, GlobalValue::InternalLinkage, 0, "mainClass");


  Constant* NameArray = ConstantArray::get(getLLVMContext(), name, true);
  GvarArrayStr->setInitializer(NameArray);
  Value* Indices[2] = { JavaIntrinsics.constantZero,
                        JavaIntrinsics.constantZero };
  Value* ArgName = ConstantExpr::getGetElementPtr(GvarArrayStr, Indices, 2);

  Function::arg_iterator FuncVals = MainFunc->arg_begin();
  Value* Argc = FuncVals++;
  Value* Argv = FuncVals++;
  Value* Args[3] = { Argc, Argv, ArgName };

  FuncArgs.push_back(Args[2]->getType());

  FuncTy = FunctionType::get(Type::getInt32Ty(getLLVMContext()), FuncArgs, false);

  Function* CalledFunc = 
    Function::Create(FuncTy, GlobalValue::ExternalLinkage,
                     jit ? "StartJnjvmWithJIT" : "StartJnjvmWithoutJIT",
                     TheModule);

  Value* res = CallInst::Create(CalledFunc, Args, Args + 3, "", currentBlock);
  ReturnInst::Create(getLLVMContext(), res, currentBlock);

}


// Use a FakeFunction to return from loadMethod, so that the compiler thinks
// the method is defined by J3.
extern "C" void __JavaAOTFakeFunction__() {}

void* JavaAOTCompiler::loadMethod(void* handle, const char* symbol) {
  Function* F = mvm::MvmModule::globalModule->getFunction(symbol);
  if (F) {
    return (void*)(uintptr_t)__JavaAOTFakeFunction__;
  }

  return JavaCompiler::loadMethod(handle, symbol);
}

#ifdef WITH_MMTK

#include <set>
extern std::set<gc*> __InternalSet__;

CommonClass* JavaAOTCompiler::getUniqueBaseClass(CommonClass* cl) {
  std::set<gc*>::iterator I = __InternalSet__.begin();
  std::set<gc*>::iterator E = __InternalSet__.end();
  CommonClass* currentClass = 0;

  for (; I != E; ++I) {
    JavaObject* obj = (JavaObject*)(*I);
    if (!VMClassLoader::isVMClassLoader(obj) && JavaObject::instanceOf(obj, cl)) {
      if (currentClass != NULL) {
        if (JavaObject::getClass(obj) != currentClass) {
          return 0;
        }
      } else {
        currentClass = JavaObject::getClass(obj);
      }
    }
  }
  return currentClass;
}

#else
CommonClass* JavaAOTCompiler::getUniqueBaseClass(CommonClass* cl) {
  return 0;
}
#endif
