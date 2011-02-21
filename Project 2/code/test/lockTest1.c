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
	CreateLock();
	Acquire();
	Release();
	DestroyLock();
	/* not reached */
}