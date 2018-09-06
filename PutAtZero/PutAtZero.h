#ifndef PUTATZERO_H
#define PUTATZERO_H

enum Color{white, gray, black};
typedef std::map<const llvm::BasicBlock*, Color> BlockList;
//each block has a color according for its treatment
//white -> untreated
//gray  -> treating in process
//black -> treated

typedef std::pair<llvm::AllocaInst*, bool> IsDead;
//each variable (which is linked to an Alloca Instruction in LLVM) has a boolean status, alive or dead

typedef std::map<const llvm::BasicBlock* , std::vector<IsDead>> InstructionList;
//each block has a set of variables with their respective status
//this allows us to know in case of branches if the variable is dead on all the possible ways

#endif

