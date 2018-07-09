#include <stdio.h>
int main(){
   int k = 4;
   switch (k) {
      case 5:
	 k = 4;
	 break;
      case 4:
	 k = 3;
	 return 1;
      case 3:
	 k = 2;
      case 2:
	 k = 1;
      case 1:
	 k = 0;
      default:
	 k = 42;
   }
   return 0;
}
