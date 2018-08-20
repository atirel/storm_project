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



#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/Analysis/LoopInfoImpl.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Analysis/PostDominators.h"

using namespace llvm;

namespace {
 struct DeadVariableHandler : public FunctionPass {

   static char ID;
   static int numSTORE0ADDED;//The number of instruction STORE 0 added

   DeadVariableHandler() : FunctionPass(ID) {}
   bool runOnFunction(Function &F) override {
      SmallVector <Instruction*,16> storage;
      SmallVector <BasicBlock*, 8> blocksInLoop;
      SmallVector <Instruction*, 8> references;
      SmallVector <Instruction*, 8> arrayHandler;
      LoopInfo& loopData = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
      for(BasicBlock &BB : F){
	 if(loopData.isLoopHeader(&BB)){
	    blocksInLoop.push_back(&BB);
	 }
      }
      int storageSize = 0;
      bool isFirstBlock = false;//is the current basic block the first one?
      Instruction* I = F.back().getTerminator();//I is the last instruction of the function
      Function::iterator itBlock = --F.end();
      DominatorTree& DT = getAnalysis<DominatorTreeWrapperPass>().getDomTree();
      DT.print(errs());
/*      for(auto node = GraphTraits<DominatorTree *>::nodes_begin(DT); node != GraphTraits<DominatorTree *>::nodes_end(DT); ++node){
	 BasicBlock *BB = node->getBlock();
	 errs() << *BB << "\n";
      }*/
      while(itBlock != F.begin() || !isFirstBlock){
   	 BasicBlock* BB = dyn_cast<BasicBlock>(itBlock);
   	 I = BB->getTerminator();
   	 while(I != nullptr){
   	    if(I->getOpcode() == 30 || I->getOpcode() == 31){
      	       if(AllocaInst *AI = dyn_cast<AllocaInst>(I->getOperand(I->getNumOperands() - 1))){
		  if(I->getOpcode() == 31 && I->getOperand(0)->getType()->getTypeID() == Type::PointerTyID){
		     references.push_back(I);
		  }
   		  storage.push_back(I);
   		  storageSize++;
	       }
   	    }
	    if(I->getOpcode() == 32){
	       errs() << I->getNumOperands() << "\n";
	       arrayHandler.push_back(I);
	    }
   	    I = I->getPrevNode();	 //'new' I is now the instruction right before 'old' I.
	 }
       	 if(itBlock == F.begin()){
    	    isFirstBlock = true;//since we move from the last to the first, we need to know when we reach the beginning
    	 }
	 else{
	    itBlock--;//goes to the previous block
	 }
      }
      int cpt = 0;
      Instruction* currentInst = nullptr;
      if(storageSize > 0){
   	 currentInst = storage[cpt];
      }
      while(currentInst != nullptr){//Look in all the instruction
 	 int cpt_bis = 0;
	 bool isEverUsedAfter = false;//is the variable used, at least once after this instruction
	 bool locked = false;
	 bool loadedInHeader = false;//is the variable loaded in a loop header
	 if(isLoadedInHeader(blocksInLoop, currentInst)){//if the variable is loaded in a header, we cannot put its value to 0 due to the return to the conditional point
	    loadedInHeader = true;
	    isEverUsedAfter = true;//hence, we can say that the variable might be reused if the loop condition is true which can't be determined at compile time
	 }
	 while(cpt_bis < cpt){//for all the store/load instruction after the current one
	    if(isLocked(references, currentInst->getOperand(currentInst->getNumOperands() - 1))){
	       isEverUsedAfter = true;
	       locked = true;
	       break;
	    }
	    if(storage[cpt_bis]->getOperand(storage[cpt_bis]->getNumOperands() - 1) == currentInst->getOperand(currentInst->getNumOperands() - 1)){
	       //if the register is the same
	       isEverUsedAfter = true;//the variable is reused
	       if(storage[cpt_bis]->getOpcode() == 31 && storage[cpt_bis]->getParent() == currentInst->getParent() && !isAStore0Inst(*currentInst)){
		  //if the value is overwritten, we can put it to 0 just after the first write
		  //technically, dse asserts that we cannot enter into this state
		  addStore0(*currentInst);
		  break;
	       }
	       if(storage[cpt_bis]->getOpcode() == 31 && !loadedInHeader){
		  isEverUsedAfter = false;//if the value is rewritten in another block then we can suppose that it will never be used
	       }
	       if(storage[cpt_bis]->getOpcode() == 30){//if the value is loaded
		  SmallVector<BasicBlock*, 8> futureStack;
		  if(isInSuccessor(*currentInst->getParent(), *storage[cpt_bis]->getParent(), futureStack)){//in a block accessible from the current one
		     break;//we shall not pass !
		  }
		  isEverUsedAfter = false;//elsewhere the value in itself is not reused since the block where it will be loaded is not reachable from the block we are currently visiting
	       }
	    }
	    cpt_bis++;
	 }
	 if(!isEverUsedAfter && !isAtZeroInTheBlock(*currentInst->getParent(), currentInst->getOperand(currentInst->getNumOperands() - 1))){//if the variable is not used anymore and is not already at 0
	    addStore0(*currentInst);
	 }
	 if(!locked && !isAtZeroInTheBlock(F.back(), currentInst->getOperand(currentInst->getNumOperands() - 1))){//whatever the case is, we put the value at 0 at the end of the program to make sure that it is really erased once and for all
   	    addStore0(*currentInst, currentInst->getOperand(currentInst->getNumOperands() - 1), F.back().getTerminator());
	 }
	 ++cpt;
	 if(cpt < storageSize){
	    currentInst = getUnlockedInstruction(references, currentInst);
	    if(currentInst  == nullptr){
   	       currentInst = storage[cpt];
	    }
	 }
	 else{
	    currentInst = nullptr;
	 }
      }
      return true;
   }

