/**
 * This LLVM pass deletes useless stores e.g store which are followed by another store
 * It also adds a STORE 0 %add after a load followed by a store in order to check with their values wether or not 2 variables are semantically equivalent
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



#include "llvm/Pass.h"
#include "llvm-c/Core.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Value.h"
#include "llvm/ADT/SmallVector.h"
#include <cstring>
#include <stdio.h>
#include <vector>

using namespace llvm;

namespace {
 struct DoubleStoreInstr : public ModulePass {

   static char ID;
   static int numSTORE0ADDED;//The number of instruction STORE 0 added
   static int numSTOREDELETED;//The number of useless STORE removed

   DoubleStoreInstr() : ModulePass(ID) {}
/**
 * @function runOnModule override
 * should also work with runOnFunction
 * @param Module M, the current module
 * @returns true since the code was modified in order to add the store 0 instructions
 **/

   bool runOnModule(Module &M) override {
      for(Function &F : M){
	 errs() << "\n";
         for(BasicBlock &B : F){
	    SmallVector<Instruction*, 64> storage;
            for(Instruction &I : B){
	       update_storage(storage, I);
	    }
	    int cpt = storage.end() - storage.begin() - 1;
	    while(cpt >= 0){
	       //getelementpointerinbounds <~> load:opcode = 32
	       if(IntrinsicInst* II = dyn_cast<IntrinsicInst>(storage[cpt])){
		  errs() << *II->getOperand(1) << "\n";
		  if(User* Us = dyn_cast<User>(II->getOperand(1))){
		     errs() << *Us->getOperand(0) << "\n";
		  }
	       }
     	       if(storage[cpt]->getOpcode() == 31 || storage[cpt]->getOpcode() == 30){
		  addLastStore(storage, *storage[cpt], cpt);
	       }
	       cpt--;
	    }
  	 }
      }
      return true;
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
      errs() << "\033[0;31m Removed " << numSTOREDELETED << " useless STORE\033[0;0m\n";
      return false;
   }

   /**
    * This function adds a store0 instruction after the instruction storage[i], it needs the operand in order to know where to store the value is to be stored
    * @param storage, the current vector filled with the instructions already handled
    * @param i the index of the instruction where our store 0 is to be placed
    * @param operand, the address where we need to store our value
    * @return the new instruction, in order to ve added to the storage vector and for debug purposes
    **/ 
   StoreInst* addStore0(SmallVector<Instruction*, 64> &storage, int i, Value* operand, int place){
      DebugLoc Dloc = storage[i]->getDebugLoc();
      IRBuilder<> Builder(storage[place]);
      StoreInst* Store0 =  nullptr;
      int ID = 0;
      if(storage[i]->getOpcode() == 30){
	 ID = storage[i]->getType()->getTypeID();
      }
      else{
	 ID = storage[i]->getOperand(0)->getType()->getTypeID();
      }
      switch (ID) {//each case allows to check the type and store 0 type get<Typename>Ty at @operand with volatile=true in order to survive other pass

	 case Type::IntegerTyID:
	    if(storage[i]->getOpcode() == 30){
   	       Store0 = Builder.CreateStore(ConstantInt::get(Builder.getIntNTy(cast<IntegerType>(storage[i]->getType())->getBitWidth()), 0), operand, true);
	    }
	    else{ 
   	       Store0 = Builder.CreateStore(ConstantInt::get(Builder.getIntNTy(cast<IntegerType>(storage[i]->getOperand(0)->getType())->getBitWidth()), 0), operand, true);
	    }
	    break;
		 
	 case Type::FloatTyID:
	    Store0 = Builder.CreateStore(ConstantFP::get(Builder.getFloatTy(), 0), operand, true);
	    break;

	 case Type::DoubleTyID:
	    Store0 = Builder.CreateStore(ConstantFP::get(Builder.getDoubleTy(), 0), operand, true);
	    break;

	 case Type::HalfTyID:
	    Store0 = Builder.CreateStore(ConstantFP::get(Builder.getHalfTy(), 0), operand, true);
	    break;
		    
	 case Type::FP128TyID:
	    Store0 = Builder.CreateStore(ConstantFP::get(Type::getFP128Ty(Builder.getContext()), 0), operand, true);
	    break;
		    
	 case Type::X86_FP80TyID:
	    Store0 = Builder.CreateStore(ConstantFP::get(Type::getX86_FP80Ty(Builder.getContext()), 0), operand, true);
	    break;

	 case Type::PPC_FP128TyID:
	    Store0 = Builder.CreateStore(ConstantFP::get(Type::getPPC_FP128Ty(Builder.getContext()), 0), operand, true);
	    break;

	 case Type::X86_MMXTyID:
	    Store0 = Builder.CreateStore(ConstantFP::get(Type::getX86_MMXTy(Builder.getContext()), 0), operand, true);
	    break;

	 case Type::PointerTyID:
	    if(StoreInst* SI = dyn_cast<StoreInst>(storage[i])){
   	       Store0 = Builder.CreateStore(Constant::getNullValue(cast<PointerType>(SI->getOperand(0)->getType())), operand, true);
	    }
	    else{
	       LoadInst* LI = dyn_cast<LoadInst>(storage[i]);
   	       Store0 = Builder.CreateStore(Constant::getNullValue(cast<PointerType>(LI->getType())), operand, true);
	    }
	    break;
/*
	 case Type::VectorTyID:
	    Store0 = Builder.CreateStore(ConstantDataVector::get(Type::getVectorTy(Builder.getContext()), 0), operand, true);
	    break;
  */    }

      if(Store0 == nullptr){
	 errs() << "can't do my stuff\n";
      }
      else{
	 errs() << "adding STORE 0 (after)\t\t\t";
	 Dloc.print(errs());
	 errs() << "\n";
      }
      numSTORE0ADDED++;
      return Store0;
   }
   /**
    * This function updates the vectore storage, calling if necessary the Store0 function and deleting pointless store instructions
    * @param storage, a vector with all the instructions before the current one in the block
    * @param I, the current instruction
    * @returns nothing, but the instruction is added to storage and useless stores are removed, if necessary, a store 0 instruction is added at the right place
    **/
   void update_storage(SmallVector<Instruction*, 64>& storage, Instruction &I){
      int size = storage.end() - storage.begin();
      unsigned int Ioperands = I.getNumOperands();
      Value* operand = I.getOperand(Ioperands - 1);
      int i = size - 1;
      storage.push_back(&I); 
      if(I.getOpcode() == 31){
      	 while(i >= 0){
           if(operand == storage[i]->getOperand(storage[i]->getNumOperands()-1)){//if the adress where the value is stored/load is the same
	      if(storage[i]->getOpcode() == 31){//31 stands for store
		 StoreInst *SI = dyn_cast< StoreInst >(storage[i]);
		 errs() << "erasing instruction\t\t\t";
		 storage[i]->getDebugLoc().print(errs());
 		 errs() << "\n";
		 storage[i]->eraseFromParent();
		 storage.erase(storage.begin()+i);	 
		 numSTOREDELETED++;
		 return;
	      }
	      if(storage[i]->getOpcode() == 30){//30 stands for load
		 StoreInst* Store0 = addStore0(storage, i, operand, i+1);
		 storage.insert(storage.begin()+i+1, Store0);
		 return;
	      }
	   }
	   --i;
	 }
      }
      //TODO check wether or not it is usefull to store 0 in a global variable after all use
      else{
	 if(I.getOpcode() == 30){
	    if(GlobalVariable* GV = dyn_cast<GlobalVariable>(operand)){
	       if(GV->hasInitializer()){//if the load variable is global and initialized, there is no need to initialize it
		  return;
	       }
	    }
	    if(Instruction* Inst = dyn_cast<Instruction>(operand)){
	       if(Inst->getOpcode() == 32){//the load's parent is a getelementptr
		  errs() << *Inst->getOperand(0) << "\n";
		  Value* getElementPtrOperand = Inst->getOperand(0);
		  //TODO extract element 0 in order to check if there is an instruction on it which could possibly initialize it
   		  while( i >= 0){
   		     if(storage[i]->getOpcode() == 31){
			if(Instruction* previousInst = dyn_cast<Instruction>(storage[i]->getOperand(1))){
			   if(previousInst->getOpcode() == 32 && previousInst->getOperand(0) == Inst->getOperand(0) && previousInst->getOperand(1) == Inst->getOperand(1) && previousInst->getOperand(2) == Inst->getOperand(2)){
			      //if there's a write (opcode 31) whith a getelementptr (opcode 32) with all its parameters in common with the load's one
			      return;//then the variable was indeed initialized
			   }
			}
		     }
		     else{
			if(storage[i]->getOperand(0) == getElementPtrOperand){
			   errs() << *storage[i] << "\n";
			}
		     }
		     i--;
		  }
	       }
	    }
	    i = size - 1;
	    while(i >= 0){
	       if(operand == storage[i]->getOperand(storage[i]->getNumOperands() - 1) && storage[i]->getOpcode() == 31){//If the variable is initialized
		  return;//do nothing
	       }
	       --i;
	    }
	    StoreInst* SI0 = addStore0(storage, size, operand, size);
	    storage.insert(storage.begin()+size, SI0);
	 }
      }
   }
   /**
    * This function adds Store0 instructions at the end of compilation:
    * If a variable is no longer of any use, we put its value to 0
    * @param storage, a vector with all the instructions of the block
    * @param I, the Instruction handled in order to check whether or not its the last one
    * @param k, an index to reduce the length manipulated
    * @returns nothing, but adds store 0 instruction for each variable which was of any use
    **/
   void addLastStore(SmallVector<Instruction*, 64> &storage, Instruction &I, int k){
      int cpt = storage.end() - storage.begin() - 1;
      Value* operand = I.getOperand(I.getNumOperands() - 1);
      if(Instruction* Inst = dyn_cast<Instruction>(operand)){
	 if(I.getOpcode() == 31 && Inst->getOpcode() == 32){//if the instruction is a store in a get elementptr addr
	    while(cpt >= k){//look in all the next instructions
	       if(storage[cpt]->getOpcode() == 30){
 		  if(Instruction* futureInst = dyn_cast<Instruction>(storage[cpt]->getOperand(0))){//if there is a load of a getelementptr addr
		     if(futureInst->getOpcode() == 32 && futureInst->getOperand(0) == Inst->getOperand(0) && futureInst->getOperand(1) == Inst->getOperand(1) && futureInst->getOperand(2) == Inst->getOperand(2)){//and the address is the same
 			return;//then no use to add a store 0 instruction cause it will be added after the matching load
			//Store Inst* Store0 = addStore0(storage, cpt, operand cpt+1) technically useless
   		     }
   		  }
	       }
	       cpt--;
	    }
	 }
      }
      cpt = storage.end() - storage.begin() - 1;
      while(cpt >= k){
	 if(operand == storage[cpt]->getOperand(storage[cpt]->getNumOperands()-1) && &I != storage[cpt]){//if the adress where the value is stored/load is the same and the instruction is not itself
	    return;
	 }
	 cpt--;
      }
      if(Constant *C = dyn_cast<Constant>(I.getOperand(0))){
	 if(C->isNullValue()){//Checks if the previous instruction was a store 0
	    return;
	 }
      }
      if(Value* V = dyn_cast<Value>(I.getOperand(0))){//asserts that the Constant cast is anihilated
 	 StoreInst* Store0 = addStore0(storage, k, operand, k+1);
	 if (Store0 != nullptr){
   	    storage.push_back(Store0);
	 }
      }
   }


 };
}

char DoubleStoreInstr::ID = 0;
int DoubleStoreInstr::numSTORE0ADDED = 0;
int DoubleStoreInstr::numSTOREDELETED = 0;
static RegisterPass<DoubleStoreInstr> X("DoubleStore", "DoubleStore Pass");
