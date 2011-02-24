#include "syscall.h"

int lock1,lock2;
int i;
int
main(){

	int a[0];
	
	Print("\nRELEASESERVERLOCK_SYSCALL TEST\n",sizeof("\nRELEASESERVERLOCK_SYSCALL TEST\n"),a,0);
	ServerRelease(0,0);
	ServerRelease(1,1);
	lock1 = CreateServerLock("Lock1",1);
	lock2 = CreateServerLock("Lock2",0);
	ServerRelease(lock1,0);
	ServerAcquire(lock2,1);
	ServerRelease(lock2,1);
	ServerAcquire(lock1,0);
	Exec("../test/testServerRelease2");
	for(i=0;i<50;i++)
		Yield();
	ServerRelease(lock1,1);
	

}
