#include <stdio.h>
#include <stdlib.h>

int pointless(void){
   char d = 'o';
   long long int a = 2;
   int b;
   float c=1.2;
   c=1.3;
   printf("%f\n", c);
   int* array = (int *) malloc(sizeof(int) * 42);
   a = 3;
   b = a;
   b = 2;
   a = 4;
   printf("%d\n",b);
   d = 'r';
   b = 3;
   array = (int *)malloc(sizeof(int) * 50);
   c = 1.1;
   array = (int *)malloc(sizeof(int) * 10);
   return a;
}


int main(){
   pointless();
   return 0;
}
