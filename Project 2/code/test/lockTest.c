/* lockTest.c
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

	Write("Destroying nonexistant locks...\n", sizeof("Destroying nonexistant locks...\n"), ConsoleOutput);
	DestroyLock(0);
	DestroyLock(-1);

	Write("Creating a lock...\n", sizeof("Creating a lock...\n"), ConsoleOutput);
	lockIndex = CreateLock("Testlock", 16);

	Write("Releasing nonexistant locks...\n", sizeof("Releasing nonexistant locks...\n"), ConsoleOutput);	
	Release(-1);
	Release(100);

	Write("Releasing non-owned lock...\n", sizeof("Releasing non-owned lock...\n"), ConsoleOutput);	
	Release(lockIndex);

	Write("Acquiring nonexistant locks...\n", sizeof("Acquiring nonexistant locks...\n"), ConsoleOutput);
	Acquire(-1);
	Acquire(100);

	Write("Acquiring lock twice...\n", sizeof("Acquiring locks twice...\n"), ConsoleOutput);
	Acquire(lockIndex);
	Acquire(lockIndex);
	Release(lockIndex);

	Write("Destroying lock already released...\n", sizeof("Destroying lock already released...\n"), ConsoleOutput);
	DestroyLock(lockIndex);

	/* not reached */
}