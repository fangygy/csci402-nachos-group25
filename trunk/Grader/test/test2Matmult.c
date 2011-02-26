/* matmult.c 
 *    Test program to do matrix multiplication on large arrays.
 *
 *    Intended to stress virtual memory system.
 *
 *    Ideally, we could read the matrices off of the file system,
 *	and store the result back to the file system!
 */

#include "syscall.h"

#define Dim 	20	/* sum total of the arrays doesn't fit in 
			 * physical memory 
			 */
	int i,j,k;
	int A[Dim][Dim];
	int B[Dim][Dim];
	int C[Dim][Dim];
	int one[1];
	
	int i2,j2,k2;
	int A2[Dim][Dim];
	int B2[Dim][Dim];
	int C2[Dim][Dim];
	int one2[1];
	int a[0];


void
matmult()
{	
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
matmult2()
{	
    for (i2 = 0; i2 < Dim; i2++)		/* first initialize the matrices */
	for (j2 = 0; j2 < Dim; j2++) {
	     A2[i2][j2] = i2;
	     B2[i2][j2] = j2;
	     C2[i2][j2] = 0;
	}

    for (i2 = 0; i2 < Dim; i2++)		/* then multiply them together */
	for (j2 = 0; j2 < Dim; j2++)
            for (k2 = 0; k2 < Dim; k2++)
		 C2[i2][j2] += A2[i2][k2] * B2[k2][j2];
    one2[0] =C2[Dim-1][Dim-1];
    Print("Exiting with status %d\n", sizeof("Exiting with status %d\n"), one2, 1);
    Exit(C2[Dim-1][Dim-1]);		/* and then we're done */
}

int
main()
{
	Print("\n2 MATMULT TEST\n",sizeof("\n2 MATMULT TEST\n"),a,0);
	Fork(matmult);
	Fork(matmult2);
}
