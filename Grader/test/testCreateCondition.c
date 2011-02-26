#include "syscall.h"

int
main(){

	int a[0];
	int condition1,condition2,condition3;
	Print("\nCREATECONDITION_SYSCALL TEST\n",sizeof("\nCREATECONDITION_SYSCALL TEST\n"),a,0);
	condition1 = CreateCondition("Condition1");
	condition2 = CreateCondition("Condition2");
	condition3= CreateCondition((void*)0);

}
