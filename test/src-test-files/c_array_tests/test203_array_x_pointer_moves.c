#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>

void manager(int * a){
   unsigned int k = 1;
   int j = 2;
   srand(time(NULL));
   unsigned int border = rand();
   while(k < border){
      j = a[k];
      assert(j == 0);
      a[k] = border/k;
      printf("%d ", k);
      k++;
   }
}

int main(){
   int array[4];
   manager(array);
   return 0;
}
