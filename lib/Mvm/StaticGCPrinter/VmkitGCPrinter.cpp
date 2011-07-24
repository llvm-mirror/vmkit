//===-- VmkitGCPrinter.cpp - Vmkit frametable emitter ---------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements printing the assembly code for a Vmkit frametable.
//
//===----------------------------------------------------------------------===//

#include "llvm/CodeGen/GCs.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/GCMetadataPrinter.h"
#include "llvm/CodeGen/GCStrategy.h"
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
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FormattedStream.h"
#include <cctype>
using namespace llvm;

namespace {
  class VmkitGC : public GCStrategy {
  public:
    VmkitGC();
  };

  class VmkitGCMetadataPrinter : public GCMetadataPrinter {
  public:
    void beginAssembly(AsmPrinter &AP);
    void finishAssembly(AsmPrinter &AP);
  };

}


static GCRegistry::Add<VmkitGC>
X("vmkit", "VMKit GC for AOT-generated functions");


static GCMetadataPrinterRegistry::Add<VmkitGCMetadataPrinter>
Y("vmkit", "VMKit GC for AOT-generated functions");


VmkitGC::VmkitGC() {
  NeededSafePoints = 1 << GC::PostCall;
  UsesMetadata = true;
}


static bool isAcceptableChar(char C) {
  if ((C < 'a' || C > 'z') &&
      (C < 'A' || C > 'Z') &&
      (C < '0' || C > '9') &&
      C != '_' && C != '$' && C != '@') {
    return false;
  }
  return true;
}

static char HexDigit(int V) {
  return V < 10 ? V+'0' : V+'A'-10;
}

static void MangleLetter(SmallVectorImpl<char> &OutName, unsigned char C) {
  OutName.push_back('_');
  OutName.push_back(HexDigit(C >> 4));
  OutName.push_back(HexDigit(C & 15));
  OutName.push_back('_');
}


static void EmitVmkitGlobal(const Module &M, AsmPrinter &AP, const char *Id) {
  const std::string &MId = M.getModuleIdentifier();

  std::string SymName;
  SymName += "vmkit";
  size_t Letter = SymName.size();
  SymName.append(MId.begin(), std::find(MId.begin(), MId.end(), '.'));
  SymName += "__";
  SymName += Id;

  // Capitalize the first letter of the module name.
  SymName[Letter] = toupper(SymName[Letter]);

  SmallString<128> TmpStr;
  AP.Mang->getNameWithPrefix(TmpStr, SymName);

  SmallString<128> FinalStr;
  for (unsigned i = 0, e = TmpStr.size(); i != e; ++i) {
    if (!isAcceptableChar(TmpStr[i])) {
      MangleLetter(FinalStr, TmpStr[i]);
    } else {
      FinalStr.push_back(TmpStr[i]);
    }
  }

  MCSymbol *Sym = AP.OutContext.GetOrCreateSymbol(FinalStr);

  AP.OutStreamer.EmitSymbolAttribute(Sym, MCSA_Global);
  AP.OutStreamer.EmitLabel(Sym);
}

void VmkitGCMetadataPrinter::beginAssembly(AsmPrinter &AP) {
}

/// emitAssembly - Print the frametable. The ocaml frametable format is thus:
///
///   extern "C" struct align(sizeof(intptr_t)) {
///     uint16_t NumDescriptors;
///     struct align(sizeof(intptr_t)) {
///       void *ReturnAddress;
///       uint16_t FrameSize;
///       uint16_t NumLiveOffsets;
///       uint16_t LiveOffsets[NumLiveOffsets];
///     } Descriptors[NumDescriptors];
///   } vmkit${module}__frametable;
///
/// Note that this precludes programs from stack frames larger than 64K
/// (FrameSize and LiveOffsets would overflow). FrameTablePrinter will abort if
/// either condition is detected in a function which uses the GC.
///
void VmkitGCMetadataPrinter::finishAssembly(AsmPrinter &AP) {
  unsigned IntPtrSize = AP.TM.getTargetData()->getPointerSize();
  AP.OutStreamer.SwitchSection(AP.getObjFileLowering().getDataSection());
  EmitVmkitGlobal(getModule(), AP, "frametable");

  int NumDescriptors = 0;
  for (iterator I = begin(), IE = end(); I != IE; ++I) {
    GCFunctionInfo &FI = **I;
    for (GCFunctionInfo::iterator J = FI.begin(), JE = FI.end(); J != JE; ++J) {
      NumDescriptors++;
    }
  }

  if (NumDescriptors >= 1<<16) {
    // Very rude!
    report_fatal_error(" Too much descriptor for vmkit GC");
  }
  AP.EmitInt16(NumDescriptors);
  AP.EmitAlignment(IntPtrSize == 4 ? 2 : 3);

  for (iterator I = begin(), IE = end(); I != IE; ++I) {
    GCFunctionInfo &FI = **I;

    uint64_t FrameSize = FI.getFrameSize();
    if (FrameSize >= 1<<16) {
      // Very rude!
      report_fatal_error("Function '" + FI.getFunction().getName() +
                         "' is too large for the vmkit GC! "
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
                           "' is too large for the vmkit GC! "
                           "Live root count "+Twine(LiveCount)+" >= 65536.");
      }

      AP.OutStreamer.EmitSymbolValue(J->Label, IntPtrSize, 0);
      AP.EmitInt16(FrameSize);
      AP.EmitInt16(LiveCount);

      for (GCFunctionInfo::live_iterator K = FI.live_begin(J),
                                         KE = FI.live_end(J); K != KE; ++K) {
        if (K->StackOffset >= 1<<16) {
          // Very rude!
          report_fatal_error(
                 "GC root stack offset is outside of fixed stack frame and out "
                 "of range for vmkit GC!");
        }
        AP.EmitInt16(K->StackOffset);
      }

      AP.EmitAlignment(IntPtrSize == 4 ? 2 : 3);
    }
  }
}
