#include <time.h>
#include <stdlib.h>
#include <stdio.h>

void displayer(const int l, const int m, const int h){
   printf("%d is the lowest, %d is in middle and %d is the highest\n", l, m, h);
}

int main(){
   srand(time(NULL));
   int k = rand();
   int l = rand();
   int m = rand();
   if(k < l){
      if(m < k){
	 displayer(m, k, l);
      }
      else{
	 if(m < l){
	    displayer(k, m, l);
	 }
	 else{
	    displayer(k, l, m);
	 }
      }
   }
   else{
      if(m < l){
	 displayer(m, l, k);
      }
      else{
	 if(k < m){
	    displayer(l, k, m);
	 }
	 else{
	    displayer(l, m, k);
	 }
      }
   }
   return 0;
}
