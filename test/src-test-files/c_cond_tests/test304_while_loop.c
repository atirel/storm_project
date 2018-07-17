#include <stdio.h>

int main(){
   int k = 1048576;
   int exec = 0;
   while(k){
      k /= 2;
      exec++;
   }
   printf("%d\n", exec);
   return 0;
}
