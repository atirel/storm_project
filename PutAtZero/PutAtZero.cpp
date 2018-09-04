/**
 * This LLVM pass adds a STORE 0 %address after a load followed by a store in order to check with their values wether or not 2 variables are semantically equivalent
 * It should be used after the dse pass since it reduces significally the number of instructions to handle.
 * @author INRIA Bordeaux STORM Project Team
 **/

#if _WIN32 || _WIN64
   #if _WIN64
      #define ENV64BIT
   #else
      #define ENV32BIT
   #endif
#endif

#if __GNUC__
   #if __x86_64__ || __ppc64__
      #define ENV64BIT
   #else
      #define ENV32BIT
   #endif
#endif



#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/Analysis/LoopInfoImpl.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Analysis/PostDominators.h"
#include <algorithm>
#include <utility>
#include "PutAtZero.h"

using namespace llvm;

namespace {
 struct PutAtZero : public FunctionPass {

   static char ID;
   static int numSTORE0ADDED;//The number of instruction STORE 0 added

   PutAtZero() : FunctionPass(ID) {} //we're building a new pass

   /**
    * @function runOnFunction override:
    * allows our pass to be run when necessary/possible
    * @param F the current function
    * @returns true if the pass was able to run since it modifies the code
    * false elsewhere
    *
    **/
   bool runOnFunction(Function &F) override {
      LoopInfo& loopData = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
      DominatorTree& DT = getAnalysis<DominatorTreeWrapperPass>().getDomTree();

      BlockList blockList;//The blocks with their colors
      InstructionList iList;//The variables and their booleans (dead or not) for each block
      std::vector<BasicBlock*> dominator_blocks;//the dominator blocks: the blocks you are forced to go through in order to reach the exit
      std::vector<BasicBlock*> deadEndBlocks;//the eventually dead end blocks (assert fail, exit values...)

      BasicBlock* BB = &F.back();//the last block
      dominator_blocks.push_back(BB);//obviously, the last block is visited since it contains the return instruction (even in void functions)

      while(BB != nullptr){

	 if(BasicBlock* B_new = BB->getSinglePredecessor()){//if the basicBlock only has one predecessor
	    if(loopData.getLoopFor(B_new) == nullptr){//and this predecessor is in a loop (in general a loop header)
	       //we put the headers away in order to avoid setting the 0 value to a potentially reused variable.
	       //in cases such as follows:
	       //	---------------
	       //	|first block  |
	       //	|             |                                     Dominator block (you have to go through the first block to reach the end)
	       //	|varia x used |
	       //	---------------
	       //              |
	       //              v
	       //	---------------
	       //	|header block |
	       //	|             |-----------                          Dominator block (only successor of the first block)
	       //	|varia x used |          |
	       //	---------------          |
	       //            |  ^                |
	       //            v  |                |
	       //	---------------          |
	       //	|loop body    |          |
	       //	|             |          |                          Non dominator block (if the loop condition is false, the block is not used)
	       //	|varia x used |          |
	       //	---------------          |
	       //                                |
	       //	----------------         |
	       //	|return block  |         |
	       //	|              |<---------                          By definition
	       //	| x unused     |
	       //	----------------
	       //
	       // If we consider the header block as a part of the dominators we use, it will be considerated as the last dominator using x, while the loop body might be executed after
	       // Hence, we cannot add the headers to our vector which purpose is to find the first dominator after the last use of each variable
   	       dominator_blocks.push_back(B_new);
	    }
	    BB = B_new;
	 }
	 else{
   	    auto it = pred_begin(BB), it_end = pred_end(BB);//if there are several predecessors, we iterate over them
	    if(it != it_end){//special case for dead ends
   	       B_new = *it;
	    }
   	    while(it != it_end){
	       B_new = DT.findNearestCommonDominator(B_new, *it);
	       //we get the block which dominates both the current one (*it) and the "new one" (B_new) in order to make sure that B_new dominates all the predecessors of BB
	       it++;
	    }
	    if(B_new != nullptr && loopData.getLoopFor(B_new) == nullptr){
   	       dominator_blocks.push_back(B_new);
	    }
	    BB = B_new;
	 }
      }

      for(BasicBlock &BB : F){//we put all the blocks to white since there is no block treated
	 if(isDeadEnd(&BB)){//if the block is a dead end, we add it to our deadEnd vector
	    deadEndBlocks.push_back(&BB);
	 } 
	 blockList[&BB] = white;
      }

      std::list<BasicBlock*> toTreat;
      // We will have to visit all the blocks from the return one to the first one.
      // We cannot iterate easily over them since some of them will be visited several times (each block with more than one successor is visited after each successor so at least twice)
      // We use a FIFO construction to handle the blocks in the right order
      toTreat.push_back(&F.back());
      
      if(loopData.isLoopHeader(&F.back())){
	 errs() << "can't do anything, fix your code, there shall not be any way to reach a loop header at the end \n";
	 return false;
      }


      while(!toTreat.empty()){//while there is an untreated block,

	 BasicBlock* header = toTreat.front();
	 Instruction* I = header->getTerminator();
	 
	 if(loopData.isLoopHeader(header)){//if the block is a loop header (we need to treat the loop aside since some variables might only be used in the loop)
	    Loop* theLoop = loopData.getLoopFor(header);
	    loop_handler(iList, blockList, theLoop);//treats the loop
  	    for(auto it = pred_begin(header), it_end = pred_end(header); it != it_end; ++it){//once the loop is treated we add the blocks outside of it in the toTreat vector
	       if(!theLoop->contains(*it)){
		  iList[(*it)] = iList[header];
	  	  toTreat.push_back(*it);
	       }
	    }
	    blockList[header] = black;
	 }


	 else{
	    //if there is a successor untreated which is not a dead end, we have to make sure it is correctly handled so we put it at the end of the vector to treat it only once every single successor was handled
	    for(auto itBlock = succ_begin(header), itBlock_end = succ_end(header); itBlock != itBlock_end; itBlock++){
	       if(blockList[*itBlock] == white && !isDeadEnd(*itBlock)){
		  toTreat.push_back(header);
		  I = nullptr;
		  break;
	       }
	    }
   	    if(I != nullptr){
	       setD(iList, header, false, getFirstDom(dominator_blocks, I->getParent(), DT));//modifies the boolean status of the variables and put them at zero if necessary
	    }
	    for(auto it = pred_begin(header), et = pred_end(header); it != et; ++it){//then, we put all the predecessor in the toTreat vector
	       BasicBlock* pred = *it;
	       if(isToBePushed(iList, toTreat, pred)){
		  toTreat.push_back(pred);
		  iList[pred] = iList[header];
	       }
	    }
   	    blockList[header] = black;
	 }
	 toTreat.pop_front();
      }//end while


      array_handler(F, dominator_blocks, DT);//then we handle the arrays
      kill_unreachables(deadEndBlocks, F);//end we had a 0 setting in the deadEndBlocks
      
      
      
      return true;
   }

