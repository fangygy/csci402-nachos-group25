/* sort.c 
 *    Test program to sort a large number of integers.
 *
 *    Intention is to stress virtual memory system.
 *
 *    Ideally, we could read the unsorted array off of the file system,
 *	and store the result back to the file system!
 */

#include "syscall.h"

#define size 4

int A[size];	/* size of physical memory; with code, we'll run out of space!*/
int B[size];
int C[size];
int D[size];

void SortA() {
	int ai, aj, atmp;

    /* first initialize the array, in reverse sorted order */
    for (ai = 0; ai < size; ai++) {
		A[ai] = size - ai;
	}

	/* then sort! */
	for (ai = 0; ai < size; ai++) {
		for (aj = 0; aj < size; aj++) {
			if (A[aj] > A[aj + 1]) {	/* out of order -> need to swap ! */
				atmp = A[aj];
				A[aj] = A[aj + 1];
				A[aj + 1] = atmp;
			}
		}
	}
	
	for (ai = 0; ai < size; ai++) {
		Trace("A: ", A[ai]);
		Trace("\n", 0x9999);
	}
	
	Exit(A[0]);		/* and then we're done -- should be 0! */
}

void SortB() {
	int bi, bj, btmp;

    /* first initialize the array, in reverse sorted order */
    for (bi = 0; bi < size; bi++) {
		B[bi] = size - bi;
	}

	/* then sort! */
	for (bi = 0; bi < size; bi++) {
		for (bj = 0; bj < size; bj++) {
			if (B[bj] > B[bj + 1]) {	/* out of order -> need to swap ! */
				btmp = B[bj];
				B[bj] = B[bj + 1];
				B[bj + 1] = btmp;
			}
		}
	}
	
	for (bi = 0; bi < size; bi++) {
		Trace("B: ", B[bi]);
		Trace("\n", 0x9999);
	}
	
	Exit(B[0]);		/* and then we're done -- should be 0! */
}

void SortC() {
	int ci, cj, ctmp;

    /* first initialize the array, in reverse sorted order */
    for (ci = 0; ci < size; ci++) {
		C[ci] = size - ci;
	}

	/* then sort! */
	for (ci = 0; ci < size; ci++) {
		for (cj = 0; cj < size; cj++) {
			if (C[cj] > C[cj + 1]) {	/* out of order -> need to swap ! */
				ctmp = C[cj];
				C[cj] = C[cj + 1];
				C[cj + 1] = ctmp;
			}
		}
	}
	
	for (ci = 0; ci < size; ci++) {
		Trace("C: ", C[ci]);
		Trace("\n", 0x9999);
	}
	
	Exit(C[0]);		/* and then we're done -- should be 0! */
}

void SortD() {
	int di, dj, dtmp;

    /* first initialize the array, in reverse sorted order */
    for (di = 0; di < size; di++) {
		D[di] = size - di;
	}

	/* then sort! */
	for (di = 0; di < size; di++) {
		for (dj = 0; dj < size; dj++) {
			if (D[dj] > D[dj + 1]) {	/* out of order -> need to swap ! */
				dtmp = D[dj];
				D[dj] = D[dj + 1];
				D[dj + 1] = dtmp;
			}
		}
	}
	
	for (di = 0; di < size; di++) {
		Trace("D: ", D[di]);
		Trace("\n", 0x9999);
	}
	
	Exit(D[0]);		/* and then we're done -- should be 0! */
}

int main() {
	Trace("Starting Sort Test.\n", 0x9999);
	Fork(SortA);
	Fork(SortB);
	Fork(SortC);
	Fork(SortD);
	Exit(0);
}