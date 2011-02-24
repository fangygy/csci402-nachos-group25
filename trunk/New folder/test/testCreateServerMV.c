#include "syscall.h"

int
main(){

	int a[0];
	int mv1,mv2,mv3,mv4;
	Print("\nCREATEMV_SYSCALL TEST\n","\nCREATESERVERMV_SYSCALL TEST\n",a,0);
	mv1 = CreateMV("MV1",1);
	mv2 = CreateMV("MV2",0);
	mv3 = CreateMV("MV1",0);
	mv4 = CreateMV((void*)0,0);
}
