#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/Transforms/Scalar/LoopUnrollPass.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/Debug.h"

#include  <iostream>
#include <array>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>

// To use LLVM_DEBUG
#define DEBUG_TYPE "SwPrefetchPass"


#ifndef IGNORE_SIZE
#define IGNORE_SIZE 0
#endif

#ifndef COMPUTE_C_CONST
#define C_CONSTANT (64)
#endif


namespace {

const std::array<double, 5> K_VALUES{-0.009309007357754038,-0.6902547322677854,-0.01174499830280426,5.290974393057368e-06,0.02107176132152629};

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
              return PN->getIncomingValueForBlock(Incoming);
            }
          }
        }
      }
    }
    
    return nullptr;
  }

  llvm::Value* getOddPhiFirst(llvm::Loop* L, llvm::PHINode* PN) const
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

    if(H != PN->getParent())
    {
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

    return PN->getIncomingValueForBlock(Incoming);
  }

  bool depthFirstSearch( llvm::Instruction* I, 
                         llvm::LoopInfo& LI, 
                         llvm::Instruction*& Phi, 
                         llvm::SmallVector<llvm::Instruction*, 8>& Instrs, 
                         llvm::SmallVector<llvm::Instruction*, 4>& Loads, 
                         llvm::SmallVector<llvm::Instruction*, 4>& Phis, 
                         std::vector<llvm::SmallVector<llvm::Instruction*, 8>>& Insts )
  {
    llvm::Use* u   = I->getOperandList();
    llvm::Use* end = u + I->getNumOperands();

    llvm::SetVector<llvm::Instruction*> roundInsts;

    bool found = false;

    for (llvm::Use* v = u; v < end; v++)
    {
      llvm::PHINode* p = llvm::dyn_cast<llvm::PHINode>(v->get());
      llvm::Loop* L = nullptr;
      if(p) 
      {
        L = LI.getLoopFor(p->getParent());
      }

      if(p && L && (p == getCanonicalishInductionVariable (L) || p == getWeirdCanonicalishInductionVariable(L))) 
      {
        LLVM_DEBUG(llvm::dbgs() << "Loop induction phi node! " << *p << "\n");

        if(Phi) 
        {
          if(Phi == p) 
          {
            //add this
            roundInsts.remove(p);
            roundInsts.insert(p);
            found = true; //should have been before anyway
          } 
          else 
          {
            //check which is older.
            if(LI.getLoopFor(Phi->getParent())->isLoopInvariant(p)) 
            {
              //do nothing
              LLVM_DEBUG(llvm::dbgs() << "not switching phis\n");
            } 
            else if (LI.getLoopFor(p->getParent())->isLoopInvariant(Phi)) 
            {
              LLVM_DEBUG(llvm::dbgs() << "switching phis\n");
              roundInsts.clear();
              roundInsts.insert(p);
              Phi = p;
              found = true;
            } 
            else 
            {
              assert(0);
            }
          }
        } 
        else 
        {
          Phi = p;
          roundInsts.remove(p);
          roundInsts.insert(p);
          found = true;
        }
      }
      else if(llvm::dyn_cast<llvm::StoreInst>(v->get()))
      {}
      else if(llvm::dyn_cast<llvm::CallInst>(v->get()))
      {}
      else if(llvm::dyn_cast<llvm::Instruction>(v->get()) && llvm::dyn_cast<llvm::Instruction>(v->get())->isTerminator())
      {}
      else if(llvm::LoadInst* linst = llvm::dyn_cast<llvm::LoadInst>(v->get())) 
      {
        //Cache results
        int lindex=-1;
        int index=0;
        for(auto l: Loads) 
        {
          if (l == linst)
          {
            lindex=index;
            break;
          }
          index++;
        }

        if(lindex!=-1)
        {
          llvm::Instruction* phi = Phis[lindex];

          if(Phi) 
          {
            if(Phi == phi) 
            {
              //add this
              for(auto q : Insts[lindex])
              {
                roundInsts.remove(q);
                roundInsts.insert(q);
              }
              found = true; //should have been before anyway
            } 
            else 
            {
              //check which is older.
              if(LI.getLoopFor(Phi->getParent())->isLoopInvariant(phi)) 
              {
                //do nothing
                LLVM_DEBUG(llvm::dbgs() << "not switching phis\n");
              } 
              else if (LI.getLoopFor(phi->getParent())->isLoopInvariant(Phi)) 
              {
                LLVM_DEBUG(llvm::dbgs() << "switching phis\n");
                roundInsts.clear();
                for(auto q : Insts[lindex]) 
                {
                  roundInsts.remove(q);
                  roundInsts.insert(q);
                }
                Phi = phi;
                found = true;
              } 
              else 
              {
                assert(0);
              }
            }
          } 
          else 
          {
            for(auto q : Insts[lindex]) 
            {
              roundInsts.remove(q);
              roundInsts.insert(q);
            }
            Phi = phi;
            found = true;
          }
        }
      }
      else if(v->get())
      { 
        if(llvm::Instruction* k = llvm::dyn_cast<llvm::Instruction>(v->get())) 
        {
          if(!((!p) || L != nullptr))
          {
            continue;
          }

          llvm::Instruction* j = k;

          llvm::Loop* L = LI.getLoopFor(j->getParent());
          
          if(L) 
          {
            llvm::SmallVector<llvm::Instruction*, 8> Instrz;

            if(p) 
            {
              LLVM_DEBUG(llvm::dbgs() << "Non-loop-induction phi node! " << *p << "\n");
              
              j = nullptr;
              
              if(!getOddPhiFirst(L,p)) 
              {
                return false;
              }
              
              j = llvm::dyn_cast<llvm::Instruction>(getOddPhiFirst(L,p));
              if(!j) 
              {
                return false;
              }

              Instrz.push_back(k);
              Instrz.push_back(j);
              L = LI.getLoopFor(j->getParent());

            } 
            else 
            {
              Instrz.push_back(k);
            }

            llvm::Instruction* phi = nullptr;
            if(depthFirstSearch(j,LI,phi, Instrz, Loads, Phis, Insts))
            {
              if(Phi) 
              {
                if(Phi == phi) 
                {
                  //add this
                  for(auto q : Instrz) 
                  {
                    roundInsts.remove(q);
                    roundInsts.insert(q);
                  }
                  found = true; //should have been before anyway
                } 
                else 
                {
                  //check which is older.
                  if(LI.getLoopFor(Phi->getParent())->isLoopInvariant(phi)) 
                  {
                    //do nothing
                    LLVM_DEBUG(llvm::dbgs() << "not switching phis\n");
                  } 
                  else if (LI.getLoopFor(phi->getParent())->isLoopInvariant(Phi)) 
                  {
                    LLVM_DEBUG(llvm::dbgs() << "switching phis\n");
                    roundInsts.clear();
                    for(auto q : Instrz) 
                    {
                      roundInsts.remove(q);
                      roundInsts.insert(q);
                    }
                    Phi = phi;
                    found = true;
                  } 
                  else 
                  {
                    assert(0);
                  }
                }
              } 
              else 
              {
                for(auto q : Instrz) 
                {
                  roundInsts.remove(q);
                  roundInsts.insert(q);
                }
                Phi = phi;
                found = true;
              }
            }
          }
        }
      }
    }

    if(found)
    {
      for(auto q : roundInsts)
      {
        Instrs.push_back(q);
      }
    }

    return found;
  }

  void initialize(llvm::Function& F)
  {
    llvm_module = F.getParent();
  }

