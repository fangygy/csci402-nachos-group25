/* traceTest.c
 *	
 * Test program to test functionality of Trace syscall
 *	
 *	
 *
 */

#include "syscall.h"

int main()
{
	Write("This is the Trace syscall test.\n", sizeof("This is the Trace syscall test.\n"), ConsoleOutput);
	
	Trace("Trace sentence with value at the end: ", 8);
	
	/* not reached */
	Exit(0);
}