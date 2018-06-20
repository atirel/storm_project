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
	 errs() << "Treating function: " << F.getName() << "\n";
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

		 switch (storage[i]->getType()->getTypeID()) {

		    case Type::IntegerTyID:
		       Store0 = Builder.CreateStore(ConstantInt::get(Builder.getIntNTy(cast<IntegerType>(storage[i]->getType())->getBitWidth()), 0), operand, true);
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
		    case Type::VectorTyID:
		       Store0 = Builder.CreateStore(ConstantDataVector::get(Type::getVectorTy(Builder.getContext()), 0), operand, true);
		       break;*/
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