   /**
    * @function isDeadEnd:
    * is the given block a dead end?
    * @param BasicBlock* BB the block to test
    * @returns true if the block contains an unreachable instruction ---> is a dead end (no successor but not a normal exit either)
    * false elsewhere
    *
    **/
   bool isDeadEnd(BasicBlock* BB){
      Instruction* I = BB->getTerminator();//for all the instructions in the block
      while(I != nullptr){
	 if(UnreachableInst* CI = dyn_cast<UnreachableInst>(I)){//if I is an unreachable instruction
	    return true;//the block is a dead end
	 }
	 I = I->getPrevNode();
      }
      return false;
   }

   /**
    * @function getFirstDom:
    * returns the first dominator block (according to the previous definition) after BB
    * @param dominators: a vector with all the dominators blocks
    * @param BB: the current basicBlock
    * @param DT: the DominatorTree advised in order to use the "dominate" methods
    * @returns: retBlock, the first dominant BasicBlock after BB (BB if BB is a dominator
    *
    **/
   BasicBlock* getFirstDom(std::vector<BasicBlock*> &dominators, BasicBlock* BB, DominatorTree& DT){
      if(std::find(dominators.begin(), dominators.end(), BB) != dominators.end()){//if BB is a dominator, then BB is the first dominator block "after" itself
 	 return BB;
      }
      BasicBlock* retBlock = &BB->getParent()->back();//else we start from the last block which contains the return instruction and by definition is a dominator block
      for(auto it = dominators.begin(), end = dominators.end(); it != end; ++it){//and we iterate over dominators to find the closest to BB
	 if(DT.properlyDominates(BB, *it)){
	    retBlock = *it;
	 }
	 else{
	    return retBlock;
	 }
      }
   }

