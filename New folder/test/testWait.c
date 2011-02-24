#include "syscall.h"

int condition1;
int lock1,lock2;

void waitThread(){

Acquire(lock1);
Wait(condition1,lock1);
Release(lock1);
Acquire(lock2);
Release(lock2);
Exit(0);
}			

void waitBadThread(){

Wait(0xfff,0xfff);
Exit(0);
}

int
main(){
int a[0];
condition1 = CreateCondition("Condition1");
lock1 = CreateLock("Lock1");
lock2 = CreateLock("Lock2");

	Print("\nWAIT_SYSCALL TEST\n",sizeof("\nWAIT_SYSCALL TEST\n"),a,0);
	Fork(waitThread);
	Yield();
	Acquire(lock1);
	Signal(condition1,lock1);
	Release(lock1);
	Print("\nNow Waiting on Garbage lock and conditional: this should fail!\n",sizeof("\nNow Waiting on Garbage lock and conditional: this should fail!\n"),a,0);
	Fork(waitBadThread);
		
}
