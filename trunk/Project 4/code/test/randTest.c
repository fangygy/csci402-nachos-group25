/* randTest.c
 *	
 * Test program to test functionality of Random syscall
 *	
 *	
 *
 */

#include "syscall.h"
#define NV 0x9999

int i;
int rand;

int main()
{
	Write("This is the Random number syscall test.\n", sizeof("This is the Random number syscall test.\n"), ConsoleOutput);
	
	Write("10 random numbers between 0 and 9:\n", sizeof("10 random numbers between 0 and 9:\n"), ConsoleOutput);
	for (i = 0; i < 10; i++) {
		rand = Random(10);
		/*rand += zOffset;*/
		
		Trace("Value ", i);
		Trace(": ", rand);
		Trace("\n", NV);
	}
	
	Trace("\n", NV);
	
	/* not reached */
	Exit(0);
}