#include "syscall.h"

int lock1,lock2;

int
main(){

	int a[0];
	
	Print("\nSERVERACQUIRELOCK_SYSCALL TEST\n",sizeof("\nSERVERACQUIRELOCK_SYSCALL TEST\n"),a,0);
	ServerAcquire(0,0);
	ServerAcquire(1,1);
	lock1 = CreateServerLock("Lock1",1);
	lock2 = CreateServerLock("Lock2",0);
	ServerAcquire(lock1,0);
	ServerAcquire(lock2,1);
	Exec("../test/testServerAcquire2");
	

}
