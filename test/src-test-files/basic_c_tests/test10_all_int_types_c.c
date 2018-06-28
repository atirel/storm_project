long long f1(void){
   return 30;
}

unsigned long f2(void){
   return 12;
}


short f3(void){
   return 8;
}

unsigned short f4(void){
   return 200;
}


unsigned f5(void){
   return 42;
}


long f6(void){
   return -15;
}


int main(){
   long long a = f1();
   a = (long long) f2();
   a = (long long) f3();
   a = (long long) f4();
   unsigned b = f5();
   long c = f6() + b;
   c = f1();
   return 0;
}
