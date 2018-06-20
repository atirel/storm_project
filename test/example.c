#include <stdio.h>

int addition(int a, int b){
	return a+b;
}

int soustraction(int a, int b){
	return a-b;
}

int multiplication(int a, int b){
	return a*b;
}

int division(int a, int b){
	return a/b;
}

int main(int argc, char **argv){

	int a=0,b=0;
	
	if(a==0)
		printf("Hello!");

	printf("Entrer une valeur pour a: ");
	scanf("%d", &a);

	printf("Entrer une valeur pour b: ");
	scanf("%d", &b);

	printf("%d + %d = %d\n", a,b,addition(a, b));
	printf("%d - %d = %d\n", a,b,soustraction(a, b));
	printf("%d * %d = %d\n", a,b,multiplication(a, b));
	printf("%d / %d = %d\n", a,b,division(a, b));

	return 0;
}
