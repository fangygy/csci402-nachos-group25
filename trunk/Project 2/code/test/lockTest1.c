/* lockTest1.c
 *	
 * Test program to test functionality of locks
 * Create, Acquire, Release and Destroy
 *	
 *	
 *
 */

#include "syscall.h"


int main()
{
	int lockIndex;
	Write("Starting lock test\n", 64, ConsoleOutput);

	Write("Attempting to destroy a lock...\n", sizeof("Attempting to destroy lock...\n"), ConsoleOutput);
	DestroyLock(0);
	DestroyLock(-1);
	lockIndex = CreateLock("Testlock\n", 64);
	Release(lockIndex);
	Release(-1);

	Acquire(lockIndex);
	Acquire(lockIndex);
	Acquire(-1);
	Release(lockIndex);
	DestroyLock(lockIndex);

	/* not reached */
}