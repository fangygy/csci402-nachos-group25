/* execTest.c
 *	
 * Test program to test functionality of Exec syscalls
 *	
 *	
 *
 */

#include "syscall.h"


int main()
{
	Exec("Blahblahfilefail");
	Exec("../test/forkTest");
	/* not reached */
}