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
	Write("Starting lock test 1\n", 64, ConsoleOutput);

	Write("Destroying nonexistant lock...\n", 64, ConsoleOutput);
	DestroyLock(0);
	DestroyLock(-1);

	Write("Creating a lock...\n", 64, ConsoleOutput);
	lockIndex = CreateLock("Testlock", 16);

	Write("Releasing locks...\n", 64, ConsoleOutput);	
	Release(lockIndex);
	Release(-1);

	Write("Acquiring locks...\n", 64, ConsoleOutput);
	Acquire(lockIndex);
	Acquire(lockIndex);
	Acquire(-1);
	Release(lockIndex);

	Write("Destroying lock already released...\n", 128, ConsoleOutput);
	DestroyLock(lockIndex);

	/* not reached */
}