   /**
    * @function array_handler:
    * handle array which are some "weird variables" and put a 0 in all their cases after the "last use" (last access to an array case)
    * @param F the current running function
    * @param dominator_blocks, the vector containing all the dominators
    * @param DT the dominator tree, for method calling purposes
    * @returns: nothing but the arrays are handled in a way such as if they were variables
    *
    **/
   void array_handler(Function& F, std::vector<BasicBlock*> &dominator_blocks, DominatorTree& DT){
      std::vector<AllocaInst*> atZeroArrays;//arrays already to 0
      BasicBlock* BB = &F.back();
      while(BB != nullptr){
	 Instruction* I = BB->getTerminator();
	 while(I != nullptr){//we iterate over the instructions from the last one to the first one to find any array access (opcode 32)
	    if(I->getOpcode() == 32){
	       if(AllocaInst* AI = dyn_cast<AllocaInst>(I->getOperand(0))){//we check that it access a 
		  if(std::find(atZeroArrays.begin(), atZeroArrays.end(), AI) == atZeroArrays.end()){
   		     dead_array(I, dominator_blocks, DT);
   		     atZeroArrays.push_back(AI);
		  }
	       }
	    }
	    I = I->getPrevNode();
	 }
	 BB = BB->getPrevNode();
      }
   }

   /**
    * @function dead_array:
    * properly set all the cases to zero once the array is dead
    * @param I, the last Instruction using the variable
    * @param dominators, the dominators
    * @param DT, the dominatorTree for method purposes
    * @returns nothing but the array is put to 0 once and for all
    *
    **/
   void dead_array(Instruction* I, std::vector<BasicBlock*> &dominators, DominatorTree &DT){
   	 BasicBlock* BB = getFirstDom(dominators, I->getParent(), DT);
	 Instruction* place = I->getNextNode();
	 if(BB != I->getParent()){
	    place = &BB->front();
	 }
	    addStore0(*I, I->getOperand(0), place);
   }

   /**
    * @function kill_unreachables:
    * put all the variables to the 0 value (regardless of both their type and current value) in all the "dead end" blocks to insure that everything is back to normal at any exit of the function.
    * @param unreachables, a vector with all the blocks containing an unreachable instruction.
    * @param F, the current function
    * @returns nothing but sets all the variable used in all the function to 0
    *
    **/

   void kill_unreachables(std::vector<BasicBlock*> unreachables, Function& F){
      BasicBlock* firstBlock = &F.front();
      for(auto it = unreachables.begin(), end = unreachables.end(); it != end; ++it){
	 for(Instruction& I : *firstBlock){
	    if(AllocaInst* AI = dyn_cast<AllocaInst>(&I)){
	       addStore0(I, AI, &(*it)->front());
	    }
	 }
      }
   }