   virtual void getAnalysisUsage(AnalysisUsage& AU) const override {
      AU.addRequired<LoopInfoWrapperPass>();
      AU.addRequired<DominatorTreeWrapperPass>();
   }
  
   Instruction* getUnlockedInstruction(SmallVector<Instruction*, 8>&references, Instruction *currentInst){
      return nullptr;
   }
   void declareAllRefDead(SmallVector<Instruction*, 8> &references, Value* V, Instruction &Iplace){
      int cpt = references.end() - references.begin();
      bool isAtZero = false;
      while(cpt){
	 if(references[cpt-1]->getOperand(1) == V){
	    if(!isAtZero){
	       isAtZero = true;
	       Instruction* INP = dyn_cast<Instruction>(references[cpt-1]->getOperand(0));
	       if(INP != nullptr){
		  addStore0(*INP, references[cpt-1]->getOperand(0), &Iplace);
	       }
	    }
	    references.erase(references.begin() + (cpt-1));
	 }
	 cpt--;
      }
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
    * @function isInSmallBasicBlockVect:
    * tests if the basicBlock in parameter is in the SmallVector in param
    * @param SVBB the SmallVector containing some BasicBlocks
    * @param BB a BasicBlock
    * @returns true if BB is in SVBB, false elsewhere
    **/
   bool isInSmallBasicBlockVect(SmallVector<BasicBlock*, 8> &SVBB, BasicBlock &BB){
      int size = SVBB.end() - SVBB.begin() - 1;
      while(size >= 0){
	 if(SVBB[size] == &BB){
	    return true;
	 }
	 size--;
      }
      return false;
   }

   /**
    * @function isAtZeroInTheBlock:
    * checks if the Value V is put at 0 at the end of the block, if there is any other instruction touching to V, it supposes that it does not store a 0
    * @param BB the BasicBlock where our test is run
    * @param V the Value supposed to be put at 0
    * @precond V is matching a register, if this precond is false, the function will always return false
    * @returns true if the last instruction on V in BB is a Store 0 => if V is put at 0 at the end of the block
    **/
   bool isAtZeroInTheBlock(BasicBlock &BB, Value* V){
      Instruction* I = BB.getTerminator();
      while(I != nullptr){
  	 if(I->getNumOperands() != 0 && I->getOperand(I->getNumOperands() - 1) == V){
  	    if(isAStore0Inst(*I)){
  	       return true;
  	    }
  	    return false;
    	 }
     	 I = I->getPrevNode();
      }
      return false;
   }

   /**
    * @function isLoadedInHeader:
    * checks if the value loaded/stored in the Instruction is ever loaded in a loop Header
    * @precond HeaderBBlocks contains only loop headers
    * @precond Ins is either a store or a load instruction
    * @param HeaderBBblocks a SmallVector containing all the headers of all the loops in the function
    * @param Ins, the instruction accessing the value
    * @returns true, if the variable accessed by Ins is loaded in any header, false elsewhere
    **/
   bool isLoadedInHeader(SmallVector<BasicBlock*,8> HeaderBBlocks, Instruction *Ins){
      BasicBlock *BB = Ins->getParent();
      while(BB != nullptr){
	 if(isInSmallBasicBlockVect(HeaderBBlocks, *BB)){
	    for(Instruction &I : *BB){
	       if((I.getOpcode() == 30 || I.getOpcode() == 31) && I.getOperand(0) == Ins->getOperand(Ins->getNumOperands() - 1)){
		  return true;
	       }
	    }
	 }
	 BB = BB->getPrevNode();
      }
      return false;
   }

   /**
    * @function isInSuccessor:
    * checks if a BasicBlock can be reached from another, if he is its successor, or a successor of its successors...
    * @param BBPrec, the supposed predecessor
    * @param BBSucc, the successor
    * @param SVBB, a SmallVector with all the BasicBlocks already visited
    * @param reval, the default return value
    * @returns true, if their is a way from BBPrec to BBSucc (or even if it's the same block), false elsewhere
    **/
   bool isInSuccessor(BasicBlock &BBPrec, BasicBlock &BBSucc, SmallVector<BasicBlock*, 8> &SVBB, bool retval = false){
      if(&BBPrec == &BBSucc){
	 return true;
      }
      if(BranchInst * BINP = dyn_cast<BranchInst>(BBPrec.getTerminator())){
	 int nSucc = BINP->getNumSuccessors();
	 while(nSucc){
	    if(!isInSmallBasicBlockVect(SVBB, *BINP->getSuccessor(nSucc - 1))){
	       SVBB.push_back(BINP->getSuccessor(nSucc - 1));
	       retval = retval || isInSuccessor(*BINP->getSuccessor(nSucc - 1), BBSucc, SVBB);
	    }
	    nSucc--;
	 }
      }
      else{
	 return retval;
      }
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
      IRBuilder<> Builder(NextI);
      StoreInst* Store0 =  nullptr;
      int ID = 0;
      if(I.getOpcode() == 30){
	 ID = I.getType()->getTypeID();
      }
      else{
	 ID = I.getOperand(0)->getType()->getTypeID();
      }
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

	 case Type::VectorTyID:
	    //VectorType* vect = dyn_cast<VectorType>(I.getType());
	    //Store0 = Builder.CreateStore(Constant::getNullValue(vect), V, true);
	    break;

	 case Type::ArrayTyID:
	    ArrayType* array = dyn_cast<ArrayType>(I.getType());
	    Store0 = Builder.CreateStore(Constant::getNullValue(array), V, true);
	    break;
      }
      if(Store0 == nullptr){
	 errs() << "can't do my stuff\n";
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
      errs() << "\n(Information are displayed in file:line:column mode)\n\n";
      errs() << "\033[0;36m======================================\033[0;0m\n";
      errs() << "\033[0;36m=======   TRACKER STATISTICS   =======\033[0;0m\n";
      errs() << "\033[0;36m======================================\033[0;0m\n";
      errs() << "\033[0;32m Added " << numSTORE0ADDED << " STORE 0 Instruction\033[0;0m\n";
      return false;
   }

   /**
    * @function hasReferences:
    * checks if the Value has a reference e.g if the value was stored in a pointer to be used in another way after
    * @precond ref only contains store instructions of references
    * @param ref a small vector with all the references of the function
    * @param V the value pointing to the current variable we're handling
    * @returns true if V is a pointer over a variable, false elsewhere
    **/
   bool isReference(SmallVector<Instruction*, 8> &ref, Value* V){
      int cpt = ref.end() - ref.begin();
      while(cpt){
	 if(ref[cpt-1]->getOperand(1) == V){
	    return true;
	 }
	 cpt--;
      }
      return false;
   }

   bool isLocked(SmallVector<Instruction*, 8> references, Value *V){
      int cpt = references.end() - references.begin() - 1;
      while(cpt >= 0){
	 if(references[cpt]->getOperand(0) == V){
	    return true;
	 }
	 cpt--;
      }
      return false;
   }
   };
 }

char DeadVariableHandler::ID = 0;
int DeadVariableHandler::numSTORE0ADDED = 0;
static RegisterPass<DeadVariableHandler> X("DVH", "DeadVariableHandler Pass");
