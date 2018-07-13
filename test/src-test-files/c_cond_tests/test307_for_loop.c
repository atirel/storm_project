#include <stdio.h>


int main(){
   int j[34] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
   int k;
   int sum = 0;
   for(k = 0; k < 34; k+=1){
      sum += j[k];
   }
   printf("%d\n", sum);
   return 0;
}
      
