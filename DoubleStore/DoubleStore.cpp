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

   bool runOnModule(Module &M) override {
      for(Function &F : M){
         for(BasicBlock &B : F){
	    SmallVector<Instruction*, 64> storage;
            for(Instruction &I : B){
	       update_storage(storage, I);
	    }
	    int cpt = storage.end() - storage.begin() - 1;
	    errs() << "\n\n\n";
	    while(cpt >= 0){
     	       if(storage[cpt]->getOpcode() == 31 || storage[cpt]->getOpcode() == 30){
		  addLastStore(storage, *storage[cpt], cpt);
	       }
	       cpt--;
	    }
  	 }
      }
      return true;
   }

   bool doFinalization(Module &M) override{
      errs() << "\n(Information are displayed in file:line:column mode)\n\n";
      errs() << "\033[0;36m======================================\033[0;0m\n";
      errs() << "\033[0;36m=======   TRACKER STATISTICS   =======\033[0;0m\n";
      errs() << "\033[0;36m======================================\033[0;0m\n";
      errs() << "\033[0;32m Added " << numSTORE0ADDED << " STORE 0 Instruction\033[0;0m\n";
      errs() << "\033[0;31m Removed " << numSTOREDELETED << " useless STORE\033[0;0m\n";
      return false;
   }

   StoreInst* addStore0(SmallVector<Instruction*, 64> &storage, int i, Value* operand){
      DebugLoc Dloc = storage[i]->getDebugLoc();
      IRBuilder<> Builder(storage[i+1]);
      StoreInst* Store0 =  nullptr;
      int ID = 0;
      if(storage[i]->getOpcode() == 30){
	 ID = storage[i]->getType()->getTypeID();
      }
      else{
	 ID = storage[i]->getOperand(0)->getType()->getTypeID();
      }
      switch (ID) {

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
/*
		    case Type::PointerTyID:
		       Store0 = Builder.CreateStore(ConstantPointerNull::getType(Type::getIntN(cast<PointerType>(storage[i]->getType())->getBitWidth()), 0), operand, true);
		       break;

		    case Type::VectorTyID:
		       Store0 = Builder.CreateStore(ConstantDataVector::get(Type::getVectorTy(Builder.getContext()), 0), operand, true);
		       break;*/
      }

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
		 storage[i]->eraseFromParent();
		 StoreInst *SI = dyn_cast< StoreInst >(storage[i]);
		 errs() << "erasing instruction\t\t\t";
		 storage[i]->getDebugLoc().print(errs());
 		 errs() << "\n";
		 storage.erase(storage.begin()+i);
		 numSTOREDELETED++;
		 return;
	      }
	      if(storage[i]->getOpcode() == 30){//30 stands for load
		 StoreInst* Store0 = addStore0(storage, i, operand);
		 storage.insert(storage.begin()+i+1, Store0);
		 return;
	      }
	   }
	   --i;
	 }
      }
   }
   //TODO create a STORE0 func and call it to check whether or not the previous instruction was a store 0 with the same type
   void addLastStore(SmallVector<Instruction*, 64> &storage, Instruction &I, int k){
      int cpt = storage.end() - storage.begin() - 1;
      Value* operand = I.getOperand(I.getNumOperands() - 1);
      while(cpt >= k){
	 if(operand == storage[cpt]->getOperand(storage[cpt]->getNumOperands()-1) && &I != storage[cpt]){//if the adress where the value is stored/load is the same
	    return;
	 }
	 
	 cpt--;
      }
      if(Constant *C = dyn_cast<Constant>(I.getOperand(0))){
	 if(C->isNullValue()){
	    errs() << *C << "\n";
	    return;
	 }
      }
      if(Value* V = dyn_cast<Value>(I.getOperand(0))){
 	 StoreInst* Store0 = addStore0(storage, k, operand);
	 storage.push_back(Store0);
      }
   }


 };
}

char DoubleStoreInstr::ID = 0;
int DoubleStoreInstr::numSTORE0ADDED = 0;
int DoubleStoreInstr::numSTOREDELETED = 0;
static RegisterPass<DoubleStoreInstr> X("DoubleStore", "DoubleStore Pass");