   /**
    * @function loop_handler:
    * handles the loops variables manipulations
    * @param iList: the boolean status of each variable in each block
    * @param bList: the colors of the blocks
    * @param loop: the current loop
    * @returns nothing but put all the blocks in the loop to black and treats the boolean values and dead status
    *
    **/
   void loop_handler(InstructionList &iList, BlockList &bList, Loop* loop){
      LoopInfo& loopData = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
      std::list<BasicBlock*> toTreat;
      toTreat.push_back(loop->getHeader());
      while(!toTreat.empty()){
   	 BasicBlock* BB = toTreat.front();
	 if(loopData.getLoopFor(BB) != loop){
	    loop_handler(iList, bList, loopData.getLoopFor(BB));
	 }
	 if(bList[BB] == white){
       	    partial_treat(BB);
	    setD(iList, BB, true);
	    for(auto it_pred = pred_begin(BB), it_pred_end = pred_end(BB); it_pred != it_pred_end; it_pred++){
	       if(isToBePushed(iList, toTreat, *it_pred) && loop->contains(*it_pred)){
		  toTreat.push_back(*it_pred);
		  iList[*it_pred] = iList[BB];
	       }
	    }
	    bList[BB] = gray;
	 }
	 else{
	    if(bList[BB] == gray || (loopData.isLoopHeader(BB) && BB != loop->getHeader())){
	       setD(iList, BB, true);
	    for(auto it_pred = pred_begin(BB), it_pred_end = pred_end(BB); it_pred != it_pred_end; it_pred++){
	       if(loop->contains(*it_pred) && isToBePushed(iList, toTreat, *it_pred)){
		  if(loop->getHeader() != *it_pred){
   		     toTreat.push_back(*it_pred);
   		     iList[*it_pred] = iList[BB];
		  }
	       }
	    }
	    bList[BB] = black;
	    }
	 }
	 toTreat.pop_front();
      }
      for(auto it_suc = succ_begin(loop->getHeader()), it_suc_end = succ_end(loop->getHeader()); it_suc != it_suc_end; it_suc++){
	 if(loop->contains(*it_suc)){
      	    iList[loop->getHeader()] = iList[*it_suc];
	 }
      }
   }


   /**
    * @function partial_treat:
    * manages the blocks which are in a loop but only to set them to 0 when necessary, not modifying their boolean status
    * @param BB a basicblock which is to be treated
    * @precond BB is part of a loop
    * @returns nothing but BB is modified to set to 0 any variable locally dead
    *
    **/
   void partial_treat(BasicBlock* BB){//technically only for loop blocks
      std::vector<std::pair<std::pair<AllocaInst*, int>, Instruction*>> varia_vect;
      Instruction* I = BB->getTerminator();
      while(I != nullptr){
	 bool found = false;
	 if(LoadInst* LI = dyn_cast<LoadInst>(I)){
	    if(AllocaInst* AI = dyn_cast<AllocaInst>(LI->getOperand(0))){
   	       for(auto it = varia_vect.begin(), it_end = varia_vect.end(); it != it_end; it++){
   		  if((*it).first.first == AI){
		     if((*it).first.second > 0 && (*it).second->getOpcode() == 31){
			addStore0(*LI);
		     }
   		     (*it).second = LI;
   		     found = true;
   		     break;
   		  }
   	       }
   	       if(!found){
   		  std::pair<std::pair<AllocaInst*, int>, Instruction*> new_var;
   		  new_var.first.first = AI;
   		  new_var.first.second = 0;
   		  new_var.second = LI;
   		  varia_vect.push_back(new_var);
   	       }
	    }
	 }
	 if(StoreInst* SI = dyn_cast<StoreInst>(I)){
	    if(AllocaInst* AI = dyn_cast<AllocaInst>(SI->getOperand(1))){
	       for(auto it = varia_vect.begin(), it_end = varia_vect.end(); it != it_end; it++){
		  if((*it).first.first == AI){
		     ((*it).first.second)++;
		     found = true;
		     (*it).second = SI;
		     break;
		  }
	       }
	       if(!found){
		  std::pair<std::pair<AllocaInst*, int>, Instruction*> new_var;
		  new_var.first.first = AI;
		  new_var.first.second = 1;
		  new_var.second = SI;
		  varia_vect.push_back(new_var);
	       }
	    }
	 }
	 I = I->getPrevNode();
      }
   }

