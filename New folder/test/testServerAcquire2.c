#include "syscall.h"

int lock1,lock2;

int
main(){

	int a[0];
	
	ServerAcquire(0,0);
	ServerAcquire(1,1);
}
