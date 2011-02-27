/* forkTest.c
 *	
 * Test program to test functionality of Fork syscalls
 *	
 *	
 *
 */

#include "syscall.h"

void testFunc(){
	Write("A forked function.\n", sizeof("A forked function.\n"), ConsoleOutput);
	Exit(0);
}

int main()
{
	Write("Testing fork syscall...\n", sizeof("Testing fork syscall...\n"), ConsoleOutput);
	Fork(testFunc);
	Fork(testFunc);
	Exit(0);

	/* not reached */
}