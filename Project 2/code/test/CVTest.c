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
int condition;
int i;

void testFunc(){
	
	Write("Forked test function running. \n", sizeof("Forked test function running. \n"), ConsoleOutput);
	Write("Acquiring lock... \n", sizeof("Acquiring lock... \n"), ConsoleOutput);
	Acquire(lock);
	Write("Acquired lock and waiting on it. \n", sizeof("Acquired lock and waiting on it. \n"), ConsoleOutput);
	Wait(condition, lock);
	Write("Woke up from wait on a signal. Releasing lock.\n", sizeof("Woke up from wait on a signal. Releasing lock.\n"), ConsoleOutput);
	Release(lock);
	
	Write("Waiting on a bad lock. \n", sizeof("Waiting on a bad lock. \n"), ConsoleOutput);
	Wait(-1, -1);
	Wait(-1, 0);
	Wait(0,-1);
	Wait(100, lock);
	Wait(condition, 100);
	Wait(lock, condition);
	Exit(0);
}

int main()
{	Write("Starting condition variable test\n", sizeof("Starting condition variable test\n"), ConsoleOutput);
	
	DestroyCondition(condition);
	DestroyCondition(-1);

	lock = CreateLock("TestLock", 16);
	condition = CreateCondition("TestCondition", 23); 

	Write("Forking test function now. \n", sizeof("Forking test function now. \n"), ConsoleOutput);
	Fork(testFunc);

	for(i = 0; i < 20; i++){
		Yield();
	}
	Write("Signalling non-existant conditions and locks \n", sizeof("Signalling non-existant conditions and locks\n"), ConsoleOutput);
	Signal(-1, -1);
	Signal(-1, 0);
	Signal(0,-1);
	Signal(100, lock);
	Signal(condition, 100);
	Signal(lock, condition);

	Write("Signalling condition on the lock\n", sizeof("Signalling condition on the lock\n"), ConsoleOutput);
	Signal(condition, lock);
	Yield();
	Write("Acquiring lock. This should succeed.\n", sizeof("Acquiring lock. This should succeed.\n"), ConsoleOutput);
	Acquire(lock);
	Write("Acquired lock. \n", sizeof("Acquired lock. \n"), ConsoleOutput);
	
	DestroyCondition(condition);

	/* not reached */
}