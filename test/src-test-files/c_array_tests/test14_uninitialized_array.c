#include <assert.h>

int main(){
   int array[4];
   int a = array[0];
   int b = array[1];
   int c = array[2];
   int d = array[3];
   assert(a == 0 && b == 0 && c == 0 && d == 0);
   return 0;
}
