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

   PutAtZero() : FunctionPass(ID) {}

   bool runOnFunction(Function &F) override {
      LoopInfo& loopData = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();

      BlockList blockList;//The blocks with their colors
      InstructionList iList;//The variables and their booleans (dead or not) for each block
      
      for(BasicBlock &BB : F){//we put all the blocks to white since there is no block treated
	 blockList[&BB] = white;
      }

      std::list<BasicBlock*> toTreat;
      toTreat.push_back(&F.back());
      
      if(loopData.isLoopHeader(&F.back())){
	 errs() << "can't do anything, fix your code, there shall not be any way to reach a dead end \n";
	 return false;
      }


      while(!toTreat.empty()){//while there is an untreated block,
	 BasicBlock* header = toTreat.front();
	 Instruction* I = header->getTerminator();
	 
	 if(loopData.isLoopHeader(header)){
	    Loop* theLoop = loopData.getLoopFor(header);
	    loop_handler(iList, blockList, theLoop);
  	    for(auto it = pred_begin(header), it_end = pred_end(header); it != it_end; ++it){
	       if(!theLoop->contains(*it)){
		  iList[(*it)] = iList[header];
	  	  toTreat.push_back(*it);
	       }
	    }
	    blockList[header] = black;
	 }


	 else{
	    for(auto itBlock = succ_begin(header), itBlock_end = succ_end(header); itBlock != itBlock_end; itBlock++){
	       if(blockList[*itBlock] == white && !hasAssertFail(*itBlock)){
		  toTreat.push_back(header);
		  I = nullptr;
		  break;
	       }
	    }
   	    if(I != nullptr){
	       setD(iList, header, false);
	    }
	    for(auto it = pred_begin(header), et = pred_end(header); it != et; ++it){
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
      array_handler(F);
      return true;
   }

   bool hasAssertFail(BasicBlock* BB){
      Instruction* I = BB->getTerminator();
      while(I != nullptr){
	 if(UnreachableInst* CI = dyn_cast<UnreachableInst>(I)){
	    return true;
	 }
	 I = I->getPrevNode();
      }
      return false;
   }

   void array_handler(Function& F){
      std::vector<BasicBlock*> dominator_blocks;
      std::vector<AllocaInst*> atZeroArrays;
      DominatorTree& DT = getAnalysis<DominatorTreeWrapperPass>().getDomTree();
      BasicBlock* BB = &F.back();
      dominator_blocks.push_back(BB);
      while(BB != nullptr){
	 if(BasicBlock* B_new = BB->getSinglePredecessor()){
	    dominator_blocks.push_back(B_new);
	    BB = B_new;
	 }
	 else{
   	    auto it = pred_begin(BB), it_end = pred_end(BB);
	    if(it != it_end){
   	       B_new = *it;
	    }
   	    while(it != it_end){
	       B_new = DT.findNearestCommonDominator(B_new, *it);
	       it++;
	    }
	    if(B_new != nullptr){
   	       dominator_blocks.push_back(B_new);
	    }
	    BB = B_new;
	 }
      }

      BB = &F.back();
      while(BB != nullptr){
	 Instruction* I = BB->getTerminator();
	 while(I != nullptr){
	    if(I->getOpcode() == 32){
	       if(AllocaInst* AI = dyn_cast<AllocaInst>(I->getOperand(0))){
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

   bool isInVect(AllocaInst* AI, std::vector<AllocaInst*> &alreadyDeadArray){
      for(auto it = alreadyDeadArray.begin(), end = alreadyDeadArray.end(); it != end; ++it){
	 if(*it == AI){
	    return true;
	 }
      }
   }

   void dead_array(Instruction* I, std::vector<BasicBlock*> &dominators, DominatorTree &DT){
   	 BasicBlock* BB = I->getParent();
	 Instruction* place = I->getNextNode();
	 if(std::find(dominators.begin(), dominators.end(), BB) != dominators.end()){
	    addStore0(*I, I->getOperand(0), place);
	    return;
	 }
   	 for(auto it = dominators.begin(), end = dominators.end(); it != end; ++it){
   	    if(DT.properlyDominates(BB, *it)){
   	       place = (*it)->front().getNextNode();
	    }
	    else{
	       addStore0(*I, I->getOperand(0), place);
	       return;
	    }
	 }
   }

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


   void partial_treat(BasicBlock* BB){//technically only for loop headers only puts at zero
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
   void setD(InstructionList& iList, BasicBlock* BB, bool isLoopHeader){
      if(isLoopHeader){
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
	 	  if(setter(iList, AI, I, BB) && !isAStore0Inst(*I) && !isAStore0Inst(*(I->getNextNode()))){
	 	     addStore0(*I);
		  }
	       }
	    }
	    I = I->getPrevNode();
	 }
      }
   }

   bool setter(InstructionList &iList, AllocaInst* AI, Instruction *I, BasicBlock* BB){
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
	// errs() << "new variable " << *AI;
	 iList[BB].push_back(varia);
	// errs() << iList[BB].size() << "\n";
      }
      else{
	 AllocaInst* AI = dyn_cast<AllocaInst>(I->getOperand(1));
	 IsDead varia;
	 varia.first = AI;
	 varia.second = true;
	// errs() << "new variable " << *AI;
	 iList[BB].push_back(varia);
	// errs() << iList[BB].size() << "\n";
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
      return false;
   }

   /**
    * @function getLastLoad:
    * returns the last load instruction in a given Basic Block
    * @param BB the basic block where the load instruction is to be found
    * @returns the last load instruction of the block or nullptr if none is found
    **/
   Instruction* getLastLoad(BasicBlock &BB){
      for(Instruction* I = BB.getTerminator() ; I != nullptr; I=I->getNextNode()){
	 if(I->getOpcode() == 30){
	    if(AllocaInst *AI = dyn_cast<AllocaInst>(I->getOperand(0))){
	       return I;
	    }
	 }
      }
      return nullptr;
   }


   /**@function addStore0
    * adds a store 0 of the Value V, before the instruction Iplace.
    * in debug mode, it tries to display where in the source code it decided to add this store 0 instruction.
    * @param I, the instruction with all the data we need
    * @param V, the Value containing the type of store 0 we want (default, the value accessed by I)
    * @param Iplace, the instruction before which we want to add a store 0 instruction (default, before the instruction following I)
    * @returns nothing but
    * @postcond a Store 0 instruction was added before Iplace or right after I
    **/
   void addStore0(Instruction &I, Value* V = nullptr, Instruction *Iplace = nullptr){
      Instruction *NextI = Iplace;
      if(NextI == nullptr){
	 NextI = I.getNextNode();
      }
      if(V == nullptr){
   	 V = I.getOperand(I.getNumOperands() - 1);
      }
      if(NextI == nullptr){
	 return;
      }
      AllocaInst* AI = nullptr;
      IRBuilder<> Builder(NextI);
      StoreInst* Store0 =  nullptr;
      int ID = 0;
      if(I.getOpcode() == 30 || I.getOpcode() == 32){
	 AI = dyn_cast<AllocaInst>(I.getOperand(0));
//	 ID = I.getType()->getTypeID();
      }
      else{
	 AI = dyn_cast<AllocaInst>(I.getOperand(1));
//	 ID = I.getOperand(0)->getType()->getTypeID();
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
	    if(I.getOpcode() == 30){
   	       Store0 = Builder.CreateStore(Constant::getNullValue(cast<PointerType>(I.getType())), V, true);
	    }
	    else{
	       Store0 = Builder.CreateStore(Constant::getNullValue(cast<PointerType>(I.getOperand(0)->getType())), V, true);
	    }
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
