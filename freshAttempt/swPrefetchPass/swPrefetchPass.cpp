#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/Transforms/Scalar/LoopUnrollPass.h"
#include "llvm/IR/Verifier.h"

#include  <iostream>

namespace {

struct SwPrefetchPass : public llvm::PassInfoMixin<SwPrefetchPass> {

  bool makeLoopInvariantSpec(llvm::Instruction *I, bool &Changed, llvm::Loop* L) const {
    // Test if the value is already loop-invariant.
    if (L->isLoopInvariant(I)) {
      return true;
    }
    // EH block instructions are immobile.
    // Determine the insertion point, unless one was given.
    if(!I) return false;
    if(!llvm::isSafeToSpeculativelyExecute(I) && !I->mayReadFromMemory()) return false; //hacky but it works for now. This is allowed because we're hoisting what are really going to be prefetches.

    llvm::BasicBlock *Preheader = L->getLoopPreheader();
    // Without a preheader, hoisting is not feasible.
    if (!Preheader) {
	    return false;
    }
    llvm::Instruction* InsertPt = Preheader->getTerminator();

    // Don't hoist instructions with loop-variant operands.
    for (llvm::Value *Operand : I->operands()) {
      if(Operand) if(llvm::Instruction* i = llvm::dyn_cast<llvm::Instruction>(Operand)) if (!makeLoopInvariantSpec(i, Changed, L)) {
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

  bool makeLoopInvariantPredecessor(llvm::Value *V, bool &Changed, llvm::Loop* L) const {
    //if predecessor always runs before the loop, then we can hoist invariant loads, at the expense of exception imprecision.
    //A better technique would retain precision by separating out the first iteration and reusing the invariant loads.

    // Test if the value is already loop-invariant.
    if (L->isLoopInvariant(V)){
      return true;
    }

    if(!V) return true;

    llvm::Instruction* I = llvm::dyn_cast<llvm::Instruction>(V);
    if(!I) return false;

    if (!llvm::isSafeToSpeculativelyExecute(I) && !I->mayReadFromMemory()) {
      return false;
    }

    // EH block instructions are immobile.
    if (I->isEHPad()) {
	    return false;
    }

    // Determine the insertion point, unless one was given.

    llvm::BasicBlock *Preheader = L->getLoopPreheader();
    // Without a preheader, hoisting is not feasible.
    if (!Preheader) {
      return false;
    }
    llvm::Instruction* InsertPt = Preheader->getTerminator();

    // Don't hoist instructions with loop-variant operands.
    for (llvm::Value *Operand : I->operands()){
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

  llvm::Value* getArrayOrAllocSize(llvm::LoadInst* l) const
  {
    // TODO
    return nullptr;
  }

  llvm::Value* getCanonicalishSizeVariable(llvm::Loop* L) const
  {
    // TODO
    return nullptr;
  }

  llvm::PHINode* getCanonicalishInductionVariable(llvm::Loop* L) const
  {
    // TODO
    return nullptr;
  }

  llvm::PHINode* getWeirdCanonicalishInductionVariable(llvm::Loop* L) const
  {
    // TODO
    return nullptr;  
  }

  llvm::GetElementPtrInst* getWeirdCanonicalishInductionVariableGep(llvm::Loop* L) const
  {
    // TODO
    return nullptr;
  }

  llvm::Value* getWeirdCanonicalishInductionVariableFirst(llvm::Loop* L) const
  {
    // TODO
    return nullptr;
  }

  llvm::Value* getOddPhiFirst(llvm::Loop* L, llvm::PHINode* PN) const
  {
    // TODO
    return nullptr;
  }

  bool depthFirstSearch( llvm::Instruction* I, 
                         llvm::LoopInfo& LI, 
                         llvm::Instruction*& Phi, 
                         llvm::SmallVector<llvm::Instruction*, 8>& Instrs, 
                         llvm::SmallVector<llvm::Instruction*, 4>& Loads, 
                         llvm::SmallVector<llvm::Instruction*, 4>& Phis, 
                         std::vector<llvm::SmallVector<llvm::Instruction*, 8>>& Insts )
  {
    // TODO
    return false;
  }

  bool swPrefetchPassImpl(llvm::Function& F)
  {
    // TODO - this is the 'runOnFunction' in the original file
    return false;
  }

  llvm::PreservedAnalyses run(llvm::Function &F, llvm::FunctionAnalysisManager &FAM) {

    std::cout << "Hello function: " << F.getName().str() << std::endl;
    // TODO: uncomment the call below to run the pass
    // swPrefetchPassImpl(F);

    // TODO: use the return from the impl function to decide which preserved analyses to return.
    return llvm::PreservedAnalyses::all();
  }
};
}

extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK llvmGetPassPluginInfo() {
  return {
    LLVM_PLUGIN_API_VERSION, "SwPrefetchPass", "v0.1",
    [](llvm::PassBuilder &PB) {
      PB.registerPipelineParsingCallback(
        [](llvm::StringRef Name, llvm::FunctionPassManager &FPM,
        llvm::ArrayRef<llvm::PassBuilder::PipelineElement>) {
          if(Name == "func-name"){
            FPM.addPass(llvm::LoopUnrollPass());
            FPM.addPass(SwPrefetchPass());
            FPM.addPass(llvm::VerifierPass());
            return true;
          }
          return false;
        }
      );
    }
  };
}