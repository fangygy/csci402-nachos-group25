/* traceTest.c
 *	
 * Test program to test functionality of Trace syscall
 *	
 *	
 *
 */

#include "syscall.h"
#define NV 0x9999

int main()
{
	Write("This is the Trace syscall test.\n", sizeof("This is the Trace syscall test.\n"), ConsoleOutput);
	
	Trace("Trace sentence with an eight at the end and no newline: ", 8);
	Trace("Sentence with no value and a newline.\n", NV);
	/* not reached */
	Exit(0);
}