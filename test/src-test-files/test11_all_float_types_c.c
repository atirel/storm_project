#include <assert.h>
float f1(int b){
   return b;
}


float f2(void){
   return 1.2;
}


double add(int a, float b){
   return a+b;
}

int main(){
   f1(3);
   f2();
   add(3,f2());
   assert(0 < 0.00001);
   return 0;
}
