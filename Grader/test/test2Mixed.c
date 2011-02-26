/* sort.c 
 *    Test program to sort a large number of integers.
 *
 *    Intention is to stress virtual memory system.
 *
 *    Ideally, we could read the unsorted array off of the file system,
 *	and store the result back to the file system!
 */

#include "syscall.h"

int Z[1024];	/* size of physical memory; with code, we'll run out of space!*/
#define Dim 	20	/* sum total of the arrays doesn't fit in 
			 * physical memory 
			 */

int A[Dim][Dim];
int B[Dim][Dim];
int C[Dim][Dim];
int one[1];
int a[0];

void
matmult()
{
    int i, j, k;
    int one[1];


    for (i = 0; i < Dim; i++)		/* first initialize the matrices */
	for (j = 0; j < Dim; j++) {
	     A[i][j] = i;
	     B[i][j] = j;
	     C[i][j] = 0;
	}

    for (i = 0; i < Dim; i++)		/* then multiply them together */
	for (j = 0; j < Dim; j++)
            for (k = 0; k < Dim; k++)
		 C[i][j] += A[i][k] * B[k][j];
    one[0] =C[Dim-1][Dim-1];
    Print("Exiting with status %d\n", sizeof("Exiting with status %d\n"), one, 1);
    Exit(C[Dim-1][Dim-1]);		/* and then we're done */
}

void
sort()
{
    int i, j, tmp;

    /* first initialize the array, in reverse sorted order */
    for (i = 0; i < 1024; i++)		
        Z[i] = 1024 - i;

    /* then sort! */
    for (i = 0; i < 1023; i++)
        for (j = i; j < (1023 - i); j++)
	   if (Z[j] > Z[j + 1]) {	/* out of order -> need to swap ! */
	      tmp = Z[j];
	      Z[j] = Z[j + 1];
	      Z[j + 1] = tmp;
    	   }
    Print("Exiting with status %d\n", sizeof("Exiting with status %d\n"), Z, 1);
    Exit(A[0]);		/* and then we're done -- should be 0! */
}
int
main()
{
	Print("\nSORT AND MATMULT TEST\n",sizeof("\nSORT AND MATMULT TEST\n"),a,0);
	Fork(sort);
	Fork(matmult);
}
