/* destroyCVTest.c
 *	
 * Test program to test functionality of CVs
 * Create, Destroy, Wait, Signal, Broadcast
 * NOTE: DO NOT RUN THIS TEST PROGRAM DIRECTLY
 * TO RUN THIS TEST, RUN "execCVTest" INSTEAD
 *
 */

#include "syscall.h"

int lockOld, lockNew;
int cvOld, cvNew;
int i;

void Waiter1(){
	Write("Waiter1: Starting up.\n", sizeof("Waiter1: Starting up.\n"), ConsoleOutput);
	
	/* LockOld section */
	Write("Waiter1: Acquiring lockOld...\n", sizeof("Waiter1: Acquiring lockOld...\n"), ConsoleOutput);
	Acquire(lockOld);
	Write("Waiter1: Acquired lockOld and waiting on cvOld.\n",
		sizeof("Waiter1: Acquired lockNew and waiting on cvOld.\n"), ConsoleOutput);
	Wait(cvOld, lockOld);
	
	Write("Waiter1: Woke up from wait on a signal. Releasing lockOld, sched. for deletion.\n",
		sizeof("Waiter1: Woke up from wait on a signal. Releasing lockOld, sched. for deletion.\n"), ConsoleOutput);
	Release(lockOld);
	
	/* Give Douche time to mess us up */
	Yield();
	Write("Waiter1: Acquiring lockOld, sched. for deletion.\n",
		sizeof("Waiter1: Acquiring lockOld, sched. for deletion.\n"), ConsoleOutput);
	Acquire(lockOld);
	
	Write("Waiter1: Waiting on a bad lock. \n", sizeof("Waiter1: Waiting on a bad lock. \n"), ConsoleOutput);
	Wait(-1, -1);
	Wait(-1, 0);
	Wait(0,-1);
	Wait(100, lockOld);
	Wait(cvOld, 100);
	Write("Waiter1: Finished.\n", sizeof("Waiter1: Finished.\n"), ConsoleOutput);
	Exit(0);
}

void Waiter2(){
	Write("Waiter2: Starting up.\n", sizeof("Waiter1: Starting up.\n"), ConsoleOutput);
	
	/* LockOld section */
	Write("Waiter2: Acquiring lockOld...\n", sizeof("Waiter2: Acquiring lockOld...\n"), ConsoleOutput);
	Acquire(lockOld);
	Write("Waiter2: Acquired lockOld and waiting on cvOld.\n",
		sizeof("Waiter2: Acquired lockNew and waiting on cvOld.\n"), ConsoleOutput);
	Wait(cvOld, lockOld);
	
	Write("Waiter2: Woke up from wait on a signal. Releasing lockOld.\n",
		sizeof("Waiter2: Woke up from wait on a signal. Releasing lockOld.\n"), ConsoleOutput);
	Release(lockOld);
	
	/* LockNew section */
	Write("Waiter2: Acquiring lockNew...\n", sizeof("Waiter2: Acquiring lockNew...\n"), ConsoleOutput);
	Acquire(lockNew);
	Write("Waiter2: Acquired lockNew and waiting on deleted cvNew.\n",
		sizeof("Waiter2: Acquired lockNew and waiting on deleted cvNew.\n"), ConsoleOutput);
	Wait(cvNew, lockNew);
	
	/*Write("Waiter2: Woke up from wait on a signal. Releasing lockNew.\n",
		sizeof("Waiter2: Woke up from wait on a signal. Releasing lockNew.\n"), ConsoleOutput);*/
	Release(lockNew);
	Exit(0);
}

