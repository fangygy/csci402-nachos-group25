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
	DestroyLock(0);
	lockIndex = CreateLock("Testlock\n", 64);
	Release(lockIndex);

	Acquire(lockIndex);
	Acquire(lockIndex);
	Release(lockIndex);
	DestroyLock(lockIndex);

	/* not reached */
}