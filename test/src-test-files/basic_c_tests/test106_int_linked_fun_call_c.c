int f1(void){
   return 131;
}


int f2(int k){
   return k * f1();
}

int f3(void){
   return 12 * f2(12);
}


int main(){
   f3();
   return 0;
}