void Signaller(){
	Write("Signaller: Starting up.\n", sizeof("Signaller: Starting up.\n"), ConsoleOutput);
	
	/* LockOld section */
	Write("Signaller: Acquiring lockOld... \n", sizeof("Signaller: Acquiring lockOld... \n"), ConsoleOutput);
	Acquire(lockOld);
	Write("Signaller: Broadcasting cvOld with lockOld.\n",
		sizeof("Signaller: Broadcasting cvOld with lockOld.\n"), ConsoleOutput);
	Broadcast(cvOld, lockOld);
	Release(lockOld);
	
	/* LockNew section */
	Write("Signaller: Acquiring lockNew...\n", sizeof("Signaller: Acquiring lockNew...\n"), ConsoleOutput);
	Acquire(lockNew);
	Write("Signaller: Acquired lockNew and waiting on cvNew.\n",
		sizeof("Signaller: Acquired lockNew and waiting on cvNew.\n"), ConsoleOutput);
	Wait(cvNew, lockNew);
	
	Write("Signaller: Acquiring lockNew... \n", sizeof("Signaller: Acquiring lockNew... \n"), ConsoleOutput);
	Acquire(lockNew);
	Write("Signaller: Broadcasting cvNew with lockNew.\n",
		sizeof("Signaller: Broadcasting cvNew with lockNew.\n"), ConsoleOutput);
	Release(lockNew);
	
	Write("Signaller: Finished.\n", sizeof("Signaller: Finished.\n"), ConsoleOutput);
	Exit(0);
}

void Douche() {
	Write("Douche: Starting up.\n", sizeof("Douche: Starting up.\n"), ConsoleOutput);
	
	/* LockNew abuse */
	Write("Douche: Acquiring lockNew.\n", sizeof("Douche: Acquiring lockNew.\n"), ConsoleOutput);
	Acquire(lockNew);
	Write("Douche: Waiting on cvOld with lockNew(bad).\n",
		sizeof("Douche: Waiting on cvOld with lockNew(bad).\n"), ConsoleOutput);
	Wait(cvOld, lockNew);
	Write("Douche: Releasing lockNew.\n", sizeof("Douche: Releasing lockNew.\n"), ConsoleOutput);
	Release(lockNew);
	
	/* LockOld section */
	Write("Douche: Acquiring lockOld... \n", sizeof("Douche: Acquiring lockOld... \n"), ConsoleOutput);
	Acquire(lockOld);
	Write("Douche: Waiting on cvOld with lockOld.\n",
		sizeof("Douche: Waiting on cvOld with lockOld.\n"), ConsoleOutput);
	Wait(cvOld, lockOld);
	
	
	/* LockOld SABOTAGE!!! >:D */
	Write("Douche: Destroying lockOld.\n",
		sizeof("Douche: Destroying lockOld.\n"), ConsoleOutput);
	DestroyLock(lockOld);
	
	Write("Douche: Releasing lockOld... \n", sizeof("Douche: Releasing lockOld... \n"), ConsoleOutput);
	Release(lockOld);
	
	/* cvNew SABOTAGE!!! >:D */
	Broadcast(cvNew, lockOld);
	Write("Douche: Destroying cvNew.\n",
		sizeof("Douche: Destroying cvNew.\n"), ConsoleOutput);
	DestroyCondition(cvNew);
	
	Write("Douche: Finished.\n", sizeof("Douche: Finished.\n"), ConsoleOutput);
	Exit(0);
}

int main() {
	Write("main: Starting destroy CV test\n", sizeof("main: Starting destroy CV test\n"), ConsoleOutput);
	
	lockOld = CreateLock("lockOld", 20);
	lockNew = CreateLock("lockNew", 20);
	cvOld = CreateCondition("cvOld", 20);
	cvNew = CreateCondition("cvNew", 20);
	
	Fork(Douche);
	Fork(Waiter1);
	Fork(Waiter2);
	
	/* Give waiters time to wait on CVs */
	for (i = 0; i < 100; i++) {
		Yield();
	}
	
	Fork(Signaller);
	
	/* Give forked threads time to finish */
	for (i = 0; i < 100; i++) {
		Yield();
	}
	
	Write("main: Finished.\n", sizeof("main: Finished.\n"), ConsoleOutput);
	Exit(0);
	/* not reached */
}