#ifdef COMPUTE_C_CONST
  double getInfoFromSysFile(std::string sys_file, std::string key)
  {
    double value = 0.0f;

    std::string current;
    std::ifstream file(sys_file);
    while (file >> current)
    {     
      if (current == key)
      {
        file >> value;
        break; 
      }
      file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

    return value;
  }

  double getInfoFromSysFileWithLine(std::string sys_file, std::string s1, std::string s2, std::string s3){
    double value = 0.0f;
    std::ifstream file(sys_file);
    std::string line;
    while (std::getline(file, line))
    {

      std::istringstream iss(line);
      std::string a, b, c;
      if (!(iss >> a >> b >> c)) { continue; }

      if (a == s1 && b == s2 && c == s3){
        iss >> value;
        break;
      }
    }

    return value;
  }

  double getCpuClockSpeed()
  {

    double value = 0.0f;
    std::ifstream file("/proc/cpuinfo");
    std::string line;
    while (std::getline(file, line))
    {
        std::istringstream iss(line);
        std::string a, b, last;
        if (!(iss >> a >> b)) { continue; }

        if (a == "model" && b == "name"){
            while(iss >> last){
            }

            if (last.length() > 3) {
                last.erase(last.length() - 3);
            }

            value = stod(last);
            break;
        }
    }

    return value;
  }

  double getTotalCores()
  {
    return getInfoFromSysFileWithLine("/proc/cpuinfo", "cpu", "cores", ":");
  }

  double getCacheSize()
  {
    return getInfoFromSysFileWithLine("/proc/cpuinfo", "cache", "size", ":");
  }

  double getRamSize()
  {
    return getInfoFromSysFile("/proc/meminfo", "MemTotal:");
  }

  double getPageSize()
  {
    return getInfoFromSysFile("/proc/meminfo", "Hugepagesize:");
  }

  double readIPCValue(int lineIndex)
  {
    std::ifstream file("../../values.txt"); // Open the file
    double ipcValue = 0.0;                  // Variable to store the IPC value
    int currentLine = 0;                    // Current line number

    if (!file.is_open())
    {
      std::cerr << "Unable to open file" << std::endl;
      return -1; // Return -1 or another error code to indicate failure
    }

    std::string line;
    while (getline(file, line)) // Read lines until end of file
    {
      if (currentLine == lineIndex) // Check if the current line is the desired line
      {
        // Convert line to double
        
          ipcValue = std::stod(line);
          file.close();
          return ipcValue;
        
      }
      currentLine++; // Increment line counter
    }

    file.close(); // Make sure to close the file if the desired line is not found
    std::cerr << "Line index out of range" << std::endl;
    return -1; // Return -1 if line index is out of range
  }

 double ipcToPercentage(double ipc)
  {
    // Define the IPC boundaries and corresponding percentages
    double x1 = 1.06, y1 = 110; // IPC = 1.06 corresponds to 110%
    double x2 = 2.07, y2 = 80;  // IPC = 2.07 corresponds to 80%

    // Calculate the percentage using linear interpolation
    double percentage = y1 + ((ipc - x1) * (y2 - y1) / (x2 - x1));

    return percentage;
  }

  int ComputeCConst()
  {
    double cpuSpeed = getCpuClockSpeed();
    double cores = getTotalCores();
    double cacheSize = getCacheSize();
    double ramSize = getRamSize();
    double pageSize = getPageSize();
    double ipc = readIPCValue(1);


    double c_const = K_VALUES[0] * cpuSpeed + K_VALUES[1] * cores + K_VALUES[2] * cacheSize + K_VALUES[3] * ramSize + K_VALUES[4] * pageSize;
    c_const = c_const * ipcToPercentage(ipc);

    std::cout << "cpu speed: " << cpuSpeed << std::endl;
    std::cout << "cores: " << cores  << std::endl;
    std::cout << "cache size: " << cacheSize  << std::endl;
    std::cout << "ram size: " << ramSize  << std::endl;
    std::cout << "page size: " << pageSize  << std::endl;
    std::cout << "IPC:" <<ipc <<std::endl;

    std::cout << "Calculated C Const Value: " << c_const << " ... will be cast to " << static_cast<int>(c_const) << std::endl;

    return static_cast<int>(c_const);
  }
#endif

  bool swPrefetchPassImpl(llvm::Function& F, llvm::FunctionAnalysisManager &FAM)
  {
    // Required to call at the beginning to initialize llvm_module
    initialize(F);

    llvm::LoopInfo& LI = FAM.getResult<llvm::LoopAnalysis>(F);

    bool modified = false;

    llvm::SmallVector<llvm::Instruction*, 4> Loads;
    llvm::SmallVector<llvm::Instruction*, 4> Phis;
    llvm::SmallVector<int, 4> Offsets;
    llvm::SmallVector<int, 4> MaxOffsets;
    std::vector<llvm::SmallVector<llvm::Instruction*, 8>> Insts;

    for(auto& BB : F)
    {
      for (auto& I : BB) 
      {
        if (llvm::LoadInst* i = llvm::dyn_cast<llvm::LoadInst>(&I)) 
        {
          if(LI.getLoopFor(&BB))
          {
            llvm::SmallVector<llvm::Instruction*, 8> Instrz;
            Instrz.push_back(i);
            llvm::Instruction* phi = nullptr;
            if(depthFirstSearch(i,LI,phi,Instrz,  Loads, Phis, Insts)) 
            {
              int loads = 0;
              for(auto &z : Instrz)
              {
                if(llvm::dyn_cast<llvm::LoadInst>(z))
                {
                  loads++;
                }
              }

              if(loads < 2) 
              {
                LLVM_DEBUG(llvm::dbgs() << "stride\n");    //don't remove the stride cases yet though. Only remove them once we know it's not in a sequence with an indirect.
#ifdef NO_STRIDES
                //add a continue in here to avoid generating strided prefetches. Make sure to reduce the value of C accordingly!
                continue;
#endif
              }

              LLVM_DEBUG(llvm::dbgs() << "Can prefetch " << *i << " from PhiNode " << *phi << "\n");
              LLVM_DEBUG(llvm::dbgs() << "need to change \n");
              for (auto &z : Instrz) 
              {
                LLVM_DEBUG(llvm::dbgs() << *z << "\n");
              }

              Loads.push_back(i);
              Insts.push_back(Instrz);
              Phis.push_back(phi);
              Offsets.push_back(0);
              MaxOffsets.push_back(1);

            }
            else
            {
              LLVM_DEBUG(llvm::dbgs() << "Can't prefetch load" << *i << "\n");
            }
          }
        }
      }
    }

    for(uint64_t x = 0; x < Loads.size(); x++) 
    {
      llvm::ValueMap<llvm::Instruction*, llvm::Value*> Transforms;

      bool ignore = true;

      llvm::Loop* L = LI.getLoopFor(Phis[x]->getParent());


      for(uint64_t y = x + 1; y < Loads.size(); y++) 
      {
        bool subset = true;
        for(auto& in : Insts[x])
        {
          if(std::find(Insts[y].begin(), Insts[y].end(), in) == Insts[y].end())
          {
            subset = false;
          }
        }
        if(subset)
        {
          MaxOffsets[x]++;
          Offsets[y]++;
          ignore=false;
        }
      }

      int loads = 0;

      llvm::LoadInst* firstLoad = NULL;

      for(auto& z : Insts[x])
      {
        if(llvm::dyn_cast<llvm::LoadInst>(z))
        {
          if(!firstLoad)
          {
            firstLoad = llvm::dyn_cast<llvm::LoadInst>(z);
          }
          loads++;
        }
      }


      //loads limited to two on second case, to avoid needing to check bound validity on later loads.
      if((getCanonicalishSizeVariable(L)) == nullptr) 
      {
        if(!getArrayOrAllocSize(firstLoad) || loads > 2)
        {
          continue;
        }
      }

      if(loads < 2 && ignore)
      {
        LLVM_DEBUG(llvm::dbgs() << "Ignoring" << *(Loads[x]) << "\n");
        continue; //remove strides with no dependent indirects.
      }


      llvm::IRBuilder<> Builder(Loads[x]);

      bool tryToPushDown = (LI.getLoopFor(Loads[x]->getParent()) != LI.getLoopFor(Phis[x]->getParent()));

      if(tryToPushDown)
      {
        LLVM_DEBUG(llvm::dbgs() << "Trying to push down!\n");
      }

      //reverse list.
      llvm::SmallVector<llvm::Instruction*, 8> newInsts;
      
      for(auto q = Insts[x].end()-1; q > Insts[x].begin()-1; q--)
      {
        newInsts.push_back(*q);
      }
      
      for(auto& z : newInsts) 
      {
        if(Transforms.count(z))
        {
          continue;
        }

        if(z == Phis[x]) 
        {
          llvm::Instruction* n;

          bool weird = false;

          llvm::Loop* L = LI.getLoopFor(Phis[x]->getParent());

          int c_const = 0;

#ifdef COMPUTE_C_CONST
          c_const = ComputeCConst();
#else
          c_const = C_CONSTANT;
#endif

          int offset = (c_const*MaxOffsets[x])/(MaxOffsets[x]+Offsets[x]);

          if(z == getCanonicalishInductionVariable(L))
          {
            n = llvm::dyn_cast<llvm::Instruction>(Builder.CreateAdd(Phis[x],
                                                                    Phis[x]->getType()->isIntegerTy(64) ? llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm_module->getContext()), offset) 
                                                                                                        : llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm_module->getContext()), offset)));
#ifdef BROKEN
            n = llvm::dyn_cast<llvm::Instruction>(Builder.CreateAnd(n, 1));
#endif
          }
          else if (z == getWeirdCanonicalishInductionVariable(L))
          {
            //This covers code where a pointer is incremented, instead of a canonical induction variable.

            n = getWeirdCanonicalishInductionVariableGep(L)->clone();
            Builder.Insert(n);
            n->setOperand(n->getNumOperands()-1, llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm_module->getContext()), offset));
            weird = true;

            bool changed = true;
            while(LI.getLoopFor(Phis[x]->getParent()) != LI.getLoopFor(n->getParent()) && changed)
            {
              llvm::Loop* ol = LI.getLoopFor(n->getParent());

              makeLoopInvariantSpec(n, changed, LI.getLoopFor(n->getParent()));

              if(ol && ol == LI.getLoopFor(n->getParent()))
              {
                break;
              }
            }
          }

          assert(L);
          assert(n);

          llvm::Value* size = getCanonicalishSizeVariable(L);
          if(!size) 
          {
            size = getArrayOrAllocSize(firstLoad);
          }
          assert(size);
          std::string type_str;
          llvm::raw_string_ostream rso(type_str);
          size->getType()->print(rso);
          rso.flush();
        
          if(loads< 2 || !size || !size->getType()->isIntegerTy() || IGNORE_SIZE)
          {
            Transforms.insert(std::pair<llvm::Instruction*, llvm::Instruction*>(z,n));
            continue;
          }

          llvm::Instruction* mod;

          if(weird)
          {
            //This covers code where a pointer is incremented, instead of a canonical induction variable.

            llvm::Instruction* endcast = llvm::dyn_cast<llvm::Instruction>(Builder.CreatePtrToInt(size, llvm::Type::getInt64Ty(llvm_module->getContext())));

            llvm::Instruction* startcast = llvm::dyn_cast<llvm::Instruction>(Builder.CreatePtrToInt(getWeirdCanonicalishInductionVariableFirst(L), llvm::Type::getInt64Ty(llvm_module->getContext())));

            llvm::Instruction* valcast =  llvm::dyn_cast<llvm::Instruction>(Builder.CreatePtrToInt(n, llvm::Type::getInt64Ty(llvm_module->getContext())));


            llvm::Instruction* sub1 = llvm::dyn_cast<llvm::Instruction>(Builder.CreateSub(valcast, startcast));
            llvm::Instruction* sub2 = llvm::dyn_cast<llvm::Instruction>(Builder.CreateSub(endcast, startcast));

            llvm::Value* cmp = Builder.CreateICmp(llvm::CmpInst::ICMP_SLT, sub1, sub2);
            llvm::Instruction* rem = llvm::dyn_cast<llvm::Instruction>(Builder.CreateSelect(cmp, sub1, sub2));

            llvm::Instruction* add = llvm::dyn_cast<llvm::Instruction>(Builder.CreateAdd(rem, startcast));

            mod = llvm::dyn_cast<llvm::Instruction>(Builder.CreateIntToPtr(add, n->getType()));
          }
          else if(size->getType() != n->getType())
          {
            llvm::Instruction* cast = llvm::CastInst::CreateIntegerCast(size, n->getType(), true);
            assert(cast);
            Builder.Insert(cast);
            llvm::Value* sub = Builder.CreateSub(cast, llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm_module->getContext()), 1));
            llvm::Value* cmp = Builder.CreateICmp(llvm::CmpInst::ICMP_SLT, sub, n);
            mod = llvm::dyn_cast<llvm::Instruction>(Builder.CreateSelect(cmp, sub, n));
          } 
          else 
          {
            llvm::Value* sub = Builder.CreateSub(size, llvm::ConstantInt::get(n->getType(), 1));
            llvm::Value* cmp = Builder.CreateICmp(llvm::CmpInst::ICMP_SLT, sub, n);
            mod = llvm::dyn_cast<llvm::Instruction>(Builder.CreateSelect(cmp, sub, n));
          }


          bool changed = true;
          while(LI.getLoopFor(Phis[x]->getParent()) != LI.getLoopFor(mod->getParent()) && changed)
          {
            llvm::Loop* ol = LI.getLoopFor(mod->getParent());
            makeLoopInvariantSpec(mod, changed,LI.getLoopFor(mod->getParent()));
            if(ol && ol == LI.getLoopFor(mod->getParent()))
            {
              break;
            }
          }

          Transforms.insert(std::pair<llvm::Instruction*, llvm::Instruction*>(z, mod));
          modified = true;
        } 
        else if (z == Loads[x])
        {
          assert(Loads[x]->getOperand(0));

          llvm::Instruction* oldGep = llvm::dyn_cast<llvm::Instruction>(Loads[x]->getOperand(0));
          assert(oldGep);

          assert(Transforms.lookup(oldGep));
          llvm::Instruction* gep = llvm::dyn_cast<llvm::Instruction>(Transforms.lookup(oldGep));
          assert(gep);
          modified = true;


          llvm::Instruction* cast = llvm::dyn_cast<llvm::Instruction>(Builder.CreateBitCast (gep, llvm::Type::getInt8PtrTy(llvm_module->getContext())));

          bool changed = true;
          while(LI.getLoopFor(Phis[x]->getParent()) != LI.getLoopFor(cast->getParent()) && changed) 
          {
            llvm::Loop* ol = LI.getLoopFor(cast->getParent());
            makeLoopInvariantSpec(cast,changed,ol);
            if(ol && ol == LI.getLoopFor(cast->getParent()))
            {
              break;
            }
          }


          llvm::Value* ar[] = { cast,
                                llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm_module->getContext()), 0),
                                llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm_module->getContext()), 3),
                                llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm_module->getContext()), 1) };
          
          llvm::ArrayRef<llvm::Type*> art = { llvm::Type::getInt8PtrTy(llvm_module->getContext()) };
          
          llvm::Function* fun = llvm::Intrinsic::getDeclaration(llvm_module, llvm::Intrinsic::prefetch, art);

          assert(fun);

          llvm::CallInst* call = llvm::CallInst::Create(fun,ar);

          call->insertBefore(cast->getParent()->getTerminator());

        } 
        else if(llvm::PHINode* pn = llvm::dyn_cast<llvm::PHINode>(z)) 
        {
          llvm::Value* v = getOddPhiFirst(LI.getLoopFor(pn->getParent()),pn);
          
          if(v)
          {
            if(llvm::Instruction* ins = llvm::dyn_cast<llvm::Instruction>(v))
            {
              v = Transforms.lookup(ins);
            }
            Transforms.insert(std::pair<llvm::Instruction*, llvm::Value*>(z,v));
            } 
            else 
            {
              Transforms.insert(std::pair<llvm::Instruction*, llvm::Value*>(z,z));
            }
        }
        else 
        {

          llvm::Instruction* n = z->clone();

          llvm::Use* u = n->getOperandList();
          int64_t size = n->getNumOperands();
        
          for(int64_t x = 0; x<size; x++)
          {
            llvm::Value* v = u[x].get();
            assert(v);
            if(llvm::Instruction* t = llvm::dyn_cast<llvm::Instruction>(v))
            {
              if(Transforms.count(t))
              {
                n->setOperand(x,Transforms.lookup(t));
              }
            }
          }

          n->insertBefore(Loads[x]);

          bool changed = true;
          while(changed && LI.getLoopFor(Phis[x]->getParent()) != LI.getLoopFor(n->getParent()))
          {
            changed = false;

            makeLoopInvariantSpec(n,changed,LI.getLoopFor(n->getParent()));
            if(changed)
            {
              LLVM_DEBUG(llvm::dbgs()<< "moved loop" << *n << "\n");
            }
            else
            {
              LLVM_DEBUG(llvm::dbgs()<< "not moved loop" << *n << "\n");
            }
          }

          Transforms.insert(std::pair<llvm::Instruction*, llvm::Instruction*>(z,n));
          modified = true;
        }
      }
    }

    return modified;
  }

  llvm::PreservedAnalyses run(llvm::Function &F, llvm::FunctionAnalysisManager &FAM) 
  {
    std::cout << "Hello function: " << F.getName().str() << std::endl;

    bool modified = swPrefetchPassImpl(F, FAM);

    std::cout << "Modified: " << modified << std::endl;

    auto ret = modified ? llvm::PreservedAnalyses::none() : llvm::PreservedAnalyses::all();
    return ret;
  }

  // members
  llvm::Module* llvm_module = nullptr;
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
            // FPM.addPass(llvm::LoopUnrollPass());
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