#include "vmkit/safepoint.h"
#include "vmkit/compiler.h"
#include "vmkit/system.h"

#include "llvm/DebugInfo.h"

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
		const llvm::MCExpr* emitName(llvm::AsmPrinter &AP, llvm::StringRef str, unsigned int* num);
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

const llvm::MCExpr* VmkitGCMetadataPrinter::emitName(llvm::AsmPrinter &AP, llvm::StringRef str, unsigned int* num) {
	AP.OutStreamer.SwitchSection(AP.getObjFileLowering().getSectionForConstant(llvm::SectionKind::getMergeable1ByteCString()));
	llvm::SmallString<256> id("gcFunc");
	id += (*num)++;

	llvm::MCSymbol *sym = AP.OutContext.GetOrCreateSymbol(id);
	AP.OutStreamer.EmitLabel(sym);
			
	uint32_t len = str.size();
	for(uint32_t n=0; n<len; n++)
		AP.EmitInt8(str.data()[n]);
	AP.EmitInt8(0);

	AP.OutStreamer.SwitchSection(AP.getObjFileLowering().getDataSection());

	return llvm::MCSymbolRefExpr::Create(sym, AP.OutStreamer.getContext());
}

/*
 *    VmkitGCMetadataPrinter
 */
void VmkitGCMetadataPrinter::finishAssembly(llvm::AsmPrinter &AP) {
	unsigned IntPtrSize = AP.TM.getDataLayout()->getPointerSize(0);
	uint32_t funcNumber = 0;
		
	AP.OutStreamer.SwitchSection(AP.getObjFileLowering().getDataSection());

	llvm::SmallString<256> symName("_");
	symName += getModule().getModuleIdentifier();
	symName += "__frametable";

	llvm::MCSymbol *sym = AP.OutContext.GetOrCreateSymbol(symName);
	AP.OutStreamer.EmitSymbolAttribute(sym, llvm::MCSA_Global);

	AP.EmitAlignment(IntPtrSize == 4 ? 2 : 3);

	AP.OutStreamer.AddComment("--- frame tables ---");
	AP.OutStreamer.AddBlankLine();
	AP.OutStreamer.EmitLabel(sym);		

	for (iterator I = begin(), IE = end(); I != IE; ++I) {
		llvm::GCFunctionInfo* gcInfo = *I;
			
		if(gcInfo->getFunction().hasGC()) {
			const llvm::MCExpr* funcName = emitName(AP, gcInfo->getFunction().getName(), &funcNumber);

			for(llvm::GCFunctionInfo::iterator safepoint=gcInfo->begin(); safepoint!=gcInfo->end(); safepoint++) {
				llvm::DebugLoc* debug = &safepoint->Loc;
				uint32_t  kind = safepoint->Kind;

				const llvm::MCExpr* address = llvm::MCSymbolRefExpr::Create(safepoint->Label, AP.OutStreamer.getContext());
				if (debug->getCol() == 1) {
					const llvm::MCExpr* one = llvm::MCConstantExpr::Create(1, AP.OutStreamer.getContext());
					address = llvm::MCBinaryExpr::CreateAdd(address, one, AP.OutStreamer.getContext());
				}

				llvm::MDNode* inlinedAt = debug->getInlinedAt(getModule().getContext());

				uint32_t inlineDepth = 1;

				if(inlinedAt) {
					llvm::DILocation cur(inlinedAt);
					while(cur.getScope()) {
						inlineDepth++;
						cur = cur.getOrigLocation();
					};
				}

				AP.OutStreamer.EmitValue(address, IntPtrSize);
				/* CompilationUnit* */
				AP.EmitInt32(0);
				if(IntPtrSize == 8)
					AP.EmitInt32(0);

				/* number of live roots */
				AP.EmitInt32(gcInfo->live_size(safepoint));
				/* inline stack depth */
				AP.EmitInt32(inlineDepth);

				//fprintf(stderr, "emitting %lu lives\n", gcInfo->live_size(safepoint));
				for(llvm::GCFunctionInfo::live_iterator live=gcInfo->live_begin(safepoint); live!=gcInfo->live_end(safepoint); live++)
					AP.EmitInt32(live->StackOffset);

				/* function names stack */
				if(inlinedAt) {
					//fprintf(stderr, "find inline location in %s: %d\n", gcInfo->getFunction().getName().data(), inlineDepth);
					llvm::DISubprogram sub(debug->getScope(getModule().getContext()));

					//fprintf(stderr, "Inlined:                %s::%d\n", sub.getName().data(), debug->getLine());
					AP.OutStreamer.EmitValue(emitName(AP, sub.getName().data(), &funcNumber), IntPtrSize);
					AP.EmitInt32(debug->getLine());

					llvm::DILocation cur(inlinedAt);
					while(cur.getScope()) {
						llvm::DISubprogram il(cur.getScope());
						//fprintf(stderr, "    =>                  %s::%d\n", il.getName().data(), cur.getLineNumber());
						if(strcmp(il.getName().data(), gcInfo->getFunction().getName().data()))
							AP.OutStreamer.EmitValue(emitName(AP, il.getName().data(), &funcNumber), IntPtrSize);
						else
							AP.OutStreamer.EmitValue(funcName, IntPtrSize);
						AP.EmitInt32(cur.getLineNumber());
						cur = cur.getOrigLocation();
					};

					llvm::DILocation   loc(inlinedAt);
					llvm::DISubprogram il(loc.getScope());

					//if(strcmp(gcInfo->getFunction().getName().data(), il.getName().data()))
					//abort();
				} else {
					AP.OutStreamer.EmitValue(funcName, IntPtrSize);
					AP.EmitInt32(debug->getLine());
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
	uintptr_t next = (uintptr_t)this + sizeof(Safepoint) + nbLives()*sizeof(uint32_t) + 
		inlineDepth()*(sizeof(const char*)+sizeof(uint32_t));
	return (Safepoint*)(((next - 1) & -sizeof(uintptr_t)) + sizeof(uintptr_t));
}

Safepoint* Safepoint::get(CompilationUnit* unit, llvm::Module* module) {
	std::string symName;
	symName += '_';
	symName += module->getModuleIdentifier();
	symName += "__frametable";
	return (vmkit::Safepoint*)unit->ee()->getGlobalValueAddress(System::mcjitSymbol(symName.c_str()));
}

void Safepoint::dump() {
	fprintf(stderr, "  [%p] safepoint at %p for %s::%d\n", this, addr(), functionName(), sourceIndex());

	for(uint32_t i=1; i<inlineDepth(); i++)
		fprintf(stderr, "          inlined in %s::%d\n", functionName(i), sourceIndex(i));
	for(uint32_t i=0; i<nbLives(); i++)
		fprintf(stderr, "    live at %d\n", liveAt(i));

	if(inlineDepth() == 4)
		abort();
}
