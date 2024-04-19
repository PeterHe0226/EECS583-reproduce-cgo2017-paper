#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/Transforms/Scalar/LoopUnrollPass.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/Debug.h"

#include  <iostream>

// To use LLVM_DEBUG
#define DEBUG_TYPE "SwPrefetchPass"

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
    LLVM_DEBUG(llvm::dbgs() << "attempting to get size of base array:\n");
    LLVM_DEBUG(l->getPointerOperand()->print(llvm::dbgs()));

    llvm::GetElementPtrInst* gep = llvm::dyn_cast<llvm::GetElementPtrInst>(l->getPointerOperand());

    if (!gep)
    {
      LLVM_DEBUG(llvm::dbgs() << "Couldn't find gep \n");
      return nullptr;
    }

    LLVM_DEBUG(llvm::dbgs() << " with ptr: ");
    LLVM_DEBUG(gep->getPointerOperand()->print(llvm::dbgs()));

    auto ArrayStart = gep->getPointerOperand();
    
    if(llvm::ArrayType* at = llvm::dyn_cast<llvm::ArrayType>(gep->getSourceElementType())) 
    {
      LLVM_DEBUG(llvm::dbgs() << " and size: ");
      LLVM_DEBUG(llvm::dbgs() << std::to_string(at->getNumElements()));

      int size = at->getNumElements();

      LLVM_DEBUG(llvm::dbgs() << "and type: " << *(at->getElementType ()));

	    return llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm_module->getContext()), size);
    }
    else if(llvm::AllocaInst* ai = llvm::dyn_cast<llvm::AllocaInst>(ArrayStart)) 
    {
      LLVM_DEBUG(llvm::dbgs() << "and dynamic allocated size " << *(ai->getArraySize()));
      LLVM_DEBUG(llvm::dbgs() << *(ai->getArraySize ()));

      LLVM_DEBUG(llvm::dbgs() << "and type: ");
      LLVM_DEBUG(llvm::dbgs() << *(gep->getSourceElementType ()));

      return ai->getArraySize ();
    }

    return nullptr;
  }

  llvm::Value* getCanonicalishSizeVariable(llvm::Loop* L) const
  {
    // Loop over all of the PHI nodes, looking for a canonical indvar.
    auto B = L->getExitingBlock();
    
    if(!B)
    {
      return nullptr;
    }

    llvm::CmpInst* CI = nullptr;

    //really, should be reverse dataflow from the terminating jump
    for (llvm::Instruction& J : *B)
    {
      llvm::Instruction* I = &J;
	    CI = llvm::dyn_cast<llvm::CmpInst>(I) ? llvm::dyn_cast<llvm::CmpInst>(I) : CI;
    }

    bool Changed = false;
    if (CI)
    {
      if(L->makeLoopInvariant(CI->getOperand(1), Changed) 
         || makeLoopInvariantPredecessor(CI->getOperand(1), Changed, L))
      {
        return CI->getOperand(1);
      }

	    if(L->makeLoopInvariant(CI->getOperand(0), Changed)
         || makeLoopInvariantPredecessor(CI->getOperand(1), Changed , L))
      {
        return CI->getOperand(0);
      }

	    LLVM_DEBUG(llvm::dbgs() << "Size not loop invariant!" << *(CI->getOperand(0)) << *(CI->getOperand(1)) << "\n");
    }

    return nullptr;
  }

  llvm::PHINode* getCanonicalishInductionVariable(llvm::Loop* L) const
  {
    llvm::BasicBlock* H = L->getHeader();

    llvm::BasicBlock* Incoming = nullptr;
    llvm::BasicBlock* Backedge = nullptr;
    llvm::pred_iterator PI = llvm::pred_begin(H);

    assert(PI != llvm::pred_end(H) && "Loop must have at least one backedge!");

    Backedge = *PI++;
    if (PI == llvm::pred_end(H))
    {
      // indicates dead loop
      return nullptr;
    }
     
    Incoming = *PI++;
    if (PI != llvm::pred_end(H))
    {
      // indicates the possibility of multiple backedges
      return nullptr;
    }

    if (L->contains(Incoming))
    {
	    if (L->contains(Backedge))
	    {
        return nullptr;
      }
	    std::swap(Incoming, Backedge);
    }
    else if (!L->contains(Backedge))
    {
      return nullptr;
    }

    for (llvm::BasicBlock::iterator I = H->begin(); llvm::isa<llvm::PHINode>(I); ++I)
    {
      llvm::PHINode* PN = llvm::cast<llvm::PHINode>(I);

      if(!PN->getIncomingValueForBlock(Backedge))
      {
        return nullptr;
      }

      if (llvm::Instruction* Inc = llvm::dyn_cast<llvm::Instruction>(PN->getIncomingValueForBlock(Backedge)))
      {
        if (Inc->getOpcode() == llvm::Instruction::Add && Inc->getOperand(0) == PN)
        {
          if (llvm::dyn_cast<llvm::ConstantInt>(Inc->getOperand(1)))
          {
            return PN;
          }
        }
      }
    }

    return nullptr;
  }

  llvm::PHINode* getWeirdCanonicalishInductionVariable(llvm::Loop* L) const
  {
    llvm::BasicBlock* H = L->getHeader();

    llvm::BasicBlock* Incoming = nullptr;
    llvm::BasicBlock* Backedge = nullptr;
    llvm::pred_iterator PI = llvm::pred_begin(H);

    assert(PI != llvm::pred_end(H) && "Loop must have at least one backedge!");

    Backedge = *PI++;
    if (PI == llvm::pred_end(H))
    {
      // indicates dead loop
      return nullptr;
    }

    Incoming = *PI++;
    if (PI != pred_end(H))
    {
      // indicates the possibility of multiple backedges
      return nullptr;
    }

    if (L->contains(Incoming))
    {
      if (L->contains(Backedge))
      {
        return nullptr;
      }
      std::swap(Incoming, Backedge);
    }
    else if (!L->contains(Backedge))
    {
      return nullptr;
    }

    // Loop over all of the PHI nodes, looking for a canonical indvar.
    for (llvm::BasicBlock::iterator I = H->begin(); llvm::isa<llvm::PHINode>(I); ++I)
    {
      llvm::PHINode* PN = llvm::cast<llvm::PHINode>(I);

      if (llvm::GetElementPtrInst* Inc = llvm::dyn_cast<llvm::GetElementPtrInst>(PN->getIncomingValueForBlock(Backedge)))
      {
        if (Inc->getOperand(0) == PN)
        {
          if (llvm::ConstantInt* CI = llvm::dyn_cast<llvm::ConstantInt>(Inc->getOperand(Inc->getNumOperands()-1)))
          {
            if (CI->equalsInt(1))
            {
              return PN;
            }
          }
        }
      }
    }
    
    return nullptr;
  }

  llvm::GetElementPtrInst* getWeirdCanonicalishInductionVariableGep(llvm::Loop* L) const
  {
    llvm::BasicBlock* H = L->getHeader();

    llvm::BasicBlock* Incoming = nullptr;
    llvm::BasicBlock* Backedge = nullptr;
    llvm::pred_iterator PI = llvm::pred_begin(H);

    assert(PI != llvm::pred_end(H) && "Loop must have at least one backedge!");

    Backedge = *PI++;
    if (PI == llvm::pred_end(H))
    {
      // indicates dead loop
      return nullptr;
    }

    Incoming = *PI++;
    if (PI != pred_end(H))
    {
      // indicates the possibility of multiple backedges
      return nullptr;
    }

    if (L->contains(Incoming))
    {
      if (L->contains(Backedge))
      {
        return nullptr;
      }
      std::swap(Incoming, Backedge);
    }
    else if (!L->contains(Backedge))
    {
      return nullptr;
    }

    // Loop over all of the PHI nodes, looking for a canonical indvar.
    for (llvm::BasicBlock::iterator I = H->begin(); llvm::isa<llvm::PHINode>(I); ++I)
    {
      llvm::PHINode* PN = llvm::cast<llvm::PHINode>(I);

      if (llvm::GetElementPtrInst* Inc = llvm::dyn_cast<llvm::GetElementPtrInst>(PN->getIncomingValueForBlock(Backedge)))
      {
        if (Inc->getOperand(0) == PN)
        {
          if (llvm::ConstantInt* CI = llvm::dyn_cast<llvm::ConstantInt>(Inc->getOperand(Inc->getNumOperands()-1)))
          {
            if (CI->equalsInt(1))
            {
              return Inc;
            }
          }
        }
      }
    }
    
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

  void initialize(llvm::Function& F)
  {
    llvm_module = F.getParent();

    if (data_layout)
    {
      delete data_layout;
    }

    data_layout = new llvm::DataLayout(llvm_module);
  }

  void deinitialize()
  {
    llvm_module = nullptr;

    if (data_layout)
    {
      delete data_layout;
      data_layout = nullptr;
    }
  }

  bool swPrefetchPassImpl(llvm::Function& F)
  {
    // Required to call at the beginning to initialize llvm_module and data_layout
    initialize(F);

    // TODO - this is the 'runOnFunction' in the original file
    
    // Ensure that this is called before any early termination as well
    deinitialize();
    return false;
  }

  llvm::PreservedAnalyses run(llvm::Function &F, llvm::FunctionAnalysisManager &FAM) 
  {
    std::cout << "Hello function: " << F.getName().str() << std::endl;

    bool modified = swPrefetchPassImpl(F);

    auto ret = modified ? llvm::PreservedAnalyses::none() : llvm::PreservedAnalyses::all();
    return ret;
  }

  // members
  llvm::Module*     llvm_module = nullptr;
  llvm::DataLayout* data_layout = nullptr;
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