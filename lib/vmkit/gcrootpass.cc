#include "llvm/CodeGen/GCs.h"
#include "llvm/CodeGen/GCStrategy.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"

#include "llvm/MC/MCContext.h"

#include "llvm/Support/Compiler.h"

#include "llvm/Target/TargetInstrInfo.h"
#include "llvm/Target/TargetMachine.h"

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
}