   /**
    *
    * @function isToBePushed:
    * uses the information in the toTreat list in order to know if a BasicBlock is to be pushed 
    * or if it is already in the list and thus should be updated.
    * updates the Block subsenquently.
    * @param iList, the variables and their boolean status for each block which will be used to update if necessary
    * @param toTreat, the list with all the untreated BasicBlocks
    * @param futureBlock, the BasicBlock tested: is futureBlock supposed to be pushed in the toTreat list
    * @returns true if the BasicBlock is not in the toTreat list and then should be added to be treated later
    * false elsewhere.
    * if false is returned, the futureBlock status in iList is updated to take into consideration the information of the current block
    *
    **/
   bool isToBePushed(InstructionList &iList, std::list<BasicBlock*> &toTreat, BasicBlock* futureBlock){
      if(toTreat.size() == 0){
	 return true;
      }
      for(auto it = ++toTreat.begin(); it != toTreat.end(); it++ ){
	 BasicBlock* BB = *it;
	 if(BB == futureBlock){
	    andBetweenBlocks(iList, toTreat.front(), futureBlock);
	    return false;
	 }
      }
      return true;
   }

   /**
    *
    * @function andBetweenBlocks:
    * updates the information in the iList (the boolean status of the variables modified) according to the fact that both current_block and an already treated block lead to futureBlock
    * @param iList, the vector with the variables and their boolean status for each block
    * @param current_block, the block we are currently treating
    * @param futureBlock, the supposedly next block to treat
    * @precond in our case, futureBlock must be a predecessor of current_block
    * @returns nothing but updates the information for the futureBlock according to the fact that the current_block is one of its successors
    *
    **/
   void andBetweenBlocks(InstructionList &iList, BasicBlock *current_block, BasicBlock* futureBlock){
      for(int i = 0; i < iList[futureBlock].size(); ++i){
	 for(int j = 0; j < iList[current_block].size(); ++j){
	    if(iList[futureBlock][i].first == iList[current_block][j].first){
	       iList[futureBlock][i].second = iList[futureBlock][i].second && iList[current_block][j].second;
	    }
	 }
      }
   }

   /**
    * @function override llvm::getAnalysisUsage:
    * this function allows us to get and use LoopInfo
    * @param void
    * @returns void
    **/
   virtual void getAnalysisUsage(AnalysisUsage& AU) const override{
      AU.addRequired<LoopInfoWrapperPass>();
      AU.addRequired<DominatorTreeWrapperPass>();
   }

