/* execDestroyTest.c
 *	
 * Test program to test functionality of Exec syscalls
 *	
 *	
 *
 */

#include "syscall.h"


int main()
{
	Write("Executing destroyCVTest...\n", sizeof("Executing destroyCVTest...\n"), ConsoleOutput);
	Exec("../test/destroyCVTest");
	/* not reached */
	Exit(0);
}