/* destroyCVTest.c
 *	
 * Test program to test functionality of Locks
 * and CVs that are being destroyed while they
 * are still in use.
 * NOTE: DO NOT RUN THIS TEST PROGRAM DIRECTLY
 * TO RUN THIS TEST, RUN "execDestroyTest" INSTEAD
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
	
	/* Waking up */
	Write("Waiter1: Woke up from wait on a signal. Releasing lockOld, sched. for deletion.\n",
		sizeof("Waiter1: Woke up from wait on a signal. Releasing lockOld, sched. for deletion.\n"), ConsoleOutput);
	Release(lockOld);
	
	Yield();
	
	Write("Waiter1: Acquiring lockOld, sched. for deletion.\n",
		sizeof("Waiter1: Acquiring lockOld, sched. for deletion.\n"), ConsoleOutput);
	Acquire(lockOld);
	
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
	
	/* Waking up */
	Write("Waiter2: Woke up from wait on a signal. Releasing lockOld.\n",
		sizeof("Waiter2: Woke up from wait on a signal. Releasing lockOld.\n"), ConsoleOutput);
	Release(lockOld);
	
	Write("Waiter2: Finished.\n", sizeof("Waiter2: Finished.\n"), ConsoleOutput);
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
	
	Write("Signaller: Waiting on cvNew again, sched. for deletion.\n",
		sizeof("Signaller: Waiting on cvNew again, sched. for deletion.\n"), ConsoleOutput);
	Wait(cvNew, lockNew);
	
	Write("Signaller: Finished.\n", sizeof("Signaller: Finished.\n"), ConsoleOutput);
	Exit(0);
}

void Troll() {
	Write("Troll: Starting up.\n", sizeof("Troll: Starting up.\n"), ConsoleOutput);
	
	/* LockNew abuse */
	Write("Troll: Acquiring lockNew.\n", sizeof("Troll: Acquiring lockNew.\n"), ConsoleOutput);
	Acquire(lockNew);
	Write("Troll: Waiting on cvOld with lockNew(bad).\n",
		sizeof("Troll: Waiting on cvOld with lockNew(bad).\n"), ConsoleOutput);
	Wait(cvOld, lockNew);
	Write("Troll: Releasing lockNew.\n", sizeof("Troll: Releasing lockNew.\n"), ConsoleOutput);
	Release(lockNew);
	
	/* LockOld section */
	Write("Troll: Acquiring lockOld... \n", sizeof("Troll: Acquiring lockOld... \n"), ConsoleOutput);
	Acquire(lockOld);
	Write("Troll: Waiting on cvOld with lockOld.\n",
		sizeof("Troll: Waiting on cvOld with lockOld.\n"), ConsoleOutput);
	Wait(cvOld, lockOld);
	
	
	/* LockOld SABOTAGE!!! >:D */
	Write("Troll: Destroying lockOld.\n",
		sizeof("Troll: Destroying lockOld.\n"), ConsoleOutput);
	DestroyLock(lockOld);
	
	Write("Troll: Releasing lockOld... \n", sizeof("Troll: Releasing lockOld... \n"), ConsoleOutput);
	Release(lockOld);
	
	/* cvNew SABOTAGE!!! >:D */
	Write("Troll: Acquiring lockNew... \n", sizeof("Troll: Acquiring lockNew... \n"), ConsoleOutput);
	Acquire(lockNew);
	Broadcast(cvNew, lockNew);
	Write("Troll: Destroying cvNew.\n",
		sizeof("Troll: Destroying cvNew.\n"), ConsoleOutput);
	DestroyCondition(cvNew);
	Write("Troll: Releasing lockNew... \n", sizeof("Troll: Releasing lockNew... \n"), ConsoleOutput);
	Release(lockNew);
	
	Write("Troll: Finished.\n", sizeof("Troll: Finished.\n"), ConsoleOutput);
	Exit(0);
}

int main() {
	Write("main: Starting destroy CV test\n", sizeof("main: Starting destroy CV test\n"), ConsoleOutput);
	
	lockOld = CreateLock("lockOld", 20);
	lockNew = CreateLock("lockNew", 20);
	cvOld = CreateCondition("cvOld", 20);
	cvNew = CreateCondition("cvNew", 20);
	
	Fork(Waiter1);
	Fork(Waiter2);
	Fork(Troll);
	
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