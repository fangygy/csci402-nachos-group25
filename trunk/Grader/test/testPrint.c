#include "syscall.h"

/*	To print: pass the string with modifiers, 
   	length in characters of print statement,
   	array of ints if you have any(enter an 
   	array of size 0 if not), and then the 
   	number of ints you have in the array you
   	are passing.
*/
/*
	int a[4] = {1,2,3,55};
  	Print("You are %d, a bitch, %d, a whore, %d, a slut, %d, an idiot\n",59,a,4);
*/

int
main(){
	int a[0];
	char* c= "Hello World  ! %d, %d, %d, %d\n";
	int i;
	int b[] = {1,2,3,4};
	
	Print("\nPRINT_SYSCALL TEST\n\n",sizeof("\nPRINT_SYSCALL TEST\n\n"),a,0);
	
	for(i=0;i<21;i++){
		c[11] = (char)(i/10+48);
		c[12] = (char)(i%10+48);
		Print(c,30,b,4);
	}

}
