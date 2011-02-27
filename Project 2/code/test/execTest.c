/* execTest.c
 *	
 * Test program to test functionality of Exec syscalls
 *	
 *	
 *
 */

#include "syscall.h"


int main()
{
	Write("Executing a bad file...\n", sizeof("Executing a bad file...\n"), ConsoleOutput);
	Exec("Blahblahfilefail");

	Write("Executing forkTest...\n", sizeof("Executing forkTest...\n"), ConsoleOutput);
	Exec("../test/forkTest");
	/* not reached */
	Exit(0);
}