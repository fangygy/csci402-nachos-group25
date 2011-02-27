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

void waiter() {
	Write("Acquiring lock and waiting on it. \n", sizeof("Acquiring lock and waiting on it. \n"), ConsoleOutput);
	Acquire(lock);
	Wait(condition, lock);
	
	Write("Woke up from wait on a signal. Releasing lock.\n", sizeof("Woke up from wait on a signal. Releasing lock.\n"), ConsoleOutput);
	Release(lock);
	
	Write("Waiting on good condition with bad lock.\n", sizeof("Waiting on good condition with bad lock.\n"), ConsoleOutput);
	Wait(condition, 100);
	
	Write("Waiting on bad condition with good lock.\n", sizeof("Waiting on bad condition with good lock.\n"), ConsoleOutput);
	Wait(-1, lock);
	
	Exit(0);
}

void signaler() {
	Write("Acquiring lock so I can signal.\n", sizeof("Acquiring lock so I can signal.\n"), ConsoleOutput);
	Acquire(lock);	/* make sure the testFunc acquires the lock first */
	Write("Signalling condition on the lock\n", sizeof("Signalling condition on the lock\n"), ConsoleOutput);
	Signal(condition, lock);
	Write("Signaller releasing lock\n", sizeof("Signaller releasing lock\n"), ConsoleOutput);
	Release(lock);
	Yield();
	
	Write("Acquiring lock. This should succeed.\n", sizeof("Acquiring lock. This should succeed.\n"), ConsoleOutput);
	Acquire(lock);
	
	Write("Releasing lock that doesn't exist.\n", sizeof("Releasing lock that doesn't exist.\n"), ConsoleOutput);
	Release(100);
	
	Write("Signaller releasing lock.\n", sizeof("Signaller releasing lock.\n"), ConsoleOutput);
	Release(lock);
	Exit(0);
}

int main()
{
	Write("Starting condition variable test\n", sizeof("Starting condition variable test\n"), ConsoleOutput);
	
	lock = CreateLock("TestLock", 16);
	condition = CreateCondition("TestCondition", 23); 

	/*Fork(testFunc);*/
	Fork(waiter);
	Yield();
	Fork(signaler);
	Yield();
	
	/* not reached */
	Exit(0);
}