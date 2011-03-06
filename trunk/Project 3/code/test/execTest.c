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

void execTestFunc(){
	Write("BINGBONG: execTestFunc running\n", sizeof("BINGBONG: execTestFunc running\n"), ConsoleOutput);
	Exit(0);

}

int main()
{
	/*trace("Thissss... is line 1.\nThis is line 2, followed by a newLine.\n");*/
	/*Write("Executing a bad file...\n", sizeof("Executing a bad file...\n"), ConsoleOutput);
	Exec("Blahblahfilefail");*/

	Write("Executing forkTest...\n", sizeof("Executing forkTest...\n"), ConsoleOutput);
	Exec("../test/forkTest");

	Write("ExecTest forking execTestFunc...\n", sizeof("ExecTest forking...\n"), ConsoleOutput);
	Fork(execTestFunc);
	/* not reached */
	Exit(0);
}