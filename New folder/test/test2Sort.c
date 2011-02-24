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
    int i, j, tmp;
    int B[1024];	/* size of physical memory; with code, we'll run out of space!*/
    int k, l, tmp2;
    int a[0];

void
sort()
{

    /* first initialize the array, in reverse sorted order */
    for (i = 0; i < 1024; i++)		
        A[i] = 1024 - i;

    /* then sort! */
    for (i = 0; i < 1023; i++)
        for (j = i; j < (1023 - i); j++)
	   if (A[j] > A[j + 1]) {	/* out of order -> need to swap ! */
	      tmp = A[j];
	      A[j] = A[j + 1];
	      A[j + 1] = tmp;
    	   }
    Print("Exiting with status %d\n", sizeof("Exiting with status %d\n"), A, 1);
    Exit(A[0]);		/* and then we're done -- should be 0! */
}
void
sort2()
{

    /* first initialize the array, in reverse sorted order */
    for (k = 0; k < 1024; k++)		
        B[k] = 1024 - k;

    /* then sort! */
    for (k = 0; k < 1023; k++)
        for (l = k; l < (1023 - k); l++)
	   if (B[l] > B[l + 1]) {	/* out of order -> need to swap ! */
	      tmp2 = B[l];
	      B[l] = B[l + 1];
	      B[l + 1] = tmp2;
    	   }
    Print("Exiting with status %d\n", sizeof("Exiting with status %d\n"), B, 1);
    Exit(B[0]);		/* and then we're done -- should be 0! */
}
int
main()
{
	Print("\n2 SORT TEST\n",sizeof("\n2 SORT TEST\n"),a,0);
	Fork(sort);
	Fork(sort2);
}
