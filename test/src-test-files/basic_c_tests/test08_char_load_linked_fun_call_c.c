char f1(void){
   return 'f';
}


char f2(char a){
   char b = f1();
   char c = a;
   b = c;
   return c;
}


int main(){
   char s = 's';
   f2(s);
   return 0;
}
