#include <stdio.h>
#include <stdlib.h>

int pointless(void){
   char d = 'o';
   long long int a = 2;
   int b;
   float c=1.2;
   c=1.3;
   float f = c;
   a = 3;
   int Integer = 3;
   b = a;
   b = 2;
   a = 4;
   d = 'r';
   char e = d;
   d = 'e';
   b = 3;
   c = 1.1;
   return e;
}


int main(){
   pointless();
   return 0;
}
