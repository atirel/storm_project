#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


int main(){
   srand(time(NULL));
   uint16_t a;
   a = rand();
   int c = 0;
   while(a > 0){
      uint16_t b = rand();
      while(b > 0){
	 c++;
	 b--;
      }
      a--;
   }
   return 0;
}
