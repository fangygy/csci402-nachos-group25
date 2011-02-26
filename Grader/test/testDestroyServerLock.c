#include "syscall.h"

int
main(){

	int a[0];
	int lock1,lock2;
	Print("\nDESTROYSERVERLOCK_SYSCALL TEST\n",sizeof("\nDESTROYSERVERLOCK_SYSCALL TEST\n"),a,0);
	DestroyServerLock(0,0);
	DestroyServerLock(1,1);
	lock1 = CreateServerLock("Lock1",0);
	lock2 = CreateServerLock("Lock2",1);
	ServerAcquire(lock1,0);
	DestroyServerLock(0,1);
	ServerRelease(lock1,0);
	DestroyServerLock(1,0);
}
