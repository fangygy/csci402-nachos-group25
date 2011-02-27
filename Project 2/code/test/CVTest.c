/* CVTest.c
 *	
 * Test program to test functionality of CVs
 * Create, Destroy, Wait, Signal, Broadcast
 * NOTE: DO NOT RUN THIS TEST PROGRAM DIRECTLY
 * TO RUN THIS TEST, RUN "execCVTest" INSTEAD
 *
 */

#include "syscall.h"

int lock;
int condition, condition2;
int i;

void testFunc(){
	
	Write("Forked testFunc running. \n", sizeof("Forked testFunc running. \n"), ConsoleOutput);
	Write("testFunc: Acquiring lock... \n", sizeof("testFunc: Acquiring lock... \n"), ConsoleOutput);
	Acquire(lock);
	Write("testFunc: Acquired lock and waiting on condition. \n", sizeof("testFunc: Acquired lock and waiting on condition. \n"), ConsoleOutput);
	Wait(condition, lock);
	Write("testFunc: Woke up from wait on a signal. Releasing lock.\n", sizeof("testFunc: Woke up from wait on a signal. Releasing lock.\n"), ConsoleOutput);
	Release(lock);
	
	Write("testFunc: Waiting on a bad lock. \n", sizeof("testFunc: Waiting on a bad lock. \n"), ConsoleOutput);
	Wait(-1, -1);
	Wait(-1, 0);
	Wait(0,-1);
	Wait(100, lock);
	Wait(condition, 100);
	Exit(0);
}

void testFunc2(){
	Write("Forked testFunc2 running. \n", sizeof("Forked testFunc2 running. \n"), ConsoleOutput);
	Write("testFunc2: Acquiring lock... \n", sizeof("testFunc2: Acquiring lock... \n"), ConsoleOutput);
	Acquire(lock);
	Write("testFunc2: Acquired lock and waiting on condition2. \n", sizeof("testFunc2: Acquired lock and waiting on condition2. \n"), ConsoleOutput);
	Wait(condition2, lock);
	Write("testFunc2: Woke up from waiting on condition2. Releasing lock.\n", sizeof("testFunc2: Woke up from waiting on condition2. Releasing lock.\n"), ConsoleOutput);
	Release(lock);

}

int main()
{	Write("main: Starting condition variable test\n", sizeof("main: Starting condition variable test\n"), ConsoleOutput);
	
	DestroyCondition(condition);
	DestroyCondition(-1);

	lock = CreateLock("TestLock", 16);
	condition = CreateCondition("TestCondition", 23); 
	condition2 = CreateCondition("TestCondition2", 24);

	Write("main: Forking test function now. \n", sizeof("main: Forking test function now. \n"), ConsoleOutput);
	Fork(testFunc);
	Fork(testFunc2);
	Fork(testFunc2);
	Fork(testFunc2);

	for(i = 0; i < 200; i++){
		Yield(); /*To ensure that all the other functions finish thier actions first*/
	}
	Write("main: Signalling non-existant conditions and locks \n", sizeof("main: Signalling non-existant conditions and locks\n"), ConsoleOutput);
	Signal(-1, -1);
	Signal(-1, 0);
	Signal(0,-1);
	Signal(100, lock);
	Signal(condition, 100);

	Write("main: Signalling condition on the lock\n", sizeof("main: Signalling condition on the lock\n"), ConsoleOutput);
	Signal(condition, lock);
	Yield();
	Write("main: Acquiring lock. This should succeed.\n", sizeof("main: Acquiring lock. This should succeed.\n"), ConsoleOutput);
	Acquire(lock);
	Write("main: Acquired lock. \n", sizeof("main: Acquired lock. \n"), ConsoleOutput);
	
	DestroyCondition(condition);

	Release(lock);
	Write("main: Released lock. \n", sizeof("main: Released lock. \n"), ConsoleOutput);

	Write("main: Broadcast non-existant conditions and locks \n", sizeof("main: Broadcast non-existant conditions and locks\n"), ConsoleOutput);
	Broadcast(-1, -1);
	Broadcast(-1, 0);
	Broadcast(0,-1);
	Broadcast(100, lock);
	Broadcast(condition2, 100);

	Write("main: Broadcasting condition2 on the lock\n", sizeof("main: Broadcasting condition2 on the lock\n"), ConsoleOutput);
	Broadcast(condition2, lock);
	Yield();
	
	

	/* not reached */
}