#include "syscall.h"

int condition1;
int lock1,lock2;
int finishedThreads;

void waitThread(){

Acquire(lock1);
Wait(condition1,lock1);
Release(lock1);
Acquire(lock2);
finishedThreads++;
Release(lock2);
Exit(0);
}			

int
main(){
int i;
int j;
int poo[1];
int a[0];
condition1 = CreateCondition("Condition1");
lock1 = CreateLock("Lock1");
lock2 = CreateLock("Lock2");
i=0;
j=0;
finishedThreads=0;

	Print("\nBROADCAST_SYSCALL TEST\n",sizeof("\nBROADCAST_SYSCALL TEST\n"),a,0);
	for(i=0;i<10;i++){
		Fork(waitThread);
	}
	for(j=0;j<3;j++){
		Yield();
	}
	Acquire(lock1);
	Broadcast(condition1,lock1);
	Release(lock1);
	for(i=0;i<3;i++){
		Yield();
	}
	Acquire(lock2);
	poo[0]=finishedThreads;
	Print("\nMain finds that %d threads have woken up and finished from broadcast\n",sizeof("\nMain finds that %d threads have woken up and finished from broadcast\n"),poo,1);	
	Release(lock2);
	Print("\nNow Broadcasting on Garbage lock and conditional: this should fail!\n",sizeof("\nNow Broadcasting on Garbage lock and conditional: this should fail!\n"),a,0);
	Broadcast(0xfff,0xfff);
		
}
