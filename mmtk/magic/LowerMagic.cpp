//===----- LowerConstantCalls.cpp - Changes arrayLength calls  --------------===//
//
//                               JnJVM
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/Constants.h"
#include "llvm/Function.h"
#include "llvm/Instructions.h"
#include "llvm/LLVMContext.h"
#include "llvm/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Debug.h"

#include <iostream>

using namespace llvm;

namespace vmmagic {

  class VISIBILITY_HIDDEN LowerMagic : public FunctionPass {
  public:
    static char ID;
    LowerMagic() : FunctionPass((intptr_t)&ID) { }

    virtual bool runOnFunction(Function &F);
  private:
  };
  char LowerMagic::ID = 0;
  static RegisterPass<LowerMagic> X("LowerMagic",
                                    "Lower magic calls");

static const char* AddressClass = "JnJVM_org_vmmagic_unboxed_Address_";
static const char* AddressZeroMethod = 0;
static const char* AddressIsZeroMethod;
static const char* AddressMaxMethod;
static const char* AddressIsMaxMethod;
static const char* AddressFromIntSignExtendMethod;
static const char* AddressFromIntZeroExtendMethod;
static const char* AddressFromLongMethod;
static const char* AddressToObjectReferenceMethod;
static const char* AddressToIntMethod;
static const char* AddressToLongMethod;
static const char* AddressToWordMethod;
static const char* AddressPlusIntMethod;
static const char* AddressPlusOffsetMethod;
static const char* AddressPlusExtentMethod;
static const char* AddressMinusIntMethod;
static const char* AddressMinusOffsetMethod;
static const char* AddressMinusExtentMethod;
static const char* AddressDiffMethod;
static const char* AddressLTMethod;
static const char* AddressLEMethod;
static const char* AddressGTMethod;
static const char* AddressGEMethod;
static const char* AddressEQMethod;
static const char* AddressNEMethod;
static const char* AddressPrefetchMethod;
static const char* AddressLoadObjectReferenceMethod;
static const char* AddressLoadObjectReferenceAtOffsetMethod;
static const char* AddressLoadByteMethod;
static const char* AddressLoadByteAtOffsetMethod;
static const char* AddressLoadCharMethod;
static const char* AddressLoadCharAtOffsetMethod;
static const char* AddressLoadShortMethod;
static const char* AddressLoadShortAtOffsetMethod;
static const char* AddressLoadFloatMethod;
static const char* AddressLoadFloatAtOffsetMethod;
static const char* AddressLoadIntMethod;
static const char* AddressLoadIntAtOffsetMethod;
static const char* AddressLoadLongMethod;
static const char* AddressLoadLongAtOffsetMethod;
static const char* AddressLoadDoubleMethod;
static const char* AddressLoadDoubleAtOffsetMethod;
static const char* AddressLoadAddressMethod;
static const char* AddressLoadAddressAtOffsetMethod;
static const char* AddressLoadWordMethod;
static const char* AddressLoadWordAtOffsetMethod;
static const char* AddressStoreObjectReferenceMethod;
static const char* AddressStoreObjectReferenceAtOffsetMethod;
static const char* AddressStoreAddressMethod;
static const char* AddressStoreAddressAtOffsetMethod;
static const char* AddressStoreFloatMethod;
static const char* AddressStoreFloatAtOffsetMethod;
static const char* AddressStoreWordMethod;
static const char* AddressStoreWordAtOffsetMethod;
static const char* AddressStoreByteMethod;
static const char* AddressStoreByteAtOffsetMethod;
static const char* AddressStoreIntMethod;
static const char* AddressStoreIntAtOffsetMethod;
static const char* AddressStoreDoubleMethod;
static const char* AddressStoreDoubleAtOffsetMethod;
static const char* AddressStoreLongMethod;
static const char* AddressStoreLongAtOffsetMethod;
static const char* AddressStoreCharMethod;
static const char* AddressStoreCharAtOffsetMethod;
static const char* AddressStoreShortMethod;
static const char* AddressStoreShortAtOffsetMethod;
static const char* AddressPrepareWordMethod;
static const char* AddressPrepareWordAtOffsetMethod;
static const char* AddressPrepareObjectReferenceMethod;
static const char* AddressPrepareObjectReferenceAtOffsetMethod;
static const char* AddressPrepareAddressMethod;
static const char* AddressPrepareAddressAtOffsetMethod;
static const char* AddressPrepareIntMethod;
static const char* AddressPrepareIntAtOffsetMethod;
static const char* AddressAttemptIntMethod;
static const char* AddressAttemptIntAtOffsetMethod;
static const char* AddressAttemptWordMethod;
static const char* AddressAttemptWordAtOffsetMethod;
static const char* AddressAttemptObjectReferenceMethod;
static const char* AddressAttemptObjectReferenceAtOffsetMethod;
static const char* AddressAttemptAddressMethod;
static const char* AddressAttemptAddressAtOffsetMethod;

static const char* ExtentClass = "JnJVM_org_vmmagic_unboxed_Extent_";
static const char* ExtentToWordMethod = 0;
static const char* ExtentFromIntSignExtendMethod;
static const char* ExtentFromIntZeroExtendMethod;
static const char* ExtentZeroMethod;
static const char* ExtentOneMethod;
static const char* ExtentMaxMethod;
static const char* ExtentToIntMethod;
static const char* ExtentToLongMethod;
static const char* ExtentPlusIntMethod;
static const char* ExtentPlusExtentMethod;
static const char* ExtentMinusIntMethod;
static const char* ExtentMinusExtentMethod;
static const char* ExtentLTMethod;
static const char* ExtentLEMethod;
static const char* ExtentGTMethod;
static const char* ExtentGEMethod;
static const char* ExtentEQMethod;
static const char* ExtentNEMethod;

static const char* ObjectReferenceClass = 
  "JnJVM_org_vmmagic_unboxed_ObjectReference_";
static const char* ObjectReferenceFromObjectMethod = 0;
static const char* ObjectReferenceNullReferenceMethod;
static const char* ObjectReferenceToObjectMethod;
static const char* ObjectReferenceToAddressMethod;
static const char* ObjectReferenceIsNullMethod;

static const char* OffsetClass = "JnJVM_org_vmmagic_unboxed_Offset_";
static const char* OffsetFromIntSignExtendMethod = 0;
static const char* OffsetFromIntZeroExtendMethod;
static const char* OffsetZeroMethod;
static const char* OffsetMaxMethod;
static const char* OffsetToIntMethod;
static const char* OffsetToLongMethod;
static const char* OffsetToWordMethod;
static const char* OffsetPlusIntMethod;
static const char* OffsetMinusIntMethod;
static const char* OffsetMinusOffsetMethod;
static const char* OffsetEQMethod;
static const char* OffsetNEMethod;
static const char* OffsetSLTMethod;
static const char* OffsetSLEMethod;
static const char* OffsetSGTMethod;
static const char* OffsetSGEMethod;
static const char* OffsetIsZeroMethod;
static const char* OffsetIsMaxMethod;

static const char* WordClass = "JnJVM_org_vmmagic_unboxed_Word_";
static const char* WordFromIntSignExtendMethod = 0;
static const char* WordFromIntZeroExtendMethod;
static const char* WordFromLongMethod;
static const char* WordZeroMethod;
static const char* WordOneMethod;
static const char* WordMaxMethod;
static const char* WordToIntMethod;
static const char* WordToLongMethod;
static const char* WordToAddressMethod;
static const char* WordToOffsetMethod;
static const char* WordToExtentMethod;
static const char* WordPlusWordMethod;
static const char* WordPlusOffsetMethod;
static const char* WordPlusExtentMethod;
static const char* WordMinusWordMethod;
static const char* WordMinusOffsetMethod;
static const char* WordMinusExtentMethod;
static const char* WordIsZeroMethod;
static const char* WordIsMaxMethod;
static const char* WordLTMethod;
static const char* WordLEMethod;
static const char* WordGTMethod;
static const char* WordGEMethod;
static const char* WordEQMethod;
static const char* WordNEMethod;
static const char* WordAndMethod;
static const char* WordOrMethod;
static const char* WordNotMethod;
static const char* WordXorMethod;
static const char* WordLshMethod;
static const char* WordRshlMethod;
static const char* WordRshaMethod;

static Function* CASPtr;
static Function* CASInt;

static void initialiseFunctions(Module* M) {
  if (!AddressZeroMethod) {
    AddressZeroMethod = "JnJVM_org_vmmagic_unboxed_Address_zero__";
    AddressMaxMethod = "JnJVM_org_vmmagic_unboxed_Address_max__";
    AddressStoreObjectReferenceMethod = "JnJVM_org_vmmagic_unboxed_Address_store__Lorg_vmmagic_unboxed_ObjectReference_2";
    AddressLoadObjectReferenceMethod = "JnJVM_org_vmmagic_unboxed_Address_loadObjectReference__";
    AddressLoadAddressMethod = "JnJVM_org_vmmagic_unboxed_Address_loadAddress__";
    AddressLoadWordMethod = "JnJVM_org_vmmagic_unboxed_Address_loadWord__";
    AddressDiffMethod = "JnJVM_org_vmmagic_unboxed_Address_diff__Lorg_vmmagic_unboxed_Address_2";
    AddressPlusOffsetMethod = "JnJVM_org_vmmagic_unboxed_Address_plus__Lorg_vmmagic_unboxed_Offset_2";
    AddressStoreAddressMethod = "JnJVM_org_vmmagic_unboxed_Address_store__Lorg_vmmagic_unboxed_Address_2";
    AddressPlusIntMethod = "JnJVM_org_vmmagic_unboxed_Address_plus__I";
    AddressLTMethod = "JnJVM_org_vmmagic_unboxed_Address_LT__Lorg_vmmagic_unboxed_Address_2";
    AddressGEMethod = "JnJVM_org_vmmagic_unboxed_Address_GE__Lorg_vmmagic_unboxed_Address_2";
    AddressStoreWordMethod = "JnJVM_org_vmmagic_unboxed_Address_store__Lorg_vmmagic_unboxed_Word_2";
    AddressToObjectReferenceMethod = "JnJVM_org_vmmagic_unboxed_Address_toObjectReference__";
    AddressToWordMethod = "JnJVM_org_vmmagic_unboxed_Address_toWord__";
    AddressPrepareWordMethod = "JnJVM_org_vmmagic_unboxed_Address_prepareWord__";
    AddressAttemptWordAtOffsetMethod = "JnJVM_org_vmmagic_unboxed_Address_attempt__Lorg_vmmagic_unboxed_Word_2Lorg_vmmagic_unboxed_Word_2Lorg_vmmagic_unboxed_Offset_2";
    AddressPrepareWordAtOffsetMethod = "JnJVM_org_vmmagic_unboxed_Address_prepareWord__Lorg_vmmagic_unboxed_Offset_2";
    AddressLoadWordAtOffsetMethod = "JnJVM_org_vmmagic_unboxed_Address_loadWord__Lorg_vmmagic_unboxed_Offset_2";
    AddressStoreWordAtOffsetMethod = "JnJVM_org_vmmagic_unboxed_Address_store__Lorg_vmmagic_unboxed_Word_2Lorg_vmmagic_unboxed_Offset_2";
    AddressPlusExtentMethod = "JnJVM_org_vmmagic_unboxed_Address_plus__Lorg_vmmagic_unboxed_Extent_2";
    AddressIsZeroMethod = "JnJVM_org_vmmagic_unboxed_Address_isZero__";
    AddressStoreAddressAtOffsetMethod = "JnJVM_org_vmmagic_unboxed_Address_store__Lorg_vmmagic_unboxed_Address_2Lorg_vmmagic_unboxed_Offset_2";
    AddressGTMethod = "JnJVM_org_vmmagic_unboxed_Address_GT__Lorg_vmmagic_unboxed_Address_2";
    AddressLoadAddressAtOffsetMethod = "JnJVM_org_vmmagic_unboxed_Address_loadAddress__Lorg_vmmagic_unboxed_Offset_2";
    AddressEQMethod = "JnJVM_org_vmmagic_unboxed_Address_EQ__Lorg_vmmagic_unboxed_Address_2";
    AddressLoadObjectReferenceAtOffsetMethod = "JnJVM_org_vmmagic_unboxed_Address_loadObjectReference__Lorg_vmmagic_unboxed_Offset_2";
    AddressLEMethod = "JnJVM_org_vmmagic_unboxed_Address_LE__Lorg_vmmagic_unboxed_Address_2";
    AddressAttemptWordMethod = "JnJVM_org_vmmagic_unboxed_Address_attempt__Lorg_vmmagic_unboxed_Word_2Lorg_vmmagic_unboxed_Word_2";
    AddressNEMethod = "JnJVM_org_vmmagic_unboxed_Address_NE__Lorg_vmmagic_unboxed_Address_2";
    AddressToLongMethod = "JnJVM_org_vmmagic_unboxed_Address_toLong__";
    AddressMinusExtentMethod = "JnJVM_org_vmmagic_unboxed_Address_minus__Lorg_vmmagic_unboxed_Extent_2";
    AddressLoadShortAtOffsetMethod = "JnJVM_org_vmmagic_unboxed_Address_loadShort__Lorg_vmmagic_unboxed_Offset_2";
    AddressStoreShortAtOffsetMethod = "JnJVM_org_vmmagic_unboxed_Address_store__SLorg_vmmagic_unboxed_Offset_2";
    AddressLoadShortMethod = "JnJVM_org_vmmagic_unboxed_Address_loadShort__";
    AddressStoreShortMethod = "JnJVM_org_vmmagic_unboxed_Address_store__S";
    AddressLoadByteMethod = "JnJVM_org_vmmagic_unboxed_Address_loadByte__";
    AddressLoadIntMethod = "JnJVM_org_vmmagic_unboxed_Address_loadInt__";
    AddressStoreIntMethod = "JnJVM_org_vmmagic_unboxed_Address_store__I";
    AddressStoreByteMethod = "JnJVM_org_vmmagic_unboxed_Address_store__B";
    AddressLoadByteAtOffsetMethod = "JnJVM_org_vmmagic_unboxed_Address_loadByte__Lorg_vmmagic_unboxed_Offset_2";
    AddressMinusIntMethod = "JnJVM_org_vmmagic_unboxed_Address_minus__I";
    AddressLoadIntAtOffsetMethod = "JnJVM_org_vmmagic_unboxed_Address_loadInt__Lorg_vmmagic_unboxed_Offset_2";
    AddressStoreByteAtOffsetMethod = "JnJVM_org_vmmagic_unboxed_Address_store__BLorg_vmmagic_unboxed_Offset_2";
    AddressFromIntZeroExtendMethod = "JnJVM_org_vmmagic_unboxed_Address_fromIntZeroExtend__I";
    AddressToIntMethod = "JnJVM_org_vmmagic_unboxed_Address_toInt__";
    
    ExtentToWordMethod = "JnJVM_org_vmmagic_unboxed_Extent_toWord__";
    ExtentMinusExtentMethod = "JnJVM_org_vmmagic_unboxed_Extent_minus__Lorg_vmmagic_unboxed_Extent_2";
    ExtentPlusExtentMethod = "JnJVM_org_vmmagic_unboxed_Extent_plus__Lorg_vmmagic_unboxed_Extent_2";
    ExtentPlusIntMethod = "JnJVM_org_vmmagic_unboxed_Extent_plus__I";
    ExtentMinusIntMethod = "JnJVM_org_vmmagic_unboxed_Extent_minus__I";
    ExtentFromIntZeroExtendMethod = "JnJVM_org_vmmagic_unboxed_Extent_fromIntZeroExtend__I";
    ExtentFromIntSignExtendMethod = "JnJVM_org_vmmagic_unboxed_Extent_fromIntSignExtend__I";
    ExtentOneMethod = "JnJVM_org_vmmagic_unboxed_Extent_one__";
    ExtentNEMethod = "JnJVM_org_vmmagic_unboxed_Extent_NE__Lorg_vmmagic_unboxed_Extent_2";
    ExtentZeroMethod = "JnJVM_org_vmmagic_unboxed_Extent_zero__";
    ExtentToLongMethod = "JnJVM_org_vmmagic_unboxed_Extent_toLong__";
    ExtentToIntMethod = "JnJVM_org_vmmagic_unboxed_Extent_toInt__";
    ExtentEQMethod = "JnJVM_org_vmmagic_unboxed_Extent_EQ__Lorg_vmmagic_unboxed_Extent_2";
    ExtentGTMethod = "JnJVM_org_vmmagic_unboxed_Extent_GT__Lorg_vmmagic_unboxed_Extent_2";
    ExtentLTMethod = "JnJVM_org_vmmagic_unboxed_Extent_LT__Lorg_vmmagic_unboxed_Extent_2";
    ExtentMaxMethod = "JnJVM_org_vmmagic_unboxed_Extent_max__";

    ObjectReferenceFromObjectMethod = "JnJVM_org_vmmagic_unboxed_ObjectReference_fromObject__Ljava_lang_Object_2";
    ObjectReferenceToObjectMethod = "JnJVM_org_vmmagic_unboxed_ObjectReference_toObject__";
    ObjectReferenceNullReferenceMethod = "JnJVM_org_vmmagic_unboxed_ObjectReference_nullReference__";
    ObjectReferenceToAddressMethod = "JnJVM_org_vmmagic_unboxed_ObjectReference_toAddress__";
    ObjectReferenceIsNullMethod = "JnJVM_org_vmmagic_unboxed_ObjectReference_isNull__";

    WordOrMethod = "JnJVM_org_vmmagic_unboxed_Word_or__Lorg_vmmagic_unboxed_Word_2";
    WordRshlMethod = "JnJVM_org_vmmagic_unboxed_Word_rshl__I";
    WordToIntMethod = "JnJVM_org_vmmagic_unboxed_Word_toInt__";
    WordNotMethod = "JnJVM_org_vmmagic_unboxed_Word_not__";
    WordZeroMethod = "JnJVM_org_vmmagic_unboxed_Word_zero__";
    WordOneMethod = "JnJVM_org_vmmagic_unboxed_Word_one__";
    WordAndMethod = "JnJVM_org_vmmagic_unboxed_Word_and__Lorg_vmmagic_unboxed_Word_2";
    WordToAddressMethod = "JnJVM_org_vmmagic_unboxed_Word_toAddress__";
    WordLshMethod = "JnJVM_org_vmmagic_unboxed_Word_lsh__I";
    WordMinusWordMethod = "JnJVM_org_vmmagic_unboxed_Word_minus__Lorg_vmmagic_unboxed_Word_2";
    WordLTMethod = "JnJVM_org_vmmagic_unboxed_Word_LT__Lorg_vmmagic_unboxed_Word_2";
    WordPlusWordMethod = "JnJVM_org_vmmagic_unboxed_Word_plus__Lorg_vmmagic_unboxed_Word_2";
    WordLEMethod = "JnJVM_org_vmmagic_unboxed_Word_LE__Lorg_vmmagic_unboxed_Word_2";
    WordGEMethod = "JnJVM_org_vmmagic_unboxed_Word_GE__Lorg_vmmagic_unboxed_Word_2";
    WordEQMethod = "JnJVM_org_vmmagic_unboxed_Word_EQ__Lorg_vmmagic_unboxed_Word_2";
    WordNEMethod = "JnJVM_org_vmmagic_unboxed_Word_NE__Lorg_vmmagic_unboxed_Word_2";
    WordFromIntSignExtendMethod = "JnJVM_org_vmmagic_unboxed_Word_fromIntSignExtend__I";
    WordIsZeroMethod = "JnJVM_org_vmmagic_unboxed_Word_isZero__";
    WordXorMethod = "JnJVM_org_vmmagic_unboxed_Word_xor__Lorg_vmmagic_unboxed_Word_2";
    WordFromIntZeroExtendMethod = "JnJVM_org_vmmagic_unboxed_Word_fromIntZeroExtend__I";
    WordToExtentMethod = "JnJVM_org_vmmagic_unboxed_Word_toExtent__";
    WordMinusExtentMethod = "JnJVM_org_vmmagic_unboxed_Word_minus__Lorg_vmmagic_unboxed_Extent_2";
    WordToLongMethod = "JnJVM_org_vmmagic_unboxed_Word_toLong__";
    WordMaxMethod = "JnJVM_org_vmmagic_unboxed_Word_max__";
    WordToOffsetMethod = "JnJVM_org_vmmagic_unboxed_Word_toOffset__";
    WordGTMethod = "JnJVM_org_vmmagic_unboxed_Word_GT__Lorg_vmmagic_unboxed_Word_2";


    OffsetSLTMethod = "JnJVM_org_vmmagic_unboxed_Offset_sLT__Lorg_vmmagic_unboxed_Offset_2";
    OffsetFromIntSignExtendMethod = "JnJVM_org_vmmagic_unboxed_Offset_fromIntSignExtend__I";
    OffsetSGTMethod = "JnJVM_org_vmmagic_unboxed_Offset_sGT__Lorg_vmmagic_unboxed_Offset_2";
    OffsetPlusIntMethod = "JnJVM_org_vmmagic_unboxed_Offset_plus__I";
    OffsetZeroMethod = "JnJVM_org_vmmagic_unboxed_Offset_zero__";
    OffsetToWordMethod = "JnJVM_org_vmmagic_unboxed_Offset_toWord__";
    OffsetFromIntZeroExtendMethod = "JnJVM_org_vmmagic_unboxed_Offset_fromIntZeroExtend__I";
    OffsetSGEMethod = "JnJVM_org_vmmagic_unboxed_Offset_sGE__Lorg_vmmagic_unboxed_Offset_2";
    OffsetToIntMethod = "JnJVM_org_vmmagic_unboxed_Offset_toInt__";
    OffsetToLongMethod = "JnJVM_org_vmmagic_unboxed_Offset_toLong__";
    OffsetIsZeroMethod = "JnJVM_org_vmmagic_unboxed_Offset_isZero__";
    OffsetMinusIntMethod = "JnJVM_org_vmmagic_unboxed_Offset_minus__I";
    OffsetSLEMethod = "JnJVM_org_vmmagic_unboxed_Offset_sLE__Lorg_vmmagic_unboxed_Offset_2";
    OffsetEQMethod = "JnJVM_org_vmmagic_unboxed_Offset_EQ__Lorg_vmmagic_unboxed_Offset_2";
    OffsetMinusOffsetMethod = "JnJVM_org_vmmagic_unboxed_Offset_minus__Lorg_vmmagic_unboxed_Offset_2";
  }
}

#include <iostream>

static bool removePotentialNullCheck(BasicBlock* Cur, Value* Obj) {
  BasicBlock* BB = Cur->getUniquePredecessor();
  LLVMContext* Context = Cur->getParent()->getContext();
  if (BB) {
    Instruction* T = BB->getTerminator();
    if (dyn_cast<BranchInst>(T) && T != BB->begin()) {
      BasicBlock::iterator BIE = BB->end();
      --BIE; // Terminator
      --BIE; // Null test
      if (ICmpInst* IE = dyn_cast<ICmpInst>(BIE)) {
        if (IE->getPredicate() == ICmpInst::ICMP_EQ &&
            IE->getOperand(0) == Obj &&
            IE->getOperand(1) == Context->getNullValue(Obj->getType())) {
          BIE->replaceAllUsesWith(ConstantInt::getFalse());
          BIE->eraseFromParent();
          return true;
        }
      }
    }
  }
  return false;
}

bool LowerMagic::runOnFunction(Function& F) {
  Module* globalModule = F.getParent();
  LLVMContext& Context = globalModule->getContext();
  bool Changed = false;
  const llvm::Type* pointerSizeType = 
    globalModule->getPointerSize() == llvm::Module::Pointer32 ?
      Type::Int32Ty : Type::Int64Ty;
  
  initialiseFunctions(globalModule);
  if (!CASPtr || CASPtr->getParent() != globalModule) {
    if (pointerSizeType == Type::Int32Ty) {
      CASPtr = globalModule->getFunction("llvm.atomic.cmp.swap.i32.p0i32");
    } else {
      CASPtr = globalModule->getFunction("llvm.atomic.cmp.swap.i64.p0i64");
    }
    CASInt = globalModule->getFunction("llvm.atomic.cmp.swap.i32.p0i32");
  }

  for (Function::iterator BI = F.begin(), BE = F.end(); BI != BE; BI++) { 
    BasicBlock *Cur = BI; 
    for (BasicBlock::iterator II = Cur->begin(), IE = Cur->end(); II != IE;) {
      Instruction *I = II;
      II++;
      CallSite Call = CallSite::get(I);
      Instruction* CI = Call.getInstruction();
      if (CI) {
        Value* V = Call.getCalledValue();
        if (Function* FCur = dyn_cast<Function>(V)) {
          const char* name = FCur->getNameStart();
          unsigned len = FCur->getNameLen();
          if (len > strlen(AddressClass) && 
              !memcmp(AddressClass, name, strlen(AddressClass))) {
            
            Changed = true;
            // Remove the null check
            if (Call.arg_begin() != Call.arg_end()) {
              removePotentialNullCheck(Cur, Call.getArgument(0));
            }

            if (!strcmp(FCur->getNameStart(), AddressZeroMethod)) {
              Constant* N = Context.getNullValue(FCur->getReturnType());
              CI->replaceAllUsesWith(N);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), AddressMaxMethod)) {
              ConstantInt* M = Context.getConstantInt(Type::Int64Ty, (uint64_t)-1);
              Constant* N = ConstantExpr::getIntToPtr(M, FCur->getReturnType());
              CI->replaceAllUsesWith(N);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), AddressStoreObjectReferenceMethod) ||
                       !strcmp(FCur->getNameStart(), AddressStoreAddressMethod) ||
                       !strcmp(FCur->getNameStart(), AddressStoreShortMethod) ||
                       !strcmp(FCur->getNameStart(), AddressStoreByteMethod) ||
                       !strcmp(FCur->getNameStart(), AddressStoreIntMethod) ||
                       !strcmp(FCur->getNameStart(), AddressStoreWordMethod)) {
              Value* Addr = Call.getArgument(0);
              Value* Obj = Call.getArgument(1);
              const llvm::Type* Ty = PointerType::getUnqual(Obj->getType());
              Addr = new BitCastInst(Addr, Ty, "", CI);
              new StoreInst(Obj, Addr, CI);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), AddressLoadObjectReferenceMethod) ||
                       !strcmp(FCur->getNameStart(), AddressLoadAddressMethod) ||
                       !strcmp(FCur->getNameStart(), AddressLoadWordMethod) ||
                       !strcmp(FCur->getNameStart(), AddressLoadShortMethod) ||
                       !strcmp(FCur->getNameStart(), AddressLoadByteMethod) ||
                       !strcmp(FCur->getNameStart(), AddressLoadIntMethod) ||
                       !strcmp(FCur->getNameStart(), AddressPrepareWordMethod)) {
              Value* Addr = Call.getArgument(0);
              const Type* Ty = PointerType::getUnqual(FCur->getReturnType());
              Addr = new BitCastInst(Addr, Ty, "", CI);
              Value* LD = new LoadInst(Addr, "", CI);
              CI->replaceAllUsesWith(LD);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), AddressDiffMethod) ||
                       !strcmp(FCur->getNameStart(), AddressMinusExtentMethod)) {
              Value* Val1 = Call.getArgument(0);
              Value* Val2 = Call.getArgument(1);
              Val1 = new PtrToIntInst(Val1, pointerSizeType, "", CI);
              Val2 = new PtrToIntInst(Val2, pointerSizeType, "", CI);
              Value* res = BinaryOperator::CreateSub(Val1, Val2, "", CI);
              res = new IntToPtrInst(res, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(res);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), AddressPlusOffsetMethod)) {
              Value* Val1 = Call.getArgument(0);
              Value* Val2 = Call.getArgument(1);
              Val1 = new PtrToIntInst(Val1, pointerSizeType, "", CI);
              Val2 = new PtrToIntInst(Val2, pointerSizeType, "", CI);
              Value* res = BinaryOperator::CreateAdd(Val1, Val2, "", CI);
              res = new IntToPtrInst(res, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(res);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), AddressPlusIntMethod)) {
              Value* Val1 = Call.getArgument(0);
              Value* Val2 = Call.getArgument(1);
              Val1 = new PtrToIntInst(Val1, pointerSizeType, "", CI);
              if (Val2->getType() != pointerSizeType)
                Val2 = new ZExtInst(Val2, pointerSizeType, "", CI);
              Value* res = BinaryOperator::CreateAdd(Val1, Val2, "", CI);
              res = new IntToPtrInst(res, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(res);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), AddressMinusIntMethod)) {
              Value* Val1 = Call.getArgument(0);
              Value* Val2 = Call.getArgument(1);
              Val1 = new PtrToIntInst(Val1, pointerSizeType, "", CI);
              if (Val2->getType() != pointerSizeType)
                Val2 = new ZExtInst(Val2, pointerSizeType, "", CI);
              Value* res = BinaryOperator::CreateSub(Val1, Val2, "", CI);
              res = new IntToPtrInst(res, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(res);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), AddressLTMethod)) {
              Value* Val1 = Call.getArgument(0);
              Value* Val2 = Call.getArgument(1);
              Val1 = new PtrToIntInst(Val1, pointerSizeType, "", CI);
              Val2 = new PtrToIntInst(Val2, pointerSizeType, "", CI);
              Value* res = new ICmpInst(CI, ICmpInst::ICMP_ULT, Val1, Val2, "");
              res = new ZExtInst(res, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(res);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), AddressGTMethod)) {
              Value* Val1 = Call.getArgument(0);
              Value* Val2 = Call.getArgument(1);
              Val1 = new PtrToIntInst(Val1, pointerSizeType, "", CI);
              Val2 = new PtrToIntInst(Val2, pointerSizeType, "", CI);
              Value* res = new ICmpInst(CI, ICmpInst::ICMP_UGT, Val1, Val2, "");
              res = new ZExtInst(res, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(res);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), AddressEQMethod)) {
              Value* Val1 = Call.getArgument(0);
              Value* Val2 = Call.getArgument(1);
              Val1 = new PtrToIntInst(Val1, pointerSizeType, "", CI);
              Val2 = new PtrToIntInst(Val2, pointerSizeType, "", CI);
              Value* res = new ICmpInst(CI, ICmpInst::ICMP_EQ, Val1, Val2, "");
              res = new ZExtInst(res, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(res);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), AddressNEMethod)) {
              Value* Val1 = Call.getArgument(0);
              Value* Val2 = Call.getArgument(1);
              Val1 = new PtrToIntInst(Val1, pointerSizeType, "", CI);
              Val2 = new PtrToIntInst(Val2, pointerSizeType, "", CI);
              Value* res = new ICmpInst(CI, ICmpInst::ICMP_NE, Val1, Val2, "");
              res = new ZExtInst(res, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(res);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), AddressLEMethod)) {
              Value* Val1 = Call.getArgument(0);
              Value* Val2 = Call.getArgument(1);
              Val1 = new PtrToIntInst(Val1, pointerSizeType, "", CI);
              Val2 = new PtrToIntInst(Val2, pointerSizeType, "", CI);
              Value* res = new ICmpInst(CI, ICmpInst::ICMP_ULE, Val1, Val2, "");
              res = new ZExtInst(res, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(res);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), AddressGEMethod)) {
              Value* Val1 = Call.getArgument(0);
              Value* Val2 = Call.getArgument(1);
              Val1 = new PtrToIntInst(Val1, pointerSizeType, "", CI);
              Val2 = new PtrToIntInst(Val2, pointerSizeType, "", CI);
              Value* res = new ICmpInst(CI, ICmpInst::ICMP_UGE, Val1, Val2, "");
              res = new ZExtInst(res, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(res);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), AddressToObjectReferenceMethod) ||
                       !strcmp(FCur->getNameStart(), AddressToWordMethod)) {
              Value* Val = Call.getArgument(0);
              Val = new BitCastInst(Val, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(Val);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), AddressAttemptWordAtOffsetMethod)) {
              Value* Ptr = Call.getArgument(0);
              Value* Old = Call.getArgument(1);
              Value* Val = Call.getArgument(2);
              Value* Offset = Call.getArgument(3);

              Ptr = new PtrToIntInst(Ptr, pointerSizeType, "", CI);
              Offset = new PtrToIntInst(Offset, pointerSizeType, "", CI);
              Ptr = BinaryOperator::CreateAdd(Ptr, Offset, "", CI);
              const Type* Ty = PointerType::getUnqual(pointerSizeType);
              Ptr = new IntToPtrInst(Ptr, Ty, "", CI);
              Old = new PtrToIntInst(Old, pointerSizeType, "", CI);
              Val = new PtrToIntInst(Val, pointerSizeType, "", CI);
              
              Value* Args[3] = { Ptr, Old, Val };
              Value* res = CallInst::Create(CASPtr, Args, Args + 3, "", CI);
              res = new ICmpInst(CI, ICmpInst::ICMP_EQ, res, Old, "");
              res = new ZExtInst(res, FCur->getReturnType(), "", CI);

              CI->replaceAllUsesWith(res);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), AddressAttemptWordMethod)) {
              Value* Ptr = Call.getArgument(0);
              Value* Old = Call.getArgument(1);
              Value* Val = Call.getArgument(2);

              const Type* Ty = PointerType::getUnqual(pointerSizeType);
              Ptr = new BitCastInst(Ptr, Ty, "", CI);
              Old = new PtrToIntInst(Old, pointerSizeType, "", CI);
              Val = new PtrToIntInst(Val, pointerSizeType, "", CI);
              
              Value* Args[3] = { Ptr, Old, Val };
              Value* res = CallInst::Create(CASPtr, Args, Args + 3, "", CI);
              res = new ICmpInst(CI, ICmpInst::ICMP_EQ, res, Old, "");
              res = new ZExtInst(res, FCur->getReturnType(), "", CI);

              CI->replaceAllUsesWith(res);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), AddressPrepareWordAtOffsetMethod) ||
                       !strcmp(FCur->getNameStart(), AddressLoadWordAtOffsetMethod) ||
                       !strcmp(FCur->getNameStart(), AddressLoadAddressAtOffsetMethod) ||
                       !strcmp(FCur->getNameStart(), AddressLoadObjectReferenceAtOffsetMethod) ||
                       !strcmp(FCur->getNameStart(), AddressLoadByteAtOffsetMethod) ||
                       !strcmp(FCur->getNameStart(), AddressLoadIntAtOffsetMethod) ||
                       !strcmp(FCur->getNameStart(), AddressLoadShortAtOffsetMethod)) {
              Value* Ptr = Call.getArgument(0);
              Value* Offset = Call.getArgument(1);

              Ptr = new PtrToIntInst(Ptr, pointerSizeType, "", CI);
              Offset = new PtrToIntInst(Offset, pointerSizeType, "", CI);
              Ptr = BinaryOperator::CreateAdd(Ptr, Offset, "", CI);
              const Type* Ty = PointerType::getUnqual(FCur->getReturnType());
              Ptr = new IntToPtrInst(Ptr, Ty, "", CI);
              Value* res = new LoadInst(Ptr, "", CI);

              CI->replaceAllUsesWith(res);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), AddressStoreWordAtOffsetMethod) ||
                       !strcmp(FCur->getNameStart(), AddressStoreAddressAtOffsetMethod) ||
                       !strcmp(FCur->getNameStart(), AddressStoreByteAtOffsetMethod) ||
                       !strcmp(FCur->getNameStart(), AddressStoreShortAtOffsetMethod)) {
              Value* Ptr = Call.getArgument(0);
              Value* Val = Call.getArgument(1);
              Value* Offset = Call.getArgument(2);

              Ptr = new PtrToIntInst(Ptr, pointerSizeType, "", CI);
              Offset = new PtrToIntInst(Offset, pointerSizeType, "", CI);
              Ptr = BinaryOperator::CreateAdd(Ptr, Offset, "", CI);
              const Type* Ty = PointerType::getUnqual(Val->getType());
              Ptr = new IntToPtrInst(Ptr, Ty, "", CI);
              new StoreInst(Val, Ptr, CI);

              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), AddressPlusExtentMethod)) {
              Value* Val1 = Call.getArgument(0);
              Value* Val2 = Call.getArgument(1);
              Val1 = new PtrToIntInst(Val1, pointerSizeType, "", CI);
              Val2 = new PtrToIntInst(Val2, pointerSizeType, "", CI);
              Value* res = BinaryOperator::CreateAdd(Val1, Val2, "", CI);
              res = new IntToPtrInst(res, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(res);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), AddressIsZeroMethod)) {
              Value* Val = Call.getArgument(0);
              Constant* N = Context.getNullValue(Val->getType());
              Value* Res = new ICmpInst(CI, ICmpInst::ICMP_EQ, Val, N, "");
              Res = new ZExtInst(Res, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(Res);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), AddressToLongMethod)) {
              Value* Val = Call.getArgument(0);
              Val = new PtrToIntInst(Val, Type::Int64Ty, "", CI);
              CI->replaceAllUsesWith(Val);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), AddressFromIntZeroExtendMethod)) {
              Value* Val = Call.getArgument(0);
              if (pointerSizeType != Type::Int32Ty)
                Val = new ZExtInst(Val, pointerSizeType, "", CI);
              Val = new IntToPtrInst(Val, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(Val);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), AddressToIntMethod)) {
              Value* Val = Call.getArgument(0);
              Val = new PtrToIntInst(Val, Type::Int32Ty, "", CI);
              CI->replaceAllUsesWith(Val);
              CI->eraseFromParent();
            } else {
              fprintf(stderr, "Implement me %s\n", name);
              abort();
            }

          } else if (len > strlen(ExtentClass) && 
              !memcmp(ExtentClass, name, strlen(ExtentClass))) {
            
            Changed = true;
            // Remove the null check
            if (Call.arg_begin() != Call.arg_end()) {
              removePotentialNullCheck(Cur, Call.getArgument(0));
            }

            if (!strcmp(FCur->getNameStart(), ExtentToWordMethod)) {
              CI->replaceAllUsesWith(Call.getArgument(0));
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), ExtentMinusExtentMethod)) {
              Value* Val1 = Call.getArgument(0);
              Value* Val2 = Call.getArgument(1);
              Val1 = new PtrToIntInst(Val1, pointerSizeType, "", CI);
              Val2 = new PtrToIntInst(Val2, pointerSizeType, "", CI);
              Value* res = BinaryOperator::CreateSub(Val1, Val2, "", CI);
              res = new IntToPtrInst(res, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(res);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), ExtentPlusExtentMethod)) {
              Value* Val1 = Call.getArgument(0);
              Value* Val2 = Call.getArgument(1);
              Val1 = new PtrToIntInst(Val1, pointerSizeType, "", CI);
              Val2 = new PtrToIntInst(Val2, pointerSizeType, "", CI);
              Value* res = BinaryOperator::CreateAdd(Val1, Val2, "", CI);
              res = new IntToPtrInst(res, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(res);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), ExtentPlusIntMethod)) {
              Value* Val1 = Call.getArgument(0);
              Value* Val2 = Call.getArgument(1);
              Val1 = new PtrToIntInst(Val1, pointerSizeType, "", CI);
              if (Val2->getType() != pointerSizeType)
                Val2 = new ZExtInst(Val2, pointerSizeType, "", CI);
              Value* res = BinaryOperator::CreateAdd(Val1, Val2, "", CI);
              res = new IntToPtrInst(res, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(res);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), ExtentMinusIntMethod)) {
              Value* Val1 = Call.getArgument(0);
              Value* Val2 = Call.getArgument(1);
              Val1 = new PtrToIntInst(Val1, pointerSizeType, "", CI);
              if (Val2->getType() != pointerSizeType)
                Val2 = new ZExtInst(Val2, pointerSizeType, "", CI);
              Value* res = BinaryOperator::CreateSub(Val1, Val2, "", CI);
              res = new IntToPtrInst(res, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(res);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), ExtentFromIntZeroExtendMethod)) {
              Value* Val = Call.getArgument(0);
              if (pointerSizeType != Type::Int32Ty)
                Val = new ZExtInst(Val, pointerSizeType, "", CI);
              Val = new IntToPtrInst(Val, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(Val);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), ExtentFromIntSignExtendMethod)) {
              Value* Val = Call.getArgument(0);
              if (pointerSizeType != Type::Int32Ty)
                Val = new SExtInst(Val, pointerSizeType, "", CI);
              Val = new IntToPtrInst(Val, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(Val);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), ExtentOneMethod)) {
              Constant* N = Context.getConstantInt(pointerSizeType, 1);
              N = ConstantExpr::getIntToPtr(N, FCur->getReturnType());
              CI->replaceAllUsesWith(N);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), ExtentNEMethod)) {
              Value* Val1 = Call.getArgument(0);
              Value* Val2 = Call.getArgument(1);
              Val1 = new PtrToIntInst(Val1, pointerSizeType, "", CI);
              Val2 = new PtrToIntInst(Val2, pointerSizeType, "", CI);
              Value* res = new ICmpInst(CI, ICmpInst::ICMP_NE, Val1, Val2, "");
              res = new ZExtInst(res, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(res);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), ExtentEQMethod)) {
              Value* Val1 = Call.getArgument(0);
              Value* Val2 = Call.getArgument(1);
              Val1 = new PtrToIntInst(Val1, pointerSizeType, "", CI);
              Val2 = new PtrToIntInst(Val2, pointerSizeType, "", CI);
              Value* res = new ICmpInst(CI, ICmpInst::ICMP_EQ, Val1, Val2, "");
              res = new ZExtInst(res, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(res);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), ExtentGTMethod)) {
              Value* Val1 = Call.getArgument(0);
              Value* Val2 = Call.getArgument(1);
              Val1 = new PtrToIntInst(Val1, pointerSizeType, "", CI);
              Val2 = new PtrToIntInst(Val2, pointerSizeType, "", CI);
              Value* res = new ICmpInst(CI, ICmpInst::ICMP_UGT, Val1, Val2, "");
              res = new ZExtInst(res, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(res);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), ExtentLTMethod)) {
              Value* Val1 = Call.getArgument(0);
              Value* Val2 = Call.getArgument(1);
              Val1 = new PtrToIntInst(Val1, pointerSizeType, "", CI);
              Val2 = new PtrToIntInst(Val2, pointerSizeType, "", CI);
              Value* res = new ICmpInst(CI, ICmpInst::ICMP_ULT, Val1, Val2, "");
              res = new ZExtInst(res, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(res);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), ExtentZeroMethod)) {
              Constant* N = Context.getNullValue(FCur->getReturnType());
              CI->replaceAllUsesWith(N);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), ExtentToLongMethod)) {
              Value* Val = Call.getArgument(0);
              Val = new PtrToIntInst(Val, Type::Int64Ty, "", CI);
              CI->replaceAllUsesWith(Val);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), ExtentToIntMethod)) {
              Value* Val = Call.getArgument(0);
              Val = new PtrToIntInst(Val, Type::Int32Ty, "", CI);
              CI->replaceAllUsesWith(Val);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), ExtentMaxMethod)) {
              ConstantInt* M = Context.getConstantInt(Type::Int64Ty, (uint64_t)-1);
              Constant* N = ConstantExpr::getIntToPtr(M, FCur->getReturnType());
              CI->replaceAllUsesWith(N);
              CI->eraseFromParent();
            } else {
              fprintf(stderr, "Implement me %s\n", name);
              abort();
            }
          } else if (len > strlen(OffsetClass) && 
              !memcmp(OffsetClass, name, strlen(OffsetClass))) {
            
            Changed = true;
            // Remove the null check
            if (Call.arg_begin() != Call.arg_end()) {
              removePotentialNullCheck(Cur, Call.getArgument(0));
            }
            
            if (!strcmp(FCur->getNameStart(), OffsetSLTMethod)) {
              Value* Val1 = Call.getArgument(0);
              Value* Val2 = Call.getArgument(1);
              Val1 = new PtrToIntInst(Val1, pointerSizeType, "", CI);
              Val2 = new PtrToIntInst(Val2, pointerSizeType, "", CI);
              Value* res = new ICmpInst(CI, ICmpInst::ICMP_SLT, Val1, Val2, "");
              res = new ZExtInst(res, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(res);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), OffsetToWordMethod)) {
              Value* Val = Call.getArgument(0);
              Val = new BitCastInst(Val, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(Val);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), OffsetZeroMethod)) {
              Constant* N = Context.getNullValue(FCur->getReturnType());
              CI->replaceAllUsesWith(N);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), OffsetSGTMethod)) {
              Value* Val1 = Call.getArgument(0);
              Value* Val2 = Call.getArgument(1);
              Val1 = new PtrToIntInst(Val1, pointerSizeType, "", CI);
              Val2 = new PtrToIntInst(Val2, pointerSizeType, "", CI);
              Value* res = new ICmpInst(CI, ICmpInst::ICMP_SGT, Val1, Val2, "");
              res = new ZExtInst(res, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(res);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), OffsetSGEMethod)) {
              Value* Val1 = Call.getArgument(0);
              Value* Val2 = Call.getArgument(1);
              Val1 = new PtrToIntInst(Val1, pointerSizeType, "", CI);
              Val2 = new PtrToIntInst(Val2, pointerSizeType, "", CI);
              Value* res = new ICmpInst(CI, ICmpInst::ICMP_SGE, Val1, Val2, "");
              res = new ZExtInst(res, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(res);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), OffsetSLEMethod)) {
              Value* Val1 = Call.getArgument(0);
              Value* Val2 = Call.getArgument(1);
              Val1 = new PtrToIntInst(Val1, pointerSizeType, "", CI);
              Val2 = new PtrToIntInst(Val2, pointerSizeType, "", CI);
              Value* res = new ICmpInst(CI, ICmpInst::ICMP_SLE, Val1, Val2, "");
              res = new ZExtInst(res, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(res);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), OffsetEQMethod)) {
              Value* Val1 = Call.getArgument(0);
              Value* Val2 = Call.getArgument(1);
              Val1 = new PtrToIntInst(Val1, pointerSizeType, "", CI);
              Val2 = new PtrToIntInst(Val2, pointerSizeType, "", CI);
              Value* res = new ICmpInst(CI, ICmpInst::ICMP_EQ, Val1, Val2, "");
              res = new ZExtInst(res, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(res);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), OffsetFromIntSignExtendMethod)) {
              Value* Val = Call.getArgument(0);
              if (pointerSizeType != Type::Int32Ty)
                Val = new SExtInst(Val, pointerSizeType, "", CI);
              Val = new IntToPtrInst(Val, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(Val);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), OffsetFromIntZeroExtendMethod)) {
              Value* Val = Call.getArgument(0);
              if (pointerSizeType != Type::Int32Ty)
                Val = new ZExtInst(Val, pointerSizeType, "", CI);
              Val = new IntToPtrInst(Val, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(Val);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), OffsetPlusIntMethod)) {
              Value* Val1 = Call.getArgument(0);
              Value* Val2 = Call.getArgument(1);
              Val1 = new PtrToIntInst(Val1, pointerSizeType, "", CI);
              if (Val2->getType() != pointerSizeType)
                Val2 = new ZExtInst(Val2, pointerSizeType, "", CI);
              Value* res = BinaryOperator::CreateAdd(Val1, Val2, "", CI);
              res = new IntToPtrInst(res, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(res);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), OffsetToIntMethod)) {
              Value* Val = Call.getArgument(0);
              Val = new PtrToIntInst(Val, Type::Int32Ty, "", CI);
              CI->replaceAllUsesWith(Val);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), OffsetToLongMethod)) {
              Value* Val = Call.getArgument(0);
              Val = new PtrToIntInst(Val, Type::Int64Ty, "", CI);
              CI->replaceAllUsesWith(Val);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), OffsetIsZeroMethod)) {
              Value* Val = Call.getArgument(0);
              Constant* N = Context.getNullValue(Val->getType());
              Value* Res = new ICmpInst(CI, ICmpInst::ICMP_EQ, Val, N, "");
              Res = new ZExtInst(Res, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(Res);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), OffsetMinusIntMethod)) {
              Value* Val1 = Call.getArgument(0);
              Value* Val2 = Call.getArgument(1);
              Val1 = new PtrToIntInst(Val1, pointerSizeType, "", CI);
              if (Val2->getType() != pointerSizeType)
                Val2 = new ZExtInst(Val2, pointerSizeType, "", CI);
              Value* res = BinaryOperator::CreateSub(Val1, Val2, "", CI);
              res = new IntToPtrInst(res, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(res);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), OffsetMinusOffsetMethod)) {
              Value* Val1 = Call.getArgument(0);
              Value* Val2 = Call.getArgument(1);
              Val1 = new PtrToIntInst(Val1, pointerSizeType, "", CI);
              Val2 = new PtrToIntInst(Val2, pointerSizeType, "", CI);
              Value* res = BinaryOperator::CreateSub(Val1, Val2, "", CI);
              res = new IntToPtrInst(res, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(res);
              CI->eraseFromParent();
            } else {
              fprintf(stderr, "Implement me %s\n", name);
              abort();
            }
          } else if (len > strlen(ObjectReferenceClass) && 
            !memcmp(ObjectReferenceClass, name, strlen(ObjectReferenceClass))) {
           
            Changed = true;
            // Remove the null check
            if (Call.arg_begin() != Call.arg_end()) {
              removePotentialNullCheck(Cur, Call.getArgument(0));
            }

            if (!strcmp(FCur->getNameStart(), ObjectReferenceNullReferenceMethod)) {
              Constant* N = Context.getNullValue(FCur->getReturnType());
              CI->replaceAllUsesWith(N);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), ObjectReferenceFromObjectMethod)) {
              Value* Val = Call.getArgument(0);
              Val = new BitCastInst(Val, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(Val);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), ObjectReferenceToAddressMethod)) {
              Value* Val = Call.getArgument(0);
              Val = new BitCastInst(Val, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(Val);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), ObjectReferenceToObjectMethod)) {
              Value* Val = Call.getArgument(0);
              Val = new BitCastInst(Val, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(Val);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), ObjectReferenceIsNullMethod)) {
              Value* Val = Call.getArgument(0);
              Constant* N = Context.getNullValue(Val->getType());
              Value* Res = new ICmpInst(CI, ICmpInst::ICMP_EQ, Val, N, "");
              Res = new ZExtInst(Res, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(Res);
              CI->eraseFromParent();
            } else {
              fprintf(stderr, "Implement me %s\n", name);
              abort();
            }
          } else if (len > strlen(WordClass) && 
              !memcmp(WordClass, name, strlen(WordClass))) {
           
            Changed = true;
            // Remove the null check
            if (Call.arg_begin() != Call.arg_end()) {
              removePotentialNullCheck(Cur, Call.getArgument(0));
            }
             
            if (!strcmp(FCur->getNameStart(), WordOrMethod)) {
              Value* Val1 = Call.getArgument(0);
              Value* Val2 = Call.getArgument(1);
              Val1 = new PtrToIntInst(Val1, pointerSizeType, "", CI);
              Val2 = new PtrToIntInst(Val2, pointerSizeType, "", CI);
              Value* Res = BinaryOperator::CreateOr(Val1, Val2, "", CI);
              Res = new IntToPtrInst(Res, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(Res);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), WordAndMethod)) {
              Value* Val1 = Call.getArgument(0);
              Value* Val2 = Call.getArgument(1);
              Val1 = new PtrToIntInst(Val1, pointerSizeType, "", CI);
              Val2 = new PtrToIntInst(Val2, pointerSizeType, "", CI);
              Value* Res = BinaryOperator::CreateAnd(Val1, Val2, "", CI);
              Res = new IntToPtrInst(Res, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(Res);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), WordXorMethod)) {
              Value* Val1 = Call.getArgument(0);
              Value* Val2 = Call.getArgument(1);
              Val1 = new PtrToIntInst(Val1, pointerSizeType, "", CI);
              Val2 = new PtrToIntInst(Val2, pointerSizeType, "", CI);
              Value* Res = BinaryOperator::CreateXor(Val1, Val2, "", CI);
              Res = new IntToPtrInst(Res, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(Res);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), WordRshlMethod)) {
              Value* Val1 = Call.getArgument(0);
              Value* Val2 = Call.getArgument(1);
              Val1 = new PtrToIntInst(Val1, pointerSizeType, "", CI);
              if (Val2->getType() != pointerSizeType)
                Val2 = new ZExtInst(Val2, pointerSizeType, "", CI);
              Value* Res = BinaryOperator::CreateLShr(Val1, Val2, "", CI);
              Res = new IntToPtrInst(Res, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(Res);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), WordLshMethod)) {
              Value* Val1 = Call.getArgument(0);
              Value* Val2 = Call.getArgument(1);
              Val1 = new PtrToIntInst(Val1, pointerSizeType, "", CI);
              if (Val2->getType() != pointerSizeType)
                Val2 = new ZExtInst(Val2, pointerSizeType, "", CI);
              Value* Res = BinaryOperator::CreateShl(Val1, Val2, "", CI);
              Res = new IntToPtrInst(Res, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(Res);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), WordToIntMethod)) {
              Value* Val = Call.getArgument(0);
              Val = new PtrToIntInst(Val, Type::Int32Ty, "", CI);
              CI->replaceAllUsesWith(Val);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), WordNotMethod)) {
              Value* Val = Call.getArgument(0);
              Val = new PtrToIntInst(Val, pointerSizeType, "", CI);
              Constant* M1 = Context.getConstantInt(pointerSizeType, -1);
              Value* Res = BinaryOperator::CreateXor(Val, M1, "", CI);
              Res = new IntToPtrInst(Res, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(Res);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), WordZeroMethod)) {
              Constant* N = Context.getNullValue(FCur->getReturnType());
              CI->replaceAllUsesWith(N);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), WordOneMethod)) {
              Constant* N = Context.getConstantInt(pointerSizeType, 1);
              N = ConstantExpr::getIntToPtr(N, FCur->getReturnType());
              CI->replaceAllUsesWith(N);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), WordToAddressMethod) ||
                       !strcmp(FCur->getNameStart(), WordToOffsetMethod) ||
                       !strcmp(FCur->getNameStart(), WordToExtentMethod)) {
              Value* Val = Call.getArgument(0);
              Val = new BitCastInst(Val, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(Val);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), WordMinusWordMethod) ||
                       !strcmp(FCur->getNameStart(), WordMinusExtentMethod)) {
              Value* Val1 = Call.getArgument(0);
              Value* Val2 = Call.getArgument(1);
              Val1 = new PtrToIntInst(Val1, pointerSizeType, "", CI);
              Val2 = new PtrToIntInst(Val2, pointerSizeType, "", CI);
              Value* res = BinaryOperator::CreateSub(Val1, Val2, "", CI);
              res = new IntToPtrInst(res, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(res);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), WordPlusWordMethod)) {
              Value* Val1 = Call.getArgument(0);
              Value* Val2 = Call.getArgument(1);
              Val1 = new PtrToIntInst(Val1, pointerSizeType, "", CI);
              Val2 = new PtrToIntInst(Val2, pointerSizeType, "", CI);
              Value* res = BinaryOperator::CreateAdd(Val1, Val2, "", CI);
              res = new IntToPtrInst(res, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(res);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), WordLTMethod)) {
              Value* Val1 = Call.getArgument(0);
              Value* Val2 = Call.getArgument(1);
              Val1 = new PtrToIntInst(Val1, pointerSizeType, "", CI);
              Val2 = new PtrToIntInst(Val2, pointerSizeType, "", CI);
              Value* res = new ICmpInst(CI, ICmpInst::ICMP_ULT, Val1, Val2, "");
              res = new ZExtInst(res, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(res);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), WordLEMethod)) {
              Value* Val1 = Call.getArgument(0);
              Value* Val2 = Call.getArgument(1);
              Val1 = new PtrToIntInst(Val1, pointerSizeType, "", CI);
              Val2 = new PtrToIntInst(Val2, pointerSizeType, "", CI);
              Value* res = new ICmpInst(CI, ICmpInst::ICMP_ULE, Val1, Val2, "");
              res = new ZExtInst(res, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(res);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), WordGEMethod)) {
              Value* Val1 = Call.getArgument(0);
              Value* Val2 = Call.getArgument(1);
              Val1 = new PtrToIntInst(Val1, pointerSizeType, "", CI);
              Val2 = new PtrToIntInst(Val2, pointerSizeType, "", CI);
              Value* res = new ICmpInst(CI, ICmpInst::ICMP_UGE, Val1, Val2, "");
              res = new ZExtInst(res, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(res);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), WordEQMethod)) {
              Value* Val1 = Call.getArgument(0);
              Value* Val2 = Call.getArgument(1);
              Val1 = new PtrToIntInst(Val1, pointerSizeType, "", CI);
              Val2 = new PtrToIntInst(Val2, pointerSizeType, "", CI);
              Value* res = new ICmpInst(CI, ICmpInst::ICMP_EQ, Val1, Val2, "");
              res = new ZExtInst(res, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(res);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), WordGTMethod)) {
              Value* Val1 = Call.getArgument(0);
              Value* Val2 = Call.getArgument(1);
              Val1 = new PtrToIntInst(Val1, pointerSizeType, "", CI);
              Val2 = new PtrToIntInst(Val2, pointerSizeType, "", CI);
              Value* res = new ICmpInst(CI, ICmpInst::ICMP_UGT, Val1, Val2, "");
              res = new ZExtInst(res, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(res);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), WordNEMethod)) {
              Value* Val1 = Call.getArgument(0);
              Value* Val2 = Call.getArgument(1);
              Val1 = new PtrToIntInst(Val1, pointerSizeType, "", CI);
              Val2 = new PtrToIntInst(Val2, pointerSizeType, "", CI);
              Value* res = new ICmpInst(CI, ICmpInst::ICMP_NE, Val1, Val2, "");
              res = new ZExtInst(res, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(res);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), WordFromIntSignExtendMethod)) {
              Value* Val = Call.getArgument(0);
              if (pointerSizeType != Type::Int32Ty)
                Val = new SExtInst(Val, pointerSizeType, "", CI);
              Val = new IntToPtrInst(Val, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(Val);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), WordFromIntZeroExtendMethod)) {
              Value* Val = Call.getArgument(0);
              if (pointerSizeType != Type::Int32Ty)
                Val = new ZExtInst(Val, pointerSizeType, "", CI);
              Val = new IntToPtrInst(Val, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(Val);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), WordIsZeroMethod)) {
              Value* Val = Call.getArgument(0);
              Constant* N = Context.getNullValue(Val->getType());
              Value* Res = new ICmpInst(CI, ICmpInst::ICMP_EQ, Val, N, "");
              Res = new ZExtInst(Res, FCur->getReturnType(), "", CI);
              CI->replaceAllUsesWith(Res);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), WordToLongMethod)) {
              Value* Val = Call.getArgument(0);
              Val = new PtrToIntInst(Val, Type::Int64Ty, "", CI);
              CI->replaceAllUsesWith(Val);
              CI->eraseFromParent();
            } else if (!strcmp(FCur->getNameStart(), WordMaxMethod)) {
              ConstantInt* M = Context.getConstantInt(Type::Int64Ty, (uint64_t)-1);
              Constant* N = ConstantExpr::getIntToPtr(M, FCur->getReturnType());
              CI->replaceAllUsesWith(N);
              CI->eraseFromParent();
            } else {
              fprintf(stderr, "Implement me %s\n", name);
              abort();
            }
          }
        }
      }
    }
  }
  return Changed;
}


FunctionPass* createLowerMagicPass() {
  return new LowerMagic();
}

}
