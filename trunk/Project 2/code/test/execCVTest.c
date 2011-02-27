/* CVTest1.c
 *	
 * Test program to test functionality of CVs
 * Create, Destroy, Wait, Signal, Broadcast
 *	
 *	
 *
 */

#include "syscall.h"

int main()
{
	Write("Executing CVTest1...\n\n", sizeof("Executing CVTest1...\n\n"), ConsoleOutput);
	Exec("../test/CVTest1");
	/* not reached */
	Exit(0);
}