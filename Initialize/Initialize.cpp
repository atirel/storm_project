/**
 * This LLVM pass deletes useless stores e.g store which are followed by another store
 * It also adds a STORE 0 %add after a load followed by a store in order to check with their values wether or not 2 variables are semantically equivalent
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
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instruction.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Constants.h"


using namespace llvm;

namespace {
 struct Initialize : public FunctionPass {

   static char ID;
   static int numSTORE0ADDED;//The number of instruction STORE 0 added

   Initialize() : FunctionPass(ID) {}
   bool runOnFunction(Function &F) override {
      for (BasicBlock &B : F){
	 Instruction* currentInstruction = nullptr;
	 for(Instruction &I : B){
	    if(AllocaInst *AI = dyn_cast<AllocaInst>(&I)){
	       addStore0(*AI);
	    }
	 }
      }
      return false;
   }


   void addStore0(AllocaInst &AI){
      Instruction *NextI = AI.getNextNode();
      if(NextI == nullptr){
	 return;
      }
      IRBuilder<> Builder(NextI);
      StoreInst* Store0 =  nullptr;
      int ID = AI.getType()->getElementType()->getTypeID();
      errs() << *AI.getType()->getElementType() << "\n";
      switch (ID) {//each case allows to check the type and store 0 type get<Typename>Ty at @operand with volatile=true in order to survive other pass

	 case Type::IntegerTyID:
   	       Store0 = Builder.CreateStore(ConstantInt::get(Builder.getIntNTy(cast<IntegerType>(AI.getType()->getElementType())->getBitWidth()), 0), &AI, true);
	    break;
		 
	 case Type::FloatTyID:
	    Store0 = Builder.CreateStore(ConstantFP::get(Builder.getFloatTy(), 0), &AI, true);
	    break;

	 case Type::DoubleTyID:
	    Store0 = Builder.CreateStore(ConstantFP::get(Builder.getDoubleTy(), 0), &AI, true);
	    break;

	 case Type::HalfTyID:
	    Store0 = Builder.CreateStore(ConstantFP::get(Builder.getHalfTy(), 0), &AI, true);
	    break;
		    
	 case Type::FP128TyID:
	    Store0 = Builder.CreateStore(ConstantFP::get(Type::getFP128Ty(Builder.getContext()), 0), &AI, true);
	    break;
		    
	 case Type::X86_FP80TyID:
	    Store0 = Builder.CreateStore(ConstantFP::get(Type::getX86_FP80Ty(Builder.getContext()), 0), &AI, true);
	    break;

	 case Type::PPC_FP128TyID:
	    Store0 = Builder.CreateStore(ConstantFP::get(Type::getPPC_FP128Ty(Builder.getContext()), 0), &AI, true);
	    break;

	 case Type::X86_MMXTyID:
	    Store0 = Builder.CreateStore(ConstantFP::get(Type::getX86_MMXTy(Builder.getContext()), 0), &AI, true);
	    break;

	 case Type::PointerTyID:
	    Store0 = Builder.CreateStore(Constant::getNullValue(cast<PointerType>(AI.getType()->getElementType())), &AI, true);
	    break;

	 case Type::VectorTyID:
	    errs() << AI << "\n";
	    //Store0 = Builder.CreateStore(ConstantDataVector::get(Type::getVectorTy(Builder.getContext()), 0), I, true);
	    break;
	 case Type::ArrayTyID:
	    ArrayType* array = dyn_cast<ArrayType>(AI.getType()->getElementType());
	    Builder.CreateStore(Constant::getNullValue(array), &AI, true);
	    /*uint64_t elements = array->getNumElements();
	    for(elements = 0; elements < array->getNumElements(); elements++){
	       errs() << elements << "\n";
	       Value* V = ConstantInt::get(Builder.getInt64Ty(), elements);
 	       Value* val = Builder.CreateGEP(&AI, V);
	       Value* toStore = Constant::getNullValue(array->getElementType());
	       Builder.CreateStore(toStore, &AI, true);
	    }*/

	    break;
      }

      if(Store0 == nullptr){
	 errs() << "can't do my stuff\n";
      }
      else{
	 errs() << "adding STORE 0 (after)\t\t\t";
//	 I.getDebugLoc().print(errs());
	 errs() << "\n";
      }
      numSTORE0ADDED++;
   }
 };
}

char Initialize::ID = 0;
int Initialize::numSTORE0ADDED = 0;
static RegisterPass<Initialize> X("Initialize", "Initialize Pass");
