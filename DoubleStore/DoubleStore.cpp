#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/ADT/SmallVector.h"
#include <cstring>
#include <stdio.h>
#include <vector>

using namespace llvm;

namespace {
 struct DoubleStoreInstr : public ModulePass {

   static char ID;
   static int numLOAD;
   static int numSTORE;
   static int numUSTORE;

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
      errs() << "\033[0;36m Total STORE: " << numSTORE << "\033[0;0m\n";
      errs() << "\033[0;31m Useless ones: " << numUSTORE << "\033[0;0m\n";
      errs() << "\033[0;32m Usefull ones: " << (numSTORE - numUSTORE) << "\033[0;0m\n";
      errs() << "\033[0;36m Total LOAD: " << numLOAD << "\033[0;0m\n";
      return false;
   }

   void update_storage(SmallVector<Instruction*, 64>& storage, Instruction &I){
      int size = storage.end() - storage.begin();
      unsigned int Ioperands = I.getNumOperands();
      Value* operand = I.getOperand(Ioperands - 1);
      int i = size - 1;
      if(I.getOpcode() == 31){
	 numSTORE++;
      	 while(i >= 0){
           if(operand == storage[i]->getOperand(storage[i]->getNumOperands()-1)){//if the adress where the value is stored/load is the same
	      if(storage[i]->getOpcode() == 31){//31 stands for store
		 storage[i]->eraseFromParent();
		 storage.erase(storage.begin()+i);
		 storage.push_back(&I);
		 return;
	      }
	      if(storage[i]->getOpcode() == 30){//30 stands for load
		 numLOAD++;
		 return;
	      }
	   }
	   --i;
	 }
      }
      storage.push_back(&I);
    }
 };
}

char DoubleStoreInstr::ID = 0;
int DoubleStoreInstr::numLOAD = 0;
int DoubleStoreInstr::numSTORE = 0;
int DoubleStoreInstr::numUSTORE = 0;
static RegisterPass<DoubleStoreInstr> X("DoubleStore", "DoubleStore Pass");
