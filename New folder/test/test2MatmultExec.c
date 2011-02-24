#include "syscall.h"

int
main()
{
	int a[0];
	Print("\n2 MATMULT EXEC TEST\n",sizeof("\n2 MATMULT EXEC TEST\n"),a,0);
	Exec("../test/matmult");
	Exec("../test/matmult");
}
