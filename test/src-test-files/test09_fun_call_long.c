long f1(void){
   long az = 10000000;
   return az;
}


long f2(int b){
   return (long) b;
}


int main(){
   long a = f1();
   int w = 1;
   f2(w);
   a /= 2;
   return 0;
}
