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
	Write("Starting lock test 1\n", sizeof("Starting lock test 1\n"), ConsoleOutput);

	Write("Destroying nonexistant lock...\n", sizeof("Destroying nonexistant lock...\n"), ConsoleOutput);
	DestroyLock(0);
	DestroyLock(-1);

	Write("Creating a lock...\n", sizeof("Creating a lock...\n"), ConsoleOutput);
	lockIndex = CreateLock("Testlock", 16);

	Write("Releasing locks...\n", sizeof("Releasing locks...\n"), ConsoleOutput);	
	Release(lockIndex);
	Release(-1);

	Write("Acquiring locks...\n", sizeof("Acquiring locks...\n"), ConsoleOutput);
	Acquire(lockIndex);
	Acquire(lockIndex);
	Acquire(-1);
	Release(lockIndex);

	Write("Destroying lock already released...\n", sizeof("Destroying lock already released...\n"), ConsoleOutput);
	DestroyLock(lockIndex);

	/* not reached */
}