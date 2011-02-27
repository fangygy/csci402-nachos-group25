/* forkTest.c
 *	
 * Test program to test functionality of Fork syscalls
 * NOTE: DO NOT RUN THIS TEST PROGRAM DIRECTLY.
 * TO RUN THIS TEST, RUN "execTest" INSTEAD
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

	Write("Testing bad fork syscalls...\n", sizeof("Testing bad fork syscalls...\n"), ConsoleOutput);
	Fork(0);
	Fork(-1);
	Fork("sdfjks");
	Fork(forkyforky);

	Exit(0);

	/* not reached */
}