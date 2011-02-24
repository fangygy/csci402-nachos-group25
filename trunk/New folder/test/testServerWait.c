#include "syscall.h"

int condition1;
int lock1,lock2;

void waitThread(){

ServerAcquire(lock1,1);
ServerWait(condition1,lock1,0);
ServerRelease(lock1,0);
ServerAcquire(lock2,1);
ServerRelease(lock2,1);
Exit(0);
}			

void waitBadThread(){

ServerWait(0xfff,0xfff,0);
Exit(0);
}

int
main(){
	int a[0];
	int i;
	Print("\nSERVERWAIT_SYSCALL TEST\n",sizeof("\nSERVERWAIT_SYSCALL TEST\n"),a,0);
	
	condition1 = CreateServerCondition("Condition1",0);
	lock1 = CreateServerLock("Lock1",1);
	lock2 = CreateServerLock("Lock2",0);
	Fork(waitThread);
	Yield();
	ServerAcquire(lock1,0);
	ServerSignal(condition1,lock1,1);
	ServerRelease(lock1,1);
	Print("\nNow Waiting on Garbage server lock and server condition: this should fail!\n",sizeof("\nNow Waiting on Garbage server lock and server condition: this should fail!\n"),a,0);
	Fork(waitBadThread);
		
}
