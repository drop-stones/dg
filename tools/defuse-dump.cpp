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
  llvm::errs() << __func__ << "\n";
  auto *DDA = DG->getDDA();
  for (const auto &FunIter : getConstructedFunctions()) {
    for (const auto &BBIter : FunIter.second->getBlocks()) {
      for (const auto *Node : BBIter.second->getNodes()) {
        auto *Val = Node->getValue();
        if (DDA->isUse(Val)) {
          llvm::errs() << "Use: " << *Val << "\n";
          auto *ReadNode = DDA->getNode(Val);
          for (const auto &UseSite : ReadNode->getUses()) {
            llvm::errs() << "   - Off: " << UseSite.offset.offset << ", Len: " << UseSite.len.offset << ", Val: ";
            if (DDA->getValue(UseSite.target) != nullptr)
              llvm::errs() << *DDA->getValue(UseSite.target) << "\n";
            else
              llvm::errs() << "nullptr\n";
          }
          for (const auto *Def : DDA->getLLVMDefinitions(Val)) {
            llvm::errs() << " - Def: " << *Def << "\n";
            auto *WriteNode = DDA->getNode(Def);
            for (const auto &DefSite : WriteNode->getDefines()) {
              llvm::errs() << "   - Off: " << DefSite.offset.offset << ", Len: " << DefSite.len.offset << ", Val: ";
              if (DDA->getValue(DefSite.target) != nullptr)
                llvm::errs() << *DDA->getValue(DefSite.target) << "\n";
              else
                llvm::errs() << "nullptr\n";
            }
            for (const auto &DefSite : WriteNode->getOverwrites()) {
              llvm::errs() << "   - Off: " << DefSite.offset.offset << ", Len: " << DefSite.len.offset << ", Val: ";
              if (DDA->getValue(DefSite.target) != nullptr)
                llvm::errs() << *DDA->getValue(DefSite.target) << "\n";
              else
                llvm::errs() << "nullptr\n";
            }
          }
        }
      }
    }
  }
}

void printGlobals(LLVMDependenceGraph *DG) {
  llvm::errs() << __func__ << "\n";
  auto *DDA = DG->getDDA();
  auto *RWGraph = DDA->getGraph();
  for (auto *Subgraph : RWGraph->subgraphs()) {
    for (auto *BB : Subgraph->bblocks()) {
      for (auto *Node : BB->getNodes()) {
        llvm::errs() << (unsigned)Node->getType() << ": " << *DDA->getValue(Node) << "\n";
      }
    }
  }
}

int main(int argc, char *argv[]) {
  SlicerOptions Opt = parseSlicerOptions(argc, argv);
  // Opt.dgOptions.PTAOptions.analysisType = LLVMPointerAnalysisOptions::AnalysisType::svf;  // Use SVF by default
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
