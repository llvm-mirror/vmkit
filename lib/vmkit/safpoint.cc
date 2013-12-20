#include "vmkit/safepoint.h"
#include "vmkit/compiler.h"

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

#include "llvm/ExecutionEngine/ExecutionEngine.h"

namespace vmkit {
	class VmkitGCPass : public llvm::GCStrategy {
	public:
		static llvm::MCSymbol *InsertLabel(llvm::MachineBasicBlock &MBB, 
																			 llvm::MachineBasicBlock::iterator MI,
																			 llvm::DebugLoc DL);

    VmkitGCPass();
    bool findCustomSafePoints(llvm::GCFunctionInfo& FI, llvm::MachineFunction &MF);
  };

  class VmkitGCMetadataPrinter : public llvm::GCMetadataPrinter {
  public:
    void beginAssembly(llvm::AsmPrinter &AP) {}
    void finishAssembly(llvm::AsmPrinter &AP);
  };
}

using namespace vmkit;

static llvm::GCRegistry::Add<VmkitGCPass> X("vmkit", "VMKit GC for JIT-generated functions");
static llvm::GCMetadataPrinterRegistry::Add<VmkitGCMetadataPrinter> Y("vmkit", "VMKit garbage-collector");

/*
 *    VmkitGCPass
 */
VmkitGCPass::VmkitGCPass() {
	CustomSafePoints = true;
	UsesMetadata = true;
	InitRoots = 1;
}

llvm::MCSymbol *VmkitGCPass::InsertLabel(llvm::MachineBasicBlock &MBB, 
																	 llvm::MachineBasicBlock::iterator MI,
																	 llvm::DebugLoc DL) {
	const llvm::TargetInstrInfo* TII = MBB.getParent()->getTarget().getInstrInfo();
	llvm::MCSymbol *Label = MBB.getParent()->getContext().CreateTempSymbol();
	BuildMI(MBB, MI, DL, TII->get(llvm::TargetOpcode::GC_LABEL)).addSym(Label);
	return Label;
}

bool VmkitGCPass::findCustomSafePoints(llvm::GCFunctionInfo& FI, llvm::MachineFunction &MF) {
	for (llvm::MachineFunction::iterator BBI = MF.begin(),
				 BBE = MF.end(); BBI != BBE; ++BBI) {
		for (llvm::MachineBasicBlock::iterator MI = BBI->begin(),
					 ME = BBI->end(); MI != ME; ++MI) {
			if (MI->getDesc().isCall()) {
				llvm::MachineBasicBlock::iterator RAI = MI; ++RAI;                                
				llvm::MCSymbol* Label = InsertLabel(*MI->getParent(), RAI, MI->getDebugLoc());
				FI.addSafePoint(llvm::GC::PostCall, Label, MI->getDebugLoc());
			} else if (MI->getDebugLoc().getCol() == 1) {
				llvm::MCSymbol* Label = InsertLabel(*MI->getParent(), MI, MI->getDebugLoc());
				FI.addSafePoint(llvm::GC::Loop, Label, MI->getDebugLoc());
			}
		}
	}

	return false;
}

/*
 *    VmkitGCMetadataPrinter
 */
void VmkitGCMetadataPrinter::finishAssembly(llvm::AsmPrinter &AP) {
	AP.OutStreamer.SwitchSection(AP.getObjFileLowering().getDataSection());

	unsigned IntPtrSize = AP.TM.getDataLayout()->getPointerSize(0);
		
	fprintf(stderr, "generating frame tables for: %s\n", getModule().getModuleIdentifier().c_str());
	llvm::SmallString<256> symName("_");
	symName += getModule().getModuleIdentifier();
	symName += "__frametable";

	llvm::MCSymbol *sym = AP.OutContext.GetOrCreateSymbol(symName);
	AP.OutStreamer.EmitSymbolAttribute(sym, llvm::MCSA_Global);

	AP.EmitAlignment(IntPtrSize == 4 ? 2 : 3);

	AP.OutStreamer.AddComment("--- module frame tables ---");
	AP.OutStreamer.AddBlankLine();
	AP.OutStreamer.EmitLabel(sym);		

	for (iterator I = begin(), IE = end(); I != IE; ++I) {
		llvm::GCFunctionInfo* gcInfo = *I;
			
		if(gcInfo->getFunction().hasGC()) {
			AP.OutStreamer.AddComment("live roots for " + llvm::Twine(gcInfo->getFunction().getName()));
			AP.OutStreamer.AddBlankLine();
			for(llvm::GCFunctionInfo::iterator safepoint=gcInfo->begin(); safepoint!=gcInfo->end(); safepoint++) {
				llvm::DebugLoc* debug = &safepoint->Loc;
				uint32_t  kind = safepoint->Kind;

				const llvm::MCExpr* address = llvm::MCSymbolRefExpr::Create(safepoint->Label, AP.OutStreamer.getContext());
				if (debug->getCol() == 1) {
					const llvm::MCExpr* one = llvm::MCConstantExpr::Create(1, AP.OutStreamer.getContext());
					address = llvm::MCBinaryExpr::CreateAdd(address, one, AP.OutStreamer.getContext());
				}

				AP.OutStreamer.EmitValue(address, IntPtrSize);
				AP.EmitGlobalConstant(&gcInfo->getFunction());
				AP.EmitInt32(0);
				if(IntPtrSize == 8)
					AP.EmitInt32(0);
				AP.EmitInt32(debug->getLine());
				AP.EmitInt32(gcInfo->live_size(safepoint));

				//fprintf(stderr, "emitting %lu lives\n", gcInfo->live_size(safepoint));
				for(llvm::GCFunctionInfo::live_iterator live=gcInfo->live_begin(safepoint); live!=gcInfo->live_end(safepoint); live++) {
					AP.EmitInt32(live->StackOffset);
				}
				AP.EmitAlignment(IntPtrSize == 4 ? 2 : 3);
			}
		}
	}

	AP.OutStreamer.EmitValue(llvm::MCConstantExpr::Create(0, AP.OutStreamer.getContext()), IntPtrSize);
}

/* 
 *    Safepoint
 */
Safepoint* Safepoint::getNext() {
	uintptr_t next = (uintptr_t)this + sizeof(Safepoint) + nbLives()*sizeof(uint32_t);
	return (Safepoint*)(((next - 1) & -sizeof(uintptr_t)) + sizeof(uintptr_t));
}

Safepoint* Safepoint::get(CompilationUnit* unit, llvm::Module* module) {
	llvm::SmallString<256> symName;
	symName += module->getModuleIdentifier();
	symName += "__frametable";
	return (vmkit::Safepoint*)unit->ee()->getGlobalValueAddress(symName.c_str());
}

void Safepoint::dump() {
	fprintf(stderr, "  [%p] safepoint at 0x%lx for function %p::%d\n", this, addr(), code(), sourceIndex());
	for(uint32_t i=0; i<nbLives(); i++)
		fprintf(stderr, "    live at %d\n", liveAt(i));
}
