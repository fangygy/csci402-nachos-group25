#include "syscall.h"

int
main(){

	int a[0];
	int condition1,condition2,condition3,condition4;
	Print("\nCREATESERVERCONDITION_SYSCALL TEST\n",sizeof("\nCREATESERVERCONDITION_SYSCALL TEST\n"),a,0);
	condition1 = CreateServerCondition("Condition1",1);
	condition2 = CreateServerCondition("Condition2",0);
	condition3 = CreateServerCondition("Condition1",0);
	condition4= CreateServerCondition((void*)0,0);

}