   /**
    * @function setD:
    * this function sets the boolean status of the variable (dead or alive)
    * @param iList, the Block list with its vector of variables and their respectives status
    * @param AI, the Allocation Instruction which created the current variable
    * @param I, the current Instruction
    * @param BB, the current BasicBlock
    * @param isLoopHeader, a boolean which tells us whether or not we are in a loop header.
    * @returns nothing but the variable matching AI, has its status updated in iList after Instruction I in block BB
    **/
   void setD(InstructionList& iList, BasicBlock* BB, bool isLoopHeader, BasicBlock* firstDominatorBlock = nullptr){
      if(isLoopHeader){
	 firstDominatorBlock = &BB->getParent()->back();
	 Instruction* I = BB->getTerminator();
	 while(I != nullptr){
   	    bool found = false;
   	    if(I->getOpcode() == 30){
	       if(AllocaInst* AI = dyn_cast<AllocaInst>(I->getOperand(0))){
   		  for(int k = 0; k < iList[BB].size(); ++k){
   		     if(iList[BB][k].first == AI){
   			iList[BB][k].second = false;
   			found = true;
   		     }
   		  }
   		  if(!found){
   		     IsDead varia;
   		     varia.first = AI;
   		     varia.second = false;
   		     iList[BB].push_back(varia);
	    	     addStore0(*I, I->getOperand(I->getNumOperands() - 1), &firstDominatorBlock->front());
   		  }
   	       }
	    }
	    if(I->getOpcode() == 31){
	       if(AllocaInst *AI = dyn_cast<AllocaInst>(I->getOperand(1))){
   		  for(int k = 0; k < iList[BB].size(); ++k){
   		     if(iList[BB][k].first == AI){
   			iList[BB][k].second = true;
   			found = true;
   		     }
   		  }
   		  if(!found){
   		     IsDead varia;
   		     varia.first = AI;
  		     varia.second = true;
   		     iList[BB].push_back(varia);
	    	     addStore0(*I, I->getOperand(I->getNumOperands() - 1), &firstDominatorBlock->front());
   		  }
   	       }
	    }
	       if(AllocaInst* AI = dyn_cast<AllocaInst>(I->getOperand(0))){
   		  for(int k = 0; k < iList[BB].size(); ++k){
   		     if(iList[BB][k].first == AI){
   			iList[BB][k].second = false;
   			found = true;
   		     }
   		  }
   		  if(!found){
   		     IsDead varia;
   		     varia.first = AI;
   		     varia.second = false;
   		     iList[BB].push_back(varia);
	    	     addStore0(*I, I->getOperand(I->getNumOperands() - 1), &firstDominatorBlock->front());
   		  }
   	       }
	    }
	    if(I->getOpcode() == 31){
	       if(AllocaInst *AI = dyn_cast<AllocaInst>(I->getOperand(1))){
   		  for(int k = 0; k < iList[BB].size(); ++k){
   		     if(iList[BB][k].first == AI){
   			iList[BB][k].second = true;
   			found = true;
   		     }
   		  }
   		  if(!found){
   		     IsDead varia;
   		     varia.first = AI;
  		     varia.second = true;
   		     iList[BB].push_back(varia);
	    	     addStore0(*I, I->getOperand(I->getNumOperands() - 1), &firstDominatorBlock->front());
   		  }
   	       }
	    }
	    I = I->getPrevNode();
	 }
      }
      else{
	 Instruction* I = BB->getTerminator();
	 while(I != nullptr){
	    if(I->getOpcode() == 30 || I->getOpcode() == 31){
	       if(AllocaInst* AI = dyn_cast<AllocaInst>(I->getOperand(I->getNumOperands() - 1))){
	 	  if(setter(iList, AI, I, BB, firstDominatorBlock) && !isAStore0Inst(*I) && !isAStore0Inst(*(I->getNextNode()))){
	 	     addStore0(*I);
		  }
	       }
	    }
	    I = I->getPrevNode();
	 }
      }
   }

   /**
    * @function setter:
    * modifies the boolean status (if necessary) and returns information over it
    * @param iList, the Variable with their status in each block
    * @param AI, the Allocation Instruction which represents the variable
    * @param I, the current instruction
    * @param BB, the current BasicBlock
    * @param firstDominatorBlock, the first Dominator block after BB (or BB if BB is a dominator)
    * @returns true if the variable is dead after instruction I and is to be set at 0
    *
    **/
   bool setter(InstructionList &iList, AllocaInst* AI, Instruction *I, BasicBlock* BB, BasicBlock* firstDominatorBlock){
      for(int k = 0; k < iList[BB].size(); ++k){
	 if(iList[BB][k].first == AI){
	    if(iList[BB][k].second){
	       if(I->getOpcode() == 30){
		  iList[BB][k].second = false;
	       }
	       return true;
	    }
	    else{
	       if(I->getOpcode() == 31){
		  iList[BB][k].second = true;
	       }
	       return false;
	    }
	 }
      }
      if(I->getOpcode() == 30){
	 AllocaInst* AI = dyn_cast<AllocaInst>(I->getOperand(0));
	 IsDead varia;
 	 varia.first = AI;
	 varia.second = false;
	 iList[BB].push_back(varia);
      }
      else{
	 AllocaInst* AI = dyn_cast<AllocaInst>(I->getOperand(1));
	 IsDead varia;
	 varia.first = AI;
	 varia.second = true;
	 iList[BB].push_back(varia);
      }
      if(BB != firstDominatorBlock){
	 addStore0(*I, I->getOperand(I->getNumOperands() - 1), &firstDominatorBlock->front());
      }
      return true;
   }

