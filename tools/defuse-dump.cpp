#include "dg/MemorySSA/MemorySSA.h"
#include "dg/ReadWriteGraph/ReadWriteGraph.h"
#include "dg/llvm/LLVMDependenceGraph.h"
#include "dg/llvm/LLVMDependenceGraphBuilder.h"
#include "dg/llvm/LLVMDG2Dot.h"
#include "dg/tools/llvm-slicer-opts.h"
#include "dg/tools/llvm-slicer-utils.h"
#include "llvm/ReadWriteGraph/LLVMReadWriteGraphBuilder.h"

using namespace dg;

void printUseDefFromDG(LLVMDependenceGraph *DG) {
  auto *DDA = DG->getDDA();
  for (const auto &FunIter : getConstructedFunctions()) {
    for (const auto &BBIter : FunIter.second->getBlocks()) {
      for (const auto *Node : BBIter.second->getNodes()) {
        auto *Val = Node->getValue();
        if (DDA->isUse(Val)) {
          llvm::errs() << "Use: " << *Val << "\n";
          auto *ReadNode = DDA->getNode(Val);
          for (const auto &UseSite : ReadNode->getUses())
            llvm::errs() << "   - Off: " << UseSite.offset.offset << ", Len: " << UseSite.len.offset << ", Val: " << *DDA->getValue(UseSite.target) << "\n";
          for (const auto *Def : DDA->getLLVMDefinitions(Val)) {
            llvm::errs() << " - Def: " << *Def << "\n";
            auto *WriteNode = DDA->getNode(Def);
            for (const auto &DefSite : WriteNode->getDefines())
              llvm::errs() << "   - Off: " << DefSite.offset.offset << ", Len: " << DefSite.len.offset << ", Val: " << *DDA->getValue(DefSite.target) << "\n";
            for (const auto &DefSite : WriteNode->getOverwrites())
              llvm::errs() << "   - Off: " << DefSite.offset.offset << ", Len: " << DefSite.len.offset << ", Val: " << *DDA->getValue(DefSite.target) << "\n";
          }
        }
      }
    }
  }
}

int main(int argc, char *argv[]) {
  SlicerOptions Opt = parseSlicerOptions(argc, argv);
  Opt.dgOptions.PTAOptions.analysisType = LLVMPointerAnalysisOptions::AnalysisType::svf;  // Use SVF by default
  llvm::LLVMContext Ctx;
  std::unique_ptr<llvm::Module> M = parseModule("defuse-dump", Ctx, Opt);

  llvmdg::LLVMDependenceGraphBuilder Builder(M.get(), Opt.dgOptions);
  auto DG = Builder.build();
  auto *RWGraph = DG->getDDA()->getGraph();

  printUseDefFromDG(DG.get());

  debug::LLVMDG2Dot Dumper(DG.get());
  Dumper.dump("dg.dot");

  return 0;
}
