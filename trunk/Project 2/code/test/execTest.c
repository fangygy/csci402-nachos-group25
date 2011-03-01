/* execTest.c
 *	
 * Test program to test functionality of Exec syscalls
 *	
 *	
 *
 */

#include "syscall.h"

void trace(char* buffer) {
	/* ASK GRADER HOW THE HELL TO DO THIS */
	/*Write( buffer, strlen(buffer), ConsoleOutput);*/
}

int main()
{
	/*trace("Thissss... is line 1.\nThis is line 2, followed by a newLine.\n");*/
	/*Write("Executing a bad file...\n", sizeof("Executing a bad file...\n"), ConsoleOutput);
	Exec("Blahblahfilefail");*/

	Write("Executing forkTest...\n", sizeof("Executing forkTest...\n"), ConsoleOutput);
	Exec("../test/forkTest");
	/* not reached */
	Exit(0);
}