/* forkTest.c
 *	
 * Test program to test functionality of Fork syscalls
 *	
 *	
 *
 */

#include "syscall.h"

void testFunc(){
	Write("A forked function.", 64, ConsoleOutput);
	Exit();
}

int main()
{
	Write("Testing fork syscall...", 64, ConsoleOutput);
	Fork(testFork());
	Fork(testFork());

	/* not reached */
}