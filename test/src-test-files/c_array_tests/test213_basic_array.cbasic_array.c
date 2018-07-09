#include <assert.h>

int main(){
   int array[3] = {1,2,3};
   int a = array[0];
   int b = array[1];
   int c = array[2];
   assert(a == 1);
   assert(b == 2);
   assert(c == 3);
   return 0;
}
