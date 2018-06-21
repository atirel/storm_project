#include <stdio.h>


int pointless2(int a){
   return a;
}

int pointless(void){
   int a = 3;
   pointless2(a);
   a=3;
   return a;
}

int main(){

   pointless();
   return 0;
}
