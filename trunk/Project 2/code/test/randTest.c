/* randTest.c
 *	
 * Test program to test functionality of Random syscall
 *	
 *	
 *
 */

#include "syscall.h"

const int zOffset = 48;
int i;
int rand;
char numChar;

int main()
{
	Write("This is the Random number syscall test.\n", sizeof("This is the Random number syscall test.\n"), ConsoleOutput);
	
	Write("10 random numbers between 0 and 9:\n", sizeof("10 random numbers between 0 and 9:\n"), ConsoleOutput);
	for (i = 0; i < 10; i++) {
		rand = Random(10);
		/*rand += zOffset;*/
		
		numChar = (char) rand + '0';
		
		Write((char)numChar, (int)2, ConsoleOutput);
		Write(", ", sizeof(", "), ConsoleOutput);
	}
	Write("\n", sizeof("\n"), ConsoleOutput);
	
	/* not reached */
	Exit(0);
}