/* sort.c 
 *    Test program to sort a large number of integers.
 *
 *    Intention is to stress virtual memory system.
 *
 *    Ideally, we could read the unsorted array off of the file system,
 *	and store the result back to the file system!
 */

#include "syscall.h"

int A[1024];	/* size of physical memory; with code, we'll run out of space!*/
int a[0];

int
main()
{
	Print("\n2 SORT EXEC TEST\n",sizeof("\n2 SORT EXEC TEST\n"),a,0);
	Exec("../test/sort");
	Exec("../test/sort");
}
