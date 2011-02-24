#include "syscall.h"

int condition1;
int lock1,lock2;
int finishedThreads;

void waitThread(){

	ServerAcquire(lock1,1);
	ServerWait(condition1,lock1,0);
	finishedThreads++;
	if(finishedThreads==5)
		ServerSignal(condition1,lock1,1);
	ServerRelease(lock1,0);
	ServerAcquire(lock2,1);
	ServerRelease(lock2,0);
	Exit(0);
}			

int
main(){
	int i;
	int j;
	int poo[1];
	int a[0];
	Print("\nSERVERBROADCAST_SYSCALL TEST\n",sizeof("\nSERVERBROADCAST_SYSCALL TEST\n"),a,0);
	
	condition1 = CreateServerCondition("Condition1",1);
	lock1 = CreateServerLock("Lock1",0);
	lock2 = CreateServerLock("Lock2",1);
	i=0;
	j=0;
	finishedThreads=0;

	for(i=0;i<5;i++){
		Fork(waitThread);
	}
	for(j=0;j<10;j++){
		Yield();
	}
	ServerAcquire(lock1,1);
	ServerBroadcast(condition1,lock1,0);
	ServerWait(condition1,lock1,0);
	ServerRelease(lock1,0);
	ServerAcquire(lock2,0);
	poo[0]=finishedThreads;
	Print("\nMAIN FINDS THAT %d THREADS HAVE WOKEN UP AND FINISHED FROM BROADCAST\n",sizeof("\nMAIN FINDS THAT %d THREADS HAVE WOKEN UP AND FINISHED FROM BROADCAST\n"),poo,1);	
	ServerRelease(lock2,1);
	Print("\nNOW BROADCASTING ON GARBAGE SERVER LOCK AND SERVER CONDITION: THIS SHOULD FAIL!\n",sizeof("\nNOW BROADCASTING ON GARBAGE SERVER LOCK AND SERVER CONDITION: THIS SHOULD FAIL!\n"),a,0);
	ServerBroadcast(0xfff,0xfff,0);
		
}
