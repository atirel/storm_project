#include <stdio.h>
#include <stdlib.h>

int pointless(void){
   int a=2;
   int b;
   int* array = (int *) malloc(sizeof(int) * 42);
   a=3;
   b=a;
   a=4;
   array = (int *)malloc(sizeof(int) * 50);

   array = (int *)malloc(sizeof(int) * 10);
   return a;
}


int main(){
   pointless();
   return 0;
}
