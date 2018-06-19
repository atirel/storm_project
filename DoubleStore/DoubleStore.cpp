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
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/LLVMContext.h"
#include <cstring>
#include <stdio.h>
#include <vector>

using namespace llvm;

namespace {
 struct DoubleStoreInstr : public ModulePass {

   static char ID;
   static int numSTORE0ADDED;
   static int numSTOREDELETED;

   DoubleStoreInstr() : ModulePass(ID) {}

   bool runOnModule(Module &M) override {
      Value *lst_opnd_store = NULL;
      for(Function &F : M){
         for(BasicBlock &B : F){
	    SmallVector<Instruction*, 64> storage;
            for(Instruction &I : B){
	       errs() << I << "\n";
	       update_storage(storage, I);
	    }
	    errs() << "\n\n";
	    for(Instruction &I : B){
	       errs() << I << "\n";
	    }
   	 }
      }
      return true;
   }

   bool doFinalization(Module &M) override{
      errs() << "\033[0;36m======================================\033[0;0m\n";
      errs() << "\033[0;36m=======   TRACKER STATISTICS   =======\033[0;0m\n";
      errs() << "\033[0;36m======================================\033[0;0m\n";
      errs() << "\033[0;32m Added " << numSTORE0ADDED << " STORE 0 Instruction\033[0;0m\n";
      errs() << "\033[0;31m Removed " << numSTOREDELETED << " useless STORE\033[0;0m\n";
      return false;
   }

   void update_storage(SmallVector<Instruction*, 64>& storage, Instruction &I){
      int size = storage.end() - storage.begin();
      unsigned int Ioperands = I.getNumOperands();
      Value* operand = I.getOperand(Ioperands - 1);
      int i = size - 1;
      storage.push_back(&I);
      if(I.getType()->isIntegerTy(64)){
	 errs() << "c\n";
      }
      if(I.getOpcode() == 31){
      	 while(i >= 0){
           if(operand == storage[i]->getOperand(storage[i]->getNumOperands()-1)){//if the adress where the value is stored/load is the same
	      if(storage[i]->getOpcode() == 31){//31 stands for store
		 storage[i]->eraseFromParent();
		 storage.erase(storage.begin()+i);
		 storage.push_back(&I);
		 numSTOREDELETED++;
		 return;
	      }
	      if(storage[i]->getOpcode() == 30){//30 stands for load
		 errs() << "\n";
		 IRBuilder<> Builder(storage[i+1]);
		 StoreInst* Store0 = nullptr;
		 
		 if(storage[i]->getType()->isIntegerTy(32)){
		    errs() << "abnormal result\n";
		    Store0 = Builder.CreateStore(ConstantInt::get(Builder.getInt32Ty(), 0), operand, true);
		 }

		 if(storage[i]->getType()->isIntegerTy(64)){
   		    Store0 = Builder.CreateStore(ConstantInt::get(Builder.getInt64Ty(), 0), operand, true);
		 }

		 if(storage[i]->getType()->isFloatTy()){
		    Store0 = Builder.CreateStore(ConstantFP::get(Builder.getFloatTy(), 0), operand, true);
		 }
 		 if(Store0 == nullptr){
		    errs() << "can't do my stuff\n";
		 }
		 numSTORE0ADDED++;
		 storage.push_back(Store0);
		 return;
	      }
	   }
	   --i;
	 }
      }
    }
 };
}

char DoubleStoreInstr::ID = 0;
int DoubleStoreInstr::numSTORE0ADDED = 0;
int DoubleStoreInstr::numSTOREDELETED = 0;
static RegisterPass<DoubleStoreInstr> X("DoubleStore", "DoubleStore Pass");
