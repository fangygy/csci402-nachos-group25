/* writeTest.c
 *	
 * Test program to test functionality of Write syscall
 *	
 *	
 *
 */

#include "syscall.h"

int i;
char* buf;

int main()
{
	Write("This is a normal sentence.\n", sizeof("This is a normal sentence.\n"), ConsoleOutput);
	
	buf = "This is a char pointer.\n";
	Write(buf, 36, ConsoleOutput);
	
	/* not reached */
	Exit(0);
}