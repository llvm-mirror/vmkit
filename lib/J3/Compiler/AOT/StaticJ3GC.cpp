//===----- StaticJ3GC.cpp - Support for Ahead of Time Compiler GC -------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/CodeGen/GCs.h"
#include "llvm/CodeGen/GCStrategy.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/GCMetadataPrinter.h"
#include "llvm/Module.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/Target/Mangler.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Target/TargetLoweringObjectFile.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FormattedStream.h"
#include <cctype>

using namespace llvm;

namespace {
  class StaticJ3GC : public GCStrategy {
  public:
    StaticJ3GC();
  };
}

static GCRegistry::Add<StaticJ3GC>
X("java_aot", "Java GC for AOT-generated functions");

StaticJ3GC::StaticJ3GC() {
  NeededSafePoints = 1 << GC::PostCall;
  UsesMetadata = true;
}

namespace {

  class StaticJ3GCMetadataPrinter : public GCMetadataPrinter {
  public:
    void beginAssembly(AsmPrinter &AP);
    void finishAssembly(AsmPrinter &AP);
  };

}

static GCMetadataPrinterRegistry::Add<StaticJ3GCMetadataPrinter>
Y("java_aot", "Java GC for AOT-generated functions");

void StaticJ3GCMetadataPrinter::beginAssembly(AsmPrinter &AP) {
}

/// emitAssembly - Print the frametable. The ocaml frametable format is thus:
///
///   extern "C" struct align(sizeof(intptr_t)) {
///     void *FunctionAddress;
///     uint16_t NumDescriptors;
///     struct align(sizeof(intptr_t)) {
///       void *ReturnAddress;
///       uint16_t BytecodeIndex; 
///       uint16_t FrameSize;
///       uint16_t NumLiveOffsets;
///       uint16_t LiveOffsets[NumLiveOffsets];
///     } Descriptors[NumDescriptors];
///   } ${method}__frame;
///
/// Note that this precludes programs from stack frames larger than 64K
/// (FrameSize and LiveOffsets would overflow). FrameTablePrinter will abort if
/// either condition is detected in a function which uses the GC.
///
void StaticJ3GCMetadataPrinter::finishAssembly(AsmPrinter &AP) {
  unsigned IntPtrSize = AP.TM.getTargetData()->getPointerSize();

  AP.OutStreamer.SwitchSection(AP.getObjFileLowering().getDataSection());


  for (iterator I = begin(), IE = end(); I != IE; ++I) {
    GCFunctionInfo &FI = **I;

    // Emit the frame symbol
    SmallString<128> TmpStr;
    AP.Mang->getNameWithPrefix(TmpStr, FI.getFunction().getName() + "_frame");
    MCSymbol *Sym = AP.OutContext.GetOrCreateSymbol(TmpStr);
    AP.OutStreamer.EmitSymbolAttribute(Sym, MCSA_Global);
    AP.OutStreamer.EmitLabel(Sym);

    // Emit the method symbol
    MCSymbol *FunctionSym = AP.Mang->getSymbol(&FI.getFunction());
    AP.OutStreamer.EmitSymbolValue(FunctionSym, IntPtrSize, 0);
  
    int NumDescriptors = 0;
    for (GCFunctionInfo::iterator J = FI.begin(), JE = FI.end(); J != JE; ++J) {
        NumDescriptors++;
    }
    if (NumDescriptors >= 1<<16) {
      // Very rude!
      report_fatal_error(" Too much descriptor for J3 AOT GC");
    }
    AP.EmitInt16(NumDescriptors);
    AP.EmitAlignment(IntPtrSize == 4 ? 2 : 3);

    uint64_t FrameSize = FI.getFrameSize();
    if (FrameSize >= 1<<16) {
      // Very rude!
      report_fatal_error("Function '" + FI.getFunction().getName() +
                         "' is too large for the J3 AOT GC! "
                         "Frame size " + Twine(FrameSize) + ">= 65536.\n"
                         "(" + Twine(uintptr_t(&FI)) + ")");
    }

    AP.OutStreamer.AddComment("live roots for " +
                              Twine(FI.getFunction().getName()));
    AP.OutStreamer.AddBlankLine();

    for (GCFunctionInfo::iterator J = FI.begin(), JE = FI.end(); J != JE; ++J) {
      size_t LiveCount = FI.live_size(J);
      if (LiveCount >= 1<<16) {
        // Very rude!
        report_fatal_error("Function '" + FI.getFunction().getName() +
                           "' is too large for the ocaml GC! "
                           "Live root count "+Twine(LiveCount)+" >= 65536.");
      }

      DebugLoc DL = J->Loc;
      uint32_t bytecodeIndex = DL.getLine();
      uint32_t second = DL.getCol();
      assert(second == 0 && "Wrong column number");

      AP.OutStreamer.EmitSymbolValue(J->Label, IntPtrSize, 0);
      AP.EmitInt16(bytecodeIndex);
      AP.EmitInt16(FrameSize);
      AP.EmitInt16(LiveCount);

      for (GCFunctionInfo::live_iterator K = FI.live_begin(J),
                                         KE = FI.live_end(J); K != KE; ++K) {
        if (K->StackOffset >= 1<<16) {
          // Very rude!
          report_fatal_error(
                 "GC root stack offset is outside of fixed stack frame and out "
                 "of range for ocaml GC!");
        }
        AP.EmitInt16(K->StackOffset);
      }

      AP.EmitAlignment(IntPtrSize == 4 ? 2 : 3);
    }
  }
}
