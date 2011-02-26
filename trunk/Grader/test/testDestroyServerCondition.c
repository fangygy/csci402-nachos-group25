#include "syscall.h"

int
main(){

	int a[0];
	int condition1,condition2;
	Print("\nDESTROYSERVERCONDITION_SYSCALL TEST\n",sizeof("\nDESTROYSERVERCONDITION_SYSCALL TEST\n"),a,0);
	DestroyServerCondition(0,0);
	DestroyServerCondition(1,1);
	condition1 = CreateServerCondition("Condition1",0);
	condition2 = CreateServerCondition("Condition2",1);
	DestroyServerCondition(0,1);
	DestroyServerCondition(1,0);
}
