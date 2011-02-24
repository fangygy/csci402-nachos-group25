#include "syscall.h"

int lock1,lock2;

int
main(){

	int a[0];
	
	Print("\nRELEASELOCK_SYSCALL TEST\n",sizeof("\nRELEASELOCK_SYSCALL TEST\n"),a,0);
	Release(0);
	Release(1);
	lock1 = CreateLock("Lock1");
	lock2 = CreateLock("Lock2");
	Release(lock1);
	Acquire("Lock2");
	Release("Lock2");
	Exec("../test/testRelease2");
	

}
