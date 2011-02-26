#include "syscall.h"

int
main(){

	int a[0];
	int lock1,lock2,lock3;
	Print("\nCREATELOCK_SYSCALL TEST\n",sizeof("\nCREATELOCK_SYSCALL TEST\n"),a,0);
	lock1 = CreateLock("Lock1");
	lock2 = CreateLock("Lock2");
	lock3 = CreateLock((void*)0);
}
