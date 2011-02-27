/* CVTest1.c
 *	
 * Test program to test functionality of CVs
 * Create, Destroy, Wait, Signal, Broadcast
 *	
 *	
 *
 */

#include "syscall.h"

int lock;
int condition;

void testFunc(){
	Write("Acquiring lock and waiting on it. \n", sizeof("Acquiring lock and waiting on it. \n"), ConsoleOutput);
	Acquire(lock);
	Wait(condition, lock);
	Write("Woke up from wait on a signal. Releasing lock.\n", sizeof("Woke up from wait on a signal. Releasing lock.\n"), ConsoleOutput);
	Release(lock);
	
	Write("Waiting on a bad lock. \n", sizeof("Waiting on a bad lock. \n"), ConsoleOutput);
	Wait(100, 100);
	Wait(-1, -1);
	Exit(0);
}

int main()
{
	Write("Starting condition variable test\n", sizeof("Starting condition variable test\n"), ConsoleOutput);
	
	lock = CreateLock("TestLock", 16);
	condition = CreateCondition("TestCondition", 23); 

	Fork(testFunc);
	Yield();
	Acquire(lock);	/* make sure the testFunc acquires the lock first */

	Write("Signalling condition on the lock\n", sizeof("Signalling condition on the lock\n"), ConsoleOutput);
	Signal(condition, lock);
	Yield();
	Write("Acquiring lock. This should succeed.\n", sizeof("Acquiring lock. This should succeed.\n"), ConsoleOutput);
	Acquire(lock);
	/* not reached */
}