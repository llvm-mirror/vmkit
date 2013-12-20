#include "llvm/IR/Function.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/DataLayout.h"

#include "llvm/CodeGen/GCs.h"
#include "llvm/CodeGen/GCStrategy.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/GCMetadataPrinter.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"

#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSection.h"

#include "llvm/Support/Compiler.h"

#include "llvm/Target/TargetInstrInfo.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/Mangler.h"
#include "llvm/Target/TargetLoweringObjectFile.h"

using namespace llvm;

namespace vmkit {
	class VmkitGCPass : public GCStrategy {
	public:
    VmkitGCPass();
    bool findCustomSafePoints(GCFunctionInfo& FI, MachineFunction &MF);
  };

  void linkVmkitGC() { }

	static GCRegistry::Add<VmkitGCPass> X("vmkit", "VMKit GC for JIT-generated functions");

	VmkitGCPass::VmkitGCPass() {
		CustomSafePoints = true;
		UsesMetadata = true;
		InitRoots = 1;
	}

	static MCSymbol *InsertLabel(MachineBasicBlock &MBB, 
															 MachineBasicBlock::iterator MI,
															 DebugLoc DL) {
		const TargetInstrInfo* TII = MBB.getParent()->getTarget().getInstrInfo();
		MCSymbol *Label = MBB.getParent()->getContext().CreateTempSymbol();
		BuildMI(MBB, MI, DL, TII->get(TargetOpcode::GC_LABEL)).addSym(Label);
		return Label;
	}

	bool VmkitGCPass::findCustomSafePoints(GCFunctionInfo& FI, MachineFunction &MF) {
		for (MachineFunction::iterator BBI = MF.begin(),
					 BBE = MF.end(); BBI != BBE; ++BBI) {
			for (MachineBasicBlock::iterator MI = BBI->begin(),
						 ME = BBI->end(); MI != ME; ++MI) {
				if (MI->getDesc().isCall()) {
					MachineBasicBlock::iterator RAI = MI; ++RAI;                                
					MCSymbol* Label = InsertLabel(*MI->getParent(), RAI, MI->getDebugLoc());
					FI.addSafePoint(GC::PostCall, Label, MI->getDebugLoc());
				} else if (MI->getDebugLoc().getCol() == 1) {
					MCSymbol* Label = InsertLabel(*MI->getParent(), MI, MI->getDebugLoc());
					FI.addSafePoint(GC::Loop, Label, MI->getDebugLoc());
				}
			}
		}

		return false;
	}

  class VmkitGCMetadataPrinter : public GCMetadataPrinter {
  public:
    void beginAssembly(AsmPrinter &AP);
    void finishAssembly(AsmPrinter &AP);
  };

	static GCMetadataPrinterRegistry::Add<VmkitGCMetadataPrinter>
	Y("vmkit", "VMKit garbage-collector");
	
	void VmkitGCMetadataPrinter::beginAssembly(AsmPrinter &AP) {}

	void VmkitGCMetadataPrinter::finishAssembly(AsmPrinter &AP) {
		AP.OutStreamer.SwitchSection(AP.getObjFileLowering().getDataSection());

		unsigned IntPtrSize = AP.TM.getDataLayout()->getPointerSize(0);
		
		SmallString<256> symName("_");
		symName += getModule().getModuleIdentifier();
		symName += "__frametable";

		MCSymbol *sym = AP.OutContext.GetOrCreateSymbol(symName);
		AP.OutStreamer.EmitSymbolAttribute(sym, MCSA_Global);

		AP.EmitAlignment(IntPtrSize == 4 ? 2 : 3);

    AP.OutStreamer.AddComment("--- module frame tables ---");
    AP.OutStreamer.AddBlankLine();
		AP.OutStreamer.EmitLabel(sym);		

		for (iterator I = begin(), IE = end(); I != IE; ++I) {
			GCFunctionInfo* gcInfo = *I;
			
			if(gcInfo->getFunction().hasGC()) {
				AP.OutStreamer.AddComment("live roots for " + Twine(gcInfo->getFunction().getName()));
				AP.OutStreamer.AddBlankLine();
				for(llvm::GCFunctionInfo::iterator safepoint=gcInfo->begin(); safepoint!=gcInfo->end(); safepoint++) {
					DebugLoc* debug = &safepoint->Loc;
					uint32_t  kind = safepoint->Kind;

					const MCExpr* address = MCSymbolRefExpr::Create(safepoint->Label, AP.OutStreamer.getContext());
					if (debug->getCol() == 1) {
						const MCExpr* one = MCConstantExpr::Create(1, AP.OutStreamer.getContext());
						address = MCBinaryExpr::CreateAdd(address, one, AP.OutStreamer.getContext());
					}

					AP.OutStreamer.EmitValue(address, IntPtrSize);
					AP.EmitGlobalConstant(&gcInfo->getFunction());
					AP.EmitInt32(debug->getLine());
					AP.EmitInt32(gcInfo->live_size(safepoint));

					//fprintf(stderr, "emitting %lu lives\n", gcInfo->live_size(safepoint));
					for(GCFunctionInfo::live_iterator live=gcInfo->live_begin(safepoint); live!=gcInfo->live_end(safepoint); live++) {
						AP.EmitInt32(live->StackOffset);
					}
					AP.EmitAlignment(IntPtrSize == 4 ? 2 : 3);
				}
			}
		}

		AP.OutStreamer.EmitValue(MCConstantExpr::Create(0, AP.OutStreamer.getContext()), IntPtrSize);
	}
}
