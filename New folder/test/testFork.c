#include "syscall.h"
int i = 1;

void forkTest(){
	int a[] = {i};
	Print("\nForking function %d.\n\n",22,a,1);  
	i= i+1; 
	Exit(0);    
}

int
main(){
	int a[0];
	Print("\nFORK_SYSCALL TEST\n",sizeof("\nFORK_SYSCALL TEST\n"),a,0);
	Fork(forkTest);
	Fork(forkTest);
	Fork(0xffff);
}
