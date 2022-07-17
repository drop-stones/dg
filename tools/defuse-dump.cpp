#include "dg/ReadWriteGraph/ReadWriteGraph.h"
#include "dg/llvm/LLVMDependenceGraph.h"
#include "dg/llvm/LLVMDependenceGraphBuilder.h"
#include "dg/tools/llvm-slicer-opts.h"
#include "dg/tools/llvm-slicer-utils.h"
#include "llvm/ReadWriteGraph/LLVMReadWriteGraphBuilder.h"

using namespace dg;

void printUseDefFromDG(LLVMDependenceGraph *DG) {
  auto *DDA = DG->getDDA();
  for (const auto &Idx : *DG) {
    auto *Val = Idx.first;
    auto *Node = Idx.second;
    if (DDA->isUse(Val)) {
      llvm::errs() << "Use: " << *Val << "\n";
      for (const auto *Def : DDA->getLLVMDefinitions(Val)) {
        llvm::errs() << " - Def: " << *Def << "\n";
      }
    }
  }
}

int main(int argc, char *argv[]) {
  SlicerOptions Opt = parseSlicerOptions(argc, argv);
  llvm::LLVMContext Ctx;
  std::unique_ptr<llvm::Module> M = parseModule("defuse-dump", Ctx, Opt);

  llvmdg::LLVMDependenceGraphBuilder Builder(M.get());
  auto DG = Builder.build();
  auto *RWGraph = DG->getDDA()->getGraph();

  printUseDefFromDG(DG.get());

  return 0;
}
