#include "syscall.h"

int
main(){

	int a[0];
	int mv1,mv2;
	Print("\nDESTROYSERVERMV_SYSCALL TEST\n",sizeof("\nDESTROYSERVERMV_SYSCALL TEST\n"),a,0);
	DestroyMV(0,0);
	DestroyMV(1,1);
	mv1 = CreateMV("MV1",1);
	mv2 = CreateMV("MV2",0);
	DestroyMV(mv1,0);
	DestroyMV(mv2,1);
}
