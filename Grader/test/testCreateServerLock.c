#include "syscall.h"

int
main(){

	int a[0];
	int lock1,lock2,lock3,lock4;
	Print("\nCREATESERVERLOCK_SYSCALL TEST\n",sizeof("\nCREATESERVERLOCK_SYSCALL TEST\n"),a,0);
	lock1 = CreateServerLock("Lock1",1);
	lock2 = CreateServerLock("Lock2",0);
	lock3 = CreateServerLock("Lock1",0);
	lock4 = CreateServerLock((void*)0,0);
}
