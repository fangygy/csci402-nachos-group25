#include "syscall.h"

int mv1,mv2;

int
main(){

	int a[0];
	
	Print("\nGETSERVERMV_SYSCALL TEST\n",sizeof("\nGETSERVERMV_SYSCALL TEST\n"),a,0);
	GetMV(0,0);
	GetMV(1,1);
	mv1 = CreateMV("MV1",1);
	mv2 = CreateMV("MV2",0);
	GetMV(mv1,0);
	GetMV(mv2,1);
	Exec("../test/testGetServerMV2");
	

}
