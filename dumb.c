#include <stdio.h>
#include <stdlib.h>

int pointless(void){
   char d = 'o';
   long long int a = 2;
   int b;
   float c=1.2;
   c=1.3;
   float f = c;
   int* array = (int *) malloc(sizeof(int) * 42);
   a = 3;
   int Integer = 3;
   int* p = &Integer;
   b = *p;
   b = a;
   b = 2;
   a = 4;
   d = 'r';
   char e = d;
   d = 'e';
   b = 3;
   //TODO faire des affectations pour pouvoir mettre à 0 lors du malloc
   //TODO Chercher la dernière instruction pour mettre à 0 avant la fin du programme/de la fonction
   //TODO ajouter un CTest pour faire tourner une batterie de tests sur les fonctions
   array = (int *)malloc(sizeof(int) * 50);
   c = 1.1;
   array = (int *)malloc(sizeof(int) * 10);
   return e;
}


int main(){
   pointless();
   return 0;
}
