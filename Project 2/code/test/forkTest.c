/* forkTest.c
 *	
 * Test program to test functionality of Fork syscalls
 * NOTE: DO NOT RUN THIS TEST PROGRAM DIRECTLY.
 * TO RUN THIS TEST, RUN "execTest" INSTEAD
 *	
 *
 */

#include "syscall.h"

int forkyforky(){
	Write("Forkyforky?\n", sizeof("Forkyforky?\n"), ConsoleOutput);
	return 1;
	Exit(0);
}

void testFunc(){
	Write("SUCCESS: A forked function.\n", sizeof("SUCCESS: A forked function.\n"), ConsoleOutput);
	Exit(0);
}

int main()
{
	Write("Testing fork syscall...\n", sizeof("Testing fork syscall...\n"), ConsoleOutput);
	Fork(testFunc);
	Fork(testFunc);

	Write("Testing bad fork syscalls...\n", sizeof("Testing bad fork syscalls...\n"), ConsoleOutput);
	Fork(0); /* This is okay */
	Trace("Forking vaddr of -1. Things may go downhill from there...\n", 0x9999);
	/* This stuff not okay...
	* 
	Fork(-1);  
	Fork("sdfjks"); 
	Fork(forkyforky);
	*/
	Exit(0);

	/* not reached */
}