   /**
    * @function isAStore0Inst:
    * tests if an Instruction is a Store 0 one
    * @param I the instruction to be tested
    * @returns true if the instruction was a Store 0, false elsewhere
    **/
   bool isAStore0Inst(Instruction &I){
      if(I.getOpcode() == 31){
    	 if(Constant *C = dyn_cast<Constant>(I.getOperand(0))){
  	    if(C->isNullValue()){
  	       return true;
  	    }
  	 }
      }
	 }
	 else{
	 AI = dyn_cast<AllocaInst>(I.getOperand(1));
	 }
      }
      ID = AI->getType()->getElementType()->getTypeID();
      switch (ID) {//each case allows to check the type and store 0 type get<Typename>Ty at @operand with volatile=true in order to survive other pass

	 case Type::IntegerTyID:
	    if(I.getOpcode() == 30){
   	       Store0 = Builder.CreateStore(ConstantInt::get(Builder.getIntNTy(cast<IntegerType>(I.getType())->getBitWidth()), 0), V, true);
	    }
	    else{
	       Store0 = Builder.CreateStore(ConstantInt::get(Builder.getIntNTy(cast<IntegerType>(I.getOperand(0)->getType())->getBitWidth()), 0), V, true);
	    }
	    break;
		 
	 case Type::FloatTyID:
	    Store0 = Builder.CreateStore(ConstantFP::get(Builder.getFloatTy(), 0), V, true);
	    break;

	 case Type::DoubleTyID:
	    Store0 = Builder.CreateStore(ConstantFP::get(Builder.getDoubleTy(), 0), V, true);
	    break;

	 case Type::HalfTyID:
	    Store0 = Builder.CreateStore(ConstantFP::get(Builder.getHalfTy(), 0), V, true);
	    break;
		    
	 case Type::FP128TyID:
	    Store0 = Builder.CreateStore(ConstantFP::get(Type::getFP128Ty(Builder.getContext()), 0), V, true);
	    break;
		    
	 case Type::X86_FP80TyID:
	    Store0 = Builder.CreateStore(ConstantFP::get(Type::getX86_FP80Ty(Builder.getContext()), 0), V, true);
	    break;

	 case Type::PPC_FP128TyID:
	    Store0 = Builder.CreateStore(ConstantFP::get(Type::getPPC_FP128Ty(Builder.getContext()), 0), V, true);
	    break;

	 case Type::X86_MMXTyID:
	    Store0 = Builder.CreateStore(ConstantFP::get(Type::getX86_MMXTy(Builder.getContext()), 0), V, true);
	    break;

	 case Type::PointerTyID:
	    Store0 = Builder.CreateStore(Constant::getNullValue(cast<PointerType>(AI->getType()->getElementType())), AI, true);
	    break;

	 case Type::ArrayTyID:
	    ArrayType* array = dyn_cast<ArrayType>(AI->getType()->getElementType());
	    Store0 = Builder.CreateStore(Constant::getNullValue(array), AI, true);
	    break;
      }
      if(Store0 == nullptr){
	 errs() << "can't do my stuff, type too complex\n";
      }
      else{
	 if(I.getDebugLoc()){
	    errs() << "adding STORE 0 (after)\t\t\t";
	    I.getDebugLoc().print(errs());
	    errs() << "\n";
	 }
      }
      numSTORE0ADDED++;
   }

   /**
    * This function is used to display usefull information in order to sum up what have been done by our pass
    * @function doFinalization the last function executed by our pass
    * @param the current module
    * @returns false because the code wasn't modified during its execution
    **/
   bool doFinalization(Module &M) override{
      errs() << "\n(Information (if displayed) are in file:line:column mode)\n\n";
      errs() << "\033[0;36m======================================\033[0;0m\n";
      errs() << "\033[0;36m=======   TRACKER STATISTICS   =======\033[0;0m\n";
      errs() << "\033[0;36m======================================\033[0;0m\n";
      errs() << "\033[0;32m Added " << numSTORE0ADDED << " STORE 0 Instruction\033[0;0m\n";
      return false;
   }


 };
}

char PutAtZero::ID = 0;
int PutAtZero::numSTORE0ADDED = 0;
static RegisterPass<PutAtZero> X("PaZ", "PutAtZero Pass");
