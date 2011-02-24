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
	Write("Starting lock test\n", 64, ConsoleOutput);
	int lockIndex = CreateLock();
	if(lockIndex != -1) 
		Write("Created lock\n", 64, ConsoleOutput);
	else
		Write("Failed to create lock\n", 64, ConsoleOutput);
	Acquire(lockIndex);
	Write("Acquired lock \n", 64, ConsoleOutput);
	Release(lockIndex);
	Write("Released lock\n", 64, ConsoleOutput);
	if(DestroyLock() == 1)
		Write("Destroyed lock\n", 64, ConsoleOutput);
	else
		Write("Failed to destroy lock\n", 64, ConsoleOutput);
	/* not reached */
}