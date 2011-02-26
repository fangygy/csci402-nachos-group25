#include "syscall.h"

int
main(){

	int a[0];
	int lock1,lock2;
	Print("\nDESTROYLOCK_SYSCALL TEST\n",sizeof("\nDESTROYLOCK_SYSCALL TEST\n"),a,0);
	DestroyLock(0);
	DestroyLock(1);
	lock1 = CreateLock("Lock1");
	lock2 = CreateLock("Lock2");
	DestroyLock(0);
	DestroyLock(1);
}
