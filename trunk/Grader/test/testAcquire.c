#include "syscall.h"

int lock1,lock2;

int
main(){

	int a[0];
	
	Print("\nACQUIRELOCK_SYSCALL TEST\n",sizeof("\nACQUIRELOCK_SYSCALL TEST\n"),a,0);
	Acquire(0);
	Acquire(1);
	lock1 = CreateLock("Lock1");
	lock2 = CreateLock("Lock2");
	Acquire(lock1);
	Acquire(lock2);
	Exec("../test/testAcquire2");
	

}
