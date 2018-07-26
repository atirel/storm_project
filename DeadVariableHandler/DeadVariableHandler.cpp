/**
 * This LLVM pass adds a STORE 0 %add after a load followed by a store in order to check with their values wether or not 2 variables are semantically equivalent
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
#include "llvm/ADT/SmallVector.h"
#include "llvm/Analysis/LoopInfo.h"

using namespace llvm;

namespace {
 struct DeadVariableHandler : public FunctionPass {

   static char ID;
   static int numSTORE0ADDED;//The number of instruction STORE 0 added

   DeadVariableHandler() : FunctionPass(ID) {}
   bool runOnFunction(Function &F) override {
      SmallVector <Instruction*,16> storage;
      SmallVector <BasicBlock*, 8> blocksInLoop;
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
      while(itBlock != F.begin() || !isFirstBlock){
   	 BasicBlock* BB = dyn_cast<BasicBlock>(itBlock);
   	 I = BB->getTerminator();
   	 while(I != nullptr){
   	    if(I->getOpcode() == 30 || I->getOpcode() == 31){
      	       if(AllocaInst *AI = dyn_cast<AllocaInst>(I->getOperand(I->getNumOperands() - 1))){
   		  storage.push_back(I);
   		  storageSize++;
	       }
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
      while(cpt <= storageSize - 1){//Look in all the instruction
	 int cpt_bis = 0;
	 bool isEverUsedAfter = false;//is the variable used, at least once after this instruction
	 bool loadedInHeader = false;//is the variable loaded in a loop header
	 if(isLoadedInHeader(blocksInLoop, storage[cpt])){//if the variable is loaded in a header, we cannot put its value to 0 due to the return to the conditional point
	    loadedInHeader = true;
	    isEverUsedAfter = true;//hence, we can say that the variable might be reused if the loop condition is true which can't be determined at compile time
	 }
	 while(cpt_bis < cpt){ 
	    if(storage[cpt_bis]->getOperand(storage[cpt_bis]->getNumOperands() - 1) == storage[cpt]->getOperand(storage[cpt]->getNumOperands() - 1)){
	       isEverUsedAfter = true;
	       if(storage[cpt_bis]->getOpcode() == 31 && storage[cpt_bis]->getParent() == storage[cpt]->getParent() && !isAStore0Inst(*storage[cpt])){
		  addStore0(*storage[cpt]);
		  break;
	       }
	       if(storage[cpt_bis]->getOpcode() == 31 && !loadedInHeader){
		  isEverUsedAfter = false;
	       }
	       if(storage[cpt_bis]->getOpcode() == 30){
		  break;
	       }
	    }
	    cpt_bis++;
	 }
	 if(!isEverUsedAfter && !isAtZeroInTheBlock(*storage[cpt]->getParent(), storage[cpt]->getOperand(storage[cpt]->getNumOperands() - 1))){
	    addStore0(*storage[cpt]);
	 }
	 if(!isAtZeroInTheBlock(F.back(), storage[cpt]->getOperand(storage[cpt]->getNumOperands() - 1))){
   	    addStore0(*storage[cpt], storage[cpt]->getOperand(storage[cpt]->getNumOperands() - 1), F.back().getTerminator());
	 }
	 
	 cpt++;
      }
   }

   virtual void getAnalysisUsage(AnalysisUsage& AU) const override {
      AU.addRequired<LoopInfoWrapperPass>();
   }
  
   bool isAStore0Inst(Instruction &I){
      if(I.getOpcode() == 31){
    	 if(Constant *C = dyn_cast<Constant>(I.getOperand(0))){
  	    if(C->isNullValue()){
  	       return true;
  	    }
  	 }
      }
   }

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

   bool isLoadedInHeader(SmallVector<BasicBlock*,8> HeaderBBlocks, Instruction *Ins){
      BasicBlock *BB = Ins->getParent();
      while(BB != nullptr){
	 if(isInSmallBasicBlockVect(HeaderBBlocks, *BB)){
	    for(Instruction &I : *BB){
	       if(I.getOpcode() == 30 && I.getOperand(0) == Ins->getOperand(Ins->getNumOperands() - 1)){
		  return true;
	       }
	    }
	 }
	 BB = BB->getPrevNode();
      }
      return false;
   }

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
	    Store0 = Builder.CreateStore(Constant::getNullValue(cast<PointerType>(I.getType())), V, true);
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

   };
 }

char DeadVariableHandler::ID = 0;
int DeadVariableHandler::numSTORE0ADDED = 0;
static RegisterPass<DeadVariableHandler> X("DVH", "DeadVariableHandler Pass");
