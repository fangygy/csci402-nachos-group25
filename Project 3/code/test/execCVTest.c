/* execCVTest.c
 *	
 * Test program to test functionality of Exec syscalls
 *	
 *	
 *
 */

#include "syscall.h"


int main()
{
	Write("Executing CVTest...\n", sizeof("Executing CVTest...\n"), ConsoleOutput);
	Exec("../test/CVTest");
	/* not reached */
	Exit(0);
}