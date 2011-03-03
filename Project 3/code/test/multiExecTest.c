/* multiExecTest.c
 *	
 * Test program to test functionality of multiple Exec syscalls
 *	
 *	
 *
 */

#include "syscall.h"

int i;

int main()
{
	Write("Executing first CVTest...\n", sizeof("Executing first CVTest...\n"), ConsoleOutput);
	Exec("../test/CVTest");
	
	Write("Executing second CVTest...\n", sizeof("Executing second CVTest...\n"), ConsoleOutput);
	Exec("../test/CVTest");
	
	for(i = 0; i < 500; i++) {
		Yield();
	}
	/* not reached */
	Exit(0);
}