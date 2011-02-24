#include "syscall.h"

int
main(){

	int a[0];
	int condition1,condition2;
	Print("\nDESTROYCONDITION_SYSCALL TEST\n",sizeof("\nDESTROYCONDITION_SYSCALL TEST\n"),a,0);
	DestroyCondition(0);
	DestroyCondition(1);
	condition1 = CreateCondition("Condition1");
	condition2 = CreateCondition("Condition2");
	DestroyCondition(0);
	DestroyCondition(1);
}
