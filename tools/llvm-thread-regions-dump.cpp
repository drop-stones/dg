#if (__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_os_ostream.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/CommandLine.h>

#if LLVM_VERSION_MAJOR >= 4
#include <llvm/Bitcode/BitcodeReader.h>
#else
#include <llvm/Bitcode/ReaderWriter.h>
#endif

#if (__clang__)
#pragma clang diagnostic pop // ignore -Wunused-parameter
#else
#pragma GCC diagnostic pop
#endif

#include <iostream>
#include <fstream>

#include "../lib/llvm/analysis/ThreadRegions/include/Graphs/ThreadRegionsBuilder.h"
#include "../lib/llvm/analysis/ThreadRegions/include/Graphs/GraphBuilder.h"
#include "dg/llvm/analysis/ThreadRegions/ControlFlowGraph.h"
#include "dg/analysis/PointsTo/PointerAnalysisFI.h"
#include "dg/llvm/analysis/PointsTo/PointerAnalysis.h"

#include <iostream>

using namespace std;

int main(int argc, const char *argv[])
{
    using namespace llvm;

    cl::opt<string> OutputFilename("o",
                                   cl::desc("Specify output filename"),
                                   cl::value_desc("filename"),
                                   cl::init(""));
    cl::opt<std::string> inputFile(cl::Positional,
                                   cl::Required,
                                   cl::desc("<input file>"),
                                   cl::init(""));

    cl::ParseCommandLineOptions(argc, argv);

    std::string module = inputFile;
    std::string graphvizFileName = OutputFilename;

    llvm::LLVMContext context;
    llvm::SMDiagnostic SMD;

    std::unique_ptr<Module> M = llvm::parseIRFile(module.c_str(), SMD, context);

    if (!M) {
        llvm::errs() << "Failed parsing '" << module << "' file:\n";
        SMD.print(argv[0], errs());
        return 1;
    }

    dg::DGLLVMPointerAnalysis pointsToAnalysis(M.get(), "main", dg::analysis::Offset::UNKNOWN, true);
    auto PA = pointsToAnalysis.createPTA<dg::analysis::pta::PointerAnalysisFI>();
    PA->run();

    ControlFlowGraph controlFlowGraph(&pointsToAnalysis);
    controlFlowGraph.buildFunction(M->getFunction("main"));

    if (graphvizFileName == "") {
        controlFlowGraph.printWithRegions(std::cout);
    } else {
        std::ofstream graphvizFile(graphvizFileName);
        controlFlowGraph.printWithRegions(graphvizFile);
    }

    return 0;
}

