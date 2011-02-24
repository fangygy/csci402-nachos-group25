#include "syscall.h"

int
main(){

	int a[0];
	Print("\nYIELD_SYSCALL TEST\n",sizeof("\nYIELD_SYSCALL TEST\n"),a,0);
	Yield();

}
