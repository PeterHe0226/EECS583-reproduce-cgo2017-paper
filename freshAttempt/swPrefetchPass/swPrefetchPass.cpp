#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/Transforms/Scalar/LoopUnrollPass.h"
#include "llvm/IR/Verifier.h"

#include  <iostream>

using namespace llvm;

namespace {

struct SwPrefetchPass : public PassInfoMixin<SwPrefetchPass> {

  bool makeLoopInvariantSpec(Instruction *I, bool &Changed, Loop* L) const {
    // Test if the value is already loop-invariant.
    if (L->isLoopInvariant(I)) {
      return true;
    }
    // EH block instructions are immobile.
    // Determine the insertion point, unless one was given.
    if(!I) return false;
    if(!isSafeToSpeculativelyExecute(I) && !I->mayReadFromMemory()) return false; //hacky but it works for now. This is allowed because we're hoisting what are really going to be prefetches.

    BasicBlock *Preheader = L->getLoopPreheader();
    // Without a preheader, hoisting is not feasible.
    if (!Preheader) {
	    return false;
    }
    Instruction* InsertPt = Preheader->getTerminator();

    // Don't hoist instructions with loop-variant operands.
    for (Value *Operand : I->operands()) {
      if(Operand) if(Instruction* i = dyn_cast<Instruction>(Operand)) if (!makeLoopInvariantSpec(i, Changed, L)) {
        Changed = false;
        return false;
      }
    }

    // Hoist.
    I->moveBefore(InsertPt);

    // There is possibility of hoisting this instruction above some arbitrary
    // condition. Any metadata defined on it can be control dependent on this
    // condition. Conservatively strip it here so that we don't give any wrong
    // information to the optimizer.

    Changed = true;
    return true;
  }

  bool makeLoopInvariantPredecessor(Value *V, bool &Changed, Loop* L) const {
    //if predecessor always runs before the loop, then we can hoist invariant loads, at the expense of exception imprecision.
    //A better technique would retain precision by separating out the first iteration and reusing the invariant loads.

    // Test if the value is already loop-invariant.
    if (L->isLoopInvariant(V)){
      return true;
    }

    if(!V) return true;

    Instruction* I = dyn_cast<Instruction>(V);
    if(!I) return false;

    if (!isSafeToSpeculativelyExecute(I) && !I->mayReadFromMemory()) {
      return false;
    }

    // EH block instructions are immobile.
    if (I->isEHPad()) {
	    return false;
    }

    // Determine the insertion point, unless one was given.

    BasicBlock *Preheader = L->getLoopPreheader();
    // Without a preheader, hoisting is not feasible.
    if (!Preheader) {
      return false;
    }
    Instruction* InsertPt = Preheader->getTerminator();

    // Don't hoist instructions with loop-variant operands.
    for (Value *Operand : I->operands()){
      if (!makeLoopInvariantPredecessor(Operand, Changed,L)) {
        return false;
      }
    }

    // Hoist.
    I->moveBefore(InsertPt);

    // There is possibility of hoisting this instruction above some arbitrary
    // condition. Any metadata defined on it can be control dependent on this
    // condition. Conservatively strip it here so that we don't give any wrong
    // information to the optimizer.
    I->dropUnknownNonDebugMetadata();

    Changed = true;
    return true;

  }

  PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM) {

    std::cout << "Hello function: " << F.getName().str() << std::endl;
    return PreservedAnalyses::all();
  }
};
}

extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK llvmGetPassPluginInfo() {
  return {
    LLVM_PLUGIN_API_VERSION, "SwPrefetchPass", "v0.1",
    [](PassBuilder &PB) {
      PB.registerPipelineParsingCallback(
        [](StringRef Name, FunctionPassManager &FPM,
        ArrayRef<PassBuilder::PipelineElement>) {
          if(Name == "func-name"){
            FPM.addPass(LoopUnrollPass());
            FPM.addPass(SwPrefetchPass());
            FPM.addPass(VerifierPass());
            return true;
          }
          return false;
        }
      );
    }
  };
}