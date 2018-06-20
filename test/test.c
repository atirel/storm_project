#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>


int main(){
      char* const paramList[] = {"/home/guest/storm/build/bin/opt", "-load", "../build/DoubleStore/LLVMDoubleStore.so", "-DoubleStore", "../dumb.bc",  NULL};
      execv("/home/guest/storm/build/bin/opt", paramList);
   return 0;
}
