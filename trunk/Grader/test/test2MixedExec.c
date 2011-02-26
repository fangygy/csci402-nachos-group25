#include "syscall.h"
int
main()
{
	int a[0];
	Print("\nSORT AND MATMULT EXEC TEST\n",sizeof("\nSORT AND MATMULT EXEC TEST\n"),a,0);
	Exec("../test/matmult");
	Exec("../test/sort